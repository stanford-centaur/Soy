#ifndef __PLConstraint_h__
#define __PLConstraint_h__

#include "AssignmentManager.h"
#include "BoundManager.h"
#include "CadicalWrapper.h"
#include "GlobalConfiguration.h"
#include "GurobiWrapper.h"
#include "LinearExpression.h"
#include "List.h"
#include "Map.h"
#include "SoyError.h"
#include "PiecewiseLinearCaseSplit.h"
#include "PiecewiseLinearFunctionType.h"
#include "Statistics.h"
#include "Tightening.h"
#include "Watcher.h"
#include "context/cdlist.h"
#include "context/cdo.h"
#include "context/context.h"

class Equation;
class InputQuery;
class String;

enum PhaseStatus : unsigned {
  PHASE_NOT_FIXED = 0,
  ABS_PHASE_POSITIVE = 1,
  ABS_PHASE_NEGATIVE = 2,

  PHASE_MAX = 1000000,
};

struct PhaseScoreEntry {
  PhaseScoreEntry(PhaseStatus phase, double score)
      : _phase(phase), _score(score){};

  bool operator<(const PhaseScoreEntry &other) const {
    if (_score == other._score)
      return _phase < other._phase;
    else
      return _score > other._score;
  }

  PhaseStatus _phase;
  double _score;
};

typedef std::set<PhaseScoreEntry> PhaseScores;

class PLConstraint : public Watchee {
  /**********************************************************************/
  /*                           CONS/DESTRUCTION METHODS                 */
  /**********************************************************************/
 public:
  PLConstraint()
      : _context(NULL),
        _assignmentManager(nullptr),
        _boundManager(nullptr),
        _satSolver(NULL),
        _statistics(NULL),
        _constraintActive(NULL),
        _phaseStatus(PHASE_NOT_FIXED) {}

  virtual ~PLConstraint() {
    if (_constraintActive) _constraintActive->deleteSelf();
    for (const auto pair : _feasiblePhases) pair.second->deleteSelf();
  }

  void registerAssignmentManager(AssignmentManager *am) {
    _assignmentManager = am;
  }
  void registerBoundManager(BoundManager *bm) {
    _lowerBounds.clear();
    _upperBounds.clear();
    _boundManager = bm;
  }
  void registerSatSolver(CadicalWrapper *solver) { _satSolver = solver; }
  void setStatistics(Statistics *statistics) { _statistics = statistics; }

  virtual void addBooleanStructure() = 0;

  void initializeCDOs(CVC4::context::Context *context) {
    ASSERT(_context == NULL);
    ASSERT(_constraintActive == NULL);
    ASSERT(_feasiblePhases.size() == 0);
    _context = context;
    _constraintActive = new (true) CVC4::context::CDO<bool>(_context, true);
    for (const auto &phase : getAllCases())
      _feasiblePhases[phase] =
          new (true) CVC4::context::CDO<bool>(_context, true);
    initializeDirectionHeuristic();
  }

  virtual PLConstraint *duplicateConstraint() const = 0;
  virtual void transformToUseAuxVariables(InputQuery &){};
  virtual void addAuxiliaryEquationsAfterPreprocessing(
      InputQuery & /* inputQuery */) {}

  /**********************************************************************/
  /*                           GENERAL METHODS                          */
  /**********************************************************************/
  virtual PiecewiseLinearFunctionType getType() const = 0;

  virtual bool participatingVariable(unsigned variable) const = 0;
  virtual List<unsigned> getParticipatingVariables() const = 0;

  virtual bool satisfied() const = 0;
  virtual bool satisfied(const Map<unsigned, double> &) const { return false; }

  virtual bool supporCaseSplit() const { return true; };

  void setActive(bool active) { *_constraintActive = active; }
  bool isActive() const {
    if (_constraintActive)
      return *_constraintActive;
    else
      return true;
  }

