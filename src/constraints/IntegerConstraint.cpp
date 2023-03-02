#include "IntegerConstraint.h"

#include <math.h>

#include <algorithm>

#include "CadicalWrapper.h"
#include "Debug.h"
#include "FloatUtils.h"
#include "GlobalConfiguration.h"
#include "InputQuery.h"
#include "List.h"
#include "MStringf.h"
#include "SoyError.h"
#include "Options.h"
#include "PiecewiseLinearCaseSplit.h"
#include "Statistics.h"

IntegerConstraint::IntegerConstraint(unsigned variable)
    : PLConstraint(), _variable(variable) {}

IntegerConstraint::~IntegerConstraint() {}

PLConstraint *IntegerConstraint::duplicateConstraint() const {
  IntegerConstraint *clone = new IntegerConstraint(_variable);
  *clone = *this;
  return clone;
}

void IntegerConstraint::addBooleanStructure() {}

PiecewiseLinearFunctionType IntegerConstraint::getType() const {
  return PiecewiseLinearFunctionType::INTEGER;
}

bool IntegerConstraint::participatingVariable(unsigned variable) const {
  return variable == _variable;
}

List<unsigned> IntegerConstraint::getParticipatingVariables() const {
  return {_variable};
}

void IntegerConstraint::notifyLowerBound(unsigned variable, double value) {
  if (existsLowerBound(variable) &&
      !FloatUtils::gt(value, getLowerBound(variable)))
    return;

  setLowerBound(variable, FloatUtils::roundUp(value));
}

void IntegerConstraint::notifyUpperBound(unsigned variable, double value) {
  if (existsUpperBound(variable) &&
      !FloatUtils::lt(value, getUpperBound(variable)))
    return;

  setUpperBound(variable, FloatUtils::roundDown(value));
}

bool IntegerConstraint::satisfied() const {
  return FloatUtils::isInteger(getAssignment(_variable));
}

PiecewiseLinearCaseSplit IntegerConstraint::getCaseSplitForInt(
    int value) const {
  PiecewiseLinearCaseSplit split;
  split.storeBoundTightening(Tightening(_variable, value, Tightening::LB));
  split.storeBoundTightening(Tightening(_variable, value, Tightening::UB));
  return split;
}

unsigned IntegerConstraint::getVariable() const { return _variable; }

void IntegerConstraint::getEntailedTightenings(
    List<Tightening> &tightening) const {
  ASSERT(!_context);
  for (const auto &pair : _lowerBounds)
    tightening.append(Tightening(pair.first, pair.second, Tightening::LB));
  for (const auto &pair : _upperBounds)
    tightening.append(Tightening(pair.first, pair.second, Tightening::UB));
}

void IntegerConstraint::dump(String &) const {
  // TODO
}
