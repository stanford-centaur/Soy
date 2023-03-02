#include "Engine.h"

#include <random>

#include "Debug.h"
#include "InfeasibleQueryException.h"
#include "InputQuery.h"
#include "LPBasedTightener.h"
#include "LocalSearchPlateauException.h"
#include "MStringf.h"
#include "SoyError.h"
#include "PLConstraint.h"
#include "Preprocessor.h"
#include "TimeUtils.h"
#include "Vector.h"

Engine::Engine()
    : _context(),
      _boundManager(_context),
      _preprocessedQuery(nullptr),
      _smtCore(this),
      _quitRequested(false),
      _exitCode(Engine::NOT_DONE),
      _verbosity(Options::get()->getInt(Options::VERBOSITY)),
      _statisticsPrintingFrequency(
          GlobalConfiguration::STATISTICS_PRINTING_FREQUENCY),
      _deepSoIRejectionThreshold(
          Options::get()->getInt(Options::DEEP_SOI_REJECTION_THRESHOLD)),
      _solveWithMILP(Options::get()->getBool(Options::SOLVE_WITH_MILP)),
      _cdcl(!Options::get()->getBool(Options::NO_CDCL)),
      _boundTightening(!Options::get()->getBool(Options::NO_BOUND_TIGHTENING)),
      _lpBasedTightener(nullptr),
      _milpEncoder(nullptr),
      _gurobi(nullptr),
      _cadical(nullptr),
      _soiManager(nullptr),
      _assignmentManager(nullptr),
      _maxLemmaLength(Options::get()->getInt(Options::MAX_LEMMA_LENGTH)),
      _maxNumberOfProposals(Options::get()->getInt(Options::MAX_PROPOSALS_PER_STATE)),
      _milpSolvingThreshold(
          Options::get()->getFloat(Options::MILP_SOLVING_THRESHOLD)),
      _cachePhasePattern(GlobalConfiguration::CACHE_PHASE_PATTERN &&
                         Options::get()->getSoISearchStrategy() !=
                             SoISearchStrategy::GREEDY_SAT) {
  _smtCore.setStatistics(&_statistics);
  _preprocessor.setStatistics(&_statistics);

  _statistics.stampStartingTime();

  setRandomSeed(Options::get()->getInt(Options::SEED));
}

Engine::~Engine() {}

void Engine::computeInitialPattern(const InputQuery &inputQuery) {
  std::cout << "computing initial pattern" << std::endl;
  struct timespec start = TimeUtils::sampleMicro();
  CVC4::context::Context ctx;
  BoundManager bm(ctx);
  bm.initialize(inputQuery.getNumberOfVariables());
  auto lbs = inputQuery.getLowerBounds();
  auto ubs = inputQuery.getUpperBounds();
  for (unsigned i = 0; i < inputQuery.getNumberOfVariables(); ++i) {
    if (lbs.exists(i)) bm.setLowerBound(i, lbs[i]);
    if (ubs.exists(i)) bm.setUpperBound(i, ubs[i]);
  }

  LPBasedTightener patternFinder(bm, inputQuery);
  patternFinder.computeKPattern(_lemmas, _maxLemmaLength);


  struct timespec end = TimeUtils::sampleMicro();
  std::cout << "time computing initial pattern: "
            << TimeUtils::timePassed(start, end) / 1000 / 1000 << std::endl;
}

void Engine::addAllLemmasToSatSolver(){
  Vector<PLConstraint *> plConstraintsV;
  for (const auto &p : _plConstraints) plConstraintsV.append(p);

  // Add the top level lemmas to SAT solver
  for (unsigned i = 0; i < plConstraintsV.size(); ++i) {
    for (const auto &lemma : _lemmas) {
      if (i + lemma.size() > plConstraintsV.size())
        continue;
      List<int> clause;
      unsigned j = i;
      for (const auto &phase : lemma) {
        if (plConstraintsV[j]->phaseStatusHasLiteral(phase))
          clause.append(-plConstraintsV[j]->getLiteralOfPhaseStatus(phase));
        ++j;
      }
      if (clause.size() == lemma.size()) _cadical->addConstraint(clause);
    }
  }
}

bool Engine::processInputQuery(InputQuery &inputQuery) {
  return processInputQuery(inputQuery,
                           GlobalConfiguration::PREPROCESS_INPUT_QUERY);
}

