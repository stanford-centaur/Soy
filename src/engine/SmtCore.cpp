#include "SmtCore.h"

#include "BoundManager.h"
#include "CadicalWrapper.h"
#include "Debug.h"
#include "Engine.h"
#include "FloatUtils.h"
#include "GlobalConfiguration.h"
#include "MStringf.h"
#include "SoyError.h"
#include "Options.h"
#include "PseudoImpactTracker.h"

SmtCore::SmtCore(Engine *engine)
    : _engine(engine),
      _context(_engine->getContext()),
      _needToSplit(false),
      _constraintForSplitting(NULL),
      _deepSoIRejectionThreshold(
          Options::get()->getInt(Options::DEEP_SOI_REJECTION_THRESHOLD)),
      _backJump(!Options::get()->getBool(Options::NO_CDCL)),
      _scoreTracker(nullptr),
      _numRejectedPhasePatternProposal(0),
      _stateId(0),
      _numberOfConflict(0),
      _numberOfRestarts(1),
      _initialConflictLimit(Options::get()->getInt(Options::RESTART_THESHOLD)),
      _statistics(NULL) {

  if (GlobalConfiguration::USE_LUBY_RESTART) {
    _conflictLimit = _initialConflictLimit * luby(_numberOfRestarts);
  } else {
    _conflictLimit = _initialConflictLimit;
  }
}

SmtCore::~SmtCore() { freeMemory(); }

void SmtCore::freeMemory() {
  for (const auto &entry : _trail) delete entry;
  _trail.clear();
}

void SmtCore::initializeScoreTrackerIfNeeded(
    const List<PLConstraint *> &plConstraints) {
  _scoreTracker =
      std::unique_ptr<PseudoImpactTracker>(new PseudoImpactTracker());
  _scoreTracker->initialize(plConstraints);

  SMT_LOG("\tTracking Pseudo Impact...");
}

void SmtCore::reportRejectedPhasePatternProposal() {
  ++_numRejectedPhasePatternProposal;

  if (_numRejectedPhasePatternProposal >= _deepSoIRejectionThreshold) {
    _needToSplit = true;
    if (!_constraintForSplitting)
      _constraintForSplitting =
          _engine->pickSplitPLConstraint(_branchingHeuristic);
  }
}

void SmtCore::resetSplitConditions() {
  _numRejectedPhasePatternProposal = 0;
  _needToSplit = false;
}

bool SmtCore::needToRestart() const {
    return _numberOfConflict >= _conflictLimit;
}

void SmtCore::restart() {
  struct timespec start = TimeUtils::sampleMicro();
  if (_statistics) {
    _statistics->incUnsignedAttribute(Statistics::NUM_RESTART);
  }

  freeMemory();
  resetSplitConditions();
  if (_branchingHeuristic == DivideStrategy::PseudoImpact)
    _scoreTracker->decayScores();

  _numberOfConflict = 0;
  ++_numberOfRestarts;
  if ( GlobalConfiguration::USE_LUBY_RESTART ) {
    _conflictLimit = _initialConflictLimit * luby(_numberOfRestarts);
  }

  _context.popto(0);
  struct timespec end = TimeUtils::sampleMicro();
  _statistics->incLongAttribute(Statistics::TOTAL_TIME_SMT_CORE_MICRO,
                                TimeUtils::timePassed(start, end));
}

bool SmtCore::needToSplit() const { return _needToSplit; }

void SmtCore::performSplit() {
  ASSERT(_needToSplit);

  // Maybe the constraint has already become inactive - if so, ignore
  if (!_constraintForSplitting->isActive()) {
    _needToSplit = false;
    _constraintForSplitting = NULL;
    return;
  }

  SMT_LOG("Performing split...");

  struct timespec start = TimeUtils::sampleMicro();

  ASSERT(_constraintForSplitting->isActive());
  _needToSplit = false;

  if (_statistics) {
    _statistics->incUnsignedAttribute(Statistics::NUM_SPLITS);
    _statistics->incUnsignedAttribute(Statistics::NUM_VISITED_TREE_STATES);
  }

  ++_stateId;
  _engine->preContextPushHook();
  _constraintForSplitting->setActive(false);
  _context.push();

  PhaseStatus phase = _constraintForSplitting->getNextFeasibleCase();
  _trail.append(new TrailEntry(_constraintForSplitting, phase));
  PiecewiseLinearCaseSplit split = _constraintForSplitting->getCaseSplit(phase);
  _engine->applySplit(split);

  if (_statistics) {
    unsigned level = getTrailLength();
    _statistics->setUnsignedAttribute(Statistics::CURRENT_DECISION_LEVEL,
                                      level);
    if (level >
        _statistics->getUnsignedAttribute(Statistics::MAX_DECISION_LEVEL))
      _statistics->setUnsignedAttribute(Statistics::MAX_DECISION_LEVEL, level);
    struct timespec end = TimeUtils::sampleMicro();
    _statistics->incLongAttribute(Statistics::TOTAL_TIME_SMT_CORE_MICRO,
                                  TimeUtils::timePassed(start, end));
  }

  _constraintForSplitting = NULL;

  SMT_LOG(Stringf("Performing split - done, current trail length: %u\n",
                  getTrailLength()).ascii());
}

