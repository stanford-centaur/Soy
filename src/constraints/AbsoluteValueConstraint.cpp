#include "AbsoluteValueConstraint.h"

#include "Debug.h"
#include "FloatUtils.h"
#include "InputQuery.h"
#include "MStringf.h"
#include "SoyError.h"
#include "PiecewiseLinearCaseSplit.h"
#include "Statistics.h"

AbsoluteValueConstraint::AbsoluteValueConstraint(unsigned b, unsigned f)
    : PLConstraint(),
      _b(b),
      _f(f),
      _posAux(0),
      _negAux(0),
      _auxVarsInUse(false),
      _haveEliminatedVariables(false) {}

void AbsoluteValueConstraint::addBooleanStructure() {}

PLConstraint *AbsoluteValueConstraint::duplicateConstraint() const {
  AbsoluteValueConstraint *clone = new AbsoluteValueConstraint(_b, _f);
  *clone = *this;
  return clone;
}

void AbsoluteValueConstraint::transformToUseAuxVariables(
    InputQuery &inputQuery) {
  /*
    We want to add the two equations

        f >= b
        f >= -b

    Which actually becomes

        f - b - posAux = 0
        f + b - negAux = 0

    posAux is non-negative, and 0 if in the positive phase
    its upper bound is (f.ub - b.lb)

    negAux is also non-negative, and 0 if in the negative phase
    its upper bound is (f.ub + b.ub)
  */
  if (_auxVarsInUse) return;

  _posAux = inputQuery.getNumberOfVariables();
  _negAux = _posAux + 1;
  inputQuery.setNumberOfVariables(_posAux + 2);

  // Create and add the pos equation
  Equation posEquation(Equation::EQ);
  posEquation.addAddend(1.0, _f);
  posEquation.addAddend(-1.0, _b);
  posEquation.addAddend(-1.0, _posAux);
  posEquation.setScalar(0);
  inputQuery.addEquation(posEquation);

  // Create and add the neg equation
  Equation negEquation(Equation::EQ);
  negEquation.addAddend(1.0, _f);
  negEquation.addAddend(1.0, _b);
  negEquation.addAddend(-1.0, _negAux);
  negEquation.setScalar(0);
  inputQuery.addEquation(negEquation);

  // Both aux variables are non-negative
  inputQuery.setLowerBound(_posAux, 0);
  inputQuery.setLowerBound(_negAux, 0);

  setLowerBound(_posAux, 0);
  setLowerBound(_negAux, 0);
  setUpperBound(_posAux, FloatUtils::infinity());
  setUpperBound(_negAux, FloatUtils::infinity());

  // Mark that the aux vars are in use
  _auxVarsInUse = true;
}

bool AbsoluteValueConstraint::participatingVariable(unsigned variable) const {
  return (variable == _b) || (variable == _f) ||
         (_auxVarsInUse && (variable == _posAux || variable == _negAux));
}

List<unsigned> AbsoluteValueConstraint::getParticipatingVariables() const {
  return _auxVarsInUse ? List<unsigned>({_b, _f, _posAux, _negAux})
                       : List<unsigned>({_b, _f});
}

void AbsoluteValueConstraint::fixPhaseIfNeeded() {
  // Option 1: b's range is strictly positive
  if (existsLowerBound(_b) && getLowerBound(_b) >= 0) {
    setPhaseStatus(ABS_PHASE_POSITIVE);
    return;
  }

  // Option 2: b's range is strictly negative:
  if (existsUpperBound(_b) && getUpperBound(_b) <= 0) {
    setPhaseStatus(ABS_PHASE_NEGATIVE);
    return;
  }

  if (!existsLowerBound(_f)) return;

  // Option 3: f's range is strictly disjoint from b's positive
  // range
  if (existsUpperBound(_b) && getLowerBound(_f) > getUpperBound(_b)) {
    setPhaseStatus(ABS_PHASE_NEGATIVE);
    return;
  }

  // Option 4: f's range is strictly disjoint from b's negative
  // range, in absolute value
  if (existsLowerBound(_b) && getLowerBound(_f) > -getLowerBound(_b)) {
    setPhaseStatus(ABS_PHASE_POSITIVE);
    return;
  }

  if (_auxVarsInUse) {
    // Option 5: posAux has become zero, phase is positive
    if (existsUpperBound(_posAux) &&
        FloatUtils::isZero(getUpperBound(_posAux))) {
      setPhaseStatus(ABS_PHASE_POSITIVE);
      return;
    }

    // Option 6: posAux can never be zero, phase is negative
    if (existsLowerBound(_posAux) &&
        FloatUtils::isPositive(getLowerBound(_posAux))) {
      setPhaseStatus(ABS_PHASE_NEGATIVE);
      return;
    }

    // Option 7: negAux has become zero, phase is negative
    if (existsUpperBound(_negAux) &&
        FloatUtils::isZero(getUpperBound(_negAux))) {
      setPhaseStatus(ABS_PHASE_NEGATIVE);
      return;
    }

    // Option 8: negAux can never be zero, phase is positive
    if (existsLowerBound(_negAux) &&
        FloatUtils::isPositive(getLowerBound(_negAux))) {
      setPhaseStatus(ABS_PHASE_POSITIVE);
      return;
    }
  }
}