bool Engine::processInputQuery(InputQuery &inputQuery, bool preprocess) {
  ENGINE_LOG("processInputQuery starting\n");
  struct timespec start = TimeUtils::sampleMicro();

  if (!_solveWithMILP) computeInitialPattern(inputQuery);

  try {
    informConstraintsOfInitialBounds(inputQuery);
    invokePreprocessor(inputQuery, preprocess);

    _plConstraints = _preprocessedQuery->getPLConstraints();
    if (GlobalConfiguration::
            PL_CONSTRAINTS_ADD_AUX_EQUATIONS_AFTER_PREPROCESSING)
      for (auto &plConstraint : _plConstraints)
        plConstraint->addAuxiliaryEquationsAfterPreprocessing(
            *_preprocessedQuery);

    unsigned n = _preprocessedQuery->getNumberOfVariables();
    _boundManager.initialize(n);
    initializeBounds(n);

    //if (_lpBasedTightening) performLPBasedBoundTightening();

    _milpEncoder = std::unique_ptr<MILPEncoder>(new MILPEncoder(_boundManager));
    _milpEncoder->setStatistics(&_statistics);
    _gurobi = std::unique_ptr<GurobiWrapper>(new GurobiWrapper());

    _cadical = std::unique_ptr<CadicalWrapper>(new CadicalWrapper());
    _cadical->setStatistics(&_statistics);

    _assignmentManager = std::unique_ptr<AssignmentManager>(
        new AssignmentManager(_boundManager));
    for (const auto &plConstraint : _plConstraints)
      for (const auto &var : plConstraint->getParticipatingVariables())
        _variablesParticipatingInPLConstraints.insert(var);

    _soiManager =
        std::unique_ptr<SoIManager>(new SoIManager(*_preprocessedQuery));
    _soiManager->setStatistics(&_statistics);
    _soiManager->setAssignmentManager(&(*_assignmentManager));
    _soiManager->setSmtCore(&_smtCore);
    _soiManager->setSatSolver(&(*_cadical));
    decideBranchingHeuristics();

    _statistics.setUnsignedAttribute(
        Statistics::NUM_VARIABLES, _preprocessedQuery->getNumberOfVariables());
    _statistics.setUnsignedAttribute(Statistics::NUM_EQUATIONS,
                                     _preprocessedQuery->getEquations().size());

    struct timespec end = TimeUtils::sampleMicro();
    _statistics.setLongAttribute(Statistics::PREPROCESSING_TIME_MICRO,
                                 TimeUtils::timePassed(start, end));
    ENGINE_LOG("processInputQuery done\n");
  } catch (const InfeasibleQueryException &) {
    ENGINE_LOG("processInputQuery done with exception\n");

    struct timespec end = TimeUtils::sampleMicro();
    _statistics.setLongAttribute(Statistics::PREPROCESSING_TIME_MICRO,
                                 TimeUtils::timePassed(start, end));

    _exitCode = Engine::UNSAT;
    return false;
  }
  return true;
}

void Engine::performLPBasedBoundTightening() {
  ENGINE_LOG("Performing LP-baesd bound tightening...");
  _lpBasedTightener = std::unique_ptr<LPBasedTightener>(
      new LPBasedTightener(_boundManager, *_preprocessedQuery));

  unsigned totalTightened = 0;
  unsigned tightened = 0;
  do {
    tightened = _lpBasedTightener->performLPBasedTightening();
    _preprocessor.preprocessLite(*_preprocessedQuery, _boundManager, true);

    if (_verbosity > 1)
      printf("Number of tightened bound this round: %u\n", tightened);
    totalTightened += tightened;
  } while (tightened > 0);

  if (_verbosity > 1)
    printf(
        "Performing LP-baesd bound tightening - done. "
        "Number of tightened bound: %u\n",
        totalTightened);
}

void Engine::informConstraintsOfInitialBounds(InputQuery &inputQuery) const {
  for (const auto &plConstraint : inputQuery.getPLConstraints()) {
    List<unsigned> variables = plConstraint->getParticipatingVariables();
    for (unsigned variable : variables) {
      plConstraint->notifyLowerBound(variable,
                                     inputQuery.getLowerBound(variable));
      plConstraint->notifyUpperBound(variable,
                                     inputQuery.getUpperBound(variable));
    }
  }
}

void Engine::invokePreprocessor(InputQuery &inputQuery, bool preprocess) {
  if (_verbosity > 0)
    printf(
        "Engine::processInputQuery: Input query (before preprocessing): "
        "%u equations, %u variables\n",
        inputQuery.getEquations().size(), inputQuery.getNumberOfVariables());

  // If processing is enabled, invoke the preprocessor
  if (preprocess) _preprocessor.preprocess(inputQuery);

  _preprocessedQuery = std::unique_ptr<InputQuery>(new InputQuery(inputQuery));

  if (_verbosity > 0)
    printf(
        "Engine::processInputQuery: Input query (after preprocessing): "
        "%u equations, %u variables\n\n",
        _preprocessedQuery->getEquations().size(),
        _preprocessedQuery->getNumberOfVariables());
}

void Engine::initializeBounds(unsigned numberOfVariables) {
  for (unsigned i = 0; i < numberOfVariables; ++i) {
    _boundManager.setLowerBound(i, _preprocessedQuery->getLowerBound(i));
    _boundManager.setUpperBound(i, _preprocessedQuery->getUpperBound(i));
  }

  _statistics.setUnsignedAttribute(Statistics::NUM_PL_CONSTRAINTS,
                                   _plConstraints.size());
}

void Engine::decideBranchingHeuristics() {
  String branch = Options::get()->getString(Options::BRANCHING_HEURISTICS);
  DivideStrategy divideStrategy = DivideStrategy::PseudoImpact;

  if (branch == "topological") {
    printf("Branching heuristics set to Topological\n");
    divideStrategy = DivideStrategy::Topological;
  } else
    printf("Branching heuristics set to PseudoImpact\n");

  _smtCore.setBranchingHeuristics(divideStrategy);
  if (divideStrategy == DivideStrategy::PseudoImpact)
    _smtCore.initializeScoreTrackerIfNeeded(_plConstraints);
}

