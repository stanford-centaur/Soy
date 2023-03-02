/*********************                                                        */
/*! \file Soy.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Haoze Wu
 ** This file is part of the Soy project.
 ** Copyright (c) 2023 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief [[ Add one-line brief description here ]]
 **
 ** [[ Add lengthier description here ]]
 **/

#include "Soy.h"

#include "DnCManager.h"
#include "File.h"
#include "GurobiWrapper.h"
#include "Map.h"
#include "MStringf.h"
#include "SoyError.h"
#include "MpsParser.h"
#include "Options.h"


Soy::Soy() : _dncManager(nullptr), _inputQuery(InputQuery()) {}

void Soy::run() {
  String inputQueryFilePath =
      Options::get()->getString(Options::INPUT_QUERY_FILE_PATH);

  if (Options::get()->getBool(Options::SOLVE_WITH_MILP)){
    struct timespec start = TimeUtils::sampleMicro();
    GurobiWrapper gurobi;
    gurobi.loadMPS(inputQueryFilePath);
    gurobi.setTimeLimit(Options::get()->getInt(Options::TIMEOUT));
    gurobi.solve();
    struct timespec end = TimeUtils::sampleMicro();

    String resultString;
    if (gurobi.haveFeasibleSolution())
      resultString = "sat";
    else if (gurobi.infeasible())
      resultString = "unsat";
    else
      resultString = "UNKNOWN";

    std::cout << resultString.ascii() << std::endl;

    // Create a summary file, if requested
    String summaryFilePath = Options::get()->getString(Options::SUMMARY_FILE);
    if (summaryFilePath != "") {
      File summaryFile(summaryFilePath);
      summaryFile.open(File::MODE_WRITE_TRUNCATE);

      // Field #1: result
      summaryFile.write(resultString);

      unsigned long long totalElapsed = TimeUtils::timePassed(start, end);
      // Field #2: total elapsed time
      summaryFile.write(Stringf(" %u ", totalElapsed / 1000000));

      // Field #3: average time per soi check
      summaryFile.write(Stringf(" %u ", gurobi.getNumberOfNodes()));

      // Field #4: average number of proposals per deep soi call
      summaryFile.write("0");

      summaryFile.write("\n");
    }
  } else {
    /*
      Step 1: extract the query
    */
    if (!File::exists(inputQueryFilePath)) {
      printf("Error: the specified inputQuery file (%s) doesn't exist!\n",
             inputQueryFilePath.ascii());
      throw SoyError(SoyError::FILE_DOESNT_EXIST,
                         inputQueryFilePath.ascii());
    }

    printf("InputQuery: %s\n", inputQueryFilePath.ascii());
    auto mpsParser = MpsParser(inputQueryFilePath);
    mpsParser.generateQuery(_inputQuery);

    /*
      Step 2: initialize the DNC core
    */
    _dncManager = std::unique_ptr<DnCManager>(new DnCManager(&_inputQuery));

    struct timespec start = TimeUtils::sampleMicro();

    _dncManager->solve();

    if (_dncManager->getExitCode() == DnCManager::SAT) {
      String solutionFilePath = Options::get()->getString(Options::SOLUTION_FILE);
      if (solutionFilePath != "") {
        Map<String, double> solution;
        _dncManager->extractSolution(mpsParser, solution);
        File solutionFile(solutionFilePath);
        solutionFile.open(File::MODE_WRITE_TRUNCATE);

        for (const auto &pair : solution) {
          solutionFile.write(Stringf("%s %.10f\n", pair.first.ascii(), pair.second));
        }
      }
    }

    struct timespec end = TimeUtils::sampleMicro();

    unsigned long long totalElapsed = TimeUtils::timePassed(start, end);
    displayResults(totalElapsed);
  }
}

void Soy::displayResults(unsigned long long microSecondsElapsed) const {
  _dncManager->printResult();
  String resultString = _dncManager->getResultString();
  // Create a summary file, if requested
  String summaryFilePath = Options::get()->getString(Options::SUMMARY_FILE);
  if (summaryFilePath != "") {
    File summaryFile(summaryFilePath);
    summaryFile.open(File::MODE_WRITE_TRUNCATE);

    // Field #1: result
    summaryFile.write(resultString);

    // Field #2: total elapsed time
    summaryFile.write(Stringf(" %u ", microSecondsElapsed / 1000000));

    // Field #3: average time per soi check
    summaryFile.write(Stringf("%u ", _dncManager->getAverageTimePerSoICheck()));

    // Field #4: average number of proposals per deep soi call
    summaryFile.write(Stringf("%.2f", _dncManager->getAverageProposalPerDeepSoI()));

    summaryFile.write("\n");
  }
}
