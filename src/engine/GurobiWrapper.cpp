/*********************                                                        */
/*! \file GurobiWrapper.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Haoze Andrew Wu, Teruhiro Tagomori
 ** This file is part of the Soy project.
 ** Copyright (c) 2023 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#ifdef ENABLE_GUROBI

#include "GurobiWrapper.h"

#include <iostream>

#include "Debug.h"
#include "FloatUtils.h"
#include "GlobalConfiguration.h"
#include "MStringf.h"
#include "Options.h"
#include "gurobi_c.h"

using namespace std;

// -------------------Methods for cons/destructing models -----------------//
GurobiWrapper::GurobiWrapper() : _environment(NULL), _model(NULL) {
  _environment = new GRBEnv;
  resetModel();
}

GurobiWrapper::~GurobiWrapper() { freeMemoryIfNeeded(); }

void GurobiWrapper::resetModel() {
  freeModelIfNeeded();
  _model = new GRBModel(*_environment);
  setVerbosity(0);
  setNumberOfThreads(1);
}

void GurobiWrapper::resetToUnsolvedState() {
  _model->reset();
}

void GurobiWrapper::freeModelIfNeeded() {
  for (auto &entry : _nameToVariable) {
    delete entry.second;
    entry.second = NULL;
  }
  _nameToVariable.clear();

  if (_model) {
    delete _model;
    _model = NULL;
  }
}

void GurobiWrapper::freeMemoryIfNeeded() {
  freeModelIfNeeded();

  if (_environment) {
    delete _environment;
    _environment = NULL;
  }
}

// ------------------------- Methods for adding constraints ---------------//
void GurobiWrapper::addVariable(const String &name, double lb, double ub,
                                VariableType type) {
  ASSERT(!_nameToVariable.exists(name));

  char variableType = GRB_CONTINUOUS;
  switch (type) {
    case CONTINUOUS:
      variableType = GRB_CONTINUOUS;
      break;

    case BINARY:
      variableType = GRB_BINARY;
      break;

    default:
      break;
  }

  try {
    GRBVar *newVar = new GRBVar;
    double objectiveValue = 0;
    *newVar =
        _model->addVar(lb, ub, objectiveValue, variableType, name.ascii());
    _nameToVariable[name] = newVar;
  } catch (GRBException e) {
    throw CommonError(
        CommonError::GUROBI_EXCEPTION,
        Stringf("Gurobi exception. Gurobi Code: %u, message: %s\n",
                e.getErrorCode(), e.getMessage().c_str())
            .ascii());
  }
}

void GurobiWrapper::setLowerBound(const String &name, double lb) {
  GRBVar var = _model->getVarByName(name.ascii());
  var.set(GRB_DoubleAttr_LB, lb);
}

void GurobiWrapper::setUpperBound(const String &name, double ub) {
  GRBVar var = _model->getVarByName(name.ascii());
  var.set(GRB_DoubleAttr_UB, ub);
}

double GurobiWrapper::getLowerBound(const String &name) {
  return _model->getVarByName(name.ascii()).get(GRB_DoubleAttr_LB);
}

double GurobiWrapper::getUpperBound(const String &name) {
  return _model->getVarByName(name.ascii()).get(GRB_DoubleAttr_UB);
}

double GurobiWrapper::getReducedCost(const String &name) {
  return _model->getVarByName(name.ascii()).get(GRB_DoubleAttr_RC);
}

void GurobiWrapper::setVariableType(const String &name, char type) {
  GRBVar var = _model->getVarByName(name.ascii());
  var.set(GRB_CharAttr_VType, type);
}

void GurobiWrapper::addLeqConstraint(const List<Term> &terms, double scalar,
                                     String name) {
  addConstraint(terms, scalar, GRB_LESS_EQUAL, name);
}

void GurobiWrapper::addGeqConstraint(const List<Term> &terms, double scalar,
                                     String name) {
  addConstraint(terms, scalar, GRB_GREATER_EQUAL, name);
}

void GurobiWrapper::addEqConstraint(const List<Term> &terms, double scalar,
                                    String name) {
  addConstraint(terms, scalar, GRB_EQUAL, name);
}

void GurobiWrapper::addConstraint(const List<Term> &terms, double scalar,
                                  char sense, String name) {
  try {
    GRBLinExpr constraint;
    for (const auto &term : terms) {
      ASSERT(_nameToVariable.exists(term._variable));
      constraint +=
          GRBLinExpr(*_nameToVariable[term._variable], term._coefficient);
    }

    _model->addConstr(constraint, sense, scalar, name.ascii());
  } catch (GRBException e) {
    throw CommonError(
        CommonError::GUROBI_EXCEPTION,
        Stringf("Gurobi exception. Gurobi Code: %u, message: %s\n",
                e.getErrorCode(), e.getMessage().c_str())
            .ascii());
  }
}

void GurobiWrapper::removeConstraintByName(const String &name) {
  _model->remove(_model->getConstrByName(name.ascii()));
}

void GurobiWrapper::setCost(const List<Term> &terms, double constant) {
  try {
    GRBLinExpr cost;

    for (const auto &term : terms) {
      ASSERT(_nameToVariable.exists(term._variable));
      cost += GRBLinExpr(*_nameToVariable[term._variable], term._coefficient);
    }

    cost += constant;

    _model->setObjective(cost, GRB_MINIMIZE);
  } catch (GRBException e) {
    throw CommonError(
        CommonError::GUROBI_EXCEPTION,
        Stringf("Gurobi exception. Gurobi Code: %u, message: %s\n",
                e.getErrorCode(), e.getMessage().c_str())
            .ascii());
  }
}

void GurobiWrapper::setObjective(const List<Term> &terms, double constant) {
  try {
    GRBLinExpr cost;

    for (const auto &term : terms) {
      ASSERT(_nameToVariable.exists(term._variable));
      cost += GRBLinExpr(*_nameToVariable[term._variable], term._coefficient);
    }

    cost += constant;

    _model->setObjective(cost, GRB_MAXIMIZE);
  } catch (GRBException e) {
    throw CommonError(
        CommonError::GUROBI_EXCEPTION,
        Stringf("Gurobi exception. Gurobi Code: %u, message: %s\n",
                e.getErrorCode(), e.getMessage().c_str())
            .ascii());
  }
}

void GurobiWrapper::updateModel() { _model->update(); }

// ----------------------- Methods for solving ----------------------------//
void GurobiWrapper::setNodeLimit(double limit) {
  _model->set(GRB_DoubleParam_NodeLimit, limit);
}

void GurobiWrapper::setCutoff(double cutoff) {
  _model->set(GRB_DoubleParam_Cutoff, cutoff);
}

void GurobiWrapper::setTimeLimit(double seconds) {
  _model->set(GRB_DoubleParam_TimeLimit, seconds);
}

void GurobiWrapper::setVerbosity(unsigned verbosity) {
  _model->getEnv().set(GRB_IntParam_OutputFlag, verbosity);
}

void GurobiWrapper::setNumberOfThreads(unsigned threads) {
  _model->getEnv().set(GRB_IntParam_Threads, threads);
}

void GurobiWrapper::setMethod(int method) {
  /*
    -1=automatic,
    0=primal simplex,
    1=dual simplex,
    2=barrier,
    3=concurrent,
    4=deterministic concurrent, and
    5=deterministic concurrent simplex.
  */
  _model->getEnv().set(GRB_IntParam_Method, method);
}

