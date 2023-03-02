/*********************                                                        */
/*! \file MILPEncoder.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Andrew Wu, Teruhiro Tagomori
 ** This file is part of the Soy project.
 ** Copyright (c) 2023 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#ifndef __MILPEncoder_h__
#define __MILPEncoder_h__

#include "AbsoluteValueConstraint.h"
#include "BoundManager.h"
#include "DisjunctionConstraint.h"
#include "GurobiWrapper.h"
#include "InputQuery.h"
#include "IntegerConstraint.h"
#include "LinearExpression.h"
#include "MStringf.h"
#include "Map.h"
#include "OneHotConstraint.h"
#include "Statistics.h"

class MILPEncoder {
 public:
  MILPEncoder(const BoundManager &boundManager);

  void reset();

  /*
    Encode the input query as a Gurobi query, variables and inequalities
    are from inputQuery, and latest variable bounds are from tableau
  */
  void encodeInputQuery(GurobiWrapper &gurobi, const InputQuery &inputQuery,
                        bool relax = false);

  void encodeInputQueryForSteps(GurobiWrapper &gurobi,
                                const InputQuery &inputQuery,
                                const List<unsigned> &steps,
                                bool relax = false);

  /*
    get variable name from a variable in the encoded inputquery
  */
  String getVariableNameFromVariable(unsigned variable);

  inline void setStatistics(Statistics *statistics) {
    _statistics = statistics;
  }

  /*
    Encode the cost function into Gurobi
  */
  void encodeCostFunction(GurobiWrapper &gurobi, const LinearExpression &cost);

  /*
    Encode an (in)equality into Gurobi.
  */
  void encodeEquation(GurobiWrapper &gurobi, const Equation &Equation);

  void enforceIntegralConstraint(GurobiWrapper &gurobi);

  void relaxIntegralConstraint(GurobiWrapper &gurobi);

 private:
  /*
    BoundManager has the latest bound
  */
  const BoundManager &_boundManager;

  /*
    The statistics object.
  */
  Statistics *_statistics;

  /*
    Index for Guroby binary variables
  */
  unsigned _binVarIndex = 0;

  Set<unsigned> _binaryVariables;

  /*
    Encode an abs constraint f = Abs(b) into Gurobi
  */
  void encodeAbsoluteValueConstraint(GurobiWrapper &gurobi,
                                     AbsoluteValueConstraint *abs, bool relax);

  void encodeOneHotConstraint(GurobiWrapper &gurobi, OneHotConstraint *oneHot,
                              bool relax);

  void encodeDisjunctionConstraint(GurobiWrapper &gurobi,
                                   DisjunctionConstraint *disj, bool relax);

  void encodeIntegerConstraint(GurobiWrapper &gurobi,
                               IntegerConstraint *integer, bool relax);
};

#endif  // __MILPEncoder_h__
