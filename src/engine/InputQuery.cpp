#include "InputQuery.h"

#include "Debug.h"
#include "FloatUtils.h"
#include "MStringf.h"
#include "SoyError.h"

#define INPUT_QUERY_LOG(x, ...) \
  LOG(GlobalConfiguration::INPUT_QUERY_LOGGING, "Input Query: %s\n", x)

InputQuery::InputQuery() {}

InputQuery::~InputQuery() { freeConstraintsIfNeeded(); }

bool InputQuery::operator==(const InputQuery &ipq2) {
  const auto &pl1 = getPLConstraints();
  const auto &pl2 = ipq2.getPLConstraints();
  if (pl1.size() != pl2.size()) return false;
  auto iter1 = pl1.begin();
  auto iter2 = pl1.begin();
  while (iter1 != pl1.end()){
    if (*iter1 != *iter2) {
      return false;
    }
    ++iter1;
    ++iter2;
  }

  return (_lowerBounds == ipq2._lowerBounds &&
          _upperBounds == ipq2._upperBounds &&
          _equations == ipq2._equations);
}

void InputQuery::setNumberOfVariables(unsigned numberOfVariables) {
  _numberOfVariables = numberOfVariables;
}

void InputQuery::setLowerBound(unsigned variable, double bound) {
  if (variable >= _numberOfVariables) {
    throw SoyError(
        SoyError::VARIABLE_INDEX_OUT_OF_RANGE,
        Stringf("Variable = %u, number of variables = %u (setLowerBound)",
                variable, _numberOfVariables)
            .ascii());
  }

  _lowerBounds[variable] = bound;
}

void InputQuery::setUpperBound(unsigned variable, double bound) {
  if (variable >= _numberOfVariables) {
    throw SoyError(
        SoyError::VARIABLE_INDEX_OUT_OF_RANGE,
        Stringf("Variable = %u, number of variables = %u (setUpperBound)",
                variable, _numberOfVariables)
            .ascii());
  }

  _upperBounds[variable] = bound;
}

void InputQuery::addEquation(const Equation &equation) {
  if (!_equations.exists(equation))
    _equations.append(equation);
}

void InputQuery::removeEquation(const Equation &equation) {
    _equations.erase(equation);
}

unsigned InputQuery::getNumberOfVariables() const { return _numberOfVariables; }

double InputQuery::getLowerBound(unsigned variable) const {
  if (variable >= _numberOfVariables) {
    throw SoyError(
        SoyError::VARIABLE_INDEX_OUT_OF_RANGE,
        Stringf("Variable = %u, number of variables = %u (getLowerBound)",
                variable, _numberOfVariables)
            .ascii());
  }

  if (!_lowerBounds.exists(variable)) return FloatUtils::negativeInfinity();

  return _lowerBounds.get(variable);
}

double InputQuery::getUpperBound(unsigned variable) const {
  if (variable >= _numberOfVariables) {
    throw SoyError(
        SoyError::VARIABLE_INDEX_OUT_OF_RANGE,
        Stringf("Variable = %u, number of variables = %u (getUpperBound)",
                variable, _numberOfVariables)
            .ascii());
  }

  if (!_upperBounds.exists(variable)) return FloatUtils::infinity();

  return _upperBounds.get(variable);
}

List<Equation> &InputQuery::getEquations() { return _equations; }

const List<Equation> &InputQuery::getEquations() const { return _equations; }

void InputQuery::markVariableToStep(unsigned variable, unsigned step) {
  if (!_stepToVariables.exists(step)) _stepToVariables[step] = Set<unsigned>();
  _stepToVariables[step].insert(variable);
  _variableToStep[variable] = step;
}

unsigned InputQuery::getStepOfVariable(unsigned variable) const {
  return _variableToStep[variable];
}

Set<unsigned> InputQuery::getVariablesOfStep(unsigned step) const {
  if (_stepToVariables.exists(step))
    return _stepToVariables[step];
  else
    return Set<unsigned>();
}

List<unsigned> InputQuery::getStepsOfPLConstraint(
    const PLConstraint *constraint) const {
  Set<unsigned> steps;
  for (const auto &var : constraint->getParticipatingVariables())
    steps.insert(getStepOfVariable(var));
  List<unsigned> stepsList;
  for (const auto &step : steps) stepsList.append(step);
  return stepsList;
}

void InputQuery::setSolutionValue(unsigned variable, double value) {
  _solution[variable] = value;
}

double InputQuery::getSolutionValue(unsigned variable) const {
  if (!_solution.exists(variable))
    throw SoyError(SoyError::VARIABLE_DOESNT_EXIST_IN_SOLUTION,
                       Stringf("Variable: %u", variable).ascii());

  return _solution.get(variable);
}

void InputQuery::addPLConstraint(PLConstraint *constraint) {
  _plConstraints.append(constraint);
}

List<PLConstraint *> &InputQuery::getPLConstraints() { return _plConstraints; }

const List<PLConstraint *> &InputQuery::getPLConstraints() const {
  return _plConstraints;
}