void GurobiWrapper::computeIIS(int method) {
  try {
    _model->getEnv().set(GRB_IntParam_IISMethod, method);
    _model->computeIIS();
  } catch (GRBException e) {
    throw CommonError(
        CommonError::GUROBI_EXCEPTION,
        Stringf("Gurobi exception. Gurobi Code: %u, message: %s\n",
                e.getErrorCode(), e.getMessage().c_str())
            .ascii());
  }
}

void GurobiWrapper::solve() {
  try {
    _model->optimize();
    log(Stringf("Model status: %u\n", _model->get(GRB_IntAttr_Status)));
  } catch (GRBException e) {
    throw CommonError(
        CommonError::GUROBI_EXCEPTION,
        Stringf("Gurobi exception. Gurobi Code: %u, message: %s\n",
                e.getErrorCode(), e.getMessage().c_str())
            .ascii());
  }
}

void GurobiWrapper::loadMPS(String filename) {
  try {
    freeModelIfNeeded();
    _model = new GRBModel(*_environment, filename.ascii());
    setNumberOfThreads(1);
    log(Stringf("Model status: %u\n", _model->get(GRB_IntAttr_Status)));
  } catch (GRBException e) {
    throw CommonError(
      CommonError::GUROBI_EXCEPTION,
      Stringf("Gurobi exception. Gurobi Code: %u, message: %s\n",
              e.getErrorCode(), e.getMessage().c_str())
      .ascii());
  }
}


