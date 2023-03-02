/*********************                                                        */
/*! \file BoundManager.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Haoze Wu, Aleksandar Zeljic
 ** This file is part of the Soy project.
 ** Copyright (c) 2023 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]
 **/

#include "BoundManager.h"

#include "Debug.h"
#include "FloatUtils.h"
#include "InfeasibleQueryException.h"
#include "Tightening.h"

using namespace CVC4::context;

BoundManager::BoundManager(Context &context) : _context(context), _size(0){};

BoundManager::~BoundManager() {
  for (unsigned i = 0; i < _size; ++i) {
    _lowerBounds[i]->deleteSelf();
    _upperBounds[i]->deleteSelf();
    _levelOfLastLowerBoundUpdate[i]->deleteSelf();
    _levelOfLastUpperBoundUpdate[i]->deleteSelf();
  }
};

unsigned BoundManager::registerNewVariable() {
  ASSERT(_lowerBounds.size() == _size);
  ASSERT(_upperBounds.size() == _size);

  unsigned newVar = _size++;

  _lowerBounds.append(new (true) CDO<double>(&_context));
  _upperBounds.append(new (true) CDO<double>(&_context));
  _levelOfLastLowerBoundUpdate.append(new (true) CDO<unsigned>(&_context));
  _levelOfLastUpperBoundUpdate.append(new (true) CDO<unsigned>(&_context));

  *_lowerBounds[newVar] = FloatUtils::negativeInfinity();
  *_upperBounds[newVar] = FloatUtils::infinity();
  *_levelOfLastLowerBoundUpdate[newVar] = _context.getLevel();
  *_levelOfLastUpperBoundUpdate[newVar] = _context.getLevel();

  ASSERT(_lowerBounds.size() == _size);
  ASSERT(_upperBounds.size() == _size);

  return newVar;
}

void BoundManager::initialize(unsigned numberOfVariables) {
  ASSERT(0 == _size);

  for (unsigned i = 0; i < numberOfVariables; ++i) registerNewVariable();

  ASSERT(numberOfVariables == _size);
}

bool BoundManager::tightenLowerBound(unsigned variable, double value) {
  bool tightened = setLowerBound(variable, value);
  return tightened;
}

bool BoundManager::tightenUpperBound(unsigned variable, double value) {
  bool tightened = setUpperBound(variable, value);
  return tightened;
}

bool BoundManager::setLowerBound(unsigned variable, double value) {
  ASSERT(variable < _size);
  if (value > getLowerBound(variable)) {
    *_lowerBounds[variable] = value;
    *_levelOfLastLowerBoundUpdate[variable] = _context.getLevel();
    if (!boundValid(variable)) throw InfeasibleQueryException();
    return true;
  }
  return false;
}

bool BoundManager::setUpperBound(unsigned variable, double value) {
  ASSERT(variable < _size);
  if (value < getUpperBound(variable)) {
    *_upperBounds[variable] = value;
    *_levelOfLastUpperBoundUpdate[variable] = _context.getLevel();
    if (!boundValid(variable)) throw InfeasibleQueryException();
    return true;
  }
  return false;
}

bool BoundManager::boundValid(unsigned variable) {
  ASSERT(variable < _size);
  return FloatUtils::gte(getUpperBound(variable), getLowerBound(variable));
}

unsigned BoundManager::getNumberOfVariables() const { return _size; }
