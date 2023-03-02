/*********************                                                        */
/*! \file Preprocessor.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Derek Huang, Shantanu Thakoor, Haoze Wu
 ** This file is part of the Soy project.
 ** Copyright (c) 2023 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#include "Preprocessor.h"

#include "Debug.h"
#include "FloatUtils.h"
#include "InfeasibleQueryException.h"
#include "InputQuery.h"
#include "MStringf.h"
#include "Map.h"
#include "SoyError.h"
#include "Options.h"
#include "PiecewiseLinearFunctionType.h"
#include "Statistics.h"
#include "Tightening.h"

#ifdef _WIN32
#undef INFINITE
#endif

Preprocessor::Preprocessor()
    : _preprocessed(nullptr),
      _statistics(NULL),
      _lowerBounds(NULL),
      _upperBounds(NULL) {}

Preprocessor::~Preprocessor() { freeMemoryIfNeeded(); }

void Preprocessor::freeMemoryIfNeeded() {
  if (_lowerBounds != NULL) {
    delete[] _lowerBounds;
    _lowerBounds = NULL;
  }
  if (_upperBounds != NULL) {
    delete[] _upperBounds;
    _upperBounds = NULL;
  }
}

void Preprocessor::preprocess(InputQuery &query) {
  _preprocessed = &query;

  //makeAllEquationsEqualities();

  /*
    Transform the piecewise linear constraints if needed so that the case
    splits can all be represented as bounds over existing variables.
  */
  transformConstraintsIfNeeded();

  /*
    Set any missing bounds
  */
  setMissingBoundsToInfinity();

  /*
    Store the bounds locally for more efficient access.
  */
  _lowerBounds = new double[_preprocessed->getNumberOfVariables()];
  _upperBounds = new double[_preprocessed->getNumberOfVariables()];

  for (unsigned i = 0; i < _preprocessed->getNumberOfVariables(); ++i) {
    _lowerBounds[i] = _preprocessed->getLowerBound(i);
    _upperBounds[i] = _preprocessed->getUpperBound(i);
  }

  /*
    Do the preprocessing steps:

    Until saturation:
      1. Tighten bounds using equations
      2. Tighten bounds using pl constraints
  */
  bool continueTightening = true && GlobalConfiguration::PERFORM_PREPROCESSING;
  while (continueTightening) {
    continueTightening = processEquations();
    continueTightening = processConstraints() || continueTightening;

    if (_statistics)
      _statistics->incUnsignedAttribute(
          Statistics::PP_NUM_TIGHTENING_ITERATIONS);
  }

  /*
    Update the bounds.
  */
  _preprocessed->clearBounds();
  for (unsigned i = 0; i < _preprocessed->getNumberOfVariables(); ++i) {
    _preprocessed->setLowerBound(i, getLowerBound(i));
    _preprocessed->setUpperBound(i, getUpperBound(i));
  }

  ASSERT(_preprocessed->getLowerBounds().size() ==
         _preprocessed->getNumberOfVariables());
  ASSERT(_preprocessed->getUpperBounds().size() ==
         _preprocessed->getNumberOfVariables());

  return;
}

bool Preprocessor::preprocessLite(InputQuery &query, BoundManager &bm,
                                  bool updateCDObjects) {
  _preprocessed = &query;

  if (!_lowerBounds)
      _lowerBounds = new double[_preprocessed->getNumberOfVariables()];
  if (!_upperBounds)
      _upperBounds = new double[_preprocessed->getNumberOfVariables()];

  for (unsigned i = 0; i < _preprocessed->getNumberOfVariables(); ++i) {
      _lowerBounds[i] = bm.getLowerBound(i);
      _upperBounds[i] = bm.getUpperBound(i);
  }

  List<PLConstraint*> plConstraintsOriginal;
  List<PLConstraint*> plConstraintsCopy;
  List<PLConstraint *> &constraints = _preprocessed->getPLConstraints();
  if (!updateCDObjects) {
      for (const auto &plConstraint : _preprocessed->getPLConstraints()) {
          plConstraintsCopy.append(plConstraint->duplicateConstraint());
          plConstraintsOriginal.append(plConstraint);
      }
      for (const auto &plConstraint : plConstraintsOriginal)
          constraints.erase(plConstraint);
      for (const auto &plConstraint : plConstraintsCopy)
          constraints.append(plConstraint);
  }

  bool feasible = true;
  try {
      bool continueTightening = true;
      while (continueTightening) {
          continueTightening = processEquationsLite();
          continueTightening = processConstraintsLite() || continueTightening;
      }
  } catch (const InfeasibleQueryException &) {
      feasible = false;
  }

  if (!updateCDObjects) {
      for (const auto &plConstraint : plConstraintsCopy) {
          delete plConstraint;
          constraints.erase(plConstraint);
      }
      for (const auto &plConstraint : plConstraintsOriginal) {
          constraints.append(plConstraint);
      }
  } else {
      // Constraints are updated along the way
      for (unsigned i = 0; i < _preprocessed->getNumberOfVariables(); ++i) {
          bm.setLowerBound(i, getLowerBound(i));
          bm.setUpperBound(i, getUpperBound(i));
      }
  }
  return feasible;
}