// --------------------- Methods for retreive results ---------------------//
unsigned GurobiWrapper::getStatusCode() {
  return _model->get(GRB_IntAttr_Status);
}

bool GurobiWrapper::optimal() {
  return _model->get(GRB_IntAttr_Status) == GRB_OPTIMAL;
}

bool GurobiWrapper::cutoffOccurred() {
  return _model->get(GRB_IntAttr_Status) == GRB_CUTOFF;
}

bool GurobiWrapper::infeasible() {
  return _model->get(GRB_IntAttr_Status) == GRB_INFEASIBLE;
}

bool GurobiWrapper::timeout() {
  return _model->get(GRB_IntAttr_Status) == GRB_TIME_LIMIT;
}

bool GurobiWrapper::haveFeasibleSolution() {
  return _model->get(GRB_IntAttr_SolCount) > 0;
}

void GurobiWrapper::extractIIS(Map<String, GurobiWrapper::IISBoundType> &bounds,
                               List<String> &constraints,
                               const List<String> &constraintNames) {
  for (const auto &variable : _nameToVariable) {
    if (variable.second->get(GRB_IntAttr_IISLB))
      bounds[variable.first] = IIS_LB;
    if (variable.second->get(GRB_IntAttr_IISUB))
      bounds[variable.first] = IIS_UB;
    if (variable.second->get(GRB_IntAttr_IISLB) &&
        variable.second->get(GRB_IntAttr_IISUB))
      bounds[variable.first] = IIS_BOTH;
  }

  for (const auto &name : constraintNames) {
    if (_model->getConstrByName(name.ascii()).get(GRB_IntAttr_IISConstr))
      constraints.append(name);
  }
}

double GurobiWrapper::getAssignment(const String &variable) {
  return _nameToVariable[variable]->get(GRB_DoubleAttr_X);
}

double GurobiWrapper::getObjectiveBound() {
  try {
    return _model->get(GRB_DoubleAttr_ObjBound);
  } catch (GRBException e) {
    log("Failed to get objective bound from Gurobi.");
    if (e.getErrorCode() == GRB_ERROR_DATA_NOT_AVAILABLE) {
      // From
      // https://www.gurobi.com/documentation/9.0/refman/py_model_setattr.html
      // due to our lazy update approach, the change won't actually take effect
      // until you update the model (using Model.update), optimize the model
      // (using Model.optimize), or write the model to disk (using Model.write).
      _model->update();

      if (_model->get(GRB_IntAttr_ModelSense) == 1)
        // case minimize
        return FloatUtils::negativeInfinity();
      else
        // case maximize
        return FloatUtils::infinity();
    }
    throw CommonError(
        CommonError::GUROBI_EXCEPTION,
        Stringf("Gurobi exception. Gurobi Code: %u, message: %s\n",
                e.getErrorCode(), e.getMessage().c_str())
            .ascii());
  }
}

double GurobiWrapper::getObjectiveValue() {
  return _model->get(GRB_DoubleAttr_ObjVal);
}

void GurobiWrapper::extractSolution(Map<String, double> &values,
                                    double &costOrObjective) {
  try {
    updateModel();
    values.clear();

    for (const auto &variable : _nameToVariable)
      values[variable.first] = variable.second->get(GRB_DoubleAttr_X);

    costOrObjective = _model->get(GRB_DoubleAttr_ObjVal);
  } catch (GRBException e) {
    throw CommonError(
        CommonError::GUROBI_EXCEPTION,
        Stringf("Gurobi exception. Gurobi Code: %u, message: %s\n",
                e.getErrorCode(), e.getMessage().c_str())
            .ascii());
  }
}

unsigned GurobiWrapper::getNumberOfNodes() {
  return _model->get(GRB_DoubleAttr_NodeCount);
}

// -------------------------- Debug methods -------------------------------//
void GurobiWrapper::dumpModel(const String &name) {
  _model->write(name.ascii());
}

void GurobiWrapper::dumpSolution() {
  std::cout << "Dumping current solution:" << std::endl;
  Map<String, double> values;
  double cost;
  extractSolution(values, cost);
  for (const auto &pair : values)
    std::cout << pair.first.ascii() << " " << pair.second << std::endl;
}

void GurobiWrapper::log(const String &message) {
  if (GlobalConfiguration::GUROBI_LOGGING)
    printf("GurobiWrapper: %s\n", message.ascii());
}

#endif  // ENABLE_GUROBI
