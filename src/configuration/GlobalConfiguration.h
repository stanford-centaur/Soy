/*********************                                                        */
/*! \file GlobalConfiguration.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Parth Shah, Derek Huang
 ** This file is part of the Soy project.
 ** Copyright (c) 2023 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#ifndef __GlobalConfiguration_h__
#define __GlobalConfiguration_h__

class GlobalConfiguration {
 public:
  static const double REGULARIZATION_FOR_SOI_COST;

  static const bool USE_LUBY_RESTART;

  static const bool CACHE_PHASE_PATTERN;

  static const unsigned THEORY_LEMMA_LENGTH_THRESHOLD;

  static const unsigned LP_BASED_TIGHTENING_ENCODING_DEPTH;

  static const bool PERFORM_PREPROCESSING;

  static const bool EXTRACT_THEORY_EXPLANATION;

  // The quantity by which the score is bumped up for PLContraints not
  // participating in the SoI. This promotes those constraints in the branching
  // order.
  static const double SCORE_BUMP_FOR_PL_CONSTRAINTS_NOT_IN_SOI;

  // The default epsilon used for comparing doubles
  static const double DEFAULT_EPSILON_FOR_COMPARISONS;

  // The default epsilon used for comparing integer varaibles
  static const double DEFAULT_EPSILON_FOR_INTEGRAL_COMPARISONS;

  // The precision level when convering doubles to strings
  static const unsigned DEFAULT_DOUBLE_TO_STRING_PRECISION;

  // How often should the main loop print statistics?
  static const unsigned STATISTICS_PRINTING_FREQUENCY;

  // Toggle query-preprocessing on/off.
  static const bool PREPROCESS_INPUT_QUERY;

  // Assuming the preprocessor is on, toggle whether or not it will attempt to
  // perform variable elimination.
  static const bool PREPROCESSOR_ELIMINATE_VARIABLES;

  // Toggle whether or not PL constraints will be called upon
  // to add auxiliary variables and equations after preprocessing.
  static const bool PL_CONSTRAINTS_ADD_AUX_EQUATIONS_AFTER_PREPROCESSING;

  // If the difference between a variable's lower and upper bounds is smaller
  // than this threshold, the preprocessor will treat it as fixed.
  static const double PREPROCESSOR_ALMOST_FIXED_THRESHOLD;

  static const double EXPONENTIAL_MOVING_AVERAGE_ALPHA_BRANCH;

  static const double EXPONENTIAL_MOVING_AVERAGE_ALPHA_DIRECTION;

  /*
    Logging options
  */
  static const bool DNC_MANAGER_LOGGING;
  static const bool ENGINE_LOGGING;
  static const bool PREPROCESSOR_LOGGING;
  static const bool INPUT_QUERY_LOGGING;
  static const bool MPS_PARSER_LOGGING;
  static const bool SCORE_TRACKER_LOGGING;
  static const bool SMT_CORE_LOGGING;
  static const bool SOI_LOGGING;
  static const bool CONFLICT_LOGGING;
  static const bool GUROBI_LOGGING;

  static const bool SOI_TRACKING;
  static const bool CONFLICT_TRACKING;
};

#endif  // __GlobalConfiguration_h__
