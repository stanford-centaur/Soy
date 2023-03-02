#ifndef __Engine_h__
#define __Engine_h__

#include <context/context.h>

#include <atomic>

#include "AssignmentManager.h"
#include "BoundManager.h"
#include "CadicalWrapper.h"
#include "Conflict.h"
#include "DivideStrategy.h"
#include "GlobalConfiguration.h"
#include "GurobiWrapper.h"
#include "InputQuery.h"
#include "LinearExpression.h"
#include "MILPEncoder.h"
#include "Map.h"
#include "Options.h"
#include "Preprocessor.h"
#include "SignalHandler.h"
#include "SmtCore.h"
#include "SoIManager.h"
#include "Statistics.h"

#ifdef _WIN32
#undef ERROR
#endif

#define ENGINE_LOG(x, ...) \
  LOG(GlobalConfiguration::ENGINE_LOGGING, "Engine: %s\n", x)

class InputQuery;
class LPBasedTightener;
class PLConstraint;
class String;

using CVC4::context::Context;

class Engine : public SignalHandler::Signalable {
 public:
  enum {
    MICROSECONDS_TO_SECONDS = 1000000,
  };

  enum ExitCode {
    UNSAT = 0,
    SAT = 1,
    ERROR = 2,
    UNKNOWN = 3,
    TIMEOUT = 4,
    QUIT_REQUESTED = 5,

    NOT_DONE = 999,
  };

  Engine();
  virtual ~Engine();

  /************************** Preprocessing **********************************/
 public:
  bool processInputQuery(InputQuery &inputQuery);
  bool processInputQuery(InputQuery &inputQuery, bool preprocess);
  void computeInitialPattern(const InputQuery &inputQuery);

 private:
  void performLPBasedBoundTightening();
  void informConstraintsOfInitialBounds(InputQuery &inputQuery) const;
  void invokePreprocessor(InputQuery &inputQuery, bool preprocess);
  void initializeBounds(unsigned numberOfVariables);
  void decideBranchingHeuristics();

  // For the most part, we only care about assignment of PLConstraints
  Set<unsigned> _variablesParticipatingInPLConstraints;

  /************************* Solve *******************************************/
 public:
  bool solve(unsigned timeoutInSeconds = 0);
  double getAssignment(unsigned variable);

 private:
  bool solveWithMILPEncoding(unsigned timeoutInSeconds);
  void checkSolutionCompliance() const;
  void checkConsistencyBetweenParties() const;
  void checkTheoryLemmaCorrectness() const;

  InputQuery _initialPreprocessedInputQuery;

  /******************************* SAT related *******************************/
 private:
  void informSatSolverOfDecisions();
  bool checkBooleanLevelFeasibility();
  void addAllLemmasToSatSolver();

  /******************************* SoI related *******************************/
 private:
  bool adjustAssignmentToSatisfyNonLinearConstraints();
  bool performDeepSoILocalSearch();

  double computeHeuristicCost(const LinearExpression &heuristicCost);
  void updatePseudoImpactWithSoICosts(double costOfLastAcceptedPhasePattern,
                                      double costOfProposedPhasePattern);

  void bumpUpPseudoImpactOfPLConstraintsNotInSoI();
  bool checkFeasibilityWithGurobi();
  bool minimizeCostWithGurobi(const LinearExpression &costFunction);

  /******************************* Constraints *******************************/
 private:
  void clearViolatedPLConstraints();
  bool applyAllValidConstraintCaseSplits();
  bool applyValidConstraintCaseSplit(PLConstraint *constraint);

  void collectViolatedPlConstraints();
  bool allPlConstraintsHold() const;
  void dumpConstraintsStatus() const;


  /****************************** Bounds *************************************/
 private:
  void informLPSolverOfBounds();
  void checkGurobiBoundConsistency() const;

  /******************************* BaB related *******************************/
 public:
  virtual void applySplit(const PiecewiseLinearCaseSplit &split);

  virtual PLConstraint *pickSplitPLConstraint(DivideStrategy strategy);

  virtual void postContextPopHook();
  virtual void preContextPushHook();

 private:
  void extractTheoryExplanation(bool isMILP);

  /**************************** Solution *************************************/
 public:
  Engine::ExitCode getExitCode() const;
  void extractSolution(InputQuery &inputQuery);

  /**************************** Parallel *************************************/
  void quitSignal();

  /**************************** Statistics ***********************************/
 private:
  void mainLoopStatistics();
  bool shouldExitDueToTimeout(unsigned timeout) const;

  /************************** Getters/Setters ********************************/
 public:
  void setVerbosity(unsigned verbosity);
  void setRandomSeed(unsigned seed);
  Context &getContext() { return _context; }
  const Statistics *getStatistics() const;
  InputQuery *getInputQuery();
  SmtCore *getSmtCore();

  std::atomic_bool *getQuitRequested();

 private:
  Context _context;
  BoundManager _boundManager;
  Statistics _statistics;
  List<PLConstraint *> _plConstraints;
  List<PLConstraint *> _violatedPlConstraints;
  std::unique_ptr<InputQuery> _preprocessedQuery;
  SmtCore _smtCore;
  Preprocessor _preprocessor;
  bool _preprocessingEnabled;
  std::atomic_bool _quitRequested;
  ExitCode _exitCode;
  unsigned _verbosity;
  unsigned _statisticsPrintingFrequency;
  unsigned _deepSoIRejectionThreshold;
  bool _solveWithMILP;
  bool _cdcl;
  bool _boundTightening;

  std::unique_ptr<LPBasedTightener> _lpBasedTightener;
  std::unique_ptr<MILPEncoder> _milpEncoder;
  std::unique_ptr<GurobiWrapper> _gurobi;
  std::unique_ptr<CadicalWrapper> _cadical;
  std::unique_ptr<SoIManager> _soiManager;
  std::unique_ptr<AssignmentManager> _assignmentManager;

  unsigned _maxLemmaLength;
  unsigned _maxNumberOfProposals;
  double _milpSolvingThreshold;
  bool _cachePhasePattern;

  // Top-level Lemmas
  List<Vector<PhaseStatus>> _lemmas;
};

#endif  // __Engine_h__
