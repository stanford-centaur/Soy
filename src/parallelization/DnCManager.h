/*********************                                                        */
/*! \file DnCManager.h
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

#ifndef __DnCManager_h__
#define __DnCManager_h__

#include <atomic>

#include "Engine.h"
#include "InputQuery.h"
#include "SubQuery.h"
#include "Vector.h"

class MpsParser;

#define DNC_MANAGER_LOG(x, ...) \
  LOG(GlobalConfiguration::DNC_MANAGER_LOGGING, "DnCManager: %s\n", x)

class DnCManager {
 public:
  enum DnCExitCode {
    UNSAT = 0,
    SAT = 1,
    ERROR = 2,
    TIMEOUT = 3,
    QUIT_REQUESTED = 4,

    NOT_DONE = 999,
  };

  DnCManager(InputQuery *inputQuery);

  ~DnCManager();

  void freeMemoryIfNeeded();

  /*
    Perform the Divide-and-conquer solving
  */
  void solve();

  /*
    Return the DnCExitCode of the DnCManager
  */
  DnCExitCode getExitCode() const;

  void extractSolution(const MpsParser &mpsParser, Map<String, double> &solution);

  /*
    Get the string representation of the exitcode
  */
  String getResultString();

  double getAverageTimePerSoICheck();

  double getAverageProposalPerDeepSoI();

  /*
    Print the result of DnC solving
  */
  void printResult();

  /*
    Store the solution into the map
  */
  void getSolution(std::map<int, double> &ret, InputQuery &inputQuery);

 private:
  /*
    Create and run a DnCWorker
  */
  static void dncSolve(WorkerQueue *workload, std::shared_ptr<Engine> engine,
                       std::unique_ptr<InputQuery> inputQuery,
                       std::atomic_int &numUnsolvedSubQueries,
                       std::atomic_bool &shouldQuitSolving, unsigned threadId,
                       unsigned verbosity, unsigned seed);

  /*
    Create the base engine from the network and property files,
    and if necessary, create engines for workers
  */
  bool createEngines(unsigned numberOfEngines);

  /*
    Read the exitCode of the engine of each thread, and update the manager's
    exitCode.
  */
  void updateDnCExitCode();

  /*
    Set _timeoutReached to true if timeout has been reached
  */
  void updateTimeoutReached(timespec startTime,
                            unsigned long long timeoutInMicroSeconds);

  /*
    The base engine that is used to perform the initial divides
  */
  std::shared_ptr<Engine> _baseEngine;

  /*
    The engines that are run in different threads
  */
  Vector<std::shared_ptr<Engine>> _engines;

  /*
    The engine with the satisfying assignment
  */
  std::shared_ptr<Engine> _engineWithSATAssignment;

  /*
    Alternatively, we could construct the DnCManager by directly providing the
    inputQuery instead of the network and property filepaths.
  */
  InputQuery *_baseInputQuery;

  /*
    The exit code of the DnCManager.
  */
  DnCExitCode _exitCode;

  /*
    Set of subQueries to be solved by workers
  */
  WorkerQueue *_workload;

  /*
    Whether the timeout has been reached
  */
  bool _timeoutReached;

  /*
    The number of currently unsolved sub queries
  */
  std::atomic_int _numUnsolvedSubQueries;

  /*
    The level of verbosity
  */
  unsigned _verbosity;
};

#endif  // __DnCManager_h__
