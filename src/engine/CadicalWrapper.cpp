#include "CadicalWrapper.h"

#include "CommonError.h"
#include "Debug.h"
#include "Statistics.h"

using namespace CaDiCaL;

// -------------------Methods for cons/destructing models -----------------//
CadicalWrapper::CadicalWrapper()
    : _satSolver(nullptr), _numberOfBooleanVariables(0), _statistics(nullptr) {}

CadicalWrapper::CadicalWrapper(const CadicalWrapper &other)
    : _satSolver(nullptr),
      _numberOfBooleanVariables(other._numberOfBooleanVariables),
      _statistics(nullptr) {
  _constraints = other._constraints;
  _assumptions = other._assumptions;
}

CaDiCaL::Solver *CadicalWrapper::getCadicalInstance() const {
  CaDiCaL::Solver *cadical = new CaDiCaL::Solver();
  cadical->set("quiet", true);
  return cadical;
}

CadicalWrapper::~CadicalWrapper() {
  if (_satSolver) delete _satSolver;
  _satSolver = nullptr;
}

// ------------------------- Methods for adding constraints ---------------//
unsigned CadicalWrapper::getFreshVariable() {
  return ++_numberOfBooleanVariables;
}

void CadicalWrapper::addConstraint(const List<int> &constraint) {
  _constraints.append(constraint);
  DEBUG({
    String s;
    for (const auto &lit : constraint) s += Stringf("%d ", lit);
  });

  if (_statistics)
    _statistics->incUnsignedAttribute(Statistics::NUM_SAT_CONSTRAINTS);
}

void CadicalWrapper::assumeLiteral(int lit) { _assumptions.append(lit); }

void CadicalWrapper::clearAssumptions() { _assumptions.clear(); }

void CadicalWrapper::setDirection(int lit) {
  if (_phase.exists(lit)) _phase.erase(lit);
  if (_phase.exists(-lit)) _phase.erase(-lit);
  _phase.append(lit);
}

void CadicalWrapper::resetDirection(unsigned bVariable) {
  if (_phase.exists(-bVariable)) _phase.erase(-bVariable);
  if (_phase.exists(bVariable)) _phase.erase(bVariable);
}

void CadicalWrapper::resetAllDirections() { _phase.clear(); }

unsigned CadicalWrapper::directionAwareSolve() {
  if (_satSolver != nullptr) delete _satSolver;
  _satSolver = getCadicalInstance();

  for (const auto &constraint : _constraints) {
    for (const auto &lit : constraint) {
      ASSERT(std::abs(lit) <= _numberOfBooleanVariables);
      _satSolver->add(lit);
    }
    _satSolver->add(0);
  }

  for (const auto &lit : _assumptions) {
    _satSolver->add(lit);
    _satSolver->add(0);
  }

  List<int> phase = _phase;
  unsigned numRejected = 0;
  // Try to assume as many literals in _phase as possible
  do {
    for (const auto &l : phase) {
      _satSolver->assume(l);
    }
    _satSolver->solve();
    if (_satSolver->status() == 20) {
      // Remove one from the _phase and try again
      for (const auto &l : phase) {
        if (_satSolver->failed(l)) {
          phase.erase(l);
          ++numRejected;
          break;
        }
      }
    } else {
      ASSERT(_satSolver->status() == 10);
      break;
    }
  } while (!phase.empty());

  if (infeasible())
    _satSolver->solve();
  return numRejected;
}

void CadicalWrapper::solve() {
  if (_satSolver != nullptr) delete _satSolver;
  _satSolver = getCadicalInstance();

  for (const auto &constraint : _constraints) {
    for (const auto &lit : constraint) {
      ASSERT(std::abs(lit) <= _numberOfBooleanVariables);
      _satSolver->add(lit);
    }
    _satSolver->add(0);
  }

  for (const auto &lit : _assumptions) {
    _satSolver->add(lit);
    _satSolver->add(0);
  }

  for (const auto &lit : _phase) _satSolver->phase(lit);

  _satSolver->solve();
}

void CadicalWrapper::preprocess() {
  if (_satSolver != nullptr) delete _satSolver;
  _satSolver = getCadicalInstance();

  for (const auto &constraint : _constraints) {
    for (const auto &lit : constraint) {
      ASSERT(std::abs(lit) <= _numberOfBooleanVariables);
      _satSolver->add(lit);
    }
    _satSolver->add(0);
  }

  for (const auto &lit : _assumptions) {
    _satSolver->add(lit);
    _satSolver->add(0);
  }
  _satSolver->simplify(5);
}

// --------------------- Methods for retreive results ---------------------//
bool CadicalWrapper::infeasible() { return _satSolver->status() == 20; }

bool CadicalWrapper::haveFeasibleSolution() {
  return _satSolver->status() == 10;
}

LiteralStatus CadicalWrapper::getAssignment(int lit) {
  int value = _satSolver->val(lit);
  if ((value == lit && lit > 0) || (value == -lit && lit < 0))
    return TRUE;
  else {
    ASSERT((value == lit && lit < 0) || (value == -lit && lit > 0));
    return FALSE;
  }
}

void CadicalWrapper::extractSolution(Map<unsigned, bool> &values) {
  values.clear();

  for (unsigned bVar = 1; bVar <= _numberOfBooleanVariables; ++bVar)
    values[bVar] = getAssignment(bVar);
}

LiteralStatus CadicalWrapper::getLiteralStatus(int lit) {
  int value = _satSolver->fixed(lit);
  if (value == 0)
    return UNFIXED;
  else if (value == 1)
    return TRUE;
  else {
    ASSERT(value == -1);
    return FALSE;
  }
}

void CadicalWrapper::retrieveAndUpdateWatcherOfFixedBVariable() {
  unsigned numFixed = 0;
  for (unsigned i = 1; i <= _numberOfBooleanVariables; ++i) {
    LiteralStatus status = getLiteralStatus(i);
    if (status == TRUE) {
      ++numFixed;
      for (const auto &watchee : _bVariableToWatchees[i])
        watchee->notifyBVariable(i, true);
    } else if (status == FALSE) {
      ++numFixed;
      for (const auto &watchee : _bVariableToWatchees[i])
        watchee->notifyBVariable(i, false);
    }
  }
  if (_statistics) {
    _statistics->setUnsignedAttribute(Statistics::NUM_FIXED_BOOLEAN_VARIABLES,
                                      numFixed);
    _statistics->setUnsignedAttribute(Statistics::NUM_BOOLEAN_VARIABLES,
                                      _numberOfBooleanVariables);
  }
}

// -------------------------- Debug methods -------------------------------//
void CadicalWrapper::dumpModel(const String &name) {
  _satSolver->write_dimacs(name.ascii());
}
