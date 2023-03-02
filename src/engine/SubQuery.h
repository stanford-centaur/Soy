/*********************                                                        */
/*! \file SubQuery.h
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

#ifndef __SubQuery_h__
#define __SubQuery_h__

#include <boost/lockfree/queue.hpp>
#include <utility>

#include "List.h"
#include "MString.h"
#include "PiecewiseLinearCaseSplit.h"

// Struct representing a subquery
struct SubQuery {
  SubQuery() {}

  String _queryId;
  std::unique_ptr<PiecewiseLinearCaseSplit> _split;
  unsigned _timeoutInSeconds;
  unsigned _depth;
};

// Synchronized Queue containing the Sub-Queries shared by workers
typedef boost::lockfree::queue<SubQuery *, boost::lockfree::fixed_sized<false>>
    WorkerQueue;

// A vector of Sub-Queries

// Guy: consider using our wrapper class Vector instead of std::vector
typedef List<SubQuery *> SubQueries;

#endif  // __SubQuery_h__
