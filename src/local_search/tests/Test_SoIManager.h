/*********************                                                        */
/*! \file Test_SoIManager.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Haoze Wu
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

#include "AssignmentManager.h"
#include "BoundManager.h"
#include "InputQuery.h"
#include "OneHotConstraint.h"
#include "Options.h"
#include "SoIManager.h"
#include "context/context.h"

class SoIManagerTestSuite : public CxxTest::TestSuite {
 public:
  void setUp() {}

  void tearDown() {}

  void test_initialize_and_walksat() {
    Options::get()->setString(Options::SOI_INITIALIZATION_STRATEGY,
                              "current-assignment");
    Options::get()->setString(Options::SOI_SEARCH_STRATEGY, "walksat");

    CVC4::context::Context context;

    InputQuery ipq;
    BoundManager bm(context);
    bm.initialize(8);
    AssignmentManager am(bm);

    PLConstraint *r1 = new OneHotConstraint({0, 1, 2});
    PLConstraint *r2 = new OneHotConstraint({3, 4, 5});
    PLConstraint *r3 = new OneHotConstraint({6, 7});
    List<PLConstraint *> constraints = {r1, r2, r3};
    for (const auto &constraint : constraints) {
      constraint->initializeCDOs(&context);
      constraint->registerBoundManager(&bm);
      constraint->registerAssignmentManager(&am);
      ipq.addPLConstraint(constraint);
    }

    SoIManager soiManager(ipq);
    soiManager.setAssignmentManager(&am);

    {
      am.setAssignment(0, 0.1);
      am.setAssignment(1, 0.1);
      am.setAssignment(2, 0.8);

      am.setAssignment(3, 0.5);
      am.setAssignment(4, 0.4);
      am.setAssignment(5, 0.1);

      am.setAssignment(6, 0.6);
      am.setAssignment(7, 0.4);

      TS_ASSERT_THROWS_NOTHING(soiManager.initializePhasePattern());
      LinearExpression correct;
      correct._constant = 3;
      correct._addends[2] = -1;
      correct._addends[3] = -1;
      correct._addends[6] = -1;

      LinearExpression e = soiManager.getCurrentSoIPhasePattern();
      TS_ASSERT_EQUALS(correct, e);
    }

    {
      am.setAssignment(0, 0.4);
      am.setAssignment(1, 0.3);
      am.setAssignment(2, 0.3);

      TS_ASSERT_THROWS_NOTHING(soiManager.initializePhasePattern());
      LinearExpression correct;
      correct._constant = 3;
      correct._addends[0] = -1;
      correct._addends[3] = -1;
      correct._addends[6] = -1;

      LinearExpression e = soiManager.getCurrentSoIPhasePattern();
      TS_ASSERT_EQUALS(correct, e);
    }

    // Update
    {
      am.setAssignment(0, 0.2);
      am.setAssignment(1, 0.3);
      am.setAssignment(2, 0.5);

      am.setAssignment(3, 1);
      am.setAssignment(4, 0);
      am.setAssignment(5, 0);

      am.setAssignment(6, 1);
      am.setAssignment(7, 0);

      TS_ASSERT_THROWS_NOTHING(soiManager.proposePhasePatternUpdate());
      LinearExpression correct;
      correct._constant = 3;
      correct._addends[2] = -1;
      correct._addends[3] = -1;
      correct._addends[6] = -1;

      LinearExpression e = soiManager.getCurrentSoIPhasePattern();
      TS_ASSERT_EQUALS(correct, e);

      TS_ASSERT_THROWS_NOTHING(soiManager.proposePhasePatternUpdate());
      e = soiManager.getCurrentSoIPhasePattern();
      TS_ASSERT_EQUALS(correct, e);
    }

    // Update
    {
      TS_ASSERT_THROWS_NOTHING(soiManager.acceptCurrentPhasePattern());

      am.setAssignment(0, 0.2);
      am.setAssignment(1, 0.3);
      am.setAssignment(2, 0.5);

      am.setAssignment(3, 0);
      am.setAssignment(4, 1);
      am.setAssignment(5, 0);

      am.setAssignment(6, 1);
      am.setAssignment(7, 0);

      TS_ASSERT_THROWS_NOTHING(soiManager.proposePhasePatternUpdate());
      LinearExpression correct;
      correct._constant = 3;
      correct._addends[2] = -1;
      correct._addends[4] = -1;
      correct._addends[6] = -1;

      LinearExpression e = soiManager.getCurrentSoIPhasePattern();
      TS_ASSERT_EQUALS(correct, e);
    }
  }

  void test_initialize_and_greedy() {
    Options::get()->setString(Options::SOI_INITIALIZATION_STRATEGY,
                              "current-assignment");
    Options::get()->setString(Options::SOI_SEARCH_STRATEGY, "greedy");

    CVC4::context::Context context;

    InputQuery ipq;
    BoundManager bm(context);
    bm.initialize(8);
    AssignmentManager am(bm);

    PLConstraint *r1 = new OneHotConstraint({0, 1, 2});
    PLConstraint *r2 = new OneHotConstraint({3, 4, 5});
    PLConstraint *r3 = new OneHotConstraint({6, 7});
    List<PLConstraint *> constraints = {r1, r2, r3};
    for (const auto &constraint : constraints) {
      constraint->initializeCDOs(&context);
      constraint->registerBoundManager(&bm);
      constraint->registerAssignmentManager(&am);
      ipq.addPLConstraint(constraint);
    }

    SoIManager soiManager(ipq);
    soiManager.setAssignmentManager(&am);

    am.setAssignment(0, 0);
    am.setAssignment(1, 0);
    am.setAssignment(2, 1);

    am.setAssignment(3, 0);
    am.setAssignment(4, 1);
    am.setAssignment(5, 0);

    am.setAssignment(6, 1);
    am.setAssignment(7, 0);

    TS_ASSERT_THROWS_NOTHING(soiManager.initializePhasePattern());

    {
      am.setAssignment(0, 0);
      am.setAssignment(1, 0);
      am.setAssignment(2, 1);

      am.setAssignment(3, 0);
      am.setAssignment(4, 1);
      am.setAssignment(5, 0);

      am.setAssignment(6, 0.9);
      am.setAssignment(7, 0.1);

      TS_ASSERT_THROWS_NOTHING(soiManager.proposePhasePatternUpdate());

      LinearExpression correct;
      correct._constant = 3;
      correct._addends[2] = -1;
      correct._addends[4] = -1;
      correct._addends[7] = -1;

      LinearExpression e = soiManager.getCurrentSoIPhasePattern();
      TS_ASSERT_EQUALS(correct, e);
    }

    {
      TS_ASSERT_THROWS_NOTHING(soiManager.acceptCurrentPhasePattern());

      am.setAssignment(0, 0);
      am.setAssignment(1, 0);
      am.setAssignment(2, 1);

      am.setAssignment(3, 0.2);
      am.setAssignment(4, 0.4);
      am.setAssignment(5, 0.6);

      am.setAssignment(6, 0);
      am.setAssignment(7, 1);

      TS_ASSERT_THROWS_NOTHING(soiManager.proposePhasePatternUpdate());

      LinearExpression correct;
      correct._constant = 3;
      correct._addends[2] = -1;
      correct._addends[5] = -1;
      correct._addends[7] = -1;

      LinearExpression e = soiManager.getCurrentSoIPhasePattern();
      TS_ASSERT_EQUALS(correct, e);
    }
  }

  void test_cache() {
    Options::get()->setString(Options::SOI_INITIALIZATION_STRATEGY,
                              "current-assignment");
    Options::get()->setString(Options::SOI_SEARCH_STRATEGY, "greedy");

    CVC4::context::Context context;

    InputQuery ipq;
    BoundManager bm(context);
    bm.initialize(8);
    AssignmentManager am(bm);

    PLConstraint *r1 = new OneHotConstraint({0, 1, 2});
    PLConstraint *r2 = new OneHotConstraint({3, 4, 5});
    PLConstraint *r3 = new OneHotConstraint({6, 7});
    List<PLConstraint *> constraints = {r1, r2, r3};
    for (const auto &constraint : constraints) {
      constraint->initializeCDOs(&context);
      constraint->registerBoundManager(&bm);
      constraint->registerAssignmentManager(&am);
      ipq.addPLConstraint(constraint);
    }

    TS_ASSERT_THROWS_NOTHING(r2->setActive(false));
    TS_ASSERT_THROWS_NOTHING(r2->setPhaseStatus(static_cast<PhaseStatus>(1)));

    SoIManager soiManager(ipq);
    soiManager.setAssignmentManager(&am);

    am.setAssignment(0, 0);
    am.setAssignment(1, 0);
    am.setAssignment(2, 1);

    am.setAssignment(3, 0);
    am.setAssignment(4, 1);
    am.setAssignment(5, 0);

    am.setAssignment(6, 1);
    am.setAssignment(7, 0);

    // Current phase pattern: 2, 4, 6
    TS_ASSERT_THROWS_NOTHING(soiManager.initializePhasePattern());

    {
      // Should not successfully load the cache and updated the
      // assignmentManager
      double cost = FloatUtils::negativeInfinity();
      TS_ASSERT(!soiManager.loadCachedPhasePattern(cost));
      TS_ASSERT(!FloatUtils::isFinite(cost));
      TS_ASSERT_EQUALS(am.getAssignment(0), 0);
      TS_ASSERT_EQUALS(am.getAssignment(1), 0);
      TS_ASSERT_EQUALS(am.getAssignment(2), 1);
      TS_ASSERT_EQUALS(am.getAssignment(3), 0);
      TS_ASSERT_EQUALS(am.getAssignment(4), 1);
      TS_ASSERT_EQUALS(am.getAssignment(5), 0);
      TS_ASSERT_EQUALS(am.getAssignment(6), 1);
      TS_ASSERT_EQUALS(am.getAssignment(7), 0);
    }

    // Simulate lp solving
    am.setAssignment(0, 0);
    am.setAssignment(1, 0);
    am.setAssignment(2, 1);

    am.setAssignment(3, 0);
    am.setAssignment(4, 1);
    am.setAssignment(5, 0);

    am.setAssignment(6, 0.9);
    am.setAssignment(7, 0.1);
    // Cache the current assignment and cost
    TS_ASSERT_THROWS_NOTHING(soiManager.cacheCurrentPhasePattern(0.1));

    // Current phase pattern: 2, 4, 7
    TS_ASSERT_THROWS_NOTHING(soiManager.acceptCurrentPhasePattern());
    TS_ASSERT_THROWS_NOTHING(soiManager.proposePhasePatternUpdate());

    {
      // Should not successfully load the cache and updated the
      // assignmentManager
      double cost = FloatUtils::negativeInfinity();
      TS_ASSERT(!soiManager.loadCachedPhasePattern(cost));
      TS_ASSERT(!FloatUtils::isFinite(cost));
      TS_ASSERT_EQUALS(am.getAssignment(0), 0);
      TS_ASSERT_EQUALS(am.getAssignment(1), 0);
      TS_ASSERT_EQUALS(am.getAssignment(2), 1);
      TS_ASSERT_EQUALS(am.getAssignment(3), 0);
      TS_ASSERT_EQUALS(am.getAssignment(4), 1);
      TS_ASSERT_EQUALS(am.getAssignment(5), 0);
      TS_ASSERT_EQUALS(am.getAssignment(6), 0.9);
      TS_ASSERT_EQUALS(am.getAssignment(7), 0.1);
    }

    am.setAssignment(0, 0);
    am.setAssignment(1, 0);
    am.setAssignment(2, 1);

    am.setAssignment(3, 0);
    am.setAssignment(4, 1);
    am.setAssignment(5, 0);

    am.setAssignment(6, 0.4);
    am.setAssignment(7, 0.6);
    // Cache the current assignment and cost
    TS_ASSERT_THROWS_NOTHING(soiManager.cacheCurrentPhasePattern(0.4));

    // Current phase pattern: 2, 4, 6
    TS_ASSERT_THROWS_NOTHING(soiManager.acceptCurrentPhasePattern());
    TS_ASSERT_THROWS_NOTHING(soiManager.proposePhasePatternUpdate());

    {
      // Should have successfully load the cache and updated the
      // assignmentManager
      double cost;
      TS_ASSERT(soiManager.loadCachedPhasePattern(cost));
      TS_ASSERT_EQUALS(cost, 0.1);
      TS_ASSERT_EQUALS(am.getAssignment(0), 0);
      TS_ASSERT_EQUALS(am.getAssignment(1), 0);
      TS_ASSERT_EQUALS(am.getAssignment(2), 1);
      TS_ASSERT_EQUALS(am.getAssignment(3), 0);
      TS_ASSERT_EQUALS(am.getAssignment(4), 1);
      TS_ASSERT_EQUALS(am.getAssignment(5), 0);
      TS_ASSERT_EQUALS(am.getAssignment(6), 0.9);
      TS_ASSERT_EQUALS(am.getAssignment(7), 0.1);
    }

    // Current phase pattern: 2, 4, 7
    TS_ASSERT_THROWS_NOTHING(soiManager.acceptCurrentPhasePattern());
    TS_ASSERT_THROWS_NOTHING(soiManager.proposePhasePatternUpdate());
    {
      // Should have successfully load the cache and updated the
      // assignmentManager
      double cost;
      TS_ASSERT(soiManager.loadCachedPhasePattern(cost));
      TS_ASSERT_EQUALS(cost, 0.4);
      TS_ASSERT_EQUALS(am.getAssignment(0), 0);
      TS_ASSERT_EQUALS(am.getAssignment(1), 0);
      TS_ASSERT_EQUALS(am.getAssignment(2), 1);
      TS_ASSERT_EQUALS(am.getAssignment(3), 0);
      TS_ASSERT_EQUALS(am.getAssignment(4), 1);
      TS_ASSERT_EQUALS(am.getAssignment(5), 0);
      TS_ASSERT_EQUALS(am.getAssignment(6), 0.4);
      TS_ASSERT_EQUALS(am.getAssignment(7), 0.6);
    }
  }
};