  virtual void setPhaseStatus(PhaseStatus phase) {
    ASSERT(*_feasiblePhases[phase]);
    for (const auto &pair : _feasiblePhases)
      if (pair.first != phase) *pair.second = false;
  }

  virtual bool hasFeasiblePhases() const {
    for (const auto &pair : _feasiblePhases)
      if (*pair.second) return true;
    return false;
  }

  virtual bool isFeasible(PhaseStatus phase) const {
    return *_feasiblePhases[phase];
  }

  virtual unsigned numberOfFeasiblePhases() const {
    unsigned numPhases = 0;
    for (const auto &pair : _feasiblePhases)
      if (*pair.second) ++numPhases;
    return numPhases;
  }

  virtual void markInfeasiblePhase(PhaseStatus phase) {
    *_feasiblePhases[phase] = false;
  }

  virtual bool phaseFixed() const { return numberOfFeasiblePhases() == 1; };

  virtual List<PhaseStatus> getAllCases() const = 0;

  virtual List<PhaseStatus> getAllFeasibleCases() const {
    List<PhaseStatus> phases;
    for (const auto &pair : _feasiblePhases)
      if (*pair.second) phases.append(pair.first);
    return phases;
  }

  PhaseStatus getNextFeasibleCase() const {
    ASSERT(_context);
    return topUnfixed();
  };
  virtual PiecewiseLinearCaseSplit getCaseSplit(PhaseStatus) const {
    throw SoyError(SoyError::FEATURE_NOT_YET_SUPPORTED);
  };
  virtual PiecewiseLinearCaseSplit getValidCaseSplit() const {
    ASSERT(_context);
    bool seenFeasiblePhase = false;
    PhaseStatus validPhase;
    auto allCases = getAllCases();
    for (const auto &phase : allCases) {
      if (*_feasiblePhases[phase]) {
        if (seenFeasiblePhase)
          return PiecewiseLinearCaseSplit();
        else {
          seenFeasiblePhase = true;
          validPhase = phase;
        }
      }
    }

    if (seenFeasiblePhase)
      return getCaseSplit(validPhase);
    else
      throw SoyError(SoyError::REQUESTED_NONEXISTENT_CASE_SPLIT,
                         "No feasible case split left.");
  }

  int getLiteralOfPhaseStatus(PhaseStatus phase) const {
    return _phaseStatusToLit[phase];
  }
  PhaseStatus getPhaseStatusOfLiteral(int lit) const {
    return _litToPhaseStatus[lit];
  }
  bool phaseStatusHasLiteral(PhaseStatus phase) const {
    return _phaseStatusToLit.exists(phase);
  }

 protected:
  CVC4::context::Context *_context;
  AssignmentManager *_assignmentManager;
  BoundManager *_boundManager;
  CadicalWrapper *_satSolver;
  Statistics *_statistics;

  CVC4::context::CDO<bool> *_constraintActive;
  Map<PhaseStatus, CVC4::context::CDO<bool> *> _feasiblePhases;

  Map<int, PhaseStatus> _litToPhaseStatus;
  Map<PhaseStatus, int> _phaseStatusToLit;

  /*
    Used only in preprocessing
  */
  Map<unsigned, double> _lowerBounds;
  Map<unsigned, double> _upperBounds;
  PhaseStatus _phaseStatus;

  /**********************************************************************/
  /*                     Direction heuristic                            */
  /**********************************************************************/
 public:
  void updatePhaseStatusScore(PhaseStatus phase, double score) {
    double alpha =
        GlobalConfiguration::EXPONENTIAL_MOVING_AVERAGE_ALPHA_DIRECTION;
    double oldScore = _phaseToScore[phase];
    double newScore = (1 - alpha) * oldScore + alpha * score;

    ASSERT(_scores.find(PhaseScoreEntry(phase, oldScore)) != _scores.end());
    _scores.erase(PhaseScoreEntry(phase, oldScore));
    _scores.insert(PhaseScoreEntry(phase, newScore));
    _phaseToScore[phase] = newScore;
  }