void Preprocessor::transformConstraintsIfNeeded() {
  for (auto &plConstraint : _preprocessed->getPLConstraints())
    plConstraint->transformToUseAuxVariables(*_preprocessed);
}

void Preprocessor::makeAllEquationsEqualities() {
  for (auto &equation : _preprocessed->getEquations()) {
    if (equation._type == Equation::EQ) continue;

    unsigned auxVariable = _preprocessed->getNumberOfVariables();
    _preprocessed->setNumberOfVariables(auxVariable + 1);

    // Auxiliary variables are always added with coefficient 1
    if (equation._type == Equation::GE)
      _preprocessed->setUpperBound(auxVariable, 0);
    else
      _preprocessed->setLowerBound(auxVariable, 0);

    equation._type = Equation::EQ;

    equation.addAddend(1, auxVariable);
  }
}

bool Preprocessor::processEquationsLite() {
  enum {
    ZERO = 0,
    POSITIVE = 1,
    NEGATIVE = 2,
    INFINITE = 3,
  };

  bool tighterBoundFound = false;

  const List<Equation> &equations(_preprocessed->getEquations());

  for (const auto &equation : equations) {
    // The equation is of the form sum (ci * xi) - b ? 0
    Equation::EquationType type = equation._type;

    unsigned maxVar = equation._addends.begin()->_variable;
    for (const auto &addend : equation._addends) {
      if (addend._variable > maxVar) maxVar = addend._variable;
    }

    ++maxVar;

    double *ciTimesLb = new double[maxVar];
    double *ciTimesUb = new double[maxVar];
    char *ciSign = new char[maxVar];

    Set<unsigned> excludedFromLB;
    Set<unsigned> excludedFromUB;

    unsigned xi;
    double xiLB;
    double xiUB;
    double ci;
    double lowerBound;
    double upperBound;
    bool validLb;
    bool validUb;

    // The first goal is to compute the LB and UB of: sum (ci * xi) - b
    // For this we first identify unbounded variables
    double auxLb = -equation._scalar;
    double auxUb = -equation._scalar;
    for (const auto &addend : equation._addends) {
      ci = addend._coefficient;
      xi = addend._variable;

      if (FloatUtils::isZero(ci)) {
        ciSign[xi] = ZERO;
        ciTimesLb[xi] = 0;
        ciTimesUb[xi] = 0;
        continue;
      }

      ciSign[xi] = ci > 0 ? POSITIVE : NEGATIVE;

      xiLB = getLowerBound(xi);
      xiUB = getUpperBound(xi);

      if (FloatUtils::isFinite(xiLB)) {
        ciTimesLb[xi] = ci * xiLB;
        if (ciSign[xi] == POSITIVE)
          auxLb += ciTimesLb[xi];
        else
          auxUb += ciTimesLb[xi];
      } else {
        if (ci > 0)
          excludedFromLB.insert(xi);
        else
          excludedFromUB.insert(xi);
      }

      if (FloatUtils::isFinite(xiUB)) {
        ciTimesUb[xi] = ci * xiUB;
        if (ciSign[xi] == POSITIVE)
          auxUb += ciTimesUb[xi];
        else
          auxLb += ciTimesUb[xi];
      } else {
        if (ci > 0)
          excludedFromUB.insert(xi);
        else
          excludedFromLB.insert(xi);
      }
    }

    // Now, go over each addend in sum (ci * xi) - b ? 0, and see what can be
    // done
    for (const auto &addend : equation._addends) {
      ci = addend._coefficient;
      xi = addend._variable;

      // If ci = 0, nothing to do.
      if (ciSign[xi] == ZERO) continue;

      /*
        The expression for xi is:

             xi ? ( -1/ci ) * ( sum_{j\neqi} ( cj * xj ) - b )

        We use the previously computed auxLb and auxUb and adjust them because
        xi is removed from the sum. We also need to pay attention to the sign of
        ci, and to the presence of infinite bounds.

        Assuming "?" stands for equality, we can compute a LB if:
          1. ci is negative, and no vars except xi were excluded from the auxLb
          2. ci is positive, and no vars except xi were excluded from the auxUb

        And vice-versa for UB.

        In case "?" is GE or LE, only one direction can be computed.
      */
      if (ciSign[xi] == NEGATIVE) {
        validLb = ((type == Equation::LE) || (type == Equation::EQ)) &&
                  (excludedFromLB.empty() ||
                   (excludedFromLB.size() == 1 && excludedFromLB.exists(xi)));
        validUb = ((type == Equation::GE) || (type == Equation::EQ)) &&
                  (excludedFromUB.empty() ||
                   (excludedFromUB.size() == 1 && excludedFromUB.exists(xi)));
      } else {
        validLb = ((type == Equation::GE) || (type == Equation::EQ)) &&
                  (excludedFromUB.empty() ||
                   (excludedFromUB.size() == 1 && excludedFromUB.exists(xi)));
        validUb = ((type == Equation::LE) || (type == Equation::EQ)) &&
                  (excludedFromLB.empty() ||
                   (excludedFromLB.size() == 1 && excludedFromLB.exists(xi)));
      }

      if (validLb) {
        if (ciSign[xi] == NEGATIVE) {
          lowerBound = auxLb;
          if (!excludedFromLB.exists(xi)) lowerBound -= ciTimesUb[xi];
        } else {
          lowerBound = auxUb;
          if (!excludedFromUB.exists(xi)) lowerBound -= ciTimesUb[xi];
        }

        lowerBound /= -ci;

        if (FloatUtils::gt(lowerBound, getLowerBound(xi))) {
          tighterBoundFound = true;
          setLowerBound(xi, lowerBound);
        }
      }

      if (validUb) {
        if (ciSign[xi] == NEGATIVE) {
          upperBound = auxUb;
          if (!excludedFromUB.exists(xi)) upperBound -= ciTimesLb[xi];
        } else {
          upperBound = auxLb;
          if (!excludedFromLB.exists(xi)) upperBound -= ciTimesLb[xi];
        }

        upperBound /= -ci;

        if (FloatUtils::lt(upperBound, getUpperBound(xi))) {
          tighterBoundFound = true;
          setUpperBound(xi, upperBound);
        }
      }

      if (FloatUtils::gt(
              getLowerBound(xi), getUpperBound(xi),
              GlobalConfiguration::PREPROCESSOR_ALMOST_FIXED_THRESHOLD)) {
        delete[] ciTimesLb;
        delete[] ciTimesUb;
        delete[] ciSign;

        throw InfeasibleQueryException();
      }
    }

    delete[] ciTimesLb;
    delete[] ciTimesUb;
    delete[] ciSign;

    /*
      Next, do another sweep over the equation.
      Look for almost-fixed variables and fix them, and remove the equation
      entirely if it has nothing left to contribute.
    */
    bool allFixed = true;
    for (const auto &addend : equation._addends) {
      unsigned var = addend._variable;
      double lb = getLowerBound(var);
      double ub = getUpperBound(var);

      if (FloatUtils::areEqual(
              lb, ub, GlobalConfiguration::PREPROCESSOR_ALMOST_FIXED_THRESHOLD))
        setUpperBound(var, getLowerBound(var));
      else
        allFixed = false;
    }

    if (!allFixed) {
      continue;
    } else {
      double sum = 0;
      for (const auto &addend : equation._addends)
        sum += addend._coefficient * getLowerBound(addend._variable);

      if (type == Equation::EQ &&
          FloatUtils::areDisequal(sum, equation._scalar)) {
        throw InfeasibleQueryException();
      } else if (type == Equation::GE &&
                 FloatUtils::lt(sum, equation._scalar)) {
              throw InfeasibleQueryException();
      } else if (type == Equation::LE &&
                 FloatUtils::gt(sum, equation._scalar)) {
          throw InfeasibleQueryException();
      }
    }
  }

  return tighterBoundFound;
}

