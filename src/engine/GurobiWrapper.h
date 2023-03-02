#ifndef __GurobiWrapper_h__
#define __GurobiWrapper_h__

#include "MString.h"
#include "Map.h"
#include "gurobi_c++.h"

class GurobiWrapper {
 public:
  enum VariableType {
    CONTINUOUS = 0,
    BINARY = 1,
  };

  enum IISBoundType {
    IIS_LB = 0,
    IIS_UB = 1,
    IIS_BOTH = 2

  };

  /*
    A term has the form: coefficient * variable
  */
  struct Term {
    Term(double coefficient, const String &variable)
        : _coefficient(coefficient), _variable(variable) {}

    double _coefficient;
    String _variable;
  };

  // -------------------Methods for cons/destructing models -----------------//
 public:
  GurobiWrapper();
  ~GurobiWrapper();
  void resetModel();
  void resetToUnsolvedState();

 private:
  void freeModelIfNeeded();
  void freeMemoryIfNeeded();

  // ------------------------- Methods for adding constraints ---------------//
 public:
  void addVariable(const String &name, double lb, double ub,
                   VariableType type = CONTINUOUS);
  void setLowerBound(const String &name, double lb);
  void setUpperBound(const String &name, double ub);
  double getLowerBound(const String &name);
  double getUpperBound(const String &name);
  double getReducedCost(const String &name);
  void setVariableType(const String &name, char type);
  void addLeqConstraint(const List<Term> &terms, double scalar,
                        String name = "");
  void addGeqConstraint(const List<Term> &terms, double scalar,
                        String name = "");
  void addEqConstraint(const List<Term> &terms, double scalar,
                       String name = "");
  void addConstraint(const List<Term> &terms, double scalar, char sense,
                     String name = "");
  void removeConstraintByName(const String &name);
  void setCost(const List<Term> &terms, double constant = 0);
  void setObjective(const List<Term> &terms, double constant = 0);
  void updateModel();

  // ----------------------- Methods for solving ----------------------------//
 public:
  void setNodeLimit(double limit);
  void setCutoff(double cutoff);
  void setTimeLimit(double seconds);
  void setVerbosity(unsigned verbosity);
  void setNumberOfThreads(unsigned threads);
  void setMethod(int method);
  void computeIIS(int method=0);
  void solve();
  void loadMPS(String filename);

  // --------------------- Methods for retreive results ---------------------//
 public:
  unsigned getStatusCode();
  bool optimal();
  bool cutoffOccurred();
  bool infeasible();
  bool timeout();
  bool haveFeasibleSolution();
  void extractIIS(Map<String, IISBoundType> &bounds, List<String> &constraints,
                  const List<String> &constraintNames);
  double getAssignment(const String &variable);
  double getObjectiveBound();
  double getObjectiveValue();
  void extractSolution(Map<String, double> &values, double &costOrObjective);
  unsigned getNumberOfNodes();

  // -------------------------- Debug methods -------------------------------//
 public:
  void dumpModel(const String &name);
  void dumpSolution();

 private:
  static void log(const String &message);

 private:
  GRBEnv *_environment;
  GRBModel *_model;
  Map<String, GRBVar *> _nameToVariable;
};

#endif  // __GurobiWrapper_h__
