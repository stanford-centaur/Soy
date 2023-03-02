/*********************                                                        */
/*! \file Test_GurobiWrapper.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Soy project.
 ** Copyright (c) 2023 by the authors gurobiWrappered in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief [[ Add one-line brief description here ]]
 **
 ** [[ Add lengthier description here ]]
 **/

#include <cxxtest/TestSuite.h>

#include "FloatUtils.h"
#include "GurobiWrapper.h"
#include "MString.h"
#include "MockErrno.h"

class GurobiWrapperTestSuite : public CxxTest::TestSuite {
 public:
  MockErrno *mockErrno;

  void setUp() { TS_ASSERT(mockErrno = new MockErrno); }

  void tearDown() { TS_ASSERT_THROWS_NOTHING(delete mockErrno); }

  void test_optimize() {
#ifdef ENABLE_GUROBI
    GurobiWrapper gurobi;

    gurobi.addVariable("x", 0, 3);
    gurobi.addVariable("y", 0, 3);
    gurobi.addVariable("z", 0, 3);

    // x + y + z <= 5
    List<GurobiWrapper::Term> contraint = {
        GurobiWrapper::Term(1, "x"),
        GurobiWrapper::Term(1, "y"),
        GurobiWrapper::Term(1, "z"),
    };

    gurobi.addLeqConstraint(contraint, 5);

    // Cost: -x - 2y + z
    List<GurobiWrapper::Term> cost = {
        GurobiWrapper::Term(-1, "x"),
        GurobiWrapper::Term(-2, "y"),
        GurobiWrapper::Term(+1, "z"),
    };

    gurobi.setCost(cost);

    // Solve and extract
    TS_ASSERT_THROWS_NOTHING(gurobi.solve());

    Map<String, double> solution;
    double costValue;

    TS_ASSERT_THROWS_NOTHING(gurobi.extractSolution(solution, costValue));

    TS_ASSERT(FloatUtils::areEqual(solution["x"], 2));
    TS_ASSERT(FloatUtils::areEqual(solution["y"], 3));
    TS_ASSERT(FloatUtils::areEqual(solution["z"], 0));

    TS_ASSERT(FloatUtils::areEqual(costValue, -8));

#else
    TS_ASSERT(true);
#endif  // ENABLE_GUROBI
  }

  void test_optimize1() {
#ifdef ENABLE_GUROBI
    GurobiWrapper gurobi;

    gurobi.addVariable("x0", 0, 1);
    gurobi.addVariable("x1", 0, 1);
    gurobi.addVariable("x2", 0, 1);
    gurobi.addVariable("x3", 0, 1);

    List<GurobiWrapper::Term> contraint = {
        GurobiWrapper::Term(1, "x0"),
        GurobiWrapper::Term(1, "x1"),
    };

    gurobi.addEqConstraint(contraint, 1);

    contraint = {
        GurobiWrapper::Term(1, "x2"),
        GurobiWrapper::Term(1, "x3"),
    };

    gurobi.addEqConstraint(contraint, 1);

    contraint = {
        GurobiWrapper::Term(1, "x0"),
        GurobiWrapper::Term(1, "x2"),
    };

    gurobi.addEqConstraint(contraint, 1.5);

    contraint = {
        GurobiWrapper::Term(1, "x0"),
        GurobiWrapper::Term(1, "x3"),
    };

    gurobi.addEqConstraint(contraint, 1.5);

    List<GurobiWrapper::Term> cost;

    gurobi.setCost(cost, 0);

    // Solve and extract
    TS_ASSERT_THROWS_NOTHING(gurobi.solve());

    Map<String, double> solution;
    double costValue;

    TS_ASSERT_THROWS_NOTHING(gurobi.extractSolution(solution, costValue));
    TS_ASSERT(FloatUtils::areEqual(costValue, 0));

    cost = {GurobiWrapper::Term(-1, "x0"), GurobiWrapper::Term(-1, "x3")};

    gurobi.setCost(cost, 2);

    // Solve and extract
    TS_ASSERT_THROWS_NOTHING(gurobi.solve());
    TS_ASSERT_THROWS_NOTHING(gurobi.extractSolution(solution, costValue));
    TS_ASSERT(FloatUtils::areEqual(costValue, 0.5));
#else
    TS_ASSERT(true);
#endif  // ENABLE_GUROBI
  }