bool Preprocessor::processConstraintsLite() {
  bool tighterBoundFound = false;

  for (auto &constraint : _preprocessed->getPLConstraints()) {
      auto vars = constraint->getParticipatingVariables();
      for (unsigned variable : vars) {
          if (constraint->participatingVariable(variable)){
              constraint->notifyLowerBound(variable, getLowerBound(variable));
              constraint->notifyUpperBound(variable, getUpperBound(variable));
          }
      }

    List<Tightening> tightenings;
    constraint->getEntailedTightenings(tightenings);

    for (const auto &tightening : tightenings) {
      if ((tightening._type == Tightening::LB) &&
          (FloatUtils::gt(tightening._value,
                          getLowerBound(tightening._variable)))) {
        tighterBoundFound = true;
        setLowerBound(tightening._variable, tightening._value);
      }

      else if ((tightening._type == Tightening::UB) &&
               (FloatUtils::lt(tightening._value,
                               getUpperBound(tightening._variable)))) {
        tighterBoundFound = true;
        setUpperBound(tightening._variable, tightening._value);
      }

      if (FloatUtils::areEqual(
              getLowerBound(tightening._variable),
              getUpperBound(tightening._variable),
              GlobalConfiguration::PREPROCESSOR_ALMOST_FIXED_THRESHOLD))
        setUpperBound(tightening._variable,
                      getLowerBound(tightening._variable));

      if (FloatUtils::gt(
              getLowerBound(tightening._variable),
              getUpperBound(tightening._variable),
              GlobalConfiguration::PREPROCESSOR_ALMOST_FIXED_THRESHOLD)) {
        throw InfeasibleQueryException();
      }
    }
  }

  return tighterBoundFound;
}

