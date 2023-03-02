/*********************                                                        */
/*! \file SoIManager.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Haoze (Andrew) Wu
 ** This file is part of the Soy project.
 ** Copyright (c) 2023 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#include "SoIManager.h"

#include "AssignmentManager.h"
#include "BoundManager.h"
#include "CadicalWrapper.h"
#include "Color.h"
#include "FloatUtils.h"
#include "GlobalConfiguration.h"
#include "GurobiWrapper.h"
#include "InfeasibleQueryException.h"
#include "LocalSearchPlateauException.h"
#include "SoyError.h"
#include "OneHotConstraint.h"
#include "Options.h"
#include "Set.h"
#include "SmtCore.h"

SoIManager::SoIManager(const InputQuery &inputQuery)
    : _plConstraints(inputQuery.getPLConstraints()),
      _initializationStrategy(Options::get()->getSoIInitializationStrategy()),
      _searchStrategy(Options::get()->getSoISearchStrategy()),
      _probabilityDensityParameter(
          Options::get()->getFloat(Options::PROBABILITY_DENSITY_PARAMETER)),
      _statistics(NULL),
      _assignmentManager(NULL),
      _satSolver(NULL),
      _smtCore(NULL),
      _tabuLength(Options::get()->getInt(Options::TABU)){}

void SoIManager::resetPhasePattern() {
  _currentPhasePattern.clear();
  _lastAcceptedPhasePattern.clear();
  _plConstraintsInCurrentPhasePattern.clear();
  _constraintsUpdatedInLastProposal.clear();
  _cachedPatternInfo.clear();
}

LinearExpression SoIManager::getCurrentSoIPhasePattern() const {
  struct timespec start = TimeUtils::sampleMicro();

  LinearExpression cost;
  for (const auto &pair : _currentPhasePattern)
    pair.first->getCostFunctionComponent(cost, pair.second);

  if (_statistics) {
    struct timespec end = TimeUtils::sampleMicro();
    _statistics->incLongAttribute(
        Statistics::TOTAL_TIME_GETTING_SOI_PHASE_PATTERN_MICRO,
        TimeUtils::timePassed(start, end));
  }
  return cost;
}

LinearExpression SoIManager::getLastAcceptedSoIPhasePattern() const {
  struct timespec start = TimeUtils::sampleMicro();

  LinearExpression cost;
  for (const auto &pair : _lastAcceptedPhasePattern)
    pair.first->getCostFunctionComponent(cost, pair.second);

  if (_statistics) {
    struct timespec end = TimeUtils::sampleMicro();
    _statistics->incLongAttribute(
        Statistics::TOTAL_TIME_GETTING_SOI_PHASE_PATTERN_MICRO,
        TimeUtils::timePassed(start, end));
  }
  return cost;
}

void SoIManager::initializePhasePattern() {
  struct timespec start = TimeUtils::sampleMicro();

  resetPhasePattern();

  if (_initializationStrategy ==
      SoIInitializationStrategy::CURRENT_ASSIGNMENT) {
    initializePhasePatternWithCurrentAssignment();
  } else if (_initializationStrategy ==
             SoIInitializationStrategy::CURRENT_ASSIGNMENT_SAT) {
    initializePhasePatternWithCurrentAssignmentSat();
  } else {
    throw SoyError(SoyError::UNABLE_TO_INITIALIZATION_PHASE_PATTERN);
  }

  // Store constraints participating in the SoI
  for (const auto &pair : _currentPhasePattern)
    _plConstraintsInCurrentPhasePattern.append(pair.first);

  // The first phase pattern is always accepted.
  _lastAcceptedPhasePattern = _currentPhasePattern;

  if (_statistics) {
    struct timespec end = TimeUtils::sampleMicro();
    _statistics->incLongAttribute(
        Statistics::TOTAL_TIME_UPDATING_SOI_PHASE_PATTERN_MICRO,
        TimeUtils::timePassed(start, end));
    _statistics->incUnsignedAttribute(
        Statistics::NUM_PHASE_PATTERN_INITIALIZATIONS);
    _statistics->setLongAttribute(
        Statistics::NUM_PROPOSALS_SINCE_LAST_REINITIALIZATION, 0);
  }
}

void SoIManager::initializePhasePatternWithCurrentAssignment() {
  for (const auto &plConstraint : _plConstraints) {
    ASSERT(!_currentPhasePattern.exists(plConstraint));
    if (plConstraint->supportSoI() && plConstraint->isActive() &&
        !plConstraint->phaseFixed()) {
      // Set the phase status corresponding to the current assignment.
      _currentPhasePattern[plConstraint] =
          plConstraint->getPhaseStatusInAssignment();
    }
  }
}

void SoIManager::initializePhasePatternWithCurrentAssignmentSat() {
  for (const auto &plConstraint : _plConstraints) {
    ASSERT(!_currentPhasePattern.exists(plConstraint));
    if (plConstraint->supportSoI() && plConstraint->isActive() &&
        !plConstraint->phaseFixed()) {
      // Set the phase status corresponding to the current assignment.
      _currentPhasePattern[plConstraint] =
          plConstraint->getPhaseStatusInAssignment();
    }
  }

  std::vector<std::pair<PLConstraint *, PhaseStatus>> temp;
  for (const auto &pair : _currentPhasePattern) {
    temp.push_back(pair);
  }
  std::random_shuffle(temp.begin(), temp.end());

  for (const auto &pair : temp) {
    _satSolver->setDirection(pair.first->getLiteralOfPhaseStatus(pair.second));
  }

  unsigned numDiffers = _satSolver->directionAwareSolve();
  _satSolver->resetAllDirections();

  if (numDiffers > 0) {
    if (_statistics) {
      _statistics->incUnsignedAttribute(
          Statistics::NUM_INITIALIZATIONS_REJECTED_BY_SAT_SOLVER);
    }

    for (const auto &pair : _currentPhasePattern) {
      for (const auto &phase : pair.first->getAllCases()) {
        if (_satSolver->getAssignment(
                pair.first->getLiteralOfPhaseStatus(phase))) {
          _currentPhasePattern[pair.first] = phase;
          break;
        }
      }
    }
  }
}

void SoIManager::proposePhasePatternUpdate() {
  struct timespec start = TimeUtils::sampleMicro();

  _currentPhasePattern = _lastAcceptedPhasePattern;
  _constraintsUpdatedInLastProposal.clear();

  if (_searchStrategy == SoISearchStrategy::MCMC) {
    proposePhasePatternUpdateRandomly();
  } else if (_searchStrategy == SoISearchStrategy::GREEDY) {
    proposePhasePatternUpdateGreedy();
  } else if (_searchStrategy == SoISearchStrategy::GREEDY_SAT) {
    proposePhasePatternUpdateGreedySat();
  } else if (_searchStrategy == SoISearchStrategy::PROP) {
    proposePhasePatternUpdateProp();
  } else {
    // Walksat
    proposePhasePatternUpdateWalksat();
  }

  if (_currentPhasePattern == _lastAcceptedPhasePattern)
    throw LocalSearchPlateauException();

  if (_statistics) {
    struct timespec end = TimeUtils::sampleMicro();
    _statistics->incLongAttribute(
        Statistics::NUM_PROPOSED_PHASE_PATTERN_UPDATE);
    _statistics->incLongAttribute(
        Statistics::NUM_PROPOSALS_SINCE_LAST_REINITIALIZATION);
    _statistics->incLongAttribute(
        Statistics::TOTAL_TIME_UPDATING_SOI_PHASE_PATTERN_MICRO,
        TimeUtils::timePassed(start, end));
  }
}

void SoIManager::proposePhasePatternUpdateRandomly() {
  SOI_LOG("Proposing phase pattern update randomly...");
  DEBUG({
    // _plConstraintsInCurrentPhasePattern should contain the same
    // plConstraints in _currentPhasePattern
    ASSERT(_plConstraintsInCurrentPhasePattern.size() ==
           _currentPhasePattern.size());
    for (const auto &pair : _currentPhasePattern)
      ASSERT(_plConstraintsInCurrentPhasePattern.exists(pair.first));
  });

  // First, pick a pl constraint whose cost component we will update.
  unsigned index =
      (unsigned)T::rand() % _plConstraintsInCurrentPhasePattern.size();
  PLConstraint *plConstraintToUpdate =
      _plConstraintsInCurrentPhasePattern[index];

  // Next, pick an alternative phase.
  PhaseStatus currentPhase = _currentPhasePattern[plConstraintToUpdate];
  List<PhaseStatus> allPhases = plConstraintToUpdate->getAllFeasibleCases();
  allPhases.erase(currentPhase);
  if (allPhases.size() == 1) {
    // There are only two possible phases. So we just flip the phase.
    PhaseStatus phase = *(allPhases.begin());
    _currentPhasePattern[plConstraintToUpdate] = phase;
    _constraintsUpdatedInLastProposal[plConstraintToUpdate] = phase;
  } else {
    auto it = allPhases.begin();
    unsigned index = (unsigned)T::rand() % allPhases.size();
    while (index > 0) {
      ++it;
      --index;
    }
    PhaseStatus phase = *it;
    _currentPhasePattern[plConstraintToUpdate] = phase;
    _constraintsUpdatedInLastProposal[plConstraintToUpdate] = phase;
  }

  SOI_LOG("Proposing phase pattern update randomly - done");
}

void SoIManager::proposePhasePatternUpdateGreedy() {
  SOI_LOG("Proposing phase pattern update greedily...");
  DEBUG({
    // _plConstraintsInCurrentPhasePattern should contain the same
    // plConstraints in _currentPhasePattern
    ASSERT(_plConstraintsInCurrentPhasePattern.size() ==
           _currentPhasePattern.size());
    for (const auto &pair : _currentPhasePattern)
      ASSERT(_plConstraintsInCurrentPhasePattern.exists(pair.first));
  });

  // First, pick a pl constraint whose cost component we will update.
  Vector<PLConstraint *> unsatisfiedConstraints;
  for (const auto &plConstraint : _plConstraintsInCurrentPhasePattern)
    if (!plConstraint->satisfied()) unsatisfiedConstraints.append(plConstraint);

  unsigned index = (unsigned)rand() % unsatisfiedConstraints.size();
  PLConstraint *plConstraintToUpdate = unsatisfiedConstraints[index];

  // Next, pick an alternative phase.
  PhaseStatus currentPhase = _currentPhasePattern[plConstraintToUpdate];
  List<PhaseStatus> allPhases = plConstraintToUpdate->getAllFeasibleCases();
  allPhases.erase(currentPhase);
  if (allPhases.size() == 1) {
    // There are only two possible phases. So we just flip the phase.
    PhaseStatus phase = *(allPhases.begin());
    _currentPhasePattern[plConstraintToUpdate] = phase;
    _constraintsUpdatedInLastProposal[plConstraintToUpdate] = phase;
  } else {
    auto it = allPhases.begin();
    unsigned index = (unsigned)rand() % allPhases.size();
    while (index > 0) {
      ++it;
      --index;
    }
    PhaseStatus phase = *it;

    // double reducedCost = 0;
    // PhaseStatus phase = PHASE_NOT_FIXED;
    // getCostReduction(plConstraintToUpdate, reducedCost, phase);

    _currentPhasePattern[plConstraintToUpdate] = phase;
    _constraintsUpdatedInLastProposal[plConstraintToUpdate] = phase;
  }

  SOI_LOG("Proposing phase pattern update greedily - done");
}

void SoIManager::proposePhasePatternUpdateGreedySat() {
  SOI_LOG("Proposing phase pattern update with sat solver...");
  DEBUG({
    // _plConstraintsInCurrentPhasePattern should contain the same
    // plConstraints in _currentPhasePattern
    ASSERT(_plConstraintsInCurrentPhasePattern.size() ==
           _currentPhasePattern.size());
    for (const auto &pair : _currentPhasePattern)
      ASSERT(_plConstraintsInCurrentPhasePattern.exists(pair.first));
  });

  // First, pick a pl constraint whose cost component we will update.
  Vector<PLConstraint *> unsatisfiedConstraints;
  for (const auto &plConstraint : _plConstraintsInCurrentPhasePattern)
    if (!plConstraint->satisfied()) unsatisfiedConstraints.append(plConstraint);

  PLConstraint *plConstraintToUpdate = nullptr;
  PhaseStatus phase;
  unsigned attempts = 10;
  do {
      unsigned index = (unsigned)rand() % unsatisfiedConstraints.size();
      plConstraintToUpdate = unsatisfiedConstraints[index];

      // Next, pick an alternative phase.
      PhaseStatus currentPhase = _currentPhasePattern[plConstraintToUpdate];
      List<PhaseStatus> allPhases = plConstraintToUpdate->getAllFeasibleCases();
      allPhases.erase(currentPhase);
      if (allPhases.size() == 1) {
        // There are only two possible phases. So we just flip the phase.
        phase = *(allPhases.begin());
        _currentPhasePattern[plConstraintToUpdate] = phase;
        _constraintsUpdatedInLastProposal[plConstraintToUpdate] = phase;
      } else {
        auto it = allPhases.begin();
        unsigned index = (unsigned)rand() % allPhases.size();
        while (index > 0) {
          ++it;
          --index;
        }
        phase = *it;
      }
  } while (remembered(plConstraintToUpdate, phase) && attempts--);

  remember(plConstraintToUpdate, phase);

  std::vector<std::pair<PLConstraint *, PhaseStatus>> temp;
  for (const auto &pair : _currentPhasePattern) {
    temp.push_back(pair);
  }
  std::random_shuffle(temp.begin(), temp.end());

  for (const auto &pair : temp) {
    if (pair.first != plConstraintToUpdate)
      _satSolver->setDirection(
          pair.first->getLiteralOfPhaseStatus(pair.second));
  }
  _satSolver->setDirection(
      plConstraintToUpdate->getLiteralOfPhaseStatus(phase));

  unsigned numDiffers = _satSolver->directionAwareSolve();
  _satSolver->resetAllDirections();

  if (numDiffers > 0) {
    if (_statistics) {
      _statistics->incUnsignedAttribute(
          Statistics::NUM_PROPOSALS_REJECTED_BY_SAT_SOLVER);
    }

    for (const auto &pair : _currentPhasePattern) {
      for (const auto &phase : pair.first->getAllCases()) {
        if (_satSolver->getAssignment(
                pair.first->getLiteralOfPhaseStatus(phase))) {
          if (pair.second != phase) {
            _constraintsUpdatedInLastProposal[pair.first] = phase;
            _currentPhasePattern[pair.first] = phase;
          }
          break;
        }
      }
    }
  } else {
    _constraintsUpdatedInLastProposal[plConstraintToUpdate] = phase;
    _currentPhasePattern[plConstraintToUpdate] = phase;
  }

  SOI_LOG("Proposing phase pattern update with sat solver - done");
}

void SoIManager::proposePhasePatternUpdateProp() {
  SOI_LOG("Proposing phase pattern update with sat solver...");
  DEBUG({
    // _plConstraintsInCurrentPhasePattern should contain the same
    // plConstraints in _currentPhasePattern
    ASSERT(_plConstraintsInCurrentPhasePattern.size() ==
           _currentPhasePattern.size());
    for (const auto &pair : _currentPhasePattern)
      ASSERT(_plConstraintsInCurrentPhasePattern.exists(pair.first));
  });

  // First, pick a pl constraint whose cost component we will update.
  Vector<PLConstraint *> unsatisfiedConstraints;
  for (const auto &plConstraint : _plConstraintsInCurrentPhasePattern)
    if (!plConstraint->satisfied()) unsatisfiedConstraints.append(plConstraint);

  PLConstraint *plConstraintToUpdate = nullptr;
  PhaseStatus phase;
  do {
      unsigned index = (unsigned)rand() % unsatisfiedConstraints.size();
      PLConstraint *plConstraintToUpdate = unsatisfiedConstraints[index];

      // Next, pick an alternative phase.
      PhaseStatus currentPhase = _currentPhasePattern[plConstraintToUpdate];
      List<PhaseStatus> allPhases = plConstraintToUpdate->getAllFeasibleCases();
      allPhases.erase(currentPhase);
      if (allPhases.size() == 1) {
        // There are only two possible phases. So we just flip the phase.
        phase = *(allPhases.begin());
        _currentPhasePattern[plConstraintToUpdate] = phase;
        _constraintsUpdatedInLastProposal[plConstraintToUpdate] = phase;
      } else {
        auto it = allPhases.begin();
        unsigned index = (unsigned)rand() % allPhases.size();
        while (index > 0) {
          ++it;
          --index;
        }
        phase = *it;
      }
  } while (remembered(plConstraintToUpdate, phase));

  remember(plConstraintToUpdate, phase);

  std::vector<std::pair<PLConstraint *, PhaseStatus>> temp;
  for (const auto &pair : _currentPhasePattern) {
    temp.push_back(pair);
  }
  std::random_shuffle(temp.begin(), temp.end());

  for (const auto &pair : temp) {
    if (pair.first != plConstraintToUpdate)
      _satSolver->setDirection(
          pair.first->getLiteralOfPhaseStatus(pair.second));
  }
  _satSolver->setDirection(
      plConstraintToUpdate->getLiteralOfPhaseStatus(phase));

  unsigned numDiffers = _satSolver->directionAwareSolve();
  _satSolver->resetAllDirections();

  if (numDiffers > 0) {
    if (_statistics) {
      _statistics->incUnsignedAttribute(
          Statistics::NUM_PROPOSALS_REJECTED_BY_SAT_SOLVER);
    }

    for (const auto &pair : _currentPhasePattern) {
      for (const auto &phase : pair.first->getAllCases()) {
        if (_satSolver->getAssignment(
                pair.first->getLiteralOfPhaseStatus(phase))) {
          if (pair.second != phase) {
            _constraintsUpdatedInLastProposal[pair.first] = phase;
            _currentPhasePattern[pair.first] = phase;
          }
          break;
        }
      }
    }
  } else {
    _constraintsUpdatedInLastProposal[plConstraintToUpdate] = phase;
    _currentPhasePattern[plConstraintToUpdate] = phase;
  }

  SOI_LOG("Proposing phase pattern update with sat solver - done");
}

void SoIManager::proposePhasePatternUpdateWalksat() {
  SOI_LOG("Proposing phase pattern update with Walksat-based strategy...");

  // Flip to the cost term that reduces the cost by the most
  PLConstraint *plConstraintToUpdate = NULL;
  PhaseStatus updatedPhase = PHASE_NOT_FIXED;
  double maxReducedCost = 0;
  for (const auto &plConstraint : _plConstraintsInCurrentPhasePattern) {
    double reducedCost = 0;
    PhaseStatus phaseStatusOfReducedCost = PHASE_NOT_FIXED;
    getCostReduction(plConstraint, reducedCost, phaseStatusOfReducedCost);

    if (reducedCost > maxReducedCost) {
      maxReducedCost = reducedCost;
      plConstraintToUpdate = plConstraint;
      updatedPhase = phaseStatusOfReducedCost;
    }
  }

  if (plConstraintToUpdate) {
    _currentPhasePattern[plConstraintToUpdate] = updatedPhase;
    _constraintsUpdatedInLastProposal[plConstraintToUpdate] = updatedPhase;
  } else {
    proposePhasePatternUpdateRandomly();
  }
  SOI_LOG("Proposing phase pattern update with Walksat-based strategy - done");
}

bool SoIManager::decideToAcceptCurrentProposal(
    double costOfCurrentPhasePattern, double costOfProposedPhasePattern) {
  costOfProposedPhasePattern += GlobalConfiguration::REGULARIZATION_FOR_SOI_COST;
  if (costOfProposedPhasePattern <= costOfCurrentPhasePattern) {
    return true;
  } else {
    // The smaller the difference between the proposed phase pattern and the
    // current phase pattern, the more likely to accept the proposal.
    double prob =
        exp(-_probabilityDensityParameter *
            (costOfProposedPhasePattern - costOfCurrentPhasePattern));
    return ((double)T::rand() / RAND_MAX) < prob;
  }
}

void SoIManager::acceptCurrentPhasePattern() {
  struct timespec start = TimeUtils::sampleMicro();

  _lastAcceptedPhasePattern = _currentPhasePattern;
  _constraintsUpdatedInLastProposal.clear();

  if (_statistics) {
    struct timespec end = TimeUtils::sampleMicro();
    _statistics->incLongAttribute(
        Statistics::TOTAL_TIME_UPDATING_SOI_PHASE_PATTERN_MICRO,
        TimeUtils::timePassed(start, end));
  }
}

void SoIManager::updateCurrentPhasePatternForSatisfiedPLConstraints() {
  for (const auto &pair : _currentPhasePattern) {
    if (pair.first->satisfied()) {
      PhaseStatus satisfiedPhaseStatus =
          pair.first->getPhaseStatusInAssignment();
      _currentPhasePattern[pair.first] = satisfiedPhaseStatus;
    }
  }
}

void SoIManager::removeCostComponentFromHeuristicCost(
    PLConstraint *constraint) {
  if (_currentPhasePattern.exists(constraint)) {
    _currentPhasePattern.erase(constraint);
    _lastAcceptedPhasePattern.erase(constraint);
    ASSERT(_plConstraintsInCurrentPhasePattern.exists(constraint));
    _plConstraintsInCurrentPhasePattern.erase(constraint);
  }
}

void SoIManager::setStatistics(Statistics *statistics) {
  _statistics = statistics;
}

void SoIManager::setAssignmentManager(AssignmentManager *assignmentManager) {
  _assignmentManager = assignmentManager;
}

void SoIManager::setSatSolver(CadicalWrapper *cadical) { _satSolver = cadical; }

void SoIManager::setSmtCore(SmtCore *smtCore) { _smtCore = smtCore; }

void SoIManager::setPhaseStatusInLastAcceptedPhasePattern(
    PLConstraint *constraint, PhaseStatus phase) {
  ASSERT(_lastAcceptedPhasePattern.exists(constraint) &&
         _plConstraintsInCurrentPhasePattern.exists(constraint));
  _lastAcceptedPhasePattern[constraint] = phase;
}

void SoIManager::setPhaseStatusInCurrentPhasePattern(PLConstraint *constraint,
                                                     PhaseStatus phase) {
  ASSERT(_currentPhasePattern.exists(constraint) &&
         _plConstraintsInCurrentPhasePattern.exists(constraint));
  _currentPhasePattern[constraint] = phase;
}

void SoIManager::getCostReduction(PLConstraint *plConstraint,
                                  double &reducedCost,
                                  PhaseStatus &phaseOfReducedCost) const {
  ASSERT(_currentPhasePattern.exists(plConstraint));
  SOI_LOG("Computing reduced cost for the current constraint...");

  // Get the list of alternative phases.
  PhaseStatus currentPhase = _currentPhasePattern[plConstraint];
  List<PhaseStatus> allPhases = plConstraint->getAllCases();
  allPhases.erase(currentPhase);
  ASSERT(allPhases.size() > 0);  // Otherwise, the constraint must be fixed.
  SOI_LOG(
      Stringf("Number of alternative phases: %u", allPhases.size()).ascii());

  // Compute the violation of the plConstraint w.r.t. the current assignment
  // and the current cost component.
  LinearExpression costComponent;
  plConstraint->getCostFunctionComponent(costComponent, currentPhase);
  double currentCost =
      costComponent.evaluate(_assignmentManager->getAssignments());

  // Next we iterate over alternative phases to see whether some phases
  // might reduce the cost and by how much.
  reducedCost = FloatUtils::negativeInfinity();
  phaseOfReducedCost = PHASE_NOT_FIXED;
  for (const auto &phase : allPhases) {
    LinearExpression otherCostComponent;
    plConstraint->getCostFunctionComponent(otherCostComponent, phase);
    double otherCost =
        otherCostComponent.evaluate(_assignmentManager->getAssignments());
    double currentReducedCost = currentCost - otherCost;
    SOI_LOG(Stringf("Reduced cost of phase %u: %.2f", phase, currentReducedCost)
                .ascii());
    if (FloatUtils::gt(currentReducedCost, reducedCost)) {
      reducedCost = currentReducedCost;
      phaseOfReducedCost = phase;
    }
  }
  ASSERT(reducedCost != FloatUtils::infinity());
  SOI_LOG(
      Stringf("Largest reduced cost is %.2f and achieved with phase status %u",
              reducedCost, phaseOfReducedCost)
          .ascii());
  SOI_LOG("Computing reduced cost for the current constraint - done");
}

void SoIManager::dumpCurrentPattern() const {
  unsigned i = 0;
  for (const auto &constraint : _plConstraints) {
    if (_currentPhasePattern.exists(constraint)) {
      if (_constraintsUpdatedInLastProposal.exists(constraint))
        std::cout << YELLOW << i++ << "("
                  << static_cast<unsigned>(_currentPhasePattern[constraint])
                  << RESET;
      else
        std::cout << i++ << "("
                  << static_cast<unsigned>(_currentPhasePattern[constraint])
                  << RESET;
      if (!constraint->satisfied())
        std::cout << RED << "U" << RESET << ") ";
      else
        std::cout << "S" << RESET << ") ";
    } else {
      std::cout << BLUE << i++ << "("
                << static_cast<unsigned>(constraint->getNextFeasibleCase());
      std::cout << "F" << RESET << ") ";
    }

  }
  std::cout << std::endl;
}

void SoIManager::cacheCurrentPhasePattern(double cost) {
  String s;
  phasePatternString(s);

  SOI_LOG(Stringf("Caching phase pattern: %s", s.ascii()).ascii());

  if (_cachedPatternInfo.exists(s)) {
    ASSERT(FloatUtils::areEqual(cost, _cachedPatternInfo[s]._cost, 0.01));
    return;
  }

  SoIManager::PhasePatternInfo info;
  info._cost = cost;

  for (const auto &plConstraint : _plConstraints) {
    for (const auto &var : plConstraint->getParticipatingVariables()) {
      info._assignment[var] = _assignmentManager->getAssignment(var);
    }
  }

  _cachedPatternInfo[s] = info;
}

bool SoIManager::loadCachedPhasePattern(double &cost) {
  String s;
  phasePatternString(s);

  if (_cachedPatternInfo.exists(s)) {
    SOI_LOG(Stringf("Loading cached for phase pattern %s", s.ascii()).ascii());
    const auto &info = _cachedPatternInfo[s];
    cost = info._cost;
    for (const auto &pair : info._assignment)
      _assignmentManager->setAssignment(pair.first, pair.second);
    return true;
  } else {
    SOI_LOG(Stringf("Phase pattern %s not cached", s.ascii()).ascii());
    return false;
  }
}

bool SoIManager::currentPhasePatternCached() {
  String s;
  phasePatternString(s);
  return _cachedPatternInfo.exists(s);
}

void SoIManager::phasePatternString(String &s) {
  s = "";
  for (const auto &plConstraint : _plConstraints) {
    if (_currentPhasePattern.exists(plConstraint))
      s += Stringf("%u-",
                   static_cast<unsigned>(_currentPhasePattern[plConstraint]));
  }
}

void SoIManager::addCurrentPhasePatternAsConflict(CadicalWrapper &cadical) {
  SOI_LOG("Adding current phase pattern as conflict");
  List<int> clause;
  for (const auto &plConstraint : _plConstraints) {
    if (_currentPhasePattern.exists(plConstraint))
      clause.append(-plConstraint->getLiteralOfPhaseStatus(
          _currentPhasePattern[plConstraint]));
    else
      clause.append(-plConstraint->getLiteralOfPhaseStatus(
          plConstraint->getNextFeasibleCase()));
  }
  cadical.addConstraint(clause);
}
