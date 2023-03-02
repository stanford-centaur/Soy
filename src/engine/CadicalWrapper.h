#ifndef __CadicalWrapper_h__
#define __CadicalWrapper_h__

#include "Debug.h"
#include "List.h"
#include "MStringf.h"
#include "Map.h"
#include "Watcher.h"
#include "cadical.hpp"

enum LiteralStatus { FALSE = 0, TRUE = 1, UNFIXED = 2 };

class Statistics;

class CadicalWrapper : public Watcher {
 public:
  // -------------------Methods for cons/destructing models -----------------//
  CadicalWrapper();
  CadicalWrapper(const CadicalWrapper &other);
  CaDiCaL::Solver *getCadicalInstance() const;
  ~CadicalWrapper();

  void setStatistics(Statistics *statistics) { _statistics = statistics; }

  // ------------------------- General Methods ------------------------------//
  unsigned getNumberOfVariables() {
    ASSERT(!_satSolver || _satSolver->vars() == (int)_numberOfBooleanVariables);
    return _numberOfBooleanVariables;
  }

  // ------------------------- Methods for adding constraints ---------------//
  unsigned getFreshVariable();
  void addConstraint(const List<int> &constraint);
  void assumeLiteral(int constraint);
  void clearAssumptions();
  bool haveAssumptions() const { return _assumptions.size() > 0; };

  // ----------------------- Methods for solving ----------------------------//
  void setDirection(int lit);
  void resetDirection(unsigned bVariable);
  void resetAllDirections();
  unsigned directionAwareSolve();
  void solve();
  void preprocess();

  // --------------------- Methods for retreive results ---------------------//
  bool infeasible();
  bool haveFeasibleSolution();
  LiteralStatus getAssignment(int lit);
  void extractSolution(Map<unsigned, bool> &values);
  LiteralStatus getLiteralStatus(int lit);
  void retrieveAndUpdateWatcherOfFixedBVariable();

  // -------------------------- Debug methods -------------------------------//
  void dumpModel(const String &name);

 private:
  CaDiCaL::Solver *_satSolver;
  unsigned _numberOfBooleanVariables;
  List<List<int>> _constraints;
  List<int> _assumptions;
  List<int> _phase;

  Statistics *_statistics;
};

#endif  // __CadicalWrapper_h__
