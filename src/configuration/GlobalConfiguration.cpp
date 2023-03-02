/*********************                                                        */
/*! \file GlobalConfiguration.cpp
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

#include "GlobalConfiguration.h"

#include <cstdio>

#include "MString.h"

const double GlobalConfiguration::REGULARIZATION_FOR_SOI_COST = 0.01;

const bool GlobalConfiguration::USE_LUBY_RESTART = false;

const bool GlobalConfiguration::CACHE_PHASE_PATTERN = true;

const unsigned GlobalConfiguration::THEORY_LEMMA_LENGTH_THRESHOLD = 1;

const unsigned GlobalConfiguration::LP_BASED_TIGHTENING_ENCODING_DEPTH = 4;

const bool GlobalConfiguration::PERFORM_PREPROCESSING = true;

const bool GlobalConfiguration::EXTRACT_THEORY_EXPLANATION = true;

const double GlobalConfiguration::SCORE_BUMP_FOR_PL_CONSTRAINTS_NOT_IN_SOI = 5;

const double GlobalConfiguration::DEFAULT_EPSILON_FOR_COMPARISONS = 1e-9;
const double GlobalConfiguration::DEFAULT_EPSILON_FOR_INTEGRAL_COMPARISONS =
    1e-6;
const unsigned GlobalConfiguration::DEFAULT_DOUBLE_TO_STRING_PRECISION = 10;
const unsigned GlobalConfiguration::STATISTICS_PRINTING_FREQUENCY = 20;

const bool GlobalConfiguration::PREPROCESS_INPUT_QUERY = true;
const bool GlobalConfiguration::PREPROCESSOR_ELIMINATE_VARIABLES = false;
const bool
    GlobalConfiguration::PL_CONSTRAINTS_ADD_AUX_EQUATIONS_AFTER_PREPROCESSING =
        true;
const double GlobalConfiguration::PREPROCESSOR_ALMOST_FIXED_THRESHOLD = 0.00001;

const double GlobalConfiguration::EXPONENTIAL_MOVING_AVERAGE_ALPHA_BRANCH = 0.9;

const double GlobalConfiguration::EXPONENTIAL_MOVING_AVERAGE_ALPHA_DIRECTION =
    0.2;

// Logging - note that it is enabled only in Debug mode
const bool GlobalConfiguration::DNC_MANAGER_LOGGING = false;
const bool GlobalConfiguration::ENGINE_LOGGING = true;
const bool GlobalConfiguration::PREPROCESSOR_LOGGING = false;
const bool GlobalConfiguration::INPUT_QUERY_LOGGING = false;
const bool GlobalConfiguration::MPS_PARSER_LOGGING = true;
const bool GlobalConfiguration::SCORE_TRACKER_LOGGING = false;
const bool GlobalConfiguration::SMT_CORE_LOGGING = true;
const bool GlobalConfiguration::SOI_LOGGING = true;
const bool GlobalConfiguration::CONFLICT_LOGGING = false;
const bool GlobalConfiguration::GUROBI_LOGGING = false;

const bool GlobalConfiguration::SOI_TRACKING = false;
const bool GlobalConfiguration::CONFLICT_TRACKING = false;