  void test_iis1() {
#ifdef ENABLE_GUROBI
    GurobiWrapper gurobi;

    gurobi.addVariable("x", 0, 10);
    gurobi.addVariable("y", 0, 10);
    gurobi.addVariable("z", 0, 10);

    // x + y + z >= 3
    List<GurobiWrapper::Term> contraint = {
        GurobiWrapper::Term(1, "x"),
        GurobiWrapper::Term(1, "y"),
        GurobiWrapper::Term(1, "z"),
    };

    gurobi.addGeqConstraint(contraint, 3, "C1");

    // x + y <= 1
    contraint = {GurobiWrapper::Term(1, "x"), GurobiWrapper::Term(1, "y")};

    gurobi.addLeqConstraint(contraint, 1, "C2");

    // y + z <= 4
    contraint = {GurobiWrapper::Term(1, "y"), GurobiWrapper::Term(1, "z")};

    gurobi.addLeqConstraint(contraint, 4, "C3");

    gurobi.solve();
    TS_ASSERT(gurobi.haveFeasibleSolution());

    // z <= 1
    contraint = {GurobiWrapper::Term(1, "z")};

    gurobi.addLeqConstraint(contraint, 1, "B1");
    gurobi.solve();
    TS_ASSERT(gurobi.infeasible());

    TS_ASSERT_THROWS_NOTHING(gurobi.computeIIS());

    Map<String, GurobiWrapper::IISBoundType> bounds;
    List<String> constraints;
    TS_ASSERT_THROWS_NOTHING(
        gurobi.extractIIS(bounds, constraints, {"C1", "C2", "C3", "B1"}));
#else
    TS_ASSERT(true);
#endif  // ENABLE_GUROBI
  }

  void test_iis2() {
#ifdef ENABLE_GUROBI
    GurobiWrapper gurobi;

    gurobi.addVariable("x", 0, 2);
    gurobi.addVariable("y", 0, 2);

    // x + y >= 4
    List<GurobiWrapper::Term> contraint = {GurobiWrapper::Term(1, "x"),
                                           GurobiWrapper::Term(1, "y")};

    gurobi.addGeqConstraint(contraint, 4, "C1");

    gurobi.solve();
    TS_ASSERT(gurobi.haveFeasibleSolution());

    gurobi.setUpperBound("x", 1);
    gurobi.solve();
    TS_ASSERT(gurobi.infeasible());

    TS_ASSERT_THROWS_NOTHING(gurobi.computeIIS());

    {
      Map<String, GurobiWrapper::IISBoundType> bounds;
      List<String> constraints;
      TS_ASSERT_THROWS_NOTHING(gurobi.extractIIS(bounds, constraints, {"C1"}));
      TS_ASSERT(bounds.exists("x"));
      TS_ASSERT_EQUALS(bounds["x"], GurobiWrapper::IIS_UB);
      TS_ASSERT(bounds.exists("y"));
      TS_ASSERT_EQUALS(bounds["y"], GurobiWrapper::IIS_UB);
      TS_ASSERT(constraints.exists("C1"));
    }

    gurobi.setUpperBound("x", 2);
    gurobi.solve();
    TS_ASSERT(gurobi.haveFeasibleSolution());

    gurobi.setUpperBound("y", 1);
    gurobi.solve();
    TS_ASSERT(gurobi.infeasible());

    TS_ASSERT_THROWS_NOTHING(gurobi.computeIIS());

    {
      Map<String, GurobiWrapper::IISBoundType> bounds;
      List<String> constraints;
      TS_ASSERT_THROWS_NOTHING(gurobi.extractIIS(bounds, constraints, {"C1"}));
      TS_ASSERT(bounds.exists("x"));
      TS_ASSERT_EQUALS(bounds["x"], GurobiWrapper::IIS_UB);
      TS_ASSERT(bounds.exists("y"));
      TS_ASSERT_EQUALS(bounds["y"], GurobiWrapper::IIS_UB);
      TS_ASSERT(constraints.exists("C1"));
    }

#else
    TS_ASSERT(true);
#endif  // ENABLE_GUROBI
  }
};
