/*********************                                                        */
/*! \file OptionParser.cpp
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

#include "OptionParser.h"

#include "Debug.h"
#include "Options.h"

OptionParser::OptionParser() {
  // This constructor should not be called
  ASSERT(false);
}

OptionParser::OptionParser(Map<unsigned, bool> *boolOptions,
                           Map<unsigned, int> *intOptions,
                           Map<unsigned, float> *floatOptions,
                           Map<unsigned, std::string> *stringOptions)
    : _positional(""),
      _common("Common options"),
      _other("Less common options"),
      _expert("More advanced internal options"),
      _boolOptions(boolOptions),
      _intOptions(intOptions),
      _floatOptions(floatOptions),
      _stringOptions(stringOptions) {}

void OptionParser::initialize() {
  _positional.add_options()(
      "input-query",
      boost::program_options::value<std::string>(
          &((*_stringOptions)[Options::INPUT_QUERY_FILE_PATH]))
          ->default_value((*_stringOptions)[Options::INPUT_QUERY_FILE_PATH]),
      "Input Query file. When specified, Soy will solve this instead of "
      "the network and property pair.");

  // Most common options
  _common.add_options()(
      "num-workers",
      boost::program_options::value<int>(&(*_intOptions)[Options::NUM_WORKERS])
          ->default_value((*_intOptions)[Options::NUM_WORKERS]),
      "Number of workers.")(
      "gurobi-threads",
      boost::program_options::value<int>(
          &(*_intOptions)[Options::GUROBI_THREADS])
          ->default_value((*_intOptions)[Options::GUROBI_THREADS]),
      "Number of threads that gurobi uses.")(
      "split-threshold",
      boost::program_options::value<int>(
          &(*_intOptions)[Options::DEEP_SOI_REJECTION_THRESHOLD])
          ->default_value(
              (*_intOptions)[Options::DEEP_SOI_REJECTION_THRESHOLD]),
      "Rejection threshold.")(
      "restart-threshold",
      boost::program_options::value<int>(
          &(*_intOptions)[Options::RESTART_THESHOLD])
          ->default_value((*_intOptions)[Options::RESTART_THESHOLD]),
      "Restart threshold.")(
       "k",
       boost::program_options::value<int>(
          &(*_intOptions)[Options::MAX_LEMMA_LENGTH])
       ->default_value((*_intOptions)[Options::MAX_LEMMA_LENGTH]),
       "Max length of lemma to compute.")(
       "max-proposals",
       boost::program_options::value<int>(
          &(*_intOptions)[Options::MAX_PROPOSALS_PER_STATE])
       ->default_value((*_intOptions)[Options::MAX_PROPOSALS_PER_STATE]),
       "Initial phase pattern.")(
                                 "tabu",
                                 boost::program_options::value<int>(
                                                                    &(*_intOptions)[Options::TABU])
                                 ->default_value((*_intOptions)[Options::TABU]),
                                 "tabu length.")(
      "help",
      boost::program_options::bool_switch(&(*_boolOptions)[Options::HELP])
          ->default_value((*_boolOptions)[Options::HELP]),
      "Prints the help message.")(
      "version",
      boost::program_options::bool_switch(&(*_boolOptions)[Options::VERSION])
          ->default_value((*_boolOptions)[Options::VERSION]),
      "Prints the version number.")(
      "timeout",
      boost::program_options::value<int>(&(*_intOptions)[Options::TIMEOUT])
          ->default_value((*_intOptions)[Options::TIMEOUT]),
      "Global timeout in seconds. 0 means no timeout.")(
      "milp",
      boost::program_options::bool_switch(
          &(*_boolOptions)[Options::SOLVE_WITH_MILP])
          ->default_value((*_boolOptions)[Options::SOLVE_WITH_MILP]),
      "Solve the input query with a MILP encoding in Gruobi.")(
      "feas",
      boost::program_options::bool_switch(
          &(*_boolOptions)[Options::FEASIBILITY_ONLY])
          ->default_value((*_boolOptions)[Options::FEASIBILITY_ONLY]),
      "Instead of minimize SoI phase pattern, just check its feasibility.")(
      "no-cdcl",
      boost::program_options::bool_switch(&(*_boolOptions)[Options::NO_CDCL])
          ->default_value((*_boolOptions)[Options::NO_CDCL]),
      "No CDCL.")(
      "no-tightening",
      boost::program_options::bool_switch(
          &(*_boolOptions)[Options::NO_BOUND_TIGHTENING])
          ->default_value((*_boolOptions)[Options::NO_BOUND_TIGHTENING]),
      "Bound tightening.")(
      "no-phase-conflict",
      boost::program_options::bool_switch(
          &(*_boolOptions)[Options::NO_PHASE_CONFLICT])
          ->default_value((*_boolOptions)[Options::NO_PHASE_CONFLICT]),
      "Do not add failed soi phae pattern as conflict.")(
      "vsids",
      boost::program_options::bool_switch(
                                          &(*_boolOptions)[Options::VSIDS])
      ->default_value((*_boolOptions)[Options::VSIDS]),
      "Use vsids.");

  // Less common options
  _other.add_options()(
      "verbosity",
      boost::program_options::value<int>(&((*_intOptions)[Options::VERBOSITY]))
          ->default_value((*_intOptions)[Options::VERBOSITY]),
      "Verbosity of engine::solve(). 0: does not print anything, 1: print"
      "out statistics in the beginning and end, 2: print out statistics during "
      "solving.")(
      "seed",
      boost::program_options::value<int>(&((*_intOptions)[Options::SEED]))
          ->default_value((*_intOptions)[Options::SEED]),
      "The random seed.")(
      "query-dump-file",
      boost::program_options::value<std::string>(
          &(*_stringOptions)[Options::QUERY_DUMP_FILE])
          ->default_value((*_stringOptions)[Options::QUERY_DUMP_FILE]),
      "Dump the verification query in Soy's input query format.")(
      "summary-file",
      boost::program_options::value<std::string>(
          &((*_stringOptions)[Options::SUMMARY_FILE]))
          ->default_value((*_stringOptions)[Options::SUMMARY_FILE]),
      "Produce a summary file of the run.")(
       "solution-file",
       boost::program_options::value<std::string>(
                                                  &((*_stringOptions)[Options::SOLUTION_FILE]))
       ->default_value((*_stringOptions)[Options::SOLUTION_FILE]),
       "Write the feasible solution to this file.")(
      "search-strategy",
      boost::program_options::value<std::string>(
          &((*_stringOptions)[Options::SOI_SEARCH_STRATEGY]))
          ->default_value((*_stringOptions)[Options::SOI_SEARCH_STRATEGY]),
      "(DeepSoI) Strategy for stochastically minimizing the soi: "
      "mcmc/walksat.")(
      "init-strategy",
      boost::program_options::value<std::string>(
          &((*_stringOptions)[Options::SOI_INITIALIZATION_STRATEGY]))
          ->default_value(
              (*_stringOptions)[Options::SOI_INITIALIZATION_STRATEGY]),
      "(DeepSoI) Strategy for initialize the soi function: "
      "input-assignment/current-assignment. default: input-assignment.")(
      "explain",
      boost::program_options::value<std::string>(
          &((*_stringOptions)[Options::EXPLANATION_STRATEGY]))
          ->default_value((*_stringOptions)[Options::EXPLANATION_STRATEGY]),
      "none/quick/farkas.")(
      "branch",
      boost::program_options::value<std::string>(
          &((*_stringOptions)[Options::BRANCHING_HEURISTICS]))
          ->default_value((*_stringOptions)[Options::BRANCHING_HEURISTICS]),
      "(DeepSoI) Strategy for initialize the soi function: "
      "input-assignment/current-assignment. default: input-assignment.");

  _expert.add_options()(
      "mcmc-beta",
      boost::program_options::value<float>(
          &((*_floatOptions)[Options::PROBABILITY_DENSITY_PARAMETER]))
          ->default_value(
              (*_floatOptions)[Options::PROBABILITY_DENSITY_PARAMETER]),
      "(DeepSoI) The beta parameter in MCMC search. The larger, the greedier. "
      "\n")(
            "milp-threshold",
            boost::program_options::value<float>(
                                               &(*_floatOptions)[Options::MILP_SOLVING_THRESHOLD])
            ->default_value((*_floatOptions)[Options::MILP_SOLVING_THRESHOLD]),
            "When tree depth is larger than this, use milp to solve the node.");

  _optionDescription.add(_positional).add(_common).add(_other).add(_expert);

  // Positional options, for the mandatory options
  _positionalOptions.add("input-query", 1);
}

void OptionParser::parse(int argc, char **argv) {
  boost::program_options::store(
      boost::program_options::command_line_parser(argc, argv)
          .options(_optionDescription)
          .positional(_positionalOptions)
          .run(),
      _variableMap);
  boost::program_options::notify(_variableMap);
}

bool OptionParser::valueExists(const String &option) {
  return _variableMap.count(option.ascii()) != 0;
}

int OptionParser::extractIntValue(const String &option) {
  ASSERT(valueExists(option));
  return _variableMap[option.ascii()].as<int>();
}

void OptionParser::printHelpMessage() const {
  std::cerr << "\nusage: ./Soy <network.nnet> <property> [<options>]\n"
            << std::endl;
  std::cerr << "OR     ./Soy <input-query-file> [<options>]\n" << std::endl;

  std::cerr << "You might also consider using the ./resources/runSoy.py "
            << "script, see README.md for more information." << std::endl;

  std::cerr << "\nBelow are the possible options:" << std::endl;

  std::cerr << "\n" << _common << std::endl;
  std::cerr << "\n" << _other << std::endl;
  std::cerr << "\n" << _expert << std::endl;
};

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