bool AbsoluteValueConstraint::phaseFixed() const {
  if (_context)
    return numberOfFeasiblePhases() == 1;
  else
    return _phaseStatus != PHASE_NOT_FIXED;
};

List<PhaseStatus> AbsoluteValueConstraint::getAllCases() const {
  if (_phaseStatus == PHASE_NOT_FIXED)
    return {ABS_PHASE_POSITIVE, ABS_PHASE_NEGATIVE};
  else {
    ASSERT(_phaseStatus == ABS_PHASE_POSITIVE ||
           _phaseStatus == ABS_PHASE_NEGATIVE);
    return {_phaseStatus};
  }
}

PiecewiseLinearCaseSplit AbsoluteValueConstraint::getCaseSplit(
    PhaseStatus phase) const {
  if (phase == ABS_PHASE_NEGATIVE)
    return getNegativeSplit();
  else if (phase == ABS_PHASE_POSITIVE)
    return getPositiveSplit();
  else
    throw SoyError(SoyError::REQUESTED_NONEXISTENT_CASE_SPLIT);
}

PiecewiseLinearCaseSplit AbsoluteValueConstraint::getNegativeSplit() const {
  ASSERT(_auxVarsInUse);
  // Negative phase: b <= 0, b + f = 0
  PiecewiseLinearCaseSplit negativePhase;
  negativePhase.storeBoundTightening(Tightening(_b, 0.0, Tightening::UB));

  negativePhase.storeBoundTightening(Tightening(_negAux, 0.0, Tightening::UB));

  return negativePhase;
}

PiecewiseLinearCaseSplit AbsoluteValueConstraint::getPositiveSplit() const {
  ASSERT(_auxVarsInUse);
  // Positive phase: b >= 0, b - f = 0
  PiecewiseLinearCaseSplit positivePhase;
  positivePhase.storeBoundTightening(Tightening(_b, 0.0, Tightening::LB));

  positivePhase.storeBoundTightening(Tightening(_posAux, 0.0, Tightening::UB));

  return positivePhase;
}

void AbsoluteValueConstraint::notifyLowerBound(unsigned variable,
                                               double bound) {
  if (_boundManager == nullptr &&
      !FloatUtils::gt(bound, getLowerBound(variable)))
    return;

  setLowerBound(variable, bound);
  // Update partner's bound
  if (variable == _b) {
    if (bound < 0) {
      double fUpperBound = FloatUtils::max(-bound, getUpperBound(_b));
      tightenUpperBound(_f, fUpperBound);

      if (_auxVarsInUse) {
        tightenUpperBound(_posAux, fUpperBound - bound);
      }
    }
  } else if (variable == _f) {
    // F's lower bound can only cause bound propagations if it
    // fixes the phase of the constraint, so no need to
    // bother. The only exception is if the lower bound is,
    // for some reason, negative
    if (bound < 0)
      tightenLowerBound(_f, 0);
    else if (FloatUtils::isNegative(getUpperBound(_b)))
      tightenUpperBound(_b, -bound);
  }

  // Any lower bound tightening on the aux variables, if they
  // are used, must have already fixed the phase, and needs not
  // be considered

  // Check whether the phase has become fixed
  fixPhaseIfNeeded();
}

void AbsoluteValueConstraint::notifyUpperBound(unsigned variable,
                                               double bound) {
  if (_boundManager == nullptr &&
      !FloatUtils::lt(bound, getUpperBound(variable)))
    return;

  setUpperBound(variable, bound);

  // Update partner's bound
  if (variable == _b) {
    if (bound > 0) {
      double fUpperBound = FloatUtils::max(bound, -getLowerBound(_b));
      tightenUpperBound(_f, fUpperBound);

      if (_auxVarsInUse) {
        tightenUpperBound(_negAux, fUpperBound + bound);
      }
    } else {
      // Phase is fixed, don't care about this case
    }
  } else if (variable == _f) {
    // F's upper bound can restrict both bounds of B
    tightenUpperBound(_b, bound);
    tightenLowerBound(_b, -bound);

    if (_auxVarsInUse) {
      if (existsLowerBound(_b)) {
        tightenUpperBound(_posAux, bound - getLowerBound(_b));
      }

      if (existsUpperBound(_b)) {
        tightenUpperBound(_negAux, bound + getUpperBound(_b));
      }
    }
  } else if (_auxVarsInUse) {
    if (variable == _posAux) {
      if (existsUpperBound(_b)) {
        tightenUpperBound(_f, getUpperBound(_b) + bound);
      }

      if (existsLowerBound(_f)) {
        tightenLowerBound(_b, getLowerBound(_f) - bound);
      }
    } else if (variable == _negAux) {
      if (existsLowerBound(_b)) {
        tightenUpperBound(_f, bound - getLowerBound(_b));
      }

      if (existsLowerBound(_f)) {
        tightenUpperBound(_b, bound - getLowerBound(_f));
      }
    }
  }

  // Check whether the phase has become fixed
  fixPhaseIfNeeded();
}

