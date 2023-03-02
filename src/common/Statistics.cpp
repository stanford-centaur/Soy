/*********************                                                        */
/*! \file Statistics.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Duligur Ibeling, Andrew Wu
 ** This file is part of the Soy project.
 ** Copyright (c) 2023 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#include "Statistics.h"

#include "FloatUtils.h"
#include "TimeUtils.h"

Statistics::Statistics() : _timedOut(false) {
  _unsignedAttributes[NUM_PL_CONSTRAINTS] = 0;
  _unsignedAttributes[NUM_ACTIVE_PL_CONSTRAINTS] = 0;
  _unsignedAttributes[NUM_PL_VALID_CONSTRAINTS] = 0;
  _unsignedAttributes[NUM_PL_SMT_ORIGINATED_SPLITS] = 0;
  _unsignedAttributes[CURRENT_DECISION_LEVEL] = 0;
  _unsignedAttributes[MAX_DECISION_LEVEL] = 0;
  _unsignedAttributes[NUM_VARIABLES] = 0;
  _unsignedAttributes[NUM_EQUATIONS] = 0;
  _unsignedAttributes[NUM_SPLITS] = 0;
  _unsignedAttributes[NUM_POPS] = 0;
  _unsignedAttributes[NUM_RESTART] = 0;
  _unsignedAttributes[NUM_REFUTATIONS_BY_SAT_SOLVER] = 0;
  _unsignedAttributes[NUM_LP_FEASIBILITY_CHECK] = 0;
  _unsignedAttributes[NUM_REFUTATIONS_BY_THEORY_SOLVER] = 0;
  _unsignedAttributes[NUM_REFUTATIONS_BY_BOUND_TIGHTENING] = 0;
  _unsignedAttributes[NUM_VISITED_TREE_STATES] = 1;
  _unsignedAttributes[PP_NUM_TIGHTENING_ITERATIONS] = 0;
  _unsignedAttributes[PP_NUM_EQUATIONS_REMOVED] = 0;
  _unsignedAttributes[PP_NUM_EQUATIONS_REMOVED] = 0;
  _unsignedAttributes[NUM_BOOLEAN_VARIABLES] = 0;
  _unsignedAttributes[NUM_FIXED_BOOLEAN_VARIABLES] = 0;
  _unsignedAttributes[NUM_PROPOSALS_REJECTED_BY_SAT_SOLVER] = 0;
  _unsignedAttributes[NUM_PHASE_PATTERN_INITIALIZATIONS] = 0;
  _unsignedAttributes[NUM_INITIALIZATIONS_REJECTED_BY_SAT_SOLVER] = 0;
  _unsignedAttributes[NUM_SAT_CONSTRAINTS] = 0;

  _longAttributes[PREPROCESSING_TIME_MICRO] = 0;
  _longAttributes[NUM_MAIN_LOOP_ITERATIONS] = 0;
  _longAttributes[TIME_MAIN_LOOP_MICRO] = 0;
  _longAttributes[TIME_LP_FEASIBILITY_CHECK_MICRO] = 0;
  _longAttributes[TIME_SAT_SOLVING_MICRO] = 0;
  _longAttributes[TIME_THEORY_EXPLANATION_MICRO] = 0;
  _longAttributes[TOTAL_TIME_SMT_CORE_MICRO] = 0;
  _longAttributes[TIME_BOUND_TIGHTENING_MICRO] = 0;
  _longAttributes[TOTAL_TIME_PERFORMING_VALID_CASE_SPLITS_MICRO] = 0;
  _longAttributes[TOTAL_TIME_LOCAL_SEARCH_MICRO] = 0;
  _longAttributes[TOTAL_TIME_HANDLING_STATISTICS_MICRO] = 0;
  _longAttributes[TOTAL_TIME_UPDATING_SOI_PHASE_PATTERN_MICRO] = 0;
  _longAttributes[NUM_PROPOSED_PHASE_PATTERN_UPDATE] = 0;
  _longAttributes[NUM_ACCEPTED_PHASE_PATTERN_UPDATE] = 0;
  _longAttributes[NUM_PROPOSALS_SINCE_LAST_REINITIALIZATION] = 0;
  _longAttributes[TOTAL_TIME_GETTING_SOI_PHASE_PATTERN_MICRO] = 0;
  _longAttributes[TIME_ADDING_CONSTRAINTS_TO_MILP_SOLVER_MICRO] = 0;
  _longAttributes[NUM_PHASE_PATTERN_CACHE_HITS] = 0;
  _longAttributes[TOTAL_TIME_MINIMIZING_SOI_COST_WITH_GUROBI_MICRO] = 0;
  _longAttributes[TOTAL_TIME_OBTAIN_CURRENT_ASSIGNMENT_SOI_MICRO] = 0;
  _longAttributes[TOTAL_TIME_UPDATING_PSEUDO_IMPACT_MICRO] = 0;
  _longAttributes[TOTAL_TIME_SAT_SOLVING_SOI_MICRO] = 0;

  _doubleAttributes[COST_OF_CURRENT_PHASE_PATTERN] = FloatUtils::infinity();
  _doubleAttributes[MIN_COST_OF_PHASE_PATTERN] = FloatUtils::infinity();
}

void Statistics::stampStartingTime() { _startTime = TimeUtils::sampleMicro(); }

void Statistics::stampMainLoopStartTime() {
  _mainLoopStartTime = TimeUtils::sampleMicro();
}

void Statistics::print() {
  printf("\n%s Statistics update:\n", TimeUtils::now().ascii());

  struct timespec now = TimeUtils::sampleMicro();

  unsigned long long totalElapsed = TimeUtils::timePassed(_startTime, now);

  unsigned seconds = totalElapsed / 1000000;
  unsigned minutes = seconds / 60;
  unsigned hours = minutes / 60;

  printf("\t--- Time Statistics ---\n");
  printf("\tTotal time elapsed: %llu milli (%02u:%02u:%02u)\n",
         totalElapsed / 1000, hours, minutes - (hours * 60),
         seconds - (minutes * 60));

  unsigned long long timeMainLoopMicro =
      TimeUtils::timePassed(_mainLoopStartTime, now);
  seconds = timeMainLoopMicro / 1000000;
  minutes = seconds / 60;
  hours = minutes / 60;
  printf("\t\tMain loop: %llu milli (%02u:%02u:%02u)\n",
         timeMainLoopMicro / 1000, hours, minutes - (hours * 60),
         seconds - (minutes * 60));

  unsigned long long preprocessingTimeMicro =
      getLongAttribute(Statistics::PREPROCESSING_TIME_MICRO);
  seconds = preprocessingTimeMicro / 1000000;
  minutes = seconds / 60;
  hours = minutes / 60;
  printf("\t\tPreprocessing time: %llu milli (%02u:%02u:%02u)\n",
         preprocessingTimeMicro / 1000, hours, minutes - (hours * 60),
         seconds - (minutes * 60));

  unsigned long long totalUnknown =
      totalElapsed - timeMainLoopMicro - preprocessingTimeMicro;

  seconds = totalUnknown / 1000000;
  minutes = seconds / 60;
  hours = minutes / 60;
  printf("\t\tUnknown: %llu milli (%02u:%02u:%02u)\n", totalUnknown / 1000,
         hours, minutes - (hours * 60), seconds - (minutes * 60));

  printf("\tBreakdown for main loop:\n");
  unsigned long long timeFeasibilityCheckMicro =
      getLongAttribute(Statistics::TIME_LP_FEASIBILITY_CHECK_MICRO);
  unsigned numFeasibilityChecks =
      getUnsignedAttribute(Statistics::NUM_LP_FEASIBILITY_CHECK);
  printf(
      "\t\t[%.2lf%%] LP feasibility check: %llu milli (per check: %.2f "
      "milli)\n",
      printPercents(timeFeasibilityCheckMicro, timeMainLoopMicro),
      timeFeasibilityCheckMicro / 1000,
      printAverage(timeFeasibilityCheckMicro, numFeasibilityChecks) / 1000);
  unsigned long long timeTheoryExplanationMicro =
      getLongAttribute(Statistics::TIME_THEORY_EXPLANATION_MICRO);
  printf("\t\t[%.2lf%%] Theory explanation: %llu milli\n",
         printPercents(timeTheoryExplanationMicro, timeMainLoopMicro),
         timeTheoryExplanationMicro / 1000);
  unsigned long long timeSatSolvingMicro =
      getLongAttribute(Statistics::TIME_SAT_SOLVING_MICRO);
  printf("\t\t[%.2lf%%] SAT solving: %llu milli\n",
         printPercents(timeSatSolvingMicro, timeMainLoopMicro),
         timeSatSolvingMicro / 1000);
  unsigned long long timeSmtCoreMicro =
      getLongAttribute(Statistics::TOTAL_TIME_SMT_CORE_MICRO);
  printf("\t\t[%.2lf%%] SmtCore: %llu milli\n",
         printPercents(timeSmtCoreMicro, timeMainLoopMicro),
         timeSmtCoreMicro / 1000);
  unsigned long long timeTighteningMicro =
    getLongAttribute(Statistics::TIME_BOUND_TIGHTENING_MICRO);
  printf("\t\t[%.2lf%%] Tightening: %llu milli\n",
         printPercents(timeTighteningMicro, timeMainLoopMicro),
         timeTighteningMicro / 1000);
  unsigned long long totalTimePerformingValidCaseSplitsMicro = getLongAttribute(
      Statistics::TOTAL_TIME_PERFORMING_VALID_CASE_SPLITS_MICRO);
  printf(
      "\t\t[%.2lf%%] Valid case splits: %llu milli.\n",
      printPercents(totalTimePerformingValidCaseSplitsMicro, timeMainLoopMicro),
      totalTimePerformingValidCaseSplitsMicro / 1000);
  unsigned long long totalTimePerformingLocalSearch =
      getLongAttribute(Statistics::TOTAL_TIME_LOCAL_SEARCH_MICRO);
  printf("\t\t[%.2lf%%] SoI-based local search: %llu milli\n",
         printPercents(totalTimePerformingLocalSearch, timeMainLoopMicro),
         totalTimePerformingLocalSearch / 1000);
  unsigned long long totalTimeAddingConstraintsToMILPSolver = getLongAttribute(
      Statistics::TIME_ADDING_CONSTRAINTS_TO_MILP_SOLVER_MICRO);
  printf(
      "\t\t[%.2lf%%] Encoding constraints to Gurobi: %llu milli\n",
      printPercents(totalTimeAddingConstraintsToMILPSolver, timeMainLoopMicro),
      totalTimeAddingConstraintsToMILPSolver / 1000);
  unsigned long long totalTimeHandlingStatisticsMicro =
      getLongAttribute(Statistics::TOTAL_TIME_HANDLING_STATISTICS_MICRO);
  printf("\t\t[%.2lf%%] Statistics handling: %llu milli\n",
         printPercents(totalTimeHandlingStatisticsMicro, timeMainLoopMicro),
         totalTimeHandlingStatisticsMicro / 1000);

  unsigned long long total =
    timeFeasibilityCheckMicro + timeTheoryExplanationMicro +
    timeSatSolvingMicro + timeSmtCoreMicro + timeTighteningMicro +
    totalTimePerformingValidCaseSplitsMicro + totalTimePerformingLocalSearch +
    totalTimeAddingConstraintsToMILPSolver + totalTimeHandlingStatisticsMicro;

  printf("\t\t[%.2lf%%] Unaccounted for: %llu milli\n",
         printPercents(timeMainLoopMicro - total, timeMainLoopMicro),
         timeMainLoopMicro > total ? (timeMainLoopMicro - total) / 1000 : 0);

  printf("\t--- Preprocessor Statistics ---\n");
  printf("\tNumber of preprocessor bound-tightening loop iterations: %u\n",
         getUnsignedAttribute(Statistics::PP_NUM_TIGHTENING_ITERATIONS));
  printf("\tNumber of equations removed due to variable elimination: %u\n",
         getUnsignedAttribute(Statistics::PP_NUM_EQUATIONS_REMOVED));

  printf("\t--- Engine Statistics ---\n");
  printf("\tNumber of variables: %u, number of equations: %u\n",
         getUnsignedAttribute(Statistics::NUM_VARIABLES),
         getUnsignedAttribute(Statistics::NUM_EQUATIONS));

  printf("\tNumber of main loop iterations: %llu, number of restarts: %u\n",
         getLongAttribute(Statistics::NUM_MAIN_LOOP_ITERATIONS),
         getUnsignedAttribute(Statistics::NUM_RESTART));

  printf(
      "\tNumber of active piecewise-linear constraints: %u / %u\n"
      "\t\tConstraints disabled by valid splits: %u. "
      "By SMT-originated splits: %u\n",
      getUnsignedAttribute(Statistics::NUM_ACTIVE_PL_CONSTRAINTS),
      getUnsignedAttribute(Statistics::NUM_PL_CONSTRAINTS),
      getUnsignedAttribute(Statistics::NUM_PL_VALID_CONSTRAINTS),
      getUnsignedAttribute(Statistics::NUM_PL_SMT_ORIGINATED_SPLITS));

  printf("\t--- SAT solver Statistics ---\n");
  printf(
      "\tNumber of boolean variables: %u,"
      " number of fixed variables: %u,"
      " number of constraints: %u \n",
      getUnsignedAttribute(Statistics::NUM_BOOLEAN_VARIABLES),
      getUnsignedAttribute(Statistics::NUM_FIXED_BOOLEAN_VARIABLES),
      getUnsignedAttribute(Statistics::NUM_SAT_CONSTRAINTS));

  printf("\t--- SMT Core Statistics ---\n");
  unsigned numVisitedTreeStates = getUnsignedAttribute(Statistics::NUM_VISITED_TREE_STATES);
  printf(
      "\tTotal depth is %u. Total visited states: %u. Number of splits: %u. "
      "Number of pops: %u\n",
      getUnsignedAttribute(Statistics::CURRENT_DECISION_LEVEL),
      numVisitedTreeStates,
      getUnsignedAttribute(Statistics::NUM_SPLITS),
      getUnsignedAttribute(Statistics::NUM_POPS));
  printf("\tMax stack depth: %u\n",
         getUnsignedAttribute(Statistics::MAX_DECISION_LEVEL));

  printf(
      "\tNumber of states refuted by SAT solver: %u\n"
      "\tNumber of states refuted by bound tightening: %u\n"
      "\tNumber of states refuted by theory solver: %u\n",
      getUnsignedAttribute(Statistics::NUM_REFUTATIONS_BY_SAT_SOLVER),
      getUnsignedAttribute(Statistics::NUM_REFUTATIONS_BY_BOUND_TIGHTENING),
      getUnsignedAttribute(Statistics::NUM_REFUTATIONS_BY_THEORY_SOLVER));

  printf("\t--- SoI-based local search ---\n");
  unsigned numPhasePatternInitializations =
      getUnsignedAttribute(Statistics::NUM_PHASE_PATTERN_INITIALIZATIONS);
  unsigned numInitializationssRejectedBySatSolver = getUnsignedAttribute(
      Statistics::NUM_INITIALIZATIONS_REJECTED_BY_SAT_SOLVER);
  unsigned numProposalsRejectedBySatSolver =
      getUnsignedAttribute(Statistics::NUM_PROPOSALS_REJECTED_BY_SAT_SOLVER);
  unsigned long long numProposedPhasePatternUpdate =
      getLongAttribute(Statistics::NUM_PROPOSED_PHASE_PATTERN_UPDATE);
  unsigned long long numProposalsSinceLastReinit =
      getLongAttribute(Statistics::NUM_PROPOSALS_SINCE_LAST_REINITIALIZATION);
  unsigned long long numAcceptedPhasePatternUpdate =
      getLongAttribute(Statistics::NUM_ACCEPTED_PHASE_PATTERN_UPDATE);

  printf(
      "\tNumber of proposed phase pattern update: %llu (this state: %llu). "
      "Number of accepted update: %llu [%.2lf%%]\n"
      "\tNumber of initializations updated by SAT solvers: %u [%.2lf%%]\n"
      "\tNumber of proposals updated by SAT solvers: %u [%.2lf%%]\n"
      "\tNumber of proposal per state: %.2f\n",
      numProposedPhasePatternUpdate, numProposalsSinceLastReinit,
      numAcceptedPhasePatternUpdate,
      printPercents(numAcceptedPhasePatternUpdate,
                    numProposedPhasePatternUpdate),
      numInitializationssRejectedBySatSolver,
      printPercents(numInitializationssRejectedBySatSolver,
                    numPhasePatternInitializations),
      numProposalsRejectedBySatSolver,
      printPercents(numProposalsRejectedBySatSolver,
                    numProposedPhasePatternUpdate),
      printAverage(numProposedPhasePatternUpdate,
                   numPhasePatternInitializations));
  unsigned long long numPhasePatternCacheHits =
      getLongAttribute(Statistics::NUM_PHASE_PATTERN_CACHE_HITS);
  printf(
      "\tNumber of cache hits: %llu [%.2lf%%]\n", numPhasePatternCacheHits,
      printPercents(numPhasePatternCacheHits, numProposedPhasePatternUpdate));

  printf("\tBreakdown for DeepSoI loop:\n");
  unsigned long long totalTimeMinimizingSoiCostWithGurobiMicro =
    getLongAttribute(Statistics::TOTAL_TIME_MINIMIZING_SOI_COST_WITH_GUROBI_MICRO);
  printf("\t\t[%.2lf%%] LP solving: %llu milli(per check: %.2f milli)\n",
         printPercents(totalTimeMinimizingSoiCostWithGurobiMicro,
                       totalTimePerformingLocalSearch),
         totalTimeMinimizingSoiCostWithGurobiMicro / 1000,
         printAverage(totalTimeMinimizingSoiCostWithGurobiMicro,
                      numProposedPhasePatternUpdate + numPhasePatternInitializations) / 1000);
  unsigned long long totalTimeUpdatingSoIPhasePattern =
      getLongAttribute(Statistics::TOTAL_TIME_UPDATING_SOI_PHASE_PATTERN_MICRO);
  printf("\t\t[%.2lf%%] Updating SoI PhasePattern: %llu milli\n",
         printPercents(totalTimeUpdatingSoIPhasePattern,
                       totalTimePerformingLocalSearch),
         totalTimeUpdatingSoIPhasePattern / 1000);
  unsigned long long totalTimeObtainCurrentAssignmentSoIMicro =
    getLongAttribute(Statistics::TOTAL_TIME_OBTAIN_CURRENT_ASSIGNMENT_SOI_MICRO);
  printf("\t\t[%.2lf%%] Obtaining assignment from LP solver: %llu milli\n",
         printPercents(totalTimeObtainCurrentAssignmentSoIMicro,
                       totalTimePerformingLocalSearch),
         totalTimeObtainCurrentAssignmentSoIMicro / 1000);
  unsigned long long totalTimeUpdatingPseudoImpactMicro =
    getLongAttribute(Statistics::TOTAL_TIME_UPDATING_PSEUDO_IMPACT_MICRO);
  printf("\t\t[%.2lf%%] Updating Pseudo Impact: %llu milli\n",
         printPercents(totalTimeUpdatingPseudoImpactMicro,
                       totalTimePerformingLocalSearch),
         totalTimeUpdatingPseudoImpactMicro / 1000);
  unsigned long long totalTimeSatSolvingSoIMicro =
    getLongAttribute(Statistics::TOTAL_TIME_SAT_SOLVING_SOI_MICRO);
  printf("\t\t[%.2lf%%] Checking prop level feasibility: %llu milli\n",
         printPercents(totalTimeSatSolvingSoIMicro,
                       totalTimePerformingLocalSearch),
         totalTimeSatSolvingSoIMicro / 1000);

  total = (totalTimeMinimizingSoiCostWithGurobiMicro +
           totalTimeUpdatingSoIPhasePattern +
           totalTimeObtainCurrentAssignmentSoIMicro +
           totalTimeUpdatingPseudoImpactMicro +
           totalTimeSatSolvingSoIMicro);

  printf("\t\t[%.2lf%%] Unaccounted for: %llu milli\n",
         printPercents(totalTimePerformingLocalSearch - total,
                       totalTimePerformingLocalSearch),
         (totalTimePerformingLocalSearch - total) / 1000);
}

unsigned long long Statistics::getTotalTimeInMicro() const {
  return TimeUtils::timePassed(_startTime, TimeUtils::sampleMicro());
}

void Statistics::timeout() { _timedOut = true; }

bool Statistics::hasTimedOut() const { return _timedOut; }

void Statistics::printStartingIteration(unsigned long long iteration,
                                        String message) {
  if (_longAttributes[NUM_MAIN_LOOP_ITERATIONS] >= iteration)
    printf("DBG_PRINT: %s\n", message.ascii());
}

double Statistics::printPercents(unsigned long long part,
                                 unsigned long long total) const {
  if (total == 0) return 0;

  return 100.0 * part / total;
}

double Statistics::printAverage(unsigned long long part,
                                unsigned long long total) const {
  if (total == 0) return 0;

  return (double)part / total;
}