bool Engine::solve(unsigned timeoutInSeconds) {
  SignalHandler::getInstance()->initialize();
  SignalHandler::getInstance()->registerClient(this);

  _initialPreprocessedInputQuery = *_preprocessedQuery;

  for (auto &plConstraint : _plConstraints) {
    plConstraint->initializeCDOs(&_context);
    plConstraint->registerAssignmentManager(&(*_assignmentManager));
    plConstraint->registerBoundManager(&_boundManager);
    plConstraint->registerSatSolver(&(*_cadical));
    plConstraint->addBooleanStructure();
    plConstraint->setStatistics(&_statistics);
  }

  addAllLemmasToSatSolver();

  DEBUG({
    _cadical->solve();
    _cadical->dumpModel("test.cnf");
  });

  DEBUG({
    // Initially, all constraints should be active
    for (const auto &plc : _plConstraints) {
      ASSERT(plc->isActive());
    }
  });

  if (_solveWithMILP) return solveWithMILPEncoding(timeoutInSeconds);

  ENGINE_LOG("Encoding convex relaxation into Gurobi...");
  _milpEncoder->encodeInputQuery(*_gurobi, *_preprocessedQuery, true);
  ENGINE_LOG("Encoding convex relaxation into Gurobi - done");

  _statistics.stampMainLoopStartTime();

  mainLoopStatistics();
  if (_verbosity > 0) {
    printf("\nEngine::solve: Initial statistics\n");
    _statistics.print();
    printf("\n---\n");
  }

  bool splitJustPerformed = true;
  while (true) {
    if (shouldExitDueToTimeout(timeoutInSeconds)) {
      if (_verbosity > 0) {
        printf("\n\nEngine: quitting due to timeout...\n\n");
        printf("Final statistics:\n");
        _statistics.print();
      }

      _exitCode = Engine::TIMEOUT;
      _statistics.timeout();
      return false;
    }

    if (_quitRequested) {
      if (_verbosity > 0) {
        printf("\n\nEngine: quitting due to external request...\n\n");
        printf("Final statistics:\n");
        _statistics.print();
      }

      _exitCode = Engine::QUIT_REQUESTED;
      return false;
    }
    try {
      mainLoopStatistics();
      if (_verbosity > 1 &&
          _statistics.getLongAttribute(Statistics::NUM_MAIN_LOOP_ITERATIONS) %
          _statisticsPrintingFrequency == 0)
        _statistics.print();

      // If true, we just entered a new subproblem
      if (splitJustPerformed) {
        // NOTHING context dependent should precede this
        if (_cdcl) {
          struct timespec satStart = TimeUtils::sampleMicro();
          informSatSolverOfDecisions();
          bool feasible = checkBooleanLevelFeasibility();
          struct timespec satEnd = TimeUtils::sampleMicro();
          _statistics.incLongAttribute(Statistics::TIME_SAT_SOLVING_MICRO,
                                       TimeUtils::timePassed(satStart, satEnd));
          if (!feasible) throw InfeasibleQueryException();
        }

        if (_boundTightening){
          struct timespec tighteningStart = TimeUtils::sampleMicro();
          bool isFeasible = _preprocessor.preprocessLite(*_preprocessedQuery, _boundManager, true);
          struct timespec tighteningEnd = TimeUtils::sampleMicro();
          _statistics.incLongAttribute(Statistics::TIME_BOUND_TIGHTENING_MICRO,
                                       TimeUtils::timePassed(tighteningStart, tighteningEnd));
          if (!isFeasible) {
            _statistics.incUnsignedAttribute(Statistics::NUM_REFUTATIONS_BY_BOUND_TIGHTENING);
            extractTheoryExplanation(true);
            throw InfeasibleQueryException();
          }
        }

        applyAllValidConstraintCaseSplits();
        informLPSolverOfBounds();
        splitJustPerformed = false;
      }

      // Perform any SmtCore-initiated case splits
      // Before doing this, apply any valid case splits
      if (_smtCore.needToSplit()) {
        _smtCore.performSplit();
        splitJustPerformed = true;
        continue;
      }

      // Perform any restart
      if (_smtCore.needToRestart()) {
        if (_verbosity > 1) printf("Restarting...\n");
        _smtCore.restart();
        struct timespec start = TimeUtils::sampleMicro();
        _gurobi->resetModel();
        _milpEncoder->reset();
        _milpEncoder->encodeInputQuery(*_gurobi, *_preprocessedQuery, true);
        struct timespec end = TimeUtils::sampleMicro();
        _statistics.incLongAttribute(
                                     Statistics::TIME_ADDING_CONSTRAINTS_TO_MILP_SOLVER_MICRO,
                                     TimeUtils::timePassed(start, end));
        // Set all constraints to active again.
        for (const auto &c : _plConstraints) {
          c->setActive(true);
          c->decayScores();
        }
        splitJustPerformed = true;
        continue;
      }

      if (_gurobi->haveFeasibleSolution()) {
        // The linear portion of the problem has been solved.
        // Check the status of the PL constraints
        bool solutionFound = adjustAssignmentToSatisfyNonLinearConstraints();
        if (solutionFound) {

          if (_verbosity > 0) {
            printf("\nEngine::solve: sat assignment found\n");
            _statistics.print();
          }
          _assignmentManager->extractAssignmentFromGurobi(*_gurobi);
          checkSolutionCompliance();
          _exitCode = Engine::SAT;
          return true;
        } else {
          applyAllValidConstraintCaseSplits();
          continue;
        }
      }
      //DEBUG(checkConsistencyBetweenParties());
      checkFeasibilityWithGurobi();
    } catch (const InfeasibleQueryException &) {
      // The current query is unsat, and we need to pop.
      // If we're at level 0, the whole query is unsat.
      if (!_smtCore.popSplit()) {
        if (_verbosity > 0) {
          printf("\nEngine::solve: unsat query\n");
          _statistics.print();
        }
        _exitCode = Engine::UNSAT;
        return false;
      } else {
        splitJustPerformed = true;
      }
    } catch (SoyError &e) {
      String message = Stringf("Caught a SoyError. Code: %u. Message: %s ",
                               e.getCode(), e.getUserMessage());
      std::cout << message.ascii() << std::endl;
      _exitCode = Engine::ERROR;
      return false;
    } catch (...) {
      _exitCode = Engine::ERROR;
      return false;
    }
  }
}

double Engine::getAssignment(unsigned variable) {
  return _assignmentManager->getAssignment(variable);
}

