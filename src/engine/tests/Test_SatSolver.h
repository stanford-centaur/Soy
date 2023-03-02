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

#include "CadicalWrapper.h"
#include "cadical.hpp"

class SatSolverTestSuite : public CxxTest::TestSuite {
 public:
  void setUp() {}

  void tearDown() {}

  void test_solve_fixed_vars() {
    // 1 -2 0
    //
    CadicalWrapper solver;
    for (unsigned i = 1; i <= 2; ++i)
      TS_ASSERT_EQUALS(solver.getFreshVariable(), i);

    solver.addConstraint({1, -2});

    TS_ASSERT(solver.getNumberOfVariables() == 2);

    TS_ASSERT_THROWS_NOTHING(solver.solve());
    TS_ASSERT(solver.getLiteralStatus(1) == UNFIXED);
    TS_ASSERT(solver.getLiteralStatus(2) == UNFIXED);
  }

  void test_fixed_with_solve() {
    // -1 2 3 0
    // -1 -2 0
    // 1 0
    // 4 5 0
    CadicalWrapper solver;
    for (unsigned i = 1; i <= 5; ++i)
      TS_ASSERT_EQUALS(solver.getFreshVariable(), i);

    solver.addConstraint({-1, 2, 3});
    solver.addConstraint({-1, -2});
    solver.addConstraint({1});
    solver.addConstraint({4, 5});

    TS_ASSERT_THROWS_NOTHING(solver.solve());

    TS_ASSERT_EQUALS(solver.getLiteralStatus(1), TRUE);
    TS_ASSERT_EQUALS(solver.getLiteralStatus(-1), FALSE);
    TS_ASSERT_EQUALS(solver.getLiteralStatus(2), FALSE);
    TS_ASSERT_EQUALS(solver.getLiteralStatus(-2), TRUE);
    TS_ASSERT_EQUALS(solver.getLiteralStatus(3), TRUE);
    TS_ASSERT_EQUALS(solver.getLiteralStatus(-3), FALSE);
    TS_ASSERT_EQUALS(solver.getLiteralStatus(4), UNFIXED);
    TS_ASSERT_EQUALS(solver.getLiteralStatus(-4), UNFIXED);

    {
      // -1 -2 3 0
      CaDiCaL::Solver solver;
      solver.set("walk", false);
      solver.set("lucky", false);
      solver.add(1);
      solver.add(2);
      solver.add(0);

      solver.phase(1);
      solver.phase(2);
      TS_ASSERT(solver.solve() == 10);
      TS_ASSERT_EQUALS(solver.val(1), 1);
      TS_ASSERT(solver.val(2) == 2);

      solver.phase(1);
      solver.phase(-2);
      TS_ASSERT(solver.solve() == 10);
      TS_ASSERT_EQUALS(solver.val(1), 1);
      TS_ASSERT(solver.val(2) == -2);

      solver.phase(-1);
      solver.phase(2);
      TS_ASSERT(solver.solve() == 10);
      TS_ASSERT_EQUALS(solver.val(1), -1);
      TS_ASSERT(solver.val(2) == 2);
    }
  }

  void test_fixed_with_preprocess() {
    // -1 2 3 0
    // -1 -2 0
    // 1 0
    // 4 5 0
    CadicalWrapper solver;
    for (unsigned i = 1; i <= 5; ++i)
      TS_ASSERT_EQUALS(solver.getFreshVariable(), i);

    solver.addConstraint({-1, 2, 3});
    solver.addConstraint({-1, -2});
    solver.addConstraint({1});
    solver.addConstraint({4, 5});

    TS_ASSERT_THROWS_NOTHING(solver.preprocess());

    TS_ASSERT_EQUALS(solver.getLiteralStatus(1), TRUE);
    TS_ASSERT_EQUALS(solver.getLiteralStatus(-1), FALSE);
    TS_ASSERT_EQUALS(solver.getLiteralStatus(2), FALSE);
    TS_ASSERT_EQUALS(solver.getLiteralStatus(-2), TRUE);
    TS_ASSERT_EQUALS(solver.getLiteralStatus(3), TRUE);
    TS_ASSERT_EQUALS(solver.getLiteralStatus(-3), FALSE);
    TS_ASSERT_EQUALS(solver.getLiteralStatus(4), UNFIXED);
    TS_ASSERT_EQUALS(solver.getLiteralStatus(-4), UNFIXED);
  }