void AbsoluteValueConstraint::getEntailedTightenings(
    List<Tightening> &tightenings) const {
  ASSERT(!_context);
  for (const auto &pair : _lowerBounds)
    tightenings.append(Tightening(pair.first, pair.second, Tightening::LB));
  for (const auto &pair : _upperBounds)
    tightenings.append(Tightening(pair.first, pair.second, Tightening::UB));
}

bool AbsoluteValueConstraint::satisfied() const {
  double bValue = getAssignment(_b);
  double fValue = getAssignment(_f);

  // Possible violations:
  //   1. f is negative
  //   2. f is positive, abs(b) and f are not equal

  if (fValue < 0) return false;

  return FloatUtils::areEqualInt(FloatUtils::abs(bValue), fValue);
}

void AbsoluteValueConstraint::setPhaseStatus(PhaseStatus phase) {
  if (_context) {
    ASSERT(*_feasiblePhases[phase]);
    for (const auto &pair : _feasiblePhases)
      if (pair.first != phase) *pair.second = false;
  } else {
    _phaseStatus = phase;
  }
}

void AbsoluteValueConstraint::dump(String &output) const {
  output = Stringf(
      "AbsoluteValueCosntraint: x%u = Abs( x%u ). Active? %s. "
      "PhaseStatus = %u (%s).\n",
      _f, _b, _constraintActive ? "Yes" : "No", _phaseStatus,
      phaseToString(_phaseStatus).ascii());

  output += Stringf(
      "b in [%s, %s], ",
      existsLowerBound(_b) ? Stringf("%lf", getLowerBound(_b)).ascii() : "-inf",
      existsUpperBound(_b) ? Stringf("%lf", getUpperBound(_b)).ascii() : "inf");

  output += Stringf(
      "f in [%s, %s]",
      existsLowerBound(_f) ? Stringf("%lf", getLowerBound(_f)).ascii() : "-inf",
      existsUpperBound(_f) ? Stringf("%lf", getUpperBound(_f)).ascii() : "inf");

  if (_auxVarsInUse) {
    output += Stringf(". PosAux: %u. Range: [%s, %s]", _posAux,
                      existsLowerBound(_posAux)
                          ? Stringf("%lf", getLowerBound(_posAux)).ascii()
                          : "-inf",
                      existsUpperBound(_posAux)
                          ? Stringf("%lf", getUpperBound(_posAux)).ascii()
                          : "inf");

    output += Stringf(". NegAux: %u. Range: [%s, %s]", _negAux,
                      existsLowerBound(_negAux)
                          ? Stringf("%lf", getLowerBound(_negAux)).ascii()
                          : "-inf",
                      existsUpperBound(_negAux)
                          ? Stringf("%lf", getUpperBound(_negAux)).ascii()
                          : "inf");
  }
}

void AbsoluteValueConstraint::getCostFunctionComponent(
    LinearExpression &cost, PhaseStatus phase) const {
  // If the constraint is not active or is fixed, it contributes nothing
  if (!isActive() || phaseFixed()) return;

  ASSERT(phase == ABS_PHASE_NEGATIVE || phase == ABS_PHASE_POSITIVE);

  if (phase == ABS_PHASE_NEGATIVE) {
    // The cost term corresponding to the negative phase is f + b,
    // since the Abs is negative and satisfied iff f + b is 0
    // and minimal. This is true when we added the constraint
    // that f >= b and f >= -b.
    if (!cost._addends.exists(_f)) cost._addends[_f] = 0;
    if (!cost._addends.exists(_b)) cost._addends[_b] = 0;
    cost._addends[_f] += 1;
    cost._addends[_b] += 1;
  } else {
    // The cost term corresponding to the positive phase is f - b,
    // since the Abs is non-negative and satisfied iff f - b is 0 and
    // minimal. This is true when we added the constraint
    // that f >= b and f >= -b.
    if (!cost._addends.exists(_f)) cost._addends[_f] = 0;
    if (!cost._addends.exists(_b)) cost._addends[_b] = 0;
    cost._addends[_f] += 1;
    cost._addends[_b] -= 1;
  }
}

PhaseStatus AbsoluteValueConstraint::getPhaseStatusInAssignment() {
  return FloatUtils::isNegative(getAssignment(_b)) ? ABS_PHASE_NEGATIVE
                                                   : ABS_PHASE_POSITIVE;
}

String AbsoluteValueConstraint::phaseToString(PhaseStatus phase) {
  switch (phase) {
    case PHASE_NOT_FIXED:
      return "PHASE_NOT_FIXED";

    case ABS_PHASE_POSITIVE:
      return "ABS_PHASE_POSITIVE";

    case ABS_PHASE_NEGATIVE:
      return "ABS_PHASE_NEGATIVE";

    default:
      return "UNKNOWN";
  }
};