bool Engine::solveWithMILPEncoding(unsigned timeoutInSeconds) {
  try {
    ENGINE_LOG("Encoding the input query with Gurobi...\n");
    _milpEncoder->encodeInputQuery(*_gurobi, *_preprocessedQuery);
    ENGINE_LOG("Query encoded in Gurobi...\n");

    if (timeoutInSeconds > 0) {
      ENGINE_LOG(
          Stringf("Gurobi timeout set to %.2f\n", timeoutInSeconds).ascii())
      _gurobi->setTimeLimit(timeoutInSeconds);
    }
    _gurobi->setVerbosity(_verbosity > 1);
    _gurobi->solve();

    if (_gurobi->haveFeasibleSolution()) {
      _assignmentManager->extractAssignmentFromGurobi(*_gurobi);
      _exitCode = Engine::SAT;
      return true;
    } else if (_gurobi->infeasible())
      _exitCode = Engine::UNSAT;
    else if (_gurobi->timeout())
      _exitCode = Engine::TIMEOUT;
    else
      throw SoyError(SoyError::UNEXPECTED_RETURN_STATUS_FROM_GUROBI);
    return false;
  } catch (SoyError &e) {
    String message = Stringf("Caught a SoyError. Code: %u. Message: %s ",
                             e.getCode(), e.getUserMessage());
    std::cout << message.ascii() << std::endl;
    _exitCode = Engine::ERROR;
    return false;
  }
}

void Engine::checkSolutionCompliance() const {
  Map<unsigned, double> assignment;
  for (unsigned i = 0; i < _preprocessedQuery->getNumberOfVariables(); ++i)
    assignment[i] = _assignmentManager->getAssignment(i);
  _initialPreprocessedInputQuery.checkSolutionCompliance(assignment);
}

void Engine::checkConsistencyBetweenParties() const {
  // The boolean variable is fixed
  // <=> the corresponding phase is fixed
  // <=> the bound is fixed
  ENGINE_LOG("Checking consistency between parties...");
  for (const auto &plConstraint : _plConstraints) {
    if (!plConstraint->isActive()) continue;
    for (const auto &phase : plConstraint->getAllCases()) {
      int lit = plConstraint->getLiteralOfPhaseStatus(phase);
      unsigned variable =
          ((OneHotConstraint *)plConstraint)->getElementOfPhase(phase);
      if (plConstraint->isFeasible(phase)) {
        if (_boundManager.getUpperBound(variable) == 0) {
          std::cout << "Phase is feasible but upper-bound is 0." << std::endl;
          ASSERT(false);
        }
        if (_cadical->getLiteralStatus(lit) == FALSE) {
          std::cout << "Phase is feasible but bVar is fixed to false."
                    << std::endl;
          ASSERT(false);
        }
      } else {
        if (_boundManager.getUpperBound(variable) != 0) {
          std::cout << "Phase is infeasible but upper-bound is not 0."
                    << std::endl;
          ASSERT(false);
        }
        if (_cadical->getLiteralStatus(lit) != FALSE) {
          std::cout << "Phase is infeasible but bVar is not fixed to false."
                    << std::endl;
          ASSERT(false);
        }
      }
      if (_cadical->getLiteralStatus(lit) == TRUE) {
        if (_boundManager.getLowerBound(variable) < 1) {
          std::cout
              << "Literal is fixed to True but variable is not fixed to 1."
              << std::endl;
          ASSERT(false);
        }
      } else if (_cadical->getLiteralStatus(lit) == FALSE) {
        if (_boundManager.getUpperBound(variable) > 0) {
          std::cout
              << "Literal is fixed to FALSE but variable is not fixed to 0."
              << std::endl;
          ASSERT(false);
        }
      } else {
        if (_boundManager.getUpperBound(variable) < 1 ||
            _boundManager.getLowerBound(variable) > 0) {
          std::cout << "Literal is not fixed but variable is fixed."
                    << std::endl;
          ASSERT(false);
        }
      }
    }
  }
  ENGINE_LOG("Checking consistency between parties - done");
}

void Engine::checkTheoryLemmaCorrectness() const {
  ENGINE_LOG("Checking theory lemma correctness...");

  CVC4::context::Context ctx;
  BoundManager bm(ctx);
  bm.initialize(_preprocessedQuery->getNumberOfVariables());
  for (unsigned i = 0; i < _preprocessedQuery->getNumberOfVariables(); ++i) {
    bm.setLowerBound(i, _preprocessedQuery->getLowerBound(i));
    bm.setUpperBound(i, _preprocessedQuery->getUpperBound(i));
  }

  MILPEncoder encoder(bm);
  GurobiWrapper gurobi;
  encoder.encodeInputQuery(gurobi, *_preprocessedQuery, true);
  gurobi.solve();
  DEBUG(gurobi.dumpModel("debug_model_pre_conflict.lp"));
  gurobi.updateModel();
  ASSERT(gurobi.haveFeasibleSolution());

  CadicalWrapper cadical(*_cadical);

  for (auto const &pair : _smtCore.getCurrentConflict()._literals) {
    OneHotConstraint *c = (OneHotConstraint *)pair.first;
    gurobi.setLowerBound(Stringf("x%u", c->getElementOfPhase(pair.second)), 1);
    cadical.addConstraint({c->getLiteralOfPhaseStatus(pair.second)});
  }

  cadical.solve();
  for (const auto &plConstraint : _plConstraints) {
    OneHotConstraint *c = (OneHotConstraint *)plConstraint;
    for (const auto &phase : c->getAllCases()) {
      int lit = c->getLiteralOfPhaseStatus(phase);
      if (cadical.getLiteralStatus(lit) == TRUE)
        gurobi.setLowerBound(Stringf("x%u", c->getElementOfPhase(phase)), 1);
      else if (cadical.getLiteralStatus(lit) == FALSE)
        gurobi.setUpperBound(Stringf("x%u", c->getElementOfPhase(phase)), 0);
    }
  }

  gurobi.solve();
  gurobi.updateModel();
  DEBUG(gurobi.dumpModel("debug_model_post_conflict.lp"));
  ASSERT(gurobi.infeasible());
  ENGINE_LOG("Checking theory lemma correctness - ok!");
}

