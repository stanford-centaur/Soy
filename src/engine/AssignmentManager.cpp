/*********************                                                        */
/*! \file AssignmentManager.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Haoze Wu
 ** This file is part of the Soy project.
 ** Copyright (c) 2023 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]
 **/

#include "AssignmentManager.h"

#include "BoundManager.h"
#include "Debug.h"
#include "FloatUtils.h"
#include "GurobiWrapper.h"
#include "InfeasibleQueryException.h"
#include "MStringf.h"
#include "Tightening.h"

using namespace CVC4::context;

AssignmentManager::AssignmentManager(const BoundManager &bm)
    : _boundManager(bm),
      _numberOfVariables(_boundManager.getNumberOfVariables()) {
  initialize();
}

void AssignmentManager::initialize() {
  _numberOfVariables = _boundManager.getNumberOfVariables();
  for (unsigned i = 0; i < _numberOfVariables; ++i)
    _assignment.append(_boundManager.getLowerBound(i));
}

void AssignmentManager::setAssignment(unsigned variable, double value) {
  ASSERT(variable < _numberOfVariables);
  _assignment[variable] = value;
}

double AssignmentManager::getAssignment(unsigned variable) const {
  ASSERT(variable < _numberOfVariables);
  return _assignment[variable];
}

const Vector<double> &AssignmentManager::getAssignments() const {
  return _assignment;
}

double AssignmentManager::getReducedCost(unsigned variable) const {
  ASSERT(_reducedCost.exists(variable));
  return _reducedCost[variable];
}

void AssignmentManager::extractAssignmentFromGurobi(GurobiWrapper &gurobi,
                                                    const Set<unsigned>
                                                    &variables) {
  for (unsigned var : variables) {
    String variableName = Stringf("x%u", var);
    setAssignment(var, gurobi.getAssignment(variableName));
  }
}

void AssignmentManager::extractAssignmentFromGurobi(GurobiWrapper &gurobi) {
  for (unsigned i = 0; i < _numberOfVariables; ++i) {
    String variableName = Stringf("x%u", i);
    setAssignment(i, gurobi.getAssignment(variableName));
  }
}

void AssignmentManager::extractReducedCostFromGurobi(GurobiWrapper &gurobi,
                                                     const Set<unsigned>
                                                     &variables) {
  _reducedCost.clear();
  for (unsigned var : variables) {
    String variableName = Stringf("x%u", var);
    _reducedCost[var] = gurobi.getReducedCost(variableName);
  }
}

void AssignmentManager::dumpAssignment() const {
  std::cout << "Dumping current assignment..." << std::endl;
  for (unsigned i = 0; i < _numberOfVariables; ++i)
    std::cout << "x" << i << " = " << getAssignment(i) << std::endl;
  std::cout << "Dumping current assignment - done" << std::endl;
}
