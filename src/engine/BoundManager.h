/*********************                                                        */
/*! \file BoundManager.h
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

#ifndef __BoundManager_h__
#define __BoundManager_h__

#include "Debug.h"
#include "List.h"
#include "Tightening.h"
#include "Vector.h"
#include "context/cdo.h"
#include "context/context.h"

typedef std::tuple<unsigned, double, bool> Bound;

class BoundManager {
 public:
  BoundManager(CVC4::context::Context &ctx);
  ~BoundManager();

  /*
   * Registers a new variable, grows the BoundManager size and bound vectors,
   * initializes bounds to +/-inf, and returns the new index as the new
   * variable.
   */
  unsigned registerNewVariable();

  void initialize(unsigned numberOfVariables);

  bool tightenLowerBound(unsigned variable, double value);
  bool tightenUpperBound(unsigned variable, double value);

  bool setLowerBound(unsigned variable, double value);
  bool setUpperBound(unsigned variable, double value);

  /*
    Returns true if the bounds for the variable is valid
  */
  bool boundValid(unsigned variable);

  double getLowerBound(unsigned variable) const {
    ASSERT(variable < _size);
    return *_lowerBounds[variable];
  }

  double getUpperBound(unsigned variable) const {
    ASSERT(variable < _size);
    return *_upperBounds[variable];
  }

  unsigned getLevelOfLastLowerUpdate(unsigned variable) const {
    ASSERT(variable < _size);
    return *_levelOfLastLowerBoundUpdate[variable];
  }

  unsigned getLevelOfLastUpperUpdate(unsigned variable) const {
    ASSERT(variable < _size);
    return *_levelOfLastUpperBoundUpdate[variable];
  }

  unsigned getNumberOfVariables() const;

 private:
  CVC4::context::Context &_context;
  unsigned _size;  // TODO: Make context sensitive, to account for growing
  // For now, assume variable number is the vector index
  Vector<CVC4::context::CDO<double> *> _lowerBounds;
  Vector<CVC4::context::CDO<double> *> _upperBounds;

  Vector<CVC4::context::CDO<unsigned> *> _levelOfLastLowerBoundUpdate;
  Vector<CVC4::context::CDO<unsigned> *> _levelOfLastUpperBoundUpdate;
};

#endif  // __BoundManager_h__