void Engine::informSatSolverOfDecisions() {
  _smtCore.informSatSolverOfDecisions(&(*_cadical));
}

bool Engine::checkBooleanLevelFeasibility() {
  ENGINE_LOG("Checking Boolean level feasibility...");
  _cadical->solve();
  if (_cadical->infeasible()) {
    _statistics.incUnsignedAttribute(Statistics::NUM_REFUTATIONS_BY_SAT_SOLVER);
    ENGINE_LOG("Checking Boolean level feasibility - infeasible!");
    return false;
  } else {
    _cadical->retrieveAndUpdateWatcherOfFixedBVariable();
    ENGINE_LOG("Checking Boolean level feasibility - feasible");
    return true;
  }
}

bool Engine::adjustAssignmentToSatisfyNonLinearConstraints() {
  ENGINE_LOG(
      "Linear constraints satisfied. Now trying to satisfy non-linear"
      " constraints...");
  collectViolatedPlConstraints();

  // If all constraints are satisfied, we are possibly done
  if (allPlConstraintsHold()) {
    return true;
  } else if (_deepSoIRejectionThreshold == 0) {
    _smtCore.reportRejectedPhasePatternProposal();
    return false;
  } else {
    return performDeepSoILocalSearch();
  }
}

bool Engine::performDeepSoILocalSearch() {
  ENGINE_LOG("Performing local search...");
  struct timespec start = TimeUtils::sampleMicro();
  ASSERT(_gurobi->haveFeasibleSolution());

  try {
    // All the linear constraints have been satisfied at this point.
    // Update the cost function
    _soiManager->initializePhasePattern();

    LinearExpression initialPhasePattern =
        _soiManager->getCurrentSoIPhasePattern();

    if (initialPhasePattern.isZero()) {
      throw LocalSearchPlateauException();
    }
    else {
      minimizeCostWithGurobi(initialPhasePattern);

      ASSERT(_gurobi->haveFeasibleSolution());
      // Always accept the first phase pattern.
      _soiManager->acceptCurrentPhasePattern();
      double costOfLastAcceptedPhasePattern = _gurobi->getObjectiveValue();

      if (_cachePhasePattern)
        _soiManager->cacheCurrentPhasePattern(costOfLastAcceptedPhasePattern);

      double costOfProposedPhasePattern = FloatUtils::infinity();
      bool lastProposalAccepted = true;

      bool solutionFound = false;
      while (!_smtCore.needToSplit()) {
        if (lastProposalAccepted) {
          /*
            Check whether the optimal solution to the last accepted phase
            is a real solution. We only check this when the last proposal
            was accepted, because rejected phase pattern must have resulted in
            increase in the SoI cost.

            HW: Another option is to only do this check when
            costOfLastAcceptedPhasePattern is 0, but this might be too strict.
            The overhead is low anyway.
          */
          collectViolatedPlConstraints();
          if (allPlConstraintsHold()) {
            solutionFound = true;
            break;
          } else if (FloatUtils::isZero(costOfLastAcceptedPhasePattern)) {
            throw LocalSearchPlateauException();
          }
        }

        // Reach maximal number of attempts at this state
        if ( _statistics.getLongAttribute
             ( Statistics::NUM_PROPOSALS_SINCE_LAST_REINITIALIZATION ) >
             _maxNumberOfProposals ) {
          throw LocalSearchPlateauException();
        }

        // Rejected phase pattern must be infeasible
        // The current phase pattern is infeasible
        struct timespec feasStart = TimeUtils::sampleMicro();
        _soiManager->addCurrentPhasePatternAsConflict(*_cadical);
        bool feasible = checkBooleanLevelFeasibility();
        struct timespec feasEnd = TimeUtils::sampleMicro();
        _statistics.incLongAttribute(Statistics::TOTAL_TIME_SAT_SOLVING_SOI_MICRO,
                                     TimeUtils::timePassed(feasStart, feasEnd));
        if (!feasible) {
          printf("\tInfeasibility detected during DeepSoI!\n");
          struct timespec end = TimeUtils::sampleMicro();
          _statistics.incLongAttribute(Statistics::TOTAL_TIME_LOCAL_SEARCH_MICRO,
                                       TimeUtils::timePassed(start, end));
          throw InfeasibleQueryException();
        }

        // No satisfying assignment found for the last accepted phase pattern,
        // propose an update to it.
        _soiManager->proposePhasePatternUpdate();
        if (_verbosity > 2) _soiManager->dumpCurrentPattern();

        if (_cachePhasePattern &&
            _soiManager->loadCachedPhasePattern(costOfProposedPhasePattern)) {
          _statistics.incLongAttribute(Statistics::NUM_PHASE_PATTERN_CACHE_HITS);
        } else {
          minimizeCostWithGurobi(_soiManager->getCurrentSoIPhasePattern());
          costOfProposedPhasePattern = _gurobi->getObjectiveValue();
          if (_cachePhasePattern)
            _soiManager->cacheCurrentPhasePattern(costOfProposedPhasePattern);
        }

        // We have the "local" effect of change the cost term of some
        // PLConstraints in the phase pattern. Use this information to influence
        // the branching decision.
        updatePseudoImpactWithSoICosts(costOfLastAcceptedPhasePattern,
                                       costOfProposedPhasePattern);

        // Decide whether to accept the last proposal.
        if (_soiManager->decideToAcceptCurrentProposal(
                costOfLastAcceptedPhasePattern, costOfProposedPhasePattern)) {
          _soiManager->acceptCurrentPhasePattern();
          _statistics.incLongAttribute(
              Statistics::NUM_ACCEPTED_PHASE_PATTERN_UPDATE);
          costOfLastAcceptedPhasePattern = costOfProposedPhasePattern;
          lastProposalAccepted = true;
        } else {
          _smtCore.reportRejectedPhasePatternProposal();
          lastProposalAccepted = false;
        }
      }

      struct timespec end = TimeUtils::sampleMicro();
      _statistics.incLongAttribute(Statistics::TOTAL_TIME_LOCAL_SEARCH_MICRO,
                                   TimeUtils::timePassed(start, end));
      ENGINE_LOG("Performing local search - done");
      return solutionFound;
    }
  } catch (const LocalSearchPlateauException &){
    while (!_smtCore.needToSplit())
      _smtCore.reportRejectedPhasePatternProposal();
    struct timespec end = TimeUtils::sampleMicro();
    _statistics.incLongAttribute(Statistics::TOTAL_TIME_LOCAL_SEARCH_MICRO,
                                 TimeUtils::timePassed(start, end));
    ENGINE_LOG("Performing local search - done");
    return false;
  }
}

