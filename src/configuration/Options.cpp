/*********************                                                        */
/*! \file Options.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Soy project.
 ** Copyright (c) 2023 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#include "Options.h"

#include "Debug.h"
#include "GlobalConfiguration.h"

Options *Options::get() {
  static Options singleton;
  return &singleton;
}

Options::Options()
    : _optionParser(&_boolOptions, &_intOptions, &_floatOptions,
                    &_stringOptions) {
  initializeDefaultValues();
  _optionParser.initialize();
}

Options::Options(const Options &) {
  // This constructor should never be called
  ASSERT(false);
}

void Options::initializeDefaultValues() {
  /*
    Bool options
  */
  _boolOptions[SOLVE_WITH_MILP] = false;
  _boolOptions[FEASIBILITY_ONLY] = false;
  _boolOptions[NO_CDCL] = false;
  _boolOptions[NO_BOUND_TIGHTENING] = false;
  _boolOptions[NO_PHASE_CONFLICT] = false;
  _boolOptions[VSIDS] = false;

  /*
    Int options
  */
  _intOptions[VERBOSITY] = 2;
  _intOptions[TIMEOUT] = 0;
  _intOptions[SEED] = 1995;
  _intOptions[NUM_WORKERS] = 1;
  _intOptions[GUROBI_THREADS] = 1;
  _intOptions[DEEP_SOI_REJECTION_THRESHOLD] = 1;
  _intOptions[RESTART_THESHOLD] = 2;
  _intOptions[MAX_LEMMA_LENGTH] = 4;
  _intOptions[MAX_PROPOSALS_PER_STATE] = 50;
  _intOptions[TABU] = 5;

  /*
    Float options
  */
  _floatOptions[PROBABILITY_DENSITY_PARAMETER] = 50;
  _floatOptions[MILP_SOLVING_THRESHOLD] = 2.0/3;

  /*
    String options
  */
  _stringOptions[INPUT_QUERY_FILE_PATH] = "";
  _stringOptions[SUMMARY_FILE] = "";
  _stringOptions[QUERY_DUMP_FILE] = "";
  _stringOptions[SOLUTION_FILE] = "";
  _stringOptions[SOI_SEARCH_STRATEGY] = "greedy-sat";
  _stringOptions[SOI_INITIALIZATION_STRATEGY] = "current-assignment-sat";
  _stringOptions[EXPLANATION_STRATEGY] = "none";
  _stringOptions[BRANCHING_HEURISTICS] = "none";
}

void Options::parseOptions(int argc, char **argv) {
  _optionParser.parse(argc, argv);
}

void Options::printHelpMessage() const { _optionParser.printHelpMessage(); }

bool Options::getBool(unsigned option) const {
  return _boolOptions.get(option);
}

int Options::getInt(unsigned option) const { return _intOptions.get(option); }

float Options::getFloat(unsigned option) const {
  return _floatOptions.get(option);
}

String Options::getString(unsigned option) const {
  return String(_stringOptions.get(option));
}

void Options::setBool(unsigned option, bool value) {
  _boolOptions[option] = value;
}

void Options::setInt(unsigned option, int value) {
  _intOptions[option] = value;
}

void Options::setFloat(unsigned option, float value) {
  _floatOptions[option] = value;
}

void Options::setString(unsigned option, std::string value) {
  _stringOptions[option] = value;
}

SoISearchStrategy Options::getSoISearchStrategy() const {
  String strategyString =
      String(_stringOptions.get(Options::SOI_SEARCH_STRATEGY));
  if (strategyString == "mcmc")
    return SoISearchStrategy::MCMC;
  else if (strategyString == "greedy")
    return SoISearchStrategy::GREEDY;
  else if (strategyString == "greedy-sat")
    return SoISearchStrategy::GREEDY_SAT;
  else if (strategyString == "walksat")
    return SoISearchStrategy::WALKSAT;
  else
    return SoISearchStrategy::MCMC;
}

SoIInitializationStrategy Options::getSoIInitializationStrategy() const {
  String strategyString =
      String(_stringOptions.get(Options::SOI_INITIALIZATION_STRATEGY));
  if (strategyString == "current-assignment")
    return SoIInitializationStrategy::CURRENT_ASSIGNMENT;
  else if (strategyString == "current-assignment-sat")
    return SoIInitializationStrategy::CURRENT_ASSIGNMENT_SAT;
  else if (strategyString == "given")
    return SoIInitializationStrategy::GIVEN;
  else
    return SoIInitializationStrategy::CURRENT_ASSIGNMENT;
}
