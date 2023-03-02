/*********************                                                        */
/*! \file Options.h
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

#ifndef __Options_h__
#define __Options_h__

#include "MString.h"
#include "Map.h"
#include "OptionParser.h"
#include "SoIInitializationStrategy.h"
#include "SoISearchStrategy.h"
#include "boost/program_options.hpp"

/*
  A singleton class that contains all the options and their values.
*/
class Options {
 public:
  enum BoolOptions {
    // Help flag
    HELP,

    // Version flag
    VERSION,

    // Solve the input query with a MILP solver
    SOLVE_WITH_MILP,

    // Feasibility only
    FEASIBILITY_ONLY,

    // CDCL
    NO_CDCL,

    // LP-based tightening to try to fix the phase of each variable
    NO_BOUND_TIGHTENING,

    NO_PHASE_CONFLICT,

    VSIDS,
  };

  enum IntOptions {
    // Engine verbosity
    VERBOSITY,

    // Global timeout
    TIMEOUT,

    // The random seed used throughout the execution.
    SEED,

    NUM_WORKERS,

    GUROBI_THREADS,

    DEEP_SOI_REJECTION_THRESHOLD,

    RESTART_THESHOLD,

    MAX_LEMMA_LENGTH,

    MAX_PROPOSALS_PER_STATE,

    TABU,
  };

  enum FloatOptions {
    // The beta parameter used in converting the soi f to a probability
    PROBABILITY_DENSITY_PARAMETER,

    MILP_SOLVING_THRESHOLD,
  };

  enum StringOptions {
    INPUT_QUERY_FILE_PATH,
    SUMMARY_FILE,
    QUERY_DUMP_FILE,
    SOLUTION_FILE,

    // The strategy used for soi minimization
    SOI_SEARCH_STRATEGY,
    // The strategy used for initializing the soi
    SOI_INITIALIZATION_STRATEGY,

    // none/quick/linear
    EXPLANATION_STRATEGY,

    BRANCHING_HEURISTICS,
  };

  /*
    The singleton instance
  */
  static Options *get();

  /*
    Parse the command line arguments and extract the option values.
  */
  void parseOptions(int argc, char **argv);

  /*
    Print all command arguments
  */
  void printHelpMessage() const;

  /*
    Retrieve the value of the various options, by type
  */
  bool getBool(unsigned option) const;
  int getInt(unsigned option) const;
  float getFloat(unsigned option) const;
  String getString(unsigned option) const;
  SoIInitializationStrategy getSoIInitializationStrategy() const;
  SoISearchStrategy getSoISearchStrategy() const;

  /*
    Retrieve the value of the various options, by type
  */
  void setBool(unsigned option, bool);
  void setInt(unsigned option, int);
  void setFloat(unsigned option, float);
  void setString(unsigned option, std::string);

 private:
  /*
    Disable default constructor and copy constructor
  */
  Options();
  Options(const Options &);

  /*
    Initialize the default option values
  */
  void initializeDefaultValues();

  OptionParser _optionParser;

  /*
    The various option values
  */
  Map<unsigned, bool> _boolOptions;
  Map<unsigned, int> _intOptions;
  Map<unsigned, float> _floatOptions;
  Map<unsigned, std::string> _stringOptions;
};

#endif  // __Options_h__