double Engine::computeHeuristicCost(const LinearExpression &heuristicCost) {
  double cost = heuristicCost._constant;
  for (const auto &pair : heuristicCost._addends) {
    double value = _assignmentManager->getAssignment(pair.first);
    cost += pair.second * value;
  }
  return cost;
}

void Engine::updatePseudoImpactWithSoICosts(
    double costOfLastAcceptedPhasePattern, double costOfProposedPhasePattern) {
  struct timespec start = TimeUtils::sampleMicro();
  ASSERT(_soiManager);

  const Map<PLConstraint *, PhaseStatus> &constraintsUpdated =
      _soiManager->getConstraintsUpdatedInLastProposal();

  // If score is negative, the proposed change is bad
  double score =
      ((costOfLastAcceptedPhasePattern - costOfProposedPhasePattern) /
       constraintsUpdated.size());

  ASSERT(constraintsUpdated.size() > 0);
  // Update the Pseudo-Impact estimation.
  for (const auto &pair : constraintsUpdated) {
    _smtCore.updatePLConstraintScore(pair.first, fabs(score));
    pair.first->updatePhaseStatusScore(pair.second, score);
  }
  struct timespec end = TimeUtils::sampleMicro();
  _statistics.incLongAttribute
    (Statistics::TOTAL_TIME_UPDATING_PSEUDO_IMPACT_MICRO,
     TimeUtils::timePassed(start, end));
}

void Engine::bumpUpPseudoImpactOfPLConstraintsNotInSoI() {
  ASSERT(_soiManager);
  for (const auto &plConstraint : _plConstraints) {
    if (plConstraint->isActive() && !plConstraint->supportSoI() &&
        !plConstraint->phaseFixed() && !plConstraint->satisfied())
      _smtCore.updatePLConstraintScore(
          plConstraint,
          GlobalConfiguration::SCORE_BUMP_FOR_PL_CONSTRAINTS_NOT_IN_SOI);
  }
}

bool Engine::checkFeasibilityWithGurobi() {
  ASSERT(_gurobi && _milpEncoder);
  ENGINE_LOG("Checking LP feasibility with Gurobi...");
  DEBUG({ checkGurobiBoundConsistency(); });
  struct timespec simplexStart = TimeUtils::sampleMicro();
  bool isMILP = _smtCore.getTrailLength() > _milpSolvingThreshold * _plConstraints.size();
  if (isMILP) _milpEncoder->enforceIntegralConstraint(*_gurobi);

  LinearExpression dontCare;
  _milpEncoder->encodeCostFunction(*_gurobi, dontCare);

  _gurobi->setTimeLimit(FloatUtils::infinity());
  _gurobi->setNumberOfThreads(1);
  //Use dual simplex method for feasibility because the explanation is faster
  if (isMILP) {
    _gurobi->setMethod(-1);
  }
  else {
    _gurobi->setMethod(3);
  }
  _gurobi->solve();
  DEBUG(_gurobi->dumpModel("lp_model.lp"));

  struct timespec simplexEnd = TimeUtils::sampleMicro();
  _statistics.incLongAttribute(Statistics::TIME_LP_FEASIBILITY_CHECK_MICRO,
                               TimeUtils::timePassed(simplexStart, simplexEnd));
  _statistics.incUnsignedAttribute(Statistics::NUM_LP_FEASIBILITY_CHECK);

  if (_gurobi->infeasible()) {
    _statistics.incUnsignedAttribute(
        Statistics::NUM_REFUTATIONS_BY_THEORY_SOLVER);

    if (_cdcl && GlobalConfiguration::EXTRACT_THEORY_EXPLANATION)
      extractTheoryExplanation(isMILP);
    if (isMILP) _milpEncoder->relaxIntegralConstraint(*_gurobi);
    throw InfeasibleQueryException();
  } else if (_gurobi->haveFeasibleSolution()) {
    if (isMILP) _milpEncoder->relaxIntegralConstraint(*_gurobi);
    _assignmentManager->extractAssignmentFromGurobi
      (*_gurobi, _variablesParticipatingInPLConstraints);
    return true;
  } else
    throw CommonError(
        CommonError::UNEXPECTED_GUROBI_STATUS,
        Stringf("Current status: %u", _gurobi->getStatusCode()).ascii());

  return false;
}

