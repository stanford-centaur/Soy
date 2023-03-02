#ifndef __SmtCore_h__
#define __SmtCore_h__

#include <memory>

#include "Conflict.h"
#include "DivideStrategy.h"
#include "MStringf.h"
#include "PLConstraint.h"
#include "PLConstraintScoreTracker.h"
#include "PiecewiseLinearCaseSplit.h"
#include "Statistics.h"
#include "TrailEntry.h"
#include "Watcher.h"
#include "context/context.h"

#define SMT_LOG(x, ...) \
  LOG(GlobalConfiguration::SMT_CORE_LOGGING, "SmtCore: %s\n", x)

class BoundManager;
class CadicalWrapper;
class Engine;

using CVC4::context::Context;

class SmtCore {
 public:
  /*************************** con/destructor ********************************/
  SmtCore(Engine *engine);
  ~SmtCore();

  void freeMemory();

 private:
  List<TrailEntry *> _trail;

  Engine *_engine;
  Context &_context;

  /************************* branching heuristic ****************************/
 public:
  void initializeScoreTrackerIfNeeded(
      const List<PLConstraint *> &plConstraints);

  void reportRejectedPhasePatternProposal();

  inline void updatePLConstraintScore(PLConstraint *constraint, double score) {
    if (_scoreTracker) _scoreTracker->updateScore(constraint, score);
  }

  inline PLConstraint *getConstraintsWithHighestScore() const {
    return _scoreTracker->topUnfixed();
  }

  void resetSplitConditions();

  bool needToSplit() const;

 private:
  DivideStrategy _branchingHeuristic;

  bool _needToSplit;
  PLConstraint *_constraintForSplitting;
  unsigned _deepSoIRejectionThreshold;
  bool _backJump;
  std::unique_ptr<PLConstraintScoreTracker> _scoreTracker;
  unsigned _numRejectedPhasePatternProposal;

  /*************************** push and pop **********************************/
 public:
  void performSplit();
  bool popSplit();
  void informSatSolverOfDecisions(CadicalWrapper *cadical);

 private:
  unsigned _stateId;

  /************************* conflict analysis *******************************/
 public:
  void extractConflict(
      const Map<String, GurobiWrapper::IISBoundType> &explanation,
      const BoundManager &boundManager);
  void extractNaiveConflict();
  void incrementConflictCount() {++_numberOfConflict;}

 private:
  Conflict _currentConflict;
  unsigned _numberOfConflict;

  /************************* restart analysis *******************************/
public:
  bool needToRestart() const;
  void restart();
  static unsigned luby(unsigned i) {
    for (unsigned k = 1; k < 32; ++k)
      if (i == (1u << k) - 1) return 1u << (k - 1);
    for (unsigned k = 1;; ++k)
      if ((1u << (k - 1)) <= i && i < (1u << k) - 1)
        return luby(i - (1 << (k-1)) + 1);
  }

private:
  unsigned _conflictLimit;
  unsigned _numberOfRestarts;
  unsigned _initialConflictLimit;

  /*************************** getter/setter *********************************/
 public:
  void setStatistics(Statistics *statistics);

  inline void setBranchingHeuristics(DivideStrategy strategy) {
    _branchingHeuristic = strategy;
  }

  unsigned getTrailLength() const;
  unsigned getStateId() const;

  const Conflict &getCurrentConflict() const;

 private:
  static unsigned variableNameToVariable(String s) {
    return atoi(s.tokenize("x").rbegin()->ascii());
  }

  Statistics *_statistics;
};

#endif  // __SmtCore_h__
