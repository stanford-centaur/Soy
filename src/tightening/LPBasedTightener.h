/*********************                                                        */
/*! \file LPBasedTightener.h
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

#ifndef __LPBasedTightener_h__
#define __LPBasedTightener_h__

#include "List.h"
#include "PLConstraint.h"

class BoundManager;
class GurobiWrapper;
class InputQuery;
class MILPEncoder;
class Tightening;

class LPBasedTightener {
 public:
  LPBasedTightener(const BoundManager &boundManager,
                   const InputQuery &inputQuery);

  void computeKPattern(List<Vector<PhaseStatus>> &lemmas, unsigned K);

  unsigned performLPBasedTightening();

 private:
  static void getStepsToEncode(List<unsigned> &steps);

  const BoundManager &_boundManager;
  const InputQuery &_inputQuery;
};

#endif  // __LPBasedTightener_h__