bool Engine::minimizeCostWithGurobi(const LinearExpression &costFunction) {
  ASSERT(_gurobi && _milpEncoder);
  struct timespec start = TimeUtils::sampleMicro();
  ENGINE_LOG("Optimizing w.r.t. the current heuristic cost...");
  _milpEncoder->encodeCostFunction(*_gurobi, costFunction);
  _gurobi->setTimeLimit(FloatUtils::infinity());
  _gurobi->setMethod(2); // Use barrier method for optimization
  _gurobi->solve();
  ENGINE_LOG("Optimizing w.r.t. the current heuristic cost - done");
  struct timespec end = TimeUtils::sampleMicro();
  _statistics.incLongAttribute
    (Statistics::TOTAL_TIME_MINIMIZING_SOI_COST_WITH_GUROBI_MICRO,
     TimeUtils::timePassed(start, end));

  if (_gurobi->infeasible()) {
    throw SoyError(SoyError::INFEASIBILITY_DURING_OPTIMIZATION);
  } else if (_gurobi->optimal()) {
    ENGINE_LOG("Optimal value found");
    start = TimeUtils::sampleMicro();
    _assignmentManager->extractAssignmentFromGurobi
      (*_gurobi, _variablesParticipatingInPLConstraints);
    _assignmentManager->extractReducedCostFromGurobi
      (*_gurobi, _variablesParticipatingInPLConstraints);
    end = TimeUtils::sampleMicro();
    _statistics.incLongAttribute
      (Statistics::TOTAL_TIME_OBTAIN_CURRENT_ASSIGNMENT_SOI_MICRO,
       TimeUtils::timePassed(start, end));
    ENGINE_LOG(
        Stringf("Optimal value: %.2f", _gurobi->getObjectiveValue()).ascii());
    return true;
  } else
    throw CommonError(
        CommonError::UNEXPECTED_GUROBI_STATUS,
        Stringf("Current status: %u", _gurobi->getStatusCode()).ascii());

  return false;
}

void Engine::clearViolatedPLConstraints() { _violatedPlConstraints.clear(); }

bool Engine::applyAllValidConstraintCaseSplits() {
  struct timespec start = TimeUtils::sampleMicro();
  bool appliedSplit = false;
  for (auto &constraint : _plConstraints)
    if (applyValidConstraintCaseSplit(constraint)) appliedSplit = true;

  struct timespec end = TimeUtils::sampleMicro();
  _statistics.incLongAttribute(
      Statistics::TOTAL_TIME_PERFORMING_VALID_CASE_SPLITS_MICRO,
      TimeUtils::timePassed(start, end));

  return appliedSplit;
}

bool Engine::applyValidConstraintCaseSplit(PLConstraint *constraint) {
  if (constraint->isActive() && constraint->phaseFixed()) {
    String constraintString;
    constraint->dump(constraintString);
    ENGINE_LOG(Stringf("A constraint has become valid. Dumping constraint: %s",
                       constraintString.ascii())
                   .ascii());

    constraint->setActive(false);
    PiecewiseLinearCaseSplit validSplit = constraint->getValidCaseSplit();
    applySplit(validSplit);
    _soiManager->removeCostComponentFromHeuristicCost(constraint);

    return true;
  }

  return false;
}

void Engine::collectViolatedPlConstraints() {
  _violatedPlConstraints.clear();
  for (const auto &constraint : _plConstraints) {
    if (constraint->isActive() && !constraint->satisfied())
      _violatedPlConstraints.append(constraint);
  }
}

bool Engine::allPlConstraintsHold() const { return _violatedPlConstraints.empty(); }

void Engine::dumpConstraintsStatus() const {
  String s;
  for (const auto &plConstraint : _plConstraints) {
    plConstraint->dump(s);
  }
  printf("%s", s.ascii());
}

void Engine::informLPSolverOfBounds() {
  struct timespec start = TimeUtils::sampleMicro();
  for (unsigned i = 0; i < _preprocessedQuery->getNumberOfVariables(); ++i) {
    String variableName = _milpEncoder->getVariableNameFromVariable(i);
    _gurobi->setLowerBound(variableName, _boundManager.getLowerBound(i));
    _gurobi->setUpperBound(variableName, _boundManager.getUpperBound(i));
  }
  _gurobi->updateModel();
  struct timespec end = TimeUtils::sampleMicro();
  _statistics.incLongAttribute(
      Statistics::TIME_ADDING_CONSTRAINTS_TO_MILP_SOLVER_MICRO,
      TimeUtils::timePassed(start, end));
}

void Engine::checkGurobiBoundConsistency() const {
  if (_gurobi && _milpEncoder) {
    for (unsigned i = 0; i < _preprocessedQuery->getNumberOfVariables(); ++i) {
      String iName = _milpEncoder->getVariableNameFromVariable(i);
      double gurobiLowerBound = _gurobi->getLowerBound(iName);
      double lowerBound = _boundManager.getLowerBound(i);
      if (!FloatUtils::areEqual(gurobiLowerBound, lowerBound)) {
        throw SoyError(SoyError::BOUNDS_NOT_UP_TO_DATE_IN_LP_SOLVER,
                           Stringf("x%u lower bound inconsistent!"
                                   " Gurobi: %f, Tableau: %f",
                                   i, gurobiLowerBound, lowerBound)
                               .ascii());
      }
      double gurobiUpperBound = _gurobi->getUpperBound(iName);
      double upperBound = _boundManager.getUpperBound(i);

      if (!FloatUtils::areEqual(gurobiUpperBound, upperBound)) {
        throw SoyError(SoyError::BOUNDS_NOT_UP_TO_DATE_IN_LP_SOLVER,
                           Stringf("x%u upper bound inconsistent!"
                                   " Gurobi: %f, Tableau: %f",
                                   i, gurobiUpperBound, upperBound)
                               .ascii());
      }
    }
  }
}

