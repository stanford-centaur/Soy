/*********************                                                        */
/*! \file Statistics.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Andrew Wu, Duligur Ibeling
 ** This file is part of the Soy project.
 ** Copyright (c) 2023 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#ifndef __Statistics_h__
#define __Statistics_h__

#include "List.h"
#include "Map.h"
#include "TimeUtils.h"

class Statistics {
 public:
  Statistics();

  enum StatisticsUnsignedAttribute {
    // Number of piecewise linear constraints (active, total, and
    // reason for split)
    NUM_PL_CONSTRAINTS,
    NUM_ACTIVE_PL_CONSTRAINTS,
    NUM_PL_VALID_CONSTRAINTS,
    NUM_PL_SMT_ORIGINATED_SPLITS,

    // Current and max stack depth in the SMT core
    CURRENT_DECISION_LEVEL,
    MAX_DECISION_LEVEL,

    NUM_VARIABLES,
    NUM_EQUATIONS,

    // Total number of splits so far
    NUM_SPLITS,

    // Total number of pops so far
    NUM_POPS,

    // Total number of restarts so far
    NUM_RESTART,

    // Total number of search state refuted by SAT solver
    NUM_REFUTATIONS_BY_SAT_SOLVER,

    // Total number of search state refuted by theory solver
    NUM_LP_FEASIBILITY_CHECK,
    NUM_REFUTATIONS_BY_THEORY_SOLVER,

    NUM_REFUTATIONS_BY_BOUND_TIGHTENING,

    // Total number of states in the search tree visited so far
    NUM_VISITED_TREE_STATES,

    // Preprocessor counters
    PP_NUM_TIGHTENING_ITERATIONS,
    PP_NUM_EQUATIONS_REMOVED,

    // SAT solver
    NUM_BOOLEAN_VARIABLES,
    NUM_FIXED_BOOLEAN_VARIABLES,
    NUM_PROPOSALS_REJECTED_BY_SAT_SOLVER,
    NUM_PHASE_PATTERN_INITIALIZATIONS,
    NUM_INITIALIZATIONS_REJECTED_BY_SAT_SOLVER,
    NUM_SAT_CONSTRAINTS,
  };

  enum StatisticsLongAttribute {
    // Preprocessing time
    PREPROCESSING_TIME_MICRO,

    // Number of iterations of the main loop
    NUM_MAIN_LOOP_ITERATIONS,

    // Total time spent in the main loop, in microseconds
    TIME_MAIN_LOOP_MICRO,

    // Total time spent on performing simplex steps, in microseconds
    TIME_LP_FEASIBILITY_CHECK_MICRO,

    // Total time spent on sat solving
    TIME_THEORY_EXPLANATION_MICRO,

    // Total time spent on sat solving
    TIME_SAT_SOLVING_MICRO,

    // Total time performing bound tightening
    TIME_BOUND_TIGHTENING_MICRO,

    // Total amount of time spent within the SMT core
    TOTAL_TIME_SMT_CORE_MICRO,

    // Total amount of time spent performing valid case splits
    TOTAL_TIME_PERFORMING_VALID_CASE_SPLITS_MICRO,

    // Total time performing SoI-based local search
    TOTAL_TIME_LOCAL_SEARCH_MICRO,

    // Total time adding constraints to (MI)LP solver.
    TIME_ADDING_CONSTRAINTS_TO_MILP_SOLVER_MICRO,

    // Total amount of time handling statistics printing
    TOTAL_TIME_HANDLING_STATISTICS_MICRO,

    // Total time heuristically updating the SoI phase pattern
    TOTAL_TIME_UPDATING_SOI_PHASE_PATTERN_MICRO,

    // Number of proposed/accepted update to the SoI phase pattern.
    NUM_PROPOSED_PHASE_PATTERN_UPDATE,
    NUM_ACCEPTED_PHASE_PATTERN_UPDATE,
    NUM_PROPOSALS_SINCE_LAST_REINITIALIZATION,

    // Total time getting the SoI phase pattern
    TOTAL_TIME_GETTING_SOI_PHASE_PATTERN_MICRO,

    // Total time solving the SoI phase pattern with Gurobi
    TOTAL_TIME_MINIMIZING_SOI_COST_WITH_GUROBI_MICRO,

    NUM_PHASE_PATTERN_CACHE_HITS,

    // Total time obtaining the current variable assignment from the tableau.
    TOTAL_TIME_OBTAIN_CURRENT_ASSIGNMENT_SOI_MICRO,

    TOTAL_TIME_UPDATING_PSEUDO_IMPACT_MICRO,

    TOTAL_TIME_SAT_SOLVING_SOI_MICRO,
  };

  enum StatisticsDoubleAttribute {
    // How close we are to the minimum of the SoI (0).
    COST_OF_CURRENT_PHASE_PATTERN,
    MIN_COST_OF_PHASE_PATTERN,
  };

  /*
    Print the current statistics.
  */
  void print();

  /*
    Set starting time of the main loop.
  */
  void stampStartingTime();
  void stampMainLoopStartTime();

  /*
    Setters for unsigned, unsigned long long, and double attributes
  */
  inline void setUnsignedAttribute(StatisticsUnsignedAttribute attr,
                                   unsigned value) {
    _unsignedAttributes[attr] = value;
  }

  inline void incUnsignedAttribute(StatisticsUnsignedAttribute attr) {
    ++_unsignedAttributes[attr];
  }

  inline void incUnsignedAttribute(StatisticsUnsignedAttribute attr,
                                   unsigned value) {
    _unsignedAttributes[attr] += value;
  }

  inline void setLongAttribute(StatisticsLongAttribute attr,
                               unsigned long long value) {
    _longAttributes[attr] = value;
  }

  inline void incLongAttribute(StatisticsLongAttribute attr) {
    ++_longAttributes[attr];
  }

  inline void incLongAttribute(StatisticsLongAttribute attr,
                               unsigned long long value) {
    _longAttributes[attr] += value;
  }

  inline void setDoubleAttribute(StatisticsDoubleAttribute attr, double value) {
    _doubleAttributes[attr] = value;
  }

  inline void incDoubleAttribute(StatisticsDoubleAttribute attr, double value) {
    _doubleAttributes[attr] += value;
  }

  /*
    Getters for unsigned, unsigned long long, and double attributes
  */
  inline unsigned getUnsignedAttribute(StatisticsUnsignedAttribute attr) const {
    return _unsignedAttributes[attr];
  }

  inline unsigned long long getLongAttribute(
      StatisticsLongAttribute attr) const {
    return _longAttributes[attr];
  }

  inline double getDoubleAttribute(StatisticsDoubleAttribute attr) const {
    return _doubleAttributes[attr];
  }

  unsigned long long getTotalTimeInMicro() const;

  /*
    Report a timeout, or check whether a timeout has occurred
  */
  void timeout();
  bool hasTimedOut() const;

  /*
    For debugging purposes
  */
  void printStartingIteration(unsigned long long iteration, String message);

 private:
  // Initial timestamp
  struct timespec _startTime;
  struct timespec _mainLoopStartTime;

  Map<StatisticsUnsignedAttribute, unsigned> _unsignedAttributes;

  Map<StatisticsLongAttribute, unsigned long long> _longAttributes;

  Map<StatisticsDoubleAttribute, double> _doubleAttributes;

  // Whether the engine quitted with a timeout
  bool _timedOut;

  // Printing helpers
  double printPercents(unsigned long long part, unsigned long long total) const;
  double printAverage(unsigned long long part, unsigned long long total) const;
};

#endif  // __Statistics_h__
