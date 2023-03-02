#include "DisjunctionConstraint.h"

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

DisjunctionConstraint::DisjunctionConstraint(const Set<unsigned> &elements)
    : PLConstraint(), _elements(elements) {
  unsigned phaseIndex = 0;
  for (const auto &element : elements) {
    PhaseStatus phase = static_cast<PhaseStatus>(phaseIndex);
    _phaseStatusToElement[phase] = element;
    _elementToPhaseStatus[element] = phase;
    ++phaseIndex;
  }
}

DisjunctionConstraint::~DisjunctionConstraint() {}

PLConstraint *DisjunctionConstraint::duplicateConstraint() const {
  ASSERT(!_context);
  DisjunctionConstraint *clone = new DisjunctionConstraint(_elements);
  *clone = *this;
  return clone;
}

void DisjunctionConstraint::addBooleanStructure() {
  List<int> clause;
  List<PhaseStatus> allCases = getAllCases();
  for (const auto &phase : allCases) {
    int index = _satSolver->getFreshVariable();
    _litToPhaseStatus[index] = phase;
    _phaseStatusToLit[phase] = index;
    clause.append(index);
  }
  _satSolver->addConstraint(clause);
}

PiecewiseLinearFunctionType DisjunctionConstraint::getType() const {
  return PiecewiseLinearFunctionType::DISJUNCT;
}

bool DisjunctionConstraint::participatingVariable(unsigned variable) const {
  return _elements.exists(variable);
}

List<unsigned> DisjunctionConstraint::getParticipatingVariables() const {
  List<unsigned> toReturn;
  for (const auto &element : _elements) toReturn.append(element);
  return toReturn;
}

void DisjunctionConstraint::notifyLowerBound(unsigned variable, double value) {
  if (existsLowerBound(variable) &&
      !FloatUtils::gt(value, getLowerBound(variable)))
    return;

  setLowerBound(variable, value);

  if (FloatUtils::isPositive(value)) {
    List<unsigned> toRemove;
    for (const auto &element : _elements) {
      if (element == variable)
        setLowerBound(element, 1);
      else {
        setUpperBound(element, 0);
        if (!_context)
          toRemove.append(element);
        else
          markInfeasiblePhase(_elementToPhaseStatus[element]);
      }
    }
    for (const auto &element : toRemove) eliminateElement(element);
  }
}

void DisjunctionConstraint::notifyUpperBound(unsigned variable, double value) {
  if (existsUpperBound(variable) &&
      !FloatUtils::lt(value, getUpperBound(variable)))
    return;

  setUpperBound(variable, value);

  if (FloatUtils::lt(value, 1)) {
    setUpperBound(variable, 0);
    if (!_context)
      eliminateElement(variable);
    else
      markInfeasiblePhase(_elementToPhaseStatus[variable]);
  }
}

bool DisjunctionConstraint::satisfied() const {
  ASSERT(_assignmentManager);

  bool oneFound = false;
  for (const auto &element : _elements) {
    double currentValue = getAssignment(element);
    if (FloatUtils::areEqualInt(currentValue, 1))
      oneFound = true;
    else if (FloatUtils::gt(currentValue, 1) || FloatUtils::lt(currentValue, 0))
      return false;
  }
  return oneFound;
}

List<PhaseStatus> DisjunctionConstraint::getAllCases() const {
  // TODO: direction heuristics
  List<PhaseStatus> cases;
  for (unsigned element : _elements)
    cases.append(_elementToPhaseStatus[element]);
  return cases;
}

PiecewiseLinearCaseSplit DisjunctionConstraint::getCaseSplit(
    PhaseStatus phase) const {
  PiecewiseLinearCaseSplit split;
  for (const auto &element : _elements) {
    if (_phaseStatusToElement[phase] == element)
      split.storeBoundTightening(Tightening(element, 1, Tightening::LB));
    else
      split.storeBoundTightening(Tightening(element, 0, Tightening::UB));
  }
  return split;
}

void DisjunctionConstraint::getEntailedTightenings(
    List<Tightening> &tightening) const {
  ASSERT(!_context);
  for (const auto &pair : _lowerBounds)
    tightening.append(Tightening(pair.first, pair.second, Tightening::LB));
  for (const auto &pair : _upperBounds)
    tightening.append(Tightening(pair.first, pair.second, Tightening::UB));
}

void DisjunctionConstraint::eliminateElement(unsigned variable) {
  if (_context)
    throw SoyError(SoyError::ELIMINATE_PHASE_AFTER_PREPROECESSING);
  _elements.erase(variable);
}

void DisjunctionConstraint::getCostFunctionComponent(LinearExpression &cost,
                                                     PhaseStatus phase) const {
  if (isActive() && !(*_feasiblePhases[phase])) return;

  unsigned element = _phaseStatusToElement[phase];
  if (!cost._addends.exists(element)) cost._addends[element] = 0;
  cost._addends[element] -= 1;
  cost._constant += 1;
}

PhaseStatus DisjunctionConstraint::getPhaseStatusInAssignment() {
  auto byAssignment = [&](const unsigned &a, const unsigned &b) {
    return getAssignment(a) < getAssignment(b);
  };
  unsigned largestVariable =
      *std::max_element(_elements.begin(), _elements.end(), byAssignment);
  return _elementToPhaseStatus[largestVariable];
}

void DisjunctionConstraint::dump(String &) const {
  // TODO
}