  void decayScores() {
    for (const auto &phase : getAllCases()) {
      double oldScore = _phaseToScore[phase];
      double newScore = oldScore / 10;
      _scores.erase(PhaseScoreEntry(phase, oldScore));
      _scores.insert(PhaseScoreEntry(phase, newScore));
      _phaseToScore[phase] = newScore;
    }
  }

  void initializeDirectionHeuristic() {
    for (const auto &phase : getAllCases()) {
      _scores.insert({phase, 0});
      _phaseToScore[phase] = 0;
    }
  }

 private:
  PhaseStatus topUnfixed() const {
    for (const auto &entry : _scores) {
      if (*_feasiblePhases[entry._phase]) {
        return entry._phase;
      }
    }
    throw SoyError(SoyError::REQUESTED_NONEXISTENT_CASE_SPLIT,
                       "No feasible case left");
  }

  PhaseScores _scores;
  Map<PhaseStatus, double> _phaseToScore;

  /**********************************************************************/
  /*                             SoI METHODS                            */
  /**********************************************************************/
 public:
  virtual bool supportSoI() const { return false; };

  virtual void getCostFunctionComponent(LinearExpression & /* cost */,
                                        PhaseStatus /* phase */) const {}

  virtual PhaseStatus getPhaseStatusInAssignment() {
    throw SoyError(SoyError::FEATURE_NOT_YET_SUPPORTED);
  }

  /**********************************************************************/
  /*                         BOUND WRAPPER METHODS                      */
  /**********************************************************************/
  /* These methods prefer using BoundManager over local bound arrays.   */

 public:
  virtual void notifyLowerBound(unsigned variable, double bound) = 0;
  virtual void notifyUpperBound(unsigned variable, double bound) = 0;
  virtual void notifyBVariable(int /*lit*/, bool /*value*/){};
  virtual void getEntailedTightenings(List<Tightening> &tightenings) const = 0;

 protected:
  bool existsLowerBound(unsigned var) const {
    return _boundManager != nullptr || _lowerBounds.exists(var);
  }

  bool existsUpperBound(unsigned var) const {
    return _boundManager != nullptr || _upperBounds.exists(var);
  }

  double getLowerBound(unsigned var) const {
    if (_boundManager != nullptr)
      return _boundManager->getLowerBound(var);
    else
      return existsLowerBound(var) ? _lowerBounds[var]
                                   : FloatUtils::negativeInfinity();
  }

  double getUpperBound(unsigned var) const {
    if (_boundManager != nullptr)
      return _boundManager->getUpperBound(var);
    else
      return existsUpperBound(var) ? _upperBounds[var] : FloatUtils::infinity();
  }

  void setLowerBound(unsigned var, double value) {
    (_boundManager != nullptr) ? _boundManager->setLowerBound(var, value)
                               : _lowerBounds[var] = value;
  }

  void setUpperBound(unsigned var, double value) {
    (_boundManager != nullptr) ? _boundManager->setUpperBound(var, value)
                               : _upperBounds[var] = value;
  }

  void tightenLowerBound(unsigned var, double value) {
    if (_boundManager != nullptr)
      _boundManager->tightenLowerBound(var, value);
    else if (!_lowerBounds.exists(var) || _lowerBounds[var] < value)
      _lowerBounds[var] = value;
  }

  void tightenUpperBound(unsigned var, double value) {
    if (_boundManager != nullptr)
      _boundManager->tightenUpperBound(var, value);
    else if (!_upperBounds.exists(var) || _upperBounds[var] > value)
      _upperBounds[var] = value;
  }

  /**********************************************************************/
  /*                      ASSIGNMENT WRAPPER METHODS                    */
  /**********************************************************************/
  double getAssignment(unsigned variable) const {
    return _assignmentManager->getAssignment(variable);
  }

  /**********************************************************************/
  /*                             DEBUG METHODS                          */
  /**********************************************************************/
 public:
  virtual void dump(String &) const {}
};

#endif  // __PLConstraint_h__