bool InputQuery::equationBelongsToStep(const Equation &equation,
                                       const List<unsigned> &steps) const {
  const auto &vars = equation.getParticipatingVariables();
  for (const auto &step : steps)
    for (const auto &var : getVariablesOfStep(step))
      if (vars.exists(var)) return true;
  return false;
}

bool InputQuery::constraintBelongsToStep(const PLConstraint *constraint,
                                         const List<unsigned> &steps) const {
  const auto &vars = constraint->getParticipatingVariables();
  for (const auto &step : steps)
    for (const auto &var : getVariablesOfStep(step))
      if (vars.exists(var)) return true;
  return false;
}

unsigned InputQuery::countInfiniteBounds() {
  unsigned result = 0;

  for (const auto &lowerBound : _lowerBounds)
    if (lowerBound.second == FloatUtils::negativeInfinity()) ++result;

  for (const auto &upperBound : _upperBounds)
    if (upperBound.second == FloatUtils::infinity()) ++result;

  for (unsigned i = 0; i < _numberOfVariables; ++i) {
    if (!_lowerBounds.exists(i)) ++result;
    if (!_upperBounds.exists(i)) ++result;
  }
  return result;
}

InputQuery &InputQuery::operator=(const InputQuery &other) {
  INPUT_QUERY_LOG("Calling deep copy constructor...");

  _numberOfVariables = other._numberOfVariables;
  _equations = other._equations;
  _lowerBounds = other._lowerBounds;
  _upperBounds = other._upperBounds;
  _solution = other._solution;
  _variableToStep = other._variableToStep;
  _stepToVariables = other._stepToVariables;

  freeConstraintsIfNeeded();

  for (const auto &constraint : other._plConstraints)
    _plConstraints.append(constraint->duplicateConstraint());

  INPUT_QUERY_LOG("Calling deep copy constructor - done\n");
  return *this;
}

InputQuery::InputQuery(const InputQuery &other) { *this = other; }

void InputQuery::freeConstraintsIfNeeded() {
  for (auto &it : _plConstraints) delete it;

  _plConstraints.clear();
}

const Map<unsigned, double> &InputQuery::getLowerBounds() const {
  return _lowerBounds;
}

const Map<unsigned, double> &InputQuery::getUpperBounds() const {
  return _upperBounds;
}

void InputQuery::clearBounds() {
  _lowerBounds.clear();
  _upperBounds.clear();
}

void InputQuery::printAllBounds() const {
  printf("InputQuery: Dumping all bounds\n");

  for (unsigned i = 0; i < _numberOfVariables; ++i) {
    printf("\tx%u: [", i);
    if (_lowerBounds.exists(i))
      printf("%lf, ", _lowerBounds[i]);
    else
      printf("-INF, ");

    if (_upperBounds.exists(i))
      printf("%lf]", _upperBounds[i]);
    else
      printf("+INF]");
    printf("\n");
  }

  printf("\n\n");
}

void InputQuery::dump() const {
  printf("Total number of variables: %u\n", _numberOfVariables);
  printf("Total number of constraints: %u\n", _plConstraints.size());
  printf("Total number of equations: %u\n", _equations.size());
}

void InputQuery::checkSolutionCompliance(
    const Map<unsigned, double> &assignment) const {
  // Check bounds:
  printf("Checking bounds compliance...\n");
  for (unsigned i = 0; i < _numberOfVariables; ++i) {
    if (FloatUtils::lt(assignment[i], getLowerBound(i)))
      printf("x%u assignment %.5f violates lower bound %.5f.\n", i,
             assignment[i], getLowerBound(i));
    if (FloatUtils::gt(assignment[i], getUpperBound(i)))
      printf("x%u assignment %.5f violates upper bound %.5f.\n", i,
             assignment[i], getUpperBound(i));
  }
  printf("\n");

  // Check equations:
  printf("Checking equations compliance...\n");
  for (const auto &eq : _equations) {
    double sum = 0;
    for (const auto &addend : eq._addends)
      sum += addend._coefficient * assignment[addend._variable];

    if ((eq._type == Equation::EQ && !FloatUtils::areEqual(sum, eq._scalar)) ||
        (eq._type == Equation::LE && !FloatUtils::lte(sum, eq._scalar)) ||
        (eq._type == Equation::GE && !FloatUtils::gte(sum, eq._scalar))) {
      printf("((In)equality violated. LHS is %.5f, RHS is %.5f \n", sum,
             eq._scalar);
      eq.dump();
    }
  }
  printf("\n");

  // Check constraints:
  printf("Checking constraints compliance...\n");
  for (const auto &c : _plConstraints) {
    if (!c->satisfied(assignment)) {
      printf("PLConstraint violated.\n");
      String s;
      c->dump(s);
      std::cout << s.ascii() << std::endl;
      printf("Participating variable assignment:\n");
      for (const auto &var : c->getParticipatingVariables())
        printf("\tx%u = %.5f\n", var, assignment[var]);
    }
  }
}
