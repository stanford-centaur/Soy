#include "OneHotConstraint.h"

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

OneHotConstraint::OneHotConstraint(const Set<unsigned> &elements)
    : PLConstraint(), _elements(elements) {
  unsigned phaseIndex = 0;
  for (const auto &element : elements) {
    PhaseStatus phase = static_cast<PhaseStatus>(phaseIndex);
    _phaseStatusToElement[phase] = element;
    _elementToPhaseStatus[element] = phase;
    ++phaseIndex;
  }
}

OneHotConstraint::~OneHotConstraint() {}

PLConstraint *OneHotConstraint::duplicateConstraint() const {
  ASSERT(!_context);
  OneHotConstraint *clone = new OneHotConstraint(_elements);
  *clone = *this;
  return clone;
}

void OneHotConstraint::addBooleanStructure() {
  // TODO: try ladder encoding or binary encoding?
  List<int> clause;
  List<PhaseStatus> allCases = getAllCases();
  for (const auto &phase : allCases) {
    int index = _satSolver->getFreshVariable();
    _satSolver->registerToWatchBVariable(this, index);
    _litToPhaseStatus[index] = phase;
    _phaseStatusToLit[phase] = index;
    clause.append(index);
  }
  _satSolver->addConstraint(clause);

  // add the constraint that one of the boolean variable is true
  for (int i = *clause.begin(); i <= *clause.rbegin(); ++i) {
    for (int j = i + 1; j <= *clause.rbegin(); ++j) {
      _satSolver->addConstraint({-i, -j});
    }
  }
}

PiecewiseLinearFunctionType OneHotConstraint::getType() const {
  return PiecewiseLinearFunctionType::ONE_HOT;
}

bool OneHotConstraint::participatingVariable(unsigned variable) const {
  return _elements.exists(variable);
}

List<unsigned> OneHotConstraint::getParticipatingVariables() const {
  List<unsigned> toReturn;
  for (const auto &element : _elements) toReturn.append(element);
  return toReturn;
}

void OneHotConstraint::notifyLowerBound(unsigned variable, double value) {
  ASSERT(participatingVariable(variable));

  if (existsLowerBound(variable) &&
      !FloatUtils::gt(value, getLowerBound(variable)))
    return;

  setLowerBound(variable, value);

  if (FloatUtils::isPositive(value)) {
    if (_context && _context->getLevel() == 0 && _satSolver) {
      int bVar = getLiteralOfPhaseStatus(_elementToPhaseStatus[variable]);
      _satSolver->addConstraint({bVar});
    }
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

void OneHotConstraint::notifyUpperBound(unsigned variable, double value) {
  ASSERT(participatingVariable(variable));
  if (existsUpperBound(variable) &&
      !FloatUtils::lt(value, getUpperBound(variable)))
    return;

  setUpperBound(variable, value);

  if (FloatUtils::lt(value, 1)) {
    if (_context && _context->getLevel() == 0 && _satSolver) {
      int bVar = getLiteralOfPhaseStatus(_elementToPhaseStatus[variable]);
      _satSolver->addConstraint({-bVar});
    }
    // std::cout << "Upper bound of x" << variable << " tightened to " << value
    // << std::endl;
    setUpperBound(variable, 0);
    if (!_context)
      eliminateElement(variable);
    else
      markInfeasiblePhase(_elementToPhaseStatus[variable]);
  }
}

void OneHotConstraint::notifyBVariable(int lit, bool value) {
  if (value)
    notifyLowerBound(_phaseStatusToElement[_litToPhaseStatus[lit]], 1);
  else
    notifyUpperBound(_phaseStatusToElement[_litToPhaseStatus[lit]], 0);
}

bool OneHotConstraint::satisfied() const {
  ASSERT(_assignmentManager);
  bool oneFound = false;
  for (const auto &element : _elements) {
    double currentValue = getAssignment(element);
    if (FloatUtils::areEqualInt(currentValue, 0))
      continue;
    else if (FloatUtils::areEqualInt(currentValue, 1) && !oneFound)
      oneFound = true;
    else
      return false;
  }
  return oneFound;
}

bool OneHotConstraint::satisfied(
    const Map<unsigned, double> &assignment) const {
  double sum = 0;
  for (const auto &element : _elements) {
    double currentValue = assignment[element];
    sum += currentValue;
    if (FloatUtils::areEqualInt(currentValue, 0) ||
        FloatUtils::areEqualInt(currentValue, 1))
      continue;
    else
      return false;
  }
  return FloatUtils::areEqual(sum, 1);
}

List<PhaseStatus> OneHotConstraint::getAllCases() const {
  // TODO: direction heuristics
  List<PhaseStatus> cases;
  for (unsigned element : _elements)
    cases.append(_elementToPhaseStatus[element]);
  return cases;
}

PiecewiseLinearCaseSplit OneHotConstraint::getCaseSplit(
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

void OneHotConstraint::getEntailedTightenings(
    List<Tightening> &tightening) const {
  if (_boundManager) {
    for (const auto &variable : getParticipatingVariables()) {
      tightening.append(Tightening(
          variable, _boundManager->getLowerBound(variable), Tightening::LB));
      tightening.append(Tightening(
          variable, _boundManager->getUpperBound(variable), Tightening::UB));
    }
  } else {
    for (const auto &pair : _lowerBounds)
      tightening.append(Tightening(pair.first, pair.second, Tightening::LB));
    for (const auto &pair : _upperBounds)
      tightening.append(Tightening(pair.first, pair.second, Tightening::UB));
  }
}

void OneHotConstraint::eliminateElement(unsigned variable) {
  if (_context)
    throw SoyError(SoyError::ELIMINATE_PHASE_AFTER_PREPROECESSING);
  _elements.erase(variable);
}

void OneHotConstraint::getCostFunctionComponent(LinearExpression &cost,
                                                PhaseStatus phase) const {
  if (isActive() && !(*_feasiblePhases[phase])) return;

  unsigned element = _phaseStatusToElement[phase];
  if (!cost._addends.exists(element)) cost._addends[element] = 0;
  cost._addends[element] -= 1;
  cost._constant += 1;
}

PhaseStatus OneHotConstraint::getPhaseStatusInAssignment() {
  auto byAssignment = [&](const unsigned &a, const unsigned &b) {
    return getAssignment(a) < getAssignment(b);
  };
  unsigned largestVariable =
      *std::max_element(_elements.begin(), _elements.end(), byAssignment);
  return _elementToPhaseStatus[largestVariable];
}

void OneHotConstraint::dump(String &s) const {
  s += "Feasible phase: ";
  for (const auto &pair : _feasiblePhases) {
    if (*pair.second)
      s += Stringf("%u ", ((unsigned) pair.first));
  }
  s += "\n";
}
