/*********************                                                        */
/*! \file MILPEncoder.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Andrew Wu, Teruhiro Tagomori
 ** This file is part of the Soy project.
 ** Copyright (c) 2023 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief [[ Add one-line brief description here ]]
 **
 ** [[ Add lengthier description here ]]
 **/

#include "MILPEncoder.h"

#include "FloatUtils.h"
#include "GurobiWrapper.h"
#include "TimeUtils.h"

MILPEncoder::MILPEncoder(const BoundManager &boundManager)
    : _boundManager(boundManager), _statistics(NULL) {}

void MILPEncoder::reset(){
  _binVarIndex = 0;
  _binaryVariables.clear();
}

void MILPEncoder::encodeInputQuery(GurobiWrapper &gurobi,
                                   const InputQuery &inputQuery, bool relax) {
  // Add variables
  for (unsigned var = 0; var < inputQuery.getNumberOfVariables(); var++) {
    double lb = _boundManager.getLowerBound(var);
    double ub = _boundManager.getUpperBound(var);
    String varName = Stringf("x%u", var);
    gurobi.addVariable(varName, lb, ub);
  }

  // Add equations
  for (const auto &equation : inputQuery.getEquations()) {
    encodeEquation(gurobi, equation);
  }
  gurobi.updateModel();

  // Add Piecewise-linear Constraints
  for (const auto &plConstraint : inputQuery.getPLConstraints()) {
    switch (plConstraint->getType()) {
      case PiecewiseLinearFunctionType::ONE_HOT:
        encodeOneHotConstraint(gurobi, (OneHotConstraint *)plConstraint, relax);
        break;
      case PiecewiseLinearFunctionType::INTEGER:
        encodeIntegerConstraint(gurobi, (IntegerConstraint *)plConstraint,
                                relax);
        break;
      case PiecewiseLinearFunctionType::ABSOLUTE_VALUE:
        encodeAbsoluteValueConstraint(
            gurobi, (AbsoluteValueConstraint *)plConstraint, relax);
        break;
      case PiecewiseLinearFunctionType::DISJUNCT:
        encodeDisjunctionConstraint(
            gurobi, (DisjunctionConstraint *)plConstraint, relax);
        break;
      default:
        throw SoyError(
            SoyError::UNSUPPORTED_PIECEWISE_LINEAR_CONSTRAINT,
            "GurobiWrapper::encodeInputQuery: "
            "Unsupported piecewise-linear constraints\n");
    }
  }
}

void MILPEncoder::encodeInputQueryForSteps(GurobiWrapper &gurobi,
                                           const InputQuery &inputQuery,
                                           const List<unsigned> &steps,
                                           bool relax) {
  struct timespec start = TimeUtils::sampleMicro();

  // Add variables
  for (unsigned var = 0; var < inputQuery.getNumberOfVariables(); var++) {
    double lb = _boundManager.getLowerBound(var);
    double ub = _boundManager.getUpperBound(var);
    String varName = Stringf("x%u", var);
    gurobi.addVariable(varName, lb, ub);
  }

  // Add equations
  for (const auto &equation : inputQuery.getEquations()) {
    if (inputQuery.equationBelongsToStep(equation, steps))
      encodeEquation(gurobi, equation);
  }

  gurobi.updateModel();

  // Add Piecewise-linear Constraints
  for (const auto &plConstraint : inputQuery.getPLConstraints()) {
    if (!inputQuery.constraintBelongsToStep(plConstraint, steps)) continue;
    switch (plConstraint->getType()) {
      case PiecewiseLinearFunctionType::ONE_HOT:
        encodeOneHotConstraint(gurobi, (OneHotConstraint *)plConstraint, relax);
        break;
      case PiecewiseLinearFunctionType::INTEGER:
        encodeIntegerConstraint(gurobi, (IntegerConstraint *)plConstraint,
                                relax);
        break;
      case PiecewiseLinearFunctionType::ABSOLUTE_VALUE:
        encodeAbsoluteValueConstraint(
            gurobi, (AbsoluteValueConstraint *)plConstraint, relax);
        break;
      case PiecewiseLinearFunctionType::DISJUNCT:
        encodeDisjunctionConstraint(
            gurobi, (DisjunctionConstraint *)plConstraint, relax);
        break;
      default:
        throw SoyError(
            SoyError::UNSUPPORTED_PIECEWISE_LINEAR_CONSTRAINT,
            "GurobiWrapper::encodeInputQuery: "
            "Unsupported piecewise-linear constraints\n");
    }
  }
  gurobi.updateModel();

  if (_statistics) {
    struct timespec end = TimeUtils::sampleMicro();
    _statistics->incLongAttribute(
        Statistics::TIME_ADDING_CONSTRAINTS_TO_MILP_SOLVER_MICRO,
        TimeUtils::timePassed(start, end));
  }
}

String MILPEncoder::getVariableNameFromVariable(unsigned variable) {
  return Stringf("x%u", variable);
}

