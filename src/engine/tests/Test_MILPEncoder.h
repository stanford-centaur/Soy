/*********************                                                        */
/*! \file Test_MILPEncoder.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Teruhiro Tagomori
 ** This file is part of the Soy project.
 ** Copyright (c) 2023 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#include <cxxtest/TestSuite.h>
#include <string.h>

#include "FloatUtils.h"
#include "InputQuery.h"
#include "MILPEncoder.h"
#include "SoyError.h"

class MILPEncoderTestSuite : public CxxTest::TestSuite {
 public:
  void setUp() {}

  void tearDown() {}

  void populateInputQuery(InputQuery &inputQuery, BoundManager &bm,
                          CVC4::context::Context &context) {
    // OneHot (x0, x1, x2)
    // Disjunction (x3, x4, x0)
    // x7 is between -2 and 2
    //
    // x5 = Abs x0
    // x6 = Abs x3
    // x8 = Abs x7
    //
    // x5 + x6 - x8 >= 2
    // x0 + x1 + x2 = 1
    // x3 + x4 + x0 >= 1
    inputQuery.setNumberOfVariables(9);

    for (unsigned i = 0; i < 7; ++i) {
      inputQuery.setLowerBound(i, 0);
      inputQuery.setUpperBound(i, 1);
    }

    inputQuery.setLowerBound(7, -2);
    inputQuery.setUpperBound(7, 2);
    inputQuery.setLowerBound(8, 0);
    inputQuery.setUpperBound(8, 2);

    inputQuery.addPLConstraint(new OneHotConstraint({0, 1, 2}));
    inputQuery.addPLConstraint(new DisjunctionConstraint({0, 3, 4}));
    inputQuery.addPLConstraint(new IntegerConstraint(7));
    inputQuery.addPLConstraint(new AbsoluteValueConstraint(0, 5));
    inputQuery.addPLConstraint(new AbsoluteValueConstraint(3, 6));
    inputQuery.addPLConstraint(new AbsoluteValueConstraint(7, 8));

    for (const auto &pl : inputQuery.getPLConstraints())
      pl->transformToUseAuxVariables(inputQuery);
    for (const auto &plConstraint : inputQuery.getPLConstraints()) {
      List<unsigned> variables = plConstraint->getParticipatingVariables();
      for (unsigned variable : variables) {
        plConstraint->notifyLowerBound(variable,
                                       inputQuery.getLowerBound(variable));
        plConstraint->notifyUpperBound(variable,
                                       inputQuery.getUpperBound(variable));
      }
      plConstraint->initializeCDOs(&context);
    }

    Equation eq1;
    eq1.setType(Equation::GE);
    eq1.addAddend(1, 5);
    eq1.addAddend(1, 6);
    eq1.addAddend(-1, 8);
    eq1.setScalar(2);

    Equation eq2;
    eq2.addAddend(1, 0);
    eq2.addAddend(1, 1);
    eq2.addAddend(1, 2);
    eq2.setScalar(1);

    Equation eq3;
    eq3.setType(Equation::GE);
    eq3.addAddend(1, 3);
    eq3.addAddend(1, 4);
    eq3.addAddend(1, 0);
    eq3.setScalar(1);

    inputQuery.addEquation(eq1);
    inputQuery.addEquation(eq2);
    inputQuery.addEquation(eq3);

    bm.initialize(inputQuery.getNumberOfVariables());
    inputQuery.setUpperBound(9, 0);
    inputQuery.setUpperBound(10, 100);
    inputQuery.setUpperBound(11, 0);
    inputQuery.setUpperBound(12, 100);
    inputQuery.setUpperBound(13, 100);
    inputQuery.setUpperBound(14, 100);
    for (unsigned i = 0; i < inputQuery.getNumberOfVariables(); ++i) {
      bm.setLowerBound(i, inputQuery.getLowerBound(i));
      bm.setUpperBound(i, inputQuery.getUpperBound(i));
    }
  }

  void populateInputQuery2(InputQuery &inputQuery, BoundManager &bm,
                           CVC4::context::Context &context) {
    // OneHot (x0, x1, x2)
    // Disjunction (x3, x4, x0)
    // x7 is between -2 and 2
    //
    // x5 = Abs x0
    // x6 = Abs x3
    // x8 = Abs x7
    //
    // x5 + x6 - x8 >= 3
    // x0 + x1 + x2 = 1
    // x3 + x4 + x0 >= 1

    inputQuery.setNumberOfVariables(9);

    for (unsigned i = 0; i < 7; ++i) {
      inputQuery.setLowerBound(i, 0);
      inputQuery.setUpperBound(i, 1);
    }

    inputQuery.setLowerBound(7, -2);
    inputQuery.setUpperBound(7, 2);
    inputQuery.setLowerBound(8, 0);
    inputQuery.setUpperBound(8, 2);

    inputQuery.addPLConstraint(new OneHotConstraint({0, 1, 2}));
    inputQuery.addPLConstraint(new DisjunctionConstraint({0, 3, 4}));
    inputQuery.addPLConstraint(new IntegerConstraint(7));
    inputQuery.addPLConstraint(new AbsoluteValueConstraint(0, 5));
    inputQuery.addPLConstraint(new AbsoluteValueConstraint(3, 6));
    inputQuery.addPLConstraint(new AbsoluteValueConstraint(7, 8));

    for (const auto &pl : inputQuery.getPLConstraints())
      pl->transformToUseAuxVariables(inputQuery);
    for (const auto &plConstraint : inputQuery.getPLConstraints()) {
      List<unsigned> variables = plConstraint->getParticipatingVariables();
      for (unsigned variable : variables) {
        plConstraint->notifyLowerBound(variable,
                                       inputQuery.getLowerBound(variable));
        plConstraint->notifyUpperBound(variable,
                                       inputQuery.getUpperBound(variable));
      }
      plConstraint->initializeCDOs(&context);
    }

    Equation eq1;
    eq1.setType(Equation::GE);
    eq1.addAddend(1, 5);
    eq1.addAddend(1, 6);
    eq1.addAddend(-1, 8);
    eq1.setScalar(3);

    Equation eq2;
    eq2.addAddend(1, 0);
    eq2.addAddend(1, 1);
    eq2.addAddend(1, 2);
    eq2.setScalar(1);

    Equation eq3;
    eq3.setType(Equation::GE);
    eq3.addAddend(1, 3);
    eq3.addAddend(1, 4);
    eq3.addAddend(1, 0);
    eq3.setScalar(1);

    inputQuery.addEquation(eq1);
    inputQuery.addEquation(eq2);
    inputQuery.addEquation(eq3);

    bm.initialize(inputQuery.getNumberOfVariables());
    inputQuery.setUpperBound(9, 0);
    inputQuery.setUpperBound(10, 100);
    inputQuery.setUpperBound(11, 0);
    inputQuery.setUpperBound(12, 100);
    inputQuery.setUpperBound(13, 100);
    inputQuery.setUpperBound(14, 100);
    for (unsigned i = 0; i < inputQuery.getNumberOfVariables(); ++i) {
      bm.setLowerBound(i, inputQuery.getLowerBound(i));
      bm.setUpperBound(i, inputQuery.getUpperBound(i));
    }
  }

  void test_eoncode_query_precise() {
    InputQuery inputQuery;
    CVC4::context::Context context;
    BoundManager bm(context);
    populateInputQuery(inputQuery, bm, context);
    GurobiWrapper gurobi;

    MILPEncoder encoder(bm);
    encoder.encodeInputQuery(gurobi, inputQuery, false);

    gurobi.dumpModel("test.lp");
    gurobi.solve();

    Map<String, double> solution;
    double cost;
    gurobi.extractSolution(solution, cost);
    TS_ASSERT_EQUALS(solution["x0"], 1);
    TS_ASSERT_EQUALS(solution["x1"], 0);
    TS_ASSERT_EQUALS(solution["x2"], 0);
    TS_ASSERT_EQUALS(solution["x3"], 1);
    TS_ASSERT_EQUALS(solution["x5"], 1);
    TS_ASSERT_EQUALS(solution["x6"], 1);
  }

  void test_eoncode_query_precise2() {
    InputQuery inputQuery;
    CVC4::context::Context context;
    BoundManager bm(context);
    populateInputQuery2(inputQuery, bm, context);
    GurobiWrapper gurobi;

    MILPEncoder encoder(bm);
    encoder.encodeInputQuery(gurobi, inputQuery, false);
    gurobi.solve();

    TS_ASSERT(gurobi.infeasible());
  }

  void test_eoncode_query_relax() {
    InputQuery inputQuery;
    CVC4::context::Context context;
    BoundManager bm(context);
    populateInputQuery(inputQuery, bm, context);
    GurobiWrapper gurobi;

    MILPEncoder encoder(bm);
    encoder.encodeInputQuery(gurobi, inputQuery, false);
    gurobi.solve();
    gurobi.haveFeasibleSolution();
  }
};
