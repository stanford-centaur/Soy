#include "DnCWorker.h"

#include <atomic>
#include <chrono>
#include <cmath>
#include <thread>

#include "Debug.h"
#include "Engine.h"
#include "MStringf.h"
#include "SoyError.h"
#include "PiecewiseLinearCaseSplit.h"
#include "SubQuery.h"

DnCWorker::DnCWorker(WorkerQueue *workload, std::shared_ptr<Engine> engine,
                     std::atomic_int &numUnsolvedSubQueries,
                     std::atomic_bool &shouldQuitSolving, unsigned threadId,
                     unsigned verbosity)
    : _workload(workload),
      _engine(engine),
      _numUnsolvedSubQueries(&numUnsolvedSubQueries),
      _shouldQuitSolving(&shouldQuitSolving),
      _threadId(threadId),
      _verbosity(verbosity) {}

void DnCWorker::popOneSubQueryAndSolve() {
  SubQuery *subQuery = NULL;
  // Boost queue stores the next element into the passed-in pointer
  // and returns true if the pop is successful (aka, the queue is not empty
  // in most cases)
  if (_workload->pop(subQuery)) {
    String queryId = subQuery->_queryId;
    auto split = std::move(subQuery->_split);
    unsigned timeoutInSeconds = subQuery->_timeoutInSeconds;

    // TODO: each worker is going to keep a map from *CaseSplit to an
    // object of class DnCStatistics, which contains some basic
    // statistics. The maps are owned by the DnCManager.

    // Apply the split and solve
    _engine->applySplit(*split);

    _engine->solve(timeoutInSeconds);
    Engine::ExitCode result = _engine->getExitCode();

    if (_verbosity > 0) printProgress(queryId, result);
    // Switch on the result
    if (result == Engine::UNSAT) {
      // If UNSAT, continue to solve
      *_numUnsolvedSubQueries -= 1;
      if (_numUnsolvedSubQueries->load() == 0) *_shouldQuitSolving = true;
      delete subQuery;
    } else if (result == Engine::QUIT_REQUESTED) {
      // If engine was asked to quit, quit
      std::cout << "Quit requested by manager!" << std::endl;
      delete subQuery;
      ASSERT(_shouldQuitSolving->load());
    } else {
      // We must set the quit flag to true  if the result is not UNSAT or
      // TIMEOUT. This way, the DnCManager will kill all the DnCWorkers.

      *_shouldQuitSolving = true;
      if (result == Engine::SAT) {
        // case SAT
        *_numUnsolvedSubQueries -= 1;
        delete subQuery;
      } else if (result == Engine::ERROR) {
        // case ERROR
        std::cout << "Error!" << std::endl;
        delete subQuery;
      } else  // result == Engine::NOT_DONE
      {
        // case NOT_DONE
        ASSERT(false);
        std::cout << "Not done! This should not happen." << std::endl;
        delete subQuery;
      }
    }
  } else {
    // If the queue is empty but the pop fails, wait and retry
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

void DnCWorker::printProgress(String queryId, Engine::ExitCode result) const {
  printf("Worker %d: Query %s %s, %d tasks remaining\n", _threadId,
         queryId.ascii(), exitCodeToString(result).ascii(),
         _numUnsolvedSubQueries->load());
}

String DnCWorker::exitCodeToString(Engine::ExitCode result) {
  switch (result) {
    case Engine::UNSAT:
      return "unsat";
    case Engine::SAT:
      return "sat";
    case Engine::ERROR:
      return "ERROR";
    case Engine::TIMEOUT:
      return "TIMEOUT";
    case Engine::QUIT_REQUESTED:
      return "QUIT_REQUESTED";
    default:
      ASSERT(false);
      return "UNKNOWN (this should never happen)";
  }
}
