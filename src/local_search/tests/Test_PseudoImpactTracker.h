/*********************                                                        */
/*! \file Test_PseudoCostTracker.h
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

#include "OneHotConstraint.h"
#include "PseudoImpactTracker.h"
#include "context/context.h"

class PseudoImpactTrackerTestSuite : public CxxTest::TestSuite {
 public:
  PLConstraintScoreTracker *_tracker;
  List<PLConstraint *> _constraints;

  void setUp() { _tracker = new PseudoImpactTracker(); }

  void tearDown() {
    delete _tracker;
    for (const auto &ele : _constraints) delete ele;
  }

  void test_updateScore() {
    CVC4::context::Context context;
    PLConstraint *r1 = new OneHotConstraint({0, 1});
    PLConstraint *r2 = new OneHotConstraint({2, 3});
    PLConstraint *r3 = new OneHotConstraint({4, 5});
    PLConstraint *r4 = new OneHotConstraint({6, 7});
    _constraints = {r1, r2, r3, r4};
    for (const auto &constraint : _constraints)
      constraint->initializeCDOs(&context);

    TS_ASSERT_THROWS_NOTHING(_tracker->initialize(_constraints));
    TS_ASSERT_THROWS_NOTHING(_tracker->updateScore(r1, 2));
    TS_ASSERT_THROWS_NOTHING(_tracker->updateScore(r2, 4));
    TS_ASSERT_THROWS_NOTHING(_tracker->updateScore(r3, 5));
    TS_ASSERT_THROWS_NOTHING(_tracker->updateScore(r3, 6));
    TS_ASSERT_THROWS_NOTHING(_tracker->updateScore(r4, 3));

    double alpha = GlobalConfiguration::EXPONENTIAL_MOVING_AVERAGE_ALPHA_BRANCH;
    TS_ASSERT_EQUALS(_tracker->getScore(r1), alpha * 2);
    TS_ASSERT_EQUALS(_tracker->getScore(r3),
                     (1 - alpha) * (alpha * 5) + alpha * 6);

    r3->setActive(false);
    r1->setActive(false);
    TS_ASSERT(_tracker->top() == r3);
    TS_ASSERT(_tracker->topUnfixed() == r2);
    r3->setActive(true);
    TS_ASSERT_THROWS_NOTHING(_tracker->updateScore(r3, 5));
    TS_ASSERT(_tracker->topUnfixed() == r3);
    r2->setActive(false);
    TS_ASSERT(_tracker->topUnfixed() == r3);
  }

  void test_set_score() {
    CVC4::context::Context context;
    PLConstraint *r1 = new OneHotConstraint({0, 1});
    PLConstraint *r2 = new OneHotConstraint({2, 3});
    PLConstraint *r3 = new OneHotConstraint({4, 5});
    PLConstraint *r4 = new OneHotConstraint({6, 7});
    _constraints = {r1, r2, r3, r4};
    for (const auto &constraint : _constraints)
      constraint->initializeCDOs(&context);

    TS_ASSERT_THROWS_NOTHING(_tracker->initialize(_constraints));
    TS_ASSERT_THROWS_NOTHING(_tracker->setScore(r1, 2));
    TS_ASSERT_THROWS_NOTHING(_tracker->setScore(r2, 4));
    TS_ASSERT_THROWS_NOTHING(_tracker->setScore(r3, 5));
    TS_ASSERT_THROWS_NOTHING(_tracker->setScore(r3, 6));

    TS_ASSERT_EQUALS(_tracker->getScore(r1), 2);
    TS_ASSERT_EQUALS(_tracker->getScore(r3), 6);

    r3->setActive(false);
    r1->setActive(false);
    TS_ASSERT(_tracker->top() == r3);
    TS_ASSERT(_tracker->topUnfixed() == r2);
  }
};
