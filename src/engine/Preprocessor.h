#ifndef __Preprocessor_h__
#define __Preprocessor_h__

#include "BoundManager.h"
#include "Equation.h"
#include "InputQuery.h"
#include "List.h"
#include "Map.h"
#include "PLConstraint.h"
#include "Set.h"

class Preprocessor {
 public:
  Preprocessor();

  ~Preprocessor();

  /*
    Main method of this class: preprocess the input query
  */
  void preprocess(InputQuery &query);

  /*
    Preprocess the input query using the bounds in bound manager
    without updating the input query or the bound manger
  */

  bool preprocessLite(InputQuery &query, BoundManager &bm, bool updateCDObjects);

  /*
    Have the preprocessor start reporting statistics.
  */
  void setStatistics(Statistics *statistics);

 private:
  void freeMemoryIfNeeded();

  inline double getLowerBound(unsigned var) { return _lowerBounds[var]; }

  inline double getUpperBound(unsigned var) { return _upperBounds[var]; }

  inline void setLowerBound(unsigned var, double value) {
    _lowerBounds[var] = value;
  }

  inline void setUpperBound(unsigned var, double value) {
    _upperBounds[var] = value;
  }

  /*
    Transform the piecewise linear constraints if needed. For instance, this
    will make sure all disjuncts in all disjunctions contain only bounds and
    no (in)equalities between variables.
  */
  void transformConstraintsIfNeeded();

  /*
    Transform all equations of type GE or LE to type EQ.
  */
  void makeAllEquationsEqualities();

  /*
    Set any missing upper bound to +INF, and any missing lower bound
    to -INF.
  */
  void setMissingBoundsToInfinity();

  /*
    Tighten bounds using the linear equations
  */
  bool processEquationsLite();

  /*
    Tighten the bounds using the piecewise linear and transcendental constraints
  */
  bool processConstraintsLite();

  /*
    Tighten bounds using the linear equations
  */
  bool processEquations();

  /*
    Tighten the bounds using the piecewise linear and transcendental constraints
  */
  bool processConstraints();

  /*
    All input/output variables
  */
  Set<unsigned> _inputOutputVariables;

  /*
    The preprocessed query
  */
  InputQuery *_preprocessed;

  /*
    Statistics collection
  */
  Statistics *_statistics;

  /*
    Used to store the bounds during the preprocessing.
  */
  double *_lowerBounds;
  double *_upperBounds;

  /*
    For debugging only
  */
  void dumpAllBounds(const String &message);
};

#endif  // __Preprocessor_h__