bool Preprocessor::processEquations() {
  enum {
    ZERO = 0,
    POSITIVE = 1,
    NEGATIVE = 2,
    INFINITE = 3,
  };

  bool tighterBoundFound = false;

  List<Equation> &equations(_preprocessed->getEquations());
  List<Equation>::iterator equation = equations.begin();

  while (equation != equations.end()) {
    // The equation is of the form sum (ci * xi) - b ? 0
    Equation::EquationType type = equation->_type;

    unsigned maxVar = equation->_addends.begin()->_variable;
    for (const auto &addend : equation->_addends) {
      if (addend._variable > maxVar) maxVar = addend._variable;
    }

    ++maxVar;

    double *ciTimesLb = new double[maxVar];
    double *ciTimesUb = new double[maxVar];
    char *ciSign = new char[maxVar];

    Set<unsigned> excludedFromLB;
    Set<unsigned> excludedFromUB;

    unsigned xi;
    double xiLB;
    double xiUB;
    double ci;
    double lowerBound;
    double upperBound;
    bool validLb;
    bool validUb;

    // The first goal is to compute the LB and UB of: sum (ci * xi) - b
    // For this we first identify unbounded variables
    double auxLb = -equation->_scalar;
    double auxUb = -equation->_scalar;
    for (const auto &addend : equation->_addends) {
      ci = addend._coefficient;
      xi = addend._variable;

      if (FloatUtils::isZero(ci)) {
        ciSign[xi] = ZERO;
        ciTimesLb[xi] = 0;
        ciTimesUb[xi] = 0;
        continue;
      }

      ciSign[xi] = ci > 0 ? POSITIVE : NEGATIVE;

      xiLB = getLowerBound(xi);
      xiUB = getUpperBound(xi);

      if (FloatUtils::isFinite(xiLB)) {
        ciTimesLb[xi] = ci * xiLB;
        if (ciSign[xi] == POSITIVE)
          auxLb += ciTimesLb[xi];
        else
          auxUb += ciTimesLb[xi];
      } else {
        if (ci > 0)
          excludedFromLB.insert(xi);
        else
          excludedFromUB.insert(xi);
      }

      if (FloatUtils::isFinite(xiUB)) {
        ciTimesUb[xi] = ci * xiUB;
        if (ciSign[xi] == POSITIVE)
          auxUb += ciTimesUb[xi];
        else
          auxLb += ciTimesUb[xi];
      } else {
        if (ci > 0)
          excludedFromUB.insert(xi);
        else
          excludedFromLB.insert(xi);
      }
    }

    // Now, go over each addend in sum (ci * xi) - b ? 0, and see what can be
    // done
    for (const auto &addend : equation->_addends) {
      ci = addend._coefficient;
      xi = addend._variable;

      // If ci = 0, nothing to do.
      if (ciSign[xi] == ZERO) continue;

      /*
        The expression for xi is:

             xi ? ( -1/ci ) * ( sum_{j\neqi} ( cj * xj ) - b )

        We use the previously computed auxLb and auxUb and adjust them because
        xi is removed from the sum. We also need to pay attention to the sign of
        ci, and to the presence of infinite bounds.

        Assuming "?" stands for equality, we can compute a LB if:
          1. ci is negative, and no vars except xi were excluded from the auxLb
          2. ci is positive, and no vars except xi were excluded from the auxUb

        And vice-versa for UB.

        In case "?" is GE or LE, only one direction can be computed.
      */
      if (ciSign[xi] == NEGATIVE) {
        validLb = ((type == Equation::LE) || (type == Equation::EQ)) &&
                  (excludedFromLB.empty() ||
                   (excludedFromLB.size() == 1 && excludedFromLB.exists(xi)));
        validUb = ((type == Equation::GE) || (type == Equation::EQ)) &&
                  (excludedFromUB.empty() ||
                   (excludedFromUB.size() == 1 && excludedFromUB.exists(xi)));
      } else {
        validLb = ((type == Equation::GE) || (type == Equation::EQ)) &&
                  (excludedFromUB.empty() ||
                   (excludedFromUB.size() == 1 && excludedFromUB.exists(xi)));
        validUb = ((type == Equation::LE) || (type == Equation::EQ)) &&
                  (excludedFromLB.empty() ||
                   (excludedFromLB.size() == 1 && excludedFromLB.exists(xi)));
      }

      if (validLb) {
        if (ciSign[xi] == NEGATIVE) {
          lowerBound = auxLb;
          if (!excludedFromLB.exists(xi)) lowerBound -= ciTimesUb[xi];
        } else {
          lowerBound = auxUb;
          if (!excludedFromUB.exists(xi)) lowerBound -= ciTimesUb[xi];
        }

        lowerBound /= -ci;

        if (FloatUtils::gt(lowerBound, getLowerBound(xi))) {
          tighterBoundFound = true;
          setLowerBound(xi, lowerBound);
        }
      }

      if (validUb) {
        if (ciSign[xi] == NEGATIVE) {
          upperBound = auxUb;
          if (!excludedFromUB.exists(xi)) upperBound -= ciTimesLb[xi];
        } else {
          upperBound = auxLb;
          if (!excludedFromLB.exists(xi)) upperBound -= ciTimesLb[xi];
        }

        upperBound /= -ci;

        if (FloatUtils::lt(upperBound, getUpperBound(xi))) {
          tighterBoundFound = true;
          setUpperBound(xi, upperBound);
        }
      }

      if (FloatUtils::gt(
              getLowerBound(xi), getUpperBound(xi),
              GlobalConfiguration::PREPROCESSOR_ALMOST_FIXED_THRESHOLD)) {
        delete[] ciTimesLb;
        delete[] ciTimesUb;
        delete[] ciSign;

        throw InfeasibleQueryException();
      }
    }

    delete[] ciTimesLb;
    delete[] ciTimesUb;
    delete[] ciSign;

    /*
      Next, do another sweep over the equation.
      Look for almost-fixed variables and fix them, and remove the equation
      entirely if it has nothing left to contribute.
    */
    bool allFixed = true;
    for (const auto &addend : equation->_addends) {
      unsigned var = addend._variable;
      double lb = getLowerBound(var);
      double ub = getUpperBound(var);

      if (FloatUtils::areEqual(
              lb, ub, GlobalConfiguration::PREPROCESSOR_ALMOST_FIXED_THRESHOLD))
        setUpperBound(var, getLowerBound(var));
      else
        allFixed = false;
    }

    if (!allFixed) {
      ++equation;
    } else {
      double sum = 0;
      for (const auto &addend : equation->_addends)
        sum += addend._coefficient * getLowerBound(addend._variable);

      if ((type == Equation::EQ && FloatUtils::areDisequal(
              sum, equation->_scalar,
              GlobalConfiguration::PREPROCESSOR_ALMOST_FIXED_THRESHOLD)) ||
          (type == Equation::LE &&
           FloatUtils::gt(sum, equation->_scalar,
                          GlobalConfiguration::PREPROCESSOR_ALMOST_FIXED_THRESHOLD)) ||
          (type == Equation::GE &&
           FloatUtils::lt(sum, equation->_scalar,
                          GlobalConfiguration::PREPROCESSOR_ALMOST_FIXED_THRESHOLD))
          ) {
        throw InfeasibleQueryException();
      }
      equation = equations.erase(equation);
      if (_statistics)
        _statistics->incUnsignedAttribute(Statistics::PP_NUM_EQUATIONS_REMOVED);
    }
  }

  return tighterBoundFound;
}

