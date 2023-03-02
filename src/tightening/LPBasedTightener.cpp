#include "LPBasedTightener.h"

#include "BoundManager.h"
#include "FloatUtils.h"
#include "GlobalConfiguration.h"
#include "GurobiWrapper.h"
#include "InfeasibleQueryException.h"
#include "InputQuery.h"
#include "MILPEncoder.h"
#include "Tightening.h"

LPBasedTightener::LPBasedTightener(const BoundManager &boundManager,
                                   const InputQuery &inputQuery)
    : _boundManager(boundManager), _inputQuery(inputQuery) {}

void LPBasedTightener::computeKPattern(List<Vector<PhaseStatus>> &lemmas,
                                       unsigned K) {
  unsigned numLemmas = 0;

  Vector<PLConstraint *> pls;

  for (unsigned i = 2; i < K + 2; ++i){
    for (const auto &constraint : _inputQuery.getPLConstraints()) {
      List<unsigned> steps = _inputQuery.getStepsOfPLConstraint(constraint);
      if (*steps.begin() == i)
        pls.append(constraint);
    }
  }
  ASSERT(pls.size() == K);

  Vector<Vector<PhaseStatus>> feasiblePatterns = {{}};

  GurobiWrapper gurobi;
  unsigned length = 1;
  for (const auto &plConstraint : pls){
    // Encode the connections
    gurobi.resetModel();
    gurobi.setVerbosity(0);
    MILPEncoder milpEncoder(_boundManager);
    List<unsigned> steps;
    for (unsigned i = 0; i < length; ++i){
      steps.append(i + 2);
    }
    milpEncoder.encodeInputQueryForSteps(gurobi, _inputQuery, steps);

    Vector<Vector<PhaseStatus>> newFeasiblePatterns;
    for (const auto &pattern : feasiblePatterns) {
      for (const auto &phase : plConstraint->getAllCases()) {
        Vector<PhaseStatus> newPattern = pattern;
        newPattern.append(phase);
        // If the pattern's suffix is already infeasible, skip
        Vector<PhaseStatus> subPattern = newPattern;
        subPattern.popFirst();
        bool feasible = true;
        while (subPattern.size() > 0) {
          if (lemmas.exists(subPattern)) {
            feasible = false;
            break;
          }
          subPattern.popFirst();
        }
        if (!feasible) continue;

        List<GurobiWrapper::Term> terms;
        for (unsigned i = 0; i < length; ++i) {
          terms.append
            (GurobiWrapper::Term(1,Stringf("x%u",
                                           ((OneHotConstraint *)pls[i])->
                                           getElementOfPhase(newPattern[i]))));
        }
        ASSERT(newPattern.size() == length);
        gurobi.addEqConstraint(terms, length, "tmp");
        gurobi.solve();
        if (gurobi.infeasible()) {
          lemmas.append(newPattern);
          ++numLemmas;
        } else {
          newFeasiblePatterns.append(newPattern);
        }
        gurobi.removeConstraintByName("tmp");
      }
    }
    feasiblePatterns = std::move(newFeasiblePatterns);
    ++length;
  }

  printf("Number of lemmas: %u\n", numLemmas);
  return;
}

unsigned LPBasedTightener::performLPBasedTightening() {
  unsigned numTightened = 0;

  GurobiWrapper gurobi;

  for (const auto &constraint : _inputQuery.getPLConstraints()) {
    List<unsigned> steps = _inputQuery.getStepsOfPLConstraint(constraint);
    getStepsToEncode(steps);

    MILPEncoder milpEncoder(_boundManager);
    gurobi.resetModel();
    // gurobi.setNodeLimit(1);
    gurobi.setVerbosity(0);
    milpEncoder.encodeInputQueryForSteps(gurobi, _inputQuery, steps, true);

    List<unsigned> variables = constraint->getParticipatingVariables();
    for (const auto &variable : variables) {
      // Tighten lower bound
      if (constraint->participatingVariable(variable)) {
        List<GurobiWrapper::Term> terms;
        terms.append(GurobiWrapper::Term(1, Stringf("x%u", variable)));
        gurobi.setCost(terms, 0);
        gurobi.solve();
        if (gurobi.infeasible()) throw InfeasibleQueryException();
        double bound = gurobi.getObjectiveBound();
        if (FloatUtils::lt(_boundManager.getLowerBound(variable), bound)) {
          ++numTightened;
          constraint->notifyLowerBound(variable, bound);
        }
      }

      // Tighten upper bound
      if (constraint->participatingVariable(variable)) {
        List<GurobiWrapper::Term> terms;
        terms.append(GurobiWrapper::Term(1, Stringf("x%u", variable)));
        gurobi.setObjective(terms, 0);
        gurobi.solve();
        if (gurobi.infeasible()) throw InfeasibleQueryException();
        double bound = gurobi.getObjectiveBound();
        if (FloatUtils::gt(_boundManager.getUpperBound(variable), bound)) {
          ++numTightened;
          constraint->notifyUpperBound(variable, bound);
        }
      }
    }
  }
  return numTightened;
}

void LPBasedTightener::getStepsToEncode(List<unsigned> &steps) {
  Set<unsigned> stepsSet;
  unsigned depth = GlobalConfiguration::LP_BASED_TIGHTENING_ENCODING_DEPTH;
  for (const unsigned &step : steps) {
    unsigned begin = step >= depth ? step - depth : 0;
    unsigned end = step + depth;
    for (unsigned i = begin; i <= end; ++i) stepsSet.insert(i);
  }
  steps.clear();
  for (const auto &step : stepsSet) steps.append(step);
}
