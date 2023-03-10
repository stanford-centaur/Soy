/*********************                                                        */
/*! \file DivideStrategy.h
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

#ifndef __DivideStrategy_h__
#define __DivideStrategy_h__

enum class DivideStrategy {
  PseudoImpact,  // The pseudo-impact heuristic associated with SoI.

  Topological,
};

#endif  // __DivideStrategy_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