bool Preprocessor::processConstraints() {
  bool tighterBoundFound = false;

  for (auto &constraint : _preprocessed->getPLConstraints()) {
      auto vars = constraint->getParticipatingVariables();
      for (unsigned variable : vars) {
          if (constraint->participatingVariable(variable)){
              constraint->notifyLowerBound(variable, getLowerBound(variable));
              constraint->notifyUpperBound(variable, getUpperBound(variable));
          }
      }

    List<Tightening> tightenings;
    constraint->getEntailedTightenings(tightenings);

    for (const auto &tightening : tightenings) {
      if ((tightening._type == Tightening::LB) &&
          (FloatUtils::gt(tightening._value,
                          getLowerBound(tightening._variable)))) {
        tighterBoundFound = true;
        setLowerBound(tightening._variable, tightening._value);
      }

      else if ((tightening._type == Tightening::UB) &&
               (FloatUtils::lt(tightening._value,
                               getUpperBound(tightening._variable)))) {
        tighterBoundFound = true;
        setUpperBound(tightening._variable, tightening._value);
      }

      if (FloatUtils::areEqual(
              getLowerBound(tightening._variable),
              getUpperBound(tightening._variable),
              GlobalConfiguration::PREPROCESSOR_ALMOST_FIXED_THRESHOLD))
        setUpperBound(tightening._variable,
                      getLowerBound(tightening._variable));

      if (FloatUtils::gt(
              getLowerBound(tightening._variable),
              getUpperBound(tightening._variable),
              GlobalConfiguration::PREPROCESSOR_ALMOST_FIXED_THRESHOLD)) {
        throw InfeasibleQueryException();
      }
    }
  }

  return tighterBoundFound;
}

void Preprocessor::setStatistics(Statistics *statistics) {
  _statistics = statistics;
}

void Preprocessor::setMissingBoundsToInfinity() {
  for (unsigned i = 0; i < _preprocessed->getNumberOfVariables(); ++i) {
    if (!_preprocessed->getLowerBounds().exists(i))
      _preprocessed->setLowerBound(i, FloatUtils::negativeInfinity());
    if (!_preprocessed->getUpperBounds().exists(i))
      _preprocessed->setUpperBound(i, FloatUtils::infinity());
  }
}

void Preprocessor::dumpAllBounds(const String &message) {
  printf("\nPP: Dumping all bounds (%s)\n", message.ascii());

  for (unsigned i = 0; i < _preprocessed->getNumberOfVariables(); ++i) {
    printf("\tx%u: [%5.2lf, %5.2lf]\n", i, getLowerBound(i), getUpperBound(i));
  }

  printf("\n");
}

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
