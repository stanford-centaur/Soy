/*********************                                                        */
/*! \file AssignmentManager.h
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

#ifndef __AssignmentManager_h__
#define __AssignmentManager_h__

#include "Debug.h"
#include "List.h"
#include "Map.h"
#include "Set.h"
#include "Vector.h"

class BoundManager;
class GurobiWrapper;
class AssignmentManager {
 public:
  AssignmentManager(const BoundManager &bm);

  void initialize();

  void setAssignment(unsigned variable, double value);
  double getAssignment(unsigned variable) const;
  const Vector<double> &getAssignments() const;
  double getReducedCost(unsigned variable) const;

  void extractAssignmentFromGurobi(GurobiWrapper &gurobi,
                                   const Set<unsigned>
                                   &variables);
  void extractAssignmentFromGurobi(GurobiWrapper &gurobi);
  void extractReducedCostFromGurobi(GurobiWrapper &gurobi,
                                    const Set<unsigned>
                                    &variables);
  void dumpAssignment() const;

 private:
  const BoundManager &_boundManager;
  unsigned _numberOfVariables;
  // For now, assume variable number is the vector index
  Vector<double> _assignment;
  Map<unsigned, double> _reducedCost;
};

#endif  // __AssignmentManager_h__
