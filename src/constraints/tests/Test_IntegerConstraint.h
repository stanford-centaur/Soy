/*********************                                                        */
/*! \file Test_IntegerConstraint.h
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

#include <cxxtest/TestSuite.h>

#include "AssignmentManager.h"
#include "BoundManager.h"
#include "CadicalWrapper.h"
#include "GurobiWrapper.h"
#include "IntegerConstraint.h"
#include "MockErrno.h"
#include "Statistics.h"
#include "context/context.h"

class IntegerConstraintTestSuite : public CxxTest::TestSuite {
 public:
  MockErrno *mockErrno;

  void setUp() { TS_ASSERT(mockErrno = new MockErrno); }

  void tearDown() { TS_ASSERT_THROWS_NOTHING(delete mockErrno); }

  void test_integer_constraint() {
    IntegerConstraint *integer = new IntegerConstraint(3);
    TS_ASSERT_EQUALS(integer->getType(), INTEGER);
    TS_ASSERT_EQUALS(integer->getVariable(), 3u);
    TS_ASSERT_EQUALS(integer->getParticipatingVariables().size(), 1u);
    TS_ASSERT_EQUALS(*integer->getParticipatingVariables().begin(), 3u);
    TS_ASSERT(!integer->participatingVariable(0));
    TS_ASSERT(integer->participatingVariable(3));

    delete integer;
  }

  void test_add_boolean_structure() {}

  void test_duplicate_constraint() {
    IntegerConstraint *integer = new IntegerConstraint(3);

    IntegerConstraint *duplicate =
        static_cast<IntegerConstraint *>(integer->duplicateConstraint());

    TS_ASSERT_EQUALS(integer->getVariable(), duplicate->getVariable());

    delete integer;
    delete duplicate;
  }

  void test_notify_bounds_preprocessing() {
    // No context, no bound manager
    {
      IntegerConstraint *integer = new IntegerConstraint(3);
      List<Tightening> tightenings;
      TS_ASSERT_THROWS_NOTHING(integer->getEntailedTightenings(tightenings));
      TS_ASSERT_EQUALS(tightenings.size(), 0u);

      integer->notifyLowerBound(3, 0);
      integer->notifyUpperBound(3, 0);
      TS_ASSERT_THROWS_NOTHING(integer->getEntailedTightenings(tightenings));
      TS_ASSERT(tightenings.exists(Tightening(3, 0, Tightening::LB)));
      TS_ASSERT(tightenings.exists(Tightening(3, 0, Tightening::UB)));
      delete integer;
    }

    {
      IntegerConstraint *integer = new IntegerConstraint(3);
      List<Tightening> tightenings;
      TS_ASSERT_THROWS_NOTHING(integer->getEntailedTightenings(tightenings));
      TS_ASSERT_EQUALS(tightenings.size(), 0u);

      integer->notifyLowerBound(3, 0);
      integer->notifyUpperBound(3, 4);
      TS_ASSERT_THROWS_NOTHING(integer->getEntailedTightenings(tightenings));
      TS_ASSERT(tightenings.exists(Tightening(3, 0, Tightening::LB)));
      TS_ASSERT(tightenings.exists(Tightening(3, 4, Tightening::UB)));

      integer->notifyUpperBound(3, 3.3);
      tightenings.clear();
      TS_ASSERT_THROWS_NOTHING(integer->getEntailedTightenings(tightenings));
      TS_ASSERT(tightenings.exists(Tightening(3, 0, Tightening::LB)));
      TS_ASSERT(tightenings.exists(Tightening(3, 3, Tightening::UB)));

      integer->notifyUpperBound(3, 3);
      tightenings.clear();
      TS_ASSERT_THROWS_NOTHING(integer->getEntailedTightenings(tightenings));
      TS_ASSERT(tightenings.exists(Tightening(3, 0, Tightening::LB)));
      TS_ASSERT(tightenings.exists(Tightening(3, 3, Tightening::UB)));

      integer->notifyLowerBound(3, 0.1);
      tightenings.clear();
      TS_ASSERT_THROWS_NOTHING(integer->getEntailedTightenings(tightenings));
      TS_ASSERT(tightenings.exists(Tightening(3, 1, Tightening::LB)));
      TS_ASSERT(tightenings.exists(Tightening(3, 3, Tightening::UB)));

      integer->notifyLowerBound(3, 2);
      tightenings.clear();
      TS_ASSERT_THROWS_NOTHING(integer->getEntailedTightenings(tightenings));
      TS_ASSERT(tightenings.exists(Tightening(3, 2, Tightening::LB)));
      TS_ASSERT(tightenings.exists(Tightening(3, 3, Tightening::UB)));

      integer->notifyLowerBound(3, 2.9);
      tightenings.clear();
      TS_ASSERT_THROWS_NOTHING(integer->getEntailedTightenings(tightenings));
      TS_ASSERT(tightenings.exists(Tightening(3, 3, Tightening::LB)));
      TS_ASSERT(tightenings.exists(Tightening(3, 3, Tightening::UB)));
      delete integer;
    }
  }

  void test_notify_bounds_during_search() {
    {
      CVC4::context::Context context;
      BoundManager bm(context);
      bm.initialize(4);
      bm.setLowerBound(3, 1);
      bm.setUpperBound(3, 5);

      IntegerConstraint *integer = new IntegerConstraint(3);
      TS_ASSERT_THROWS_NOTHING(integer->initializeCDOs(&context));
      integer->registerBoundManager(&bm);
      TS_ASSERT(integer->hasFeasiblePhases());
      TS_ASSERT(!integer->phaseFixed());

      context.push();
      integer->notifyLowerBound(3, 0);
      integer->notifyUpperBound(3, 4);

      integer->notifyUpperBound(3, 3.3);
      TS_ASSERT_EQUALS(bm.getUpperBound(3), 3);

      integer->notifyLowerBound(3, 0.1);
      TS_ASSERT_EQUALS(bm.getLowerBound(3), 1);

      TS_ASSERT(integer->hasFeasiblePhases());
      TS_ASSERT_EQUALS(integer->numberOfFeasiblePhases(), 3u);

      context.push();
      integer->notifyLowerBound(3, 2.1);
      TS_ASSERT_EQUALS(bm.getLowerBound(3), 3);
      TS_ASSERT_EQUALS(bm.getUpperBound(3), 3);
      TS_ASSERT(integer->hasFeasiblePhases());
      TS_ASSERT_EQUALS(integer->numberOfFeasiblePhases(), 1u);

      PiecewiseLinearCaseSplit split;
      split.storeBoundTightening(Tightening(3, 3, Tightening::LB));
      split.storeBoundTightening(Tightening(3, 3, Tightening::UB));
      TS_ASSERT_EQUALS(integer->getValidCaseSplit(), split);

      context.pop();
      integer->notifyUpperBound(3, 1.8);
      TS_ASSERT_EQUALS(bm.getLowerBound(3), 1);
      TS_ASSERT_EQUALS(bm.getUpperBound(3), 1);
      TS_ASSERT(integer->hasFeasiblePhases());
      TS_ASSERT_EQUALS(integer->numberOfFeasiblePhases(), 1u);

      PiecewiseLinearCaseSplit split2;
      split2.storeBoundTightening(Tightening(3, 1, Tightening::LB));
      split2.storeBoundTightening(Tightening(3, 1, Tightening::UB));
      TS_ASSERT_EQUALS(integer->getValidCaseSplit(), split2);

      delete integer;
      return;
    }
  }

  void test_case_splits() {
    IntegerConstraint *integer = new IntegerConstraint(3);
    TS_ASSERT(!integer->supportCaseSplit());
    delete integer;
  }

  void test_satisfied() {
    CVC4::context::Context context;
    BoundManager bm(context);
    AssignmentManager am(bm);
    IntegerConstraint *integer = new IntegerConstraint(3);
    integer->registerAssignmentManager(&am);

    bm.initialize(4);
    am.initialize();

    am.setAssignment(3, 1.3);
    TS_ASSERT(!integer->satisfied());

    am.setAssignment(3, 2);
    TS_ASSERT(integer->satisfied());

    am.setAssignment(3, 9.99999999999999);
    TS_ASSERT(integer->satisfied());

    delete integer;
  }

  void test_soi() {
    IntegerConstraint *integer = new IntegerConstraint(3);
    TS_ASSERT(!integer->supportSoI());

    delete integer;
  }
};
