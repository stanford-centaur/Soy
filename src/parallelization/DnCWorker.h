/*********************                                                        */
/*! \file DnCWorker.h
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

#ifndef __DnCWorker_h__
#define __DnCWorker_h__

#include <atomic>

#include "Engine.h"
#include "PiecewiseLinearCaseSplit.h"
#include "SubQuery.h"

class DnCWorker {
 public:
  DnCWorker(WorkerQueue *workload, std::shared_ptr<Engine> engine,
            std::atomic_int &numUnsolvedSubqueries,
            std::atomic_bool &shouldQuitSolving, unsigned threadId,
            unsigned verbosity);

  /*
    Pop one subQuery, solve it and handle the result
    Return true if the DnCWorker should continue running
  */
  void popOneSubQueryAndSolve();

 private:
  /*
    Convert the exitCode to string
  */
  static String exitCodeToString(Engine::ExitCode result);

  /*
    Print the current progress
  */
  void printProgress(String queryId, Engine::ExitCode result) const;

  /*
    The queue of subqueries (shared across threads)
  */
  WorkerQueue *_workload;
  std::shared_ptr<Engine> _engine;

  /*
    The number of unsolved subqueries
  */
  std::atomic_int *_numUnsolvedSubQueries;

  /*
    A boolean denoting whether a solution has been found
  */
  std::atomic_bool *_shouldQuitSolving;

  unsigned _threadId;
  unsigned _verbosity;
};

#endif  // __DnCWorker_h__