bool SmtCore::popSplit() {
  SMT_LOG("Performing a pop");

  resetSplitConditions();
  if (_trail.empty()) return false;

  struct timespec start = TimeUtils::sampleMicro();

  if (_statistics) {
    _statistics->incUnsignedAttribute(Statistics::NUM_POPS);
    // A pop always sends us to a state that we haven't seen before - whether
    // from a sibling split, or from a lower level of the tree.
    _statistics->incUnsignedAttribute(Statistics::NUM_VISITED_TREE_STATES);
  }
  ++_stateId;

  do {
    _context.pop();
    // Remove any entries that have no alternatives
    TrailEntry *trailEntry = _trail.back();
    PLConstraint *constraint = trailEntry->_constraint;
    PhaseStatus phase = trailEntry->_phase;
    constraint->markInfeasiblePhase(phase);

    if (!constraint->hasFeasiblePhases()) {
      delete trailEntry;
      _trail.popBack();
      if (_trail.empty()) return false;
    } else
      break;
  } while (true);

  TrailEntry *trailEntry = _trail.back();
  PLConstraint *constraint = trailEntry->_constraint;
  PhaseStatus phase = constraint->getNextFeasibleCase();
  trailEntry->_phase = phase;
  auto split = constraint->getCaseSplit(phase);

  _engine->preContextPushHook();
  _context.push();
  _engine->applySplit(split);

  if (_statistics) {
    unsigned level = getTrailLength();
    _statistics->setUnsignedAttribute(Statistics::CURRENT_DECISION_LEVEL,
                                      level);
    if (level >
        _statistics->getUnsignedAttribute(Statistics::MAX_DECISION_LEVEL))
      _statistics->setUnsignedAttribute(Statistics::MAX_DECISION_LEVEL, level);
    struct timespec end = TimeUtils::sampleMicro();
    _statistics->incLongAttribute(Statistics::TOTAL_TIME_SMT_CORE_MICRO,
                                  TimeUtils::timePassed(start, end));
  }

  SMT_LOG(Stringf("Performing a pop - done, current trail length: %u\n",
                  getTrailLength())
              .ascii());
  return true;
}

void SmtCore::informSatSolverOfDecisions(CadicalWrapper *cadical) {
  cadical->clearAssumptions();
  for (const auto &trailEntry : _trail)
    cadical->assumeLiteral(
        trailEntry->_constraint->getLiteralOfPhaseStatus(trailEntry->_phase));
}

void SmtCore::extractConflict(
    const Map<String, GurobiWrapper::IISBoundType> &explanation,
    const BoundManager &boundManager) {
  SMT_LOG("Performing conflict analysis...");

  _currentConflict = Conflict();
  Set<unsigned> levels;
  for (const auto &pair : explanation) {
    unsigned variable = variableNameToVariable(pair.first);
    auto type = pair.second;

    if (type == GurobiWrapper::IIS_BOTH) {
      levels.insert(boundManager.getLevelOfLastLowerUpdate(variable));
      levels.insert(boundManager.getLevelOfLastUpperUpdate(variable));
    } else if (type == GurobiWrapper::IIS_LB) {
      levels.insert(boundManager.getLevelOfLastLowerUpdate(variable));
    } else if (type == GurobiWrapper::IIS_UB) {
      levels.insert(boundManager.getLevelOfLastUpperUpdate(variable));
    } else {
    }
  }

  // Current level must be involved
  ASSERT(levels.exists(_context.getLevel()));
  unsigned level = 1;
  for (const auto &trailEntry : _trail) {
    if (levels.exists(level++))
      _currentConflict.addLiteral(trailEntry->_constraint, trailEntry->_phase);
  }

  SMT_LOG(Stringf("Conflict analysis - done, conflict length %u, level %u",
                  _currentConflict._literals.size(), _context.getLevel())
              .ascii());
}

void SmtCore::extractNaiveConflict() {
  SMT_LOG("Performing conflict analysis...");

  _currentConflict = Conflict();

  for (const auto &trailEntry : _trail) {
    _currentConflict.addLiteral(trailEntry->_constraint, trailEntry->_phase);
  }

  SMT_LOG(Stringf("Conflict analysis - done, conflict length %u, level %u",
                  _currentConflict._literals.size(), _context.getLevel())
              .ascii());
}

void SmtCore::setStatistics(Statistics *statistics) {
  _statistics = statistics;
}

unsigned SmtCore::getTrailLength() const {
  ASSERT(_trail.size() == static_cast<unsigned>(_context.getLevel()));
  return _trail.size();
}

unsigned SmtCore::getStateId() const { return _stateId; }

const Conflict &SmtCore::getCurrentConflict() const { return _currentConflict; }