void MILPEncoder::encodeEquation(GurobiWrapper &gurobi,
                                 const Equation &equation) {
  List<GurobiWrapper::Term> terms;
  double scalar = equation._scalar;
  for (const auto &term : equation._addends)
    terms.append(
        GurobiWrapper::Term(term._coefficient, Stringf("x%u", term._variable)));
  switch (equation._type) {
    case Equation::EQ:
      gurobi.addEqConstraint(terms, scalar);
      break;
    case Equation::LE:
      gurobi.addLeqConstraint(terms, scalar);
      break;
    case Equation::GE:
      gurobi.addGeqConstraint(terms, scalar);
      break;
    default:
      break;
  }
}

void MILPEncoder::enforceIntegralConstraint(GurobiWrapper &gurobi) {
  for (const auto &var : _binaryVariables)
    gurobi.setVariableType(Stringf("x%u", var), 'B');
}

void MILPEncoder::relaxIntegralConstraint(GurobiWrapper &gurobi) {
  for (const auto &var : _binaryVariables)
    gurobi.setVariableType(Stringf("x%u", var), 'C');
}

void MILPEncoder::encodeAbsoluteValueConstraint(GurobiWrapper &gurobi,
                                                AbsoluteValueConstraint *abs,
                                                bool relax) {
  ASSERT(abs->auxVariablesInUse());

  if (!abs->isActive() || abs->phaseFixed()) {
    ASSERT(
        (FloatUtils::gte(_boundManager.getLowerBound(abs->getB()), 0) &&
         FloatUtils::lte(_boundManager.getUpperBound(abs->getPosAux()), 0)) ||
        (FloatUtils::lte(_boundManager.getUpperBound(abs->getB()), 0) &&
         FloatUtils::lte(_boundManager.getUpperBound(abs->getNegAux()), 0)));
    return;
  }

  unsigned sourceVariable = abs->getB();
  unsigned targetVariable = abs->getF();
  double sourceLb = _boundManager.getLowerBound(sourceVariable);
  double sourceUb = _boundManager.getUpperBound(sourceVariable);
  double targetUb = _boundManager.getUpperBound(targetVariable);

  ASSERT(FloatUtils::isPositive(sourceUb) && FloatUtils::isNegative(sourceLb));

  /*
    We have added f - b >= 0 and f + b >= 0. We add
    f - b <= (1 - a) * (ub_f - lb_b) and f + b <= a * (ub_f + ub_b)

    When a = 1, the constraints become:
    f - b <= 0, f + b <= ub_f + ub_b.
    When a = 0, the constriants become:
    f - b <= ub_f - lb_b, f + b <= 0
  */
  gurobi.addVariable(Stringf("a%u", _binVarIndex), 0, 1,
                     relax ? GurobiWrapper::CONTINUOUS : GurobiWrapper::BINARY);

  List<GurobiWrapper::Term> terms;
  terms.append(GurobiWrapper::Term(1, Stringf("x%u", targetVariable)));
  terms.append(GurobiWrapper::Term(-1, Stringf("x%u", sourceVariable)));
  terms.append(
      GurobiWrapper::Term(targetUb - sourceLb, Stringf("a%u", _binVarIndex)));
  gurobi.addLeqConstraint(terms, targetUb - sourceLb);

  terms.clear();
  terms.append(GurobiWrapper::Term(1, Stringf("x%u", targetVariable)));
  terms.append(GurobiWrapper::Term(1, Stringf("x%u", sourceVariable)));
  terms.append(GurobiWrapper::Term(-(targetUb + sourceUb),
                                   Stringf("a%u", _binVarIndex)));
  gurobi.addLeqConstraint(terms, 0);
  ++_binVarIndex;
}

void MILPEncoder::encodeOneHotConstraint(GurobiWrapper &gurobi,
                                         OneHotConstraint *oneHot, bool relax) {
  // if (!oneHot->isActive()) return;

  const auto &participatingVariables = oneHot->getParticipatingVariables();
  for (const auto &variable : participatingVariables) {
    gurobi.setVariableType(Stringf("x%u", variable), relax ? 'C' : 'B');
    _binaryVariables.insert(variable);
  }
}

void MILPEncoder::encodeDisjunctionConstraint(GurobiWrapper &gurobi,
                                              DisjunctionConstraint *disj,
                                              bool relax) {
  // if (!disj->isActive()) return;

  const auto &participatingVariables = disj->getParticipatingVariables();
  for (const auto &variable : participatingVariables) {
    gurobi.setVariableType(Stringf("x%u", variable), relax ? 'C' : 'B');
  }
}

void MILPEncoder::encodeIntegerConstraint(GurobiWrapper &gurobi,
                                          IntegerConstraint *integer, bool) {
  // if (!integer->isActive()) return;

  // Do not relax for now.
  gurobi.setVariableType(
      Stringf("x%u", *(integer->getParticipatingVariables().begin())), 'I');
}

void MILPEncoder::encodeCostFunction(GurobiWrapper &gurobi,
                                     const LinearExpression &cost) {
  List<GurobiWrapper::Term> terms;
  for (const auto &pair : cost._addends) {
    terms.append(GurobiWrapper::Term(pair.second, Stringf("x%u", pair.first)));
  }
  gurobi.setCost(terms, cost._constant);
}