void Engine::applySplit(const PiecewiseLinearCaseSplit &split) {
  ENGINE_LOG("");
  ENGINE_LOG("Applying a split. ");

  List<Tightening> bounds = split.getBoundTightenings();

  for (auto &bound : bounds) {
    unsigned variable = bound._variable;

    if (bound._type == Tightening::LB) {
      ENGINE_LOG(
          Stringf("x%u: lower bound set to %.3lf", variable, bound._value)
              .ascii());
      _boundManager.tightenLowerBound(variable, bound._value);
    } else {
      ENGINE_LOG(
          Stringf("x%u: upper bound set to %.3lf", variable, bound._value)
              .ascii());
      _boundManager.tightenUpperBound(variable, bound._value);
    }
  }

  ENGINE_LOG("Done with split\n");
}

PLConstraint *Engine::pickSplitPLConstraint(DivideStrategy strategy) {
  ENGINE_LOG(Stringf("Picking a split PLConstraint...").ascii());

  PLConstraint *candidatePLConstraint = NULL;
  if (strategy == DivideStrategy::PseudoImpact)
    candidatePLConstraint = _smtCore.getConstraintsWithHighestScore();
  if (strategy == DivideStrategy::Topological) {
    for (const auto &p : _plConstraints) {
      if (p->isActive()) {
        candidatePLConstraint = p;
        break;
      }
    }
  }
  if (!candidatePLConstraint)
    throw SoyError(SoyError::UNABLE_TO_PICK_SPLIT_PLCONSTRAINT,
                       "Unable to pick branching pl constraints.");
  ENGINE_LOG("Picked");
  return candidatePLConstraint;
}

void Engine::preContextPushHook() {}

void Engine::postContextPopHook() {}

void Engine::extractTheoryExplanation(bool naive) {
  struct timespec start = TimeUtils::sampleMicro();

  if (_context.getLevel() == 0) return;

  if (naive) {
    // The full partial assignment is the conflict
    _smtCore.extractNaiveConflict();
  } else {
    ASSERT(_gurobi->infeasible());
    ENGINE_LOG("Extracting theory explanation...");
    _gurobi->computeIIS();

    Map<String, GurobiWrapper::IISBoundType> bounds;
    List<String> dontCare;
    List<String> names;
    _gurobi->extractIIS(bounds, dontCare, names);
    _smtCore.extractConflict(bounds, _boundManager);
  }

  _smtCore.incrementConflictCount();

  // Add to sat solver
  List<int> clause;
  for (auto const &pair : _smtCore.getCurrentConflict()._literals)
    clause.append(-pair.first->getLiteralOfPhaseStatus(pair.second));
  _cadical->addConstraint(clause);

  DEBUG(checkTheoryLemmaCorrectness());

  // Add to theory solver
  if (_smtCore.getCurrentConflict()._literals.size() <=
      GlobalConfiguration::THEORY_LEMMA_LENGTH_THRESHOLD) {
    Equation eq(Equation::LE);
    for (auto const &pair : _smtCore.getCurrentConflict()._literals) {
      OneHotConstraint *c = (OneHotConstraint *)pair.first;
      eq.addAddend(1, c->getElementOfPhase(pair.second));
      eq._scalar += 1;
    }
    eq._scalar -= 1;
    _preprocessedQuery->addEquation(eq);
    _milpEncoder->encodeEquation(*_gurobi, eq);
    _statistics.incUnsignedAttribute(Statistics::NUM_EQUATIONS);
  }

  // VSIDS
  if ( Options::get()->getBool(Options::VSIDS)){
      for (auto const &pair : _smtCore.getCurrentConflict()._literals){
        _smtCore.updatePLConstraintScore(pair.first, 1);
        pair.first->updatePhaseStatusScore(pair.second, 1);
      }
    }

  ENGINE_LOG("Extracting theory explanation - done");
  struct timespec end = TimeUtils::sampleMicro();
  _statistics.incLongAttribute(Statistics::TIME_THEORY_EXPLANATION_MICRO,
                               TimeUtils::timePassed(start, end));
}

Engine::ExitCode Engine::getExitCode() const { return _exitCode; }

void Engine::extractSolution(InputQuery &inputQuery) {
  for (unsigned i = 0; i < inputQuery.getNumberOfVariables(); ++i) {
    inputQuery.setSolutionValue(i, _assignmentManager->getAssignment(i));
  }
}

void Engine::quitSignal() { _quitRequested = true; }

void Engine::mainLoopStatistics() {
  struct timespec start = TimeUtils::sampleMicro();

  unsigned activeConstraints = 0;
  for (const auto &constraint : _plConstraints)
    if (constraint->isActive()) ++activeConstraints;

  _statistics.setUnsignedAttribute(Statistics::NUM_ACTIVE_PL_CONSTRAINTS,
                                   activeConstraints);

  _statistics.incLongAttribute(Statistics::NUM_MAIN_LOOP_ITERATIONS);

  struct timespec end = TimeUtils::sampleMicro();
  _statistics.incLongAttribute(Statistics::TOTAL_TIME_HANDLING_STATISTICS_MICRO,
                               TimeUtils::timePassed(start, end));
}

bool Engine::shouldExitDueToTimeout(unsigned timeout) const {
  // A timeout value of 0 means no time limit
  if (timeout == 0) return false;

  return _statistics.getTotalTimeInMicro() / MICROSECONDS_TO_SECONDS > timeout;
}

void Engine::setVerbosity(unsigned verbosity) { _verbosity = verbosity; }

void Engine::setRandomSeed(unsigned seed) { srand(seed); }

const Statistics *Engine::getStatistics() const { return &_statistics; }

InputQuery *Engine::getInputQuery() { return &(*_preprocessedQuery); }

SmtCore *Engine::getSmtCore() { return &_smtCore; }

std::atomic_bool *Engine::getQuitRequested() { return &_quitRequested; }