  void test_assumption() {
    // -1 2 3 0
    // -1 -2 0
    // 4 5 0
    CadicalWrapper solver;
    for (unsigned i = 1; i <= 5; ++i)
      TS_ASSERT_EQUALS(solver.getFreshVariable(), i);

    solver.addConstraint({-1, 2, 3});
    solver.addConstraint({-1, -2});
    solver.addConstraint({4, 5});

    // 1 0
    solver.assumeLiteral(1);
    TS_ASSERT_THROWS_NOTHING(solver.solve());
    TS_ASSERT_EQUALS(solver.getLiteralStatus(1), TRUE);
    TS_ASSERT_EQUALS(solver.getLiteralStatus(2), FALSE);
    TS_ASSERT_EQUALS(solver.getLiteralStatus(3), TRUE);
    TS_ASSERT_EQUALS(solver.getLiteralStatus(4), UNFIXED);
    TS_ASSERT_EQUALS(solver.getAssignment(1), TRUE);
    TS_ASSERT_EQUALS(solver.getAssignment(2), FALSE);
    TS_ASSERT_EQUALS(solver.getAssignment(3), TRUE);

    TS_ASSERT(solver.haveAssumptions());

    solver.clearAssumptions();
    TS_ASSERT(!solver.haveAssumptions());
  }

  void test_set_direction() {
    // -1 2 3 0
    // -1 -2 0
    // 4 -5 0
    // 4 5 0
    CadicalWrapper solver;
    for (unsigned i = 1; i <= 5; ++i)
      TS_ASSERT_EQUALS(solver.getFreshVariable(), i);

    solver.addConstraint({-1, 2, 3});
    solver.addConstraint({-1, -2});
    solver.addConstraint({4, -5});
    solver.addConstraint({4, 5});

    // 1 0
    Map<unsigned, bool> solution;
    solver.setDirection(1);
    TS_ASSERT_EQUALS(solver.directionAwareSolve(), 0u);
    TS_ASSERT_THROWS_NOTHING(solver.extractSolution(solution));
    TS_ASSERT(solution[1]);

    solver.setDirection(-2);
    TS_ASSERT_EQUALS(solver.directionAwareSolve(), 0u);
    TS_ASSERT_THROWS_NOTHING(solver.extractSolution(solution));
    TS_ASSERT(solution[1]);
    TS_ASSERT(!solution[2]);

    solver.setDirection(-4);
    TS_ASSERT_EQUALS(solver.directionAwareSolve(), 1u);
    TS_ASSERT_THROWS_NOTHING(solver.extractSolution(solution));
    TS_ASSERT(solution[1]);
    TS_ASSERT(!solution[2]);
    TS_ASSERT(solution[4]);
  }

  void test_set_direction1() {
    // 1 2 0
    // -1 -2 0
    //
    CadicalWrapper solver;
    for (unsigned i = 1; i <= 2; ++i)
      TS_ASSERT_EQUALS(solver.getFreshVariable(), i);

    solver.addConstraint({1, 2});
    solver.addConstraint({-1, -2});

    solver.setDirection(1);
    TS_ASSERT_EQUALS(solver.directionAwareSolve(), 0u);
    Map<unsigned, bool> solution;
    TS_ASSERT_THROWS_NOTHING(solver.extractSolution(solution));
    TS_ASSERT(solution[1]);
    TS_ASSERT(!solution[2]);

    solver.setDirection(2);
    TS_ASSERT_EQUALS(solver.directionAwareSolve(), 1u);
    TS_ASSERT_THROWS_NOTHING(solver.extractSolution(solution));
    TS_ASSERT(!solution[1]);
    TS_ASSERT(solution[2]);

    solver.setDirection(1);
    TS_ASSERT_EQUALS(solver.directionAwareSolve(), 1u);
    TS_ASSERT_THROWS_NOTHING(solver.extractSolution(solution));
    TS_ASSERT(solution[1]);
    TS_ASSERT(!solution[2]);

    solver.setDirection(-1);
    TS_ASSERT_EQUALS(solver.directionAwareSolve(), 0u);
    TS_ASSERT_THROWS_NOTHING(solver.extractSolution(solution));
    TS_ASSERT(!solution[1]);
    TS_ASSERT(solution[2]);

    solver.setDirection(-2);
    TS_ASSERT_EQUALS(solver.directionAwareSolve(), 1u);
    TS_ASSERT_THROWS_NOTHING(solver.extractSolution(solution));
    TS_ASSERT(solution[1]);
    TS_ASSERT(!solution[2]);
  }
};
