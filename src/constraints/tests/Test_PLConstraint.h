/*********************                                                        */
/*! \file Test_PLConstraint.h
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

#include "BoundManager.h"
#include "CadicalWrapper.h"
#include "GurobiWrapper.h"
#include "MockConstraint.h"
#include "MockErrno.h"
#include "Statistics.h"
#include "context/context.h"

class PLConstraintTestSuite : public CxxTest::TestSuite {
 public:
  MockErrno *mockErrno;

  void setUp() { TS_ASSERT(mockErrno = new MockErrno); }

  void tearDown() { TS_ASSERT_THROWS_NOTHING(delete mockErrno); }

  void test_initializeCDOs() {
    MockConstraint *mock = new MockConstraint(4);
    PhaseStatus phase1 = static_cast<PhaseStatus>(0);
    PhaseStatus phase2 = static_cast<PhaseStatus>(1);

    // Four cases
    List<PhaseStatus> cases = mock->getAllCases();
    TS_ASSERT_EQUALS(cases.size(), 4u);
    CVC4::context::Context context;
    TS_ASSERT_THROWS_NOTHING(mock->initializeCDOs(&context));
    TS_ASSERT_EQUALS(mock->numberOfFeasiblePhases(), 4u);
    TS_ASSERT(mock->hasFeasiblePhases());
    TS_ASSERT(!mock->phaseFixed());
    TS_ASSERT(mock->isActive());
    TS_ASSERT_EQUALS(mock->getNextFeasibleCase(), phase1);
    TS_ASSERT(mock->isFeasible(phase1));

    context.push();
    TS_ASSERT_THROWS_NOTHING(mock->setPhaseStatus(phase1));
    TS_ASSERT_EQUALS(mock->numberOfFeasiblePhases(), 1u);
    TS_ASSERT(mock->hasFeasiblePhases());
    TS_ASSERT(mock->phaseFixed());
    TS_ASSERT_THROWS_NOTHING(mock->setActive(false));

    // No feasible case left.
    TS_ASSERT_THROWS_NOTHING(mock->markInfeasiblePhase(phase1));

    TS_ASSERT_EQUALS(mock->numberOfFeasiblePhases(), 0u);
    TS_ASSERT(!mock->hasFeasiblePhases());
    TS_ASSERT(!mock->phaseFixed());
    TS_ASSERT(!mock->isActive());
    TS_ASSERT_THROWS_ANYTHING(mock->getNextFeasibleCase());
    TS_ASSERT(!mock->isFeasible(phase1));

    context.pop();
    TS_ASSERT_EQUALS(mock->getNextFeasibleCase(), phase1);
    TS_ASSERT_EQUALS(mock->numberOfFeasiblePhases(), 4u);
    TS_ASSERT(mock->hasFeasiblePhases());
    TS_ASSERT(!mock->phaseFixed());
    TS_ASSERT(mock->isActive());

    TS_ASSERT_THROWS_NOTHING(mock->markInfeasiblePhase(phase1));
    TS_ASSERT_EQUALS(mock->numberOfFeasiblePhases(), 3u);
    TS_ASSERT(mock->hasFeasiblePhases());
    TS_ASSERT(!mock->phaseFixed());
    TS_ASSERT_EQUALS(mock->getNextFeasibleCase(), phase2);

    delete mock;
  }

  void test_directionHeuristics() {
    MockConstraint *mock = new MockConstraint(4);
    CVC4::context::Context context;
    TS_ASSERT_THROWS_NOTHING(mock->initializeCDOs(&context));

    PhaseStatus phase1 = static_cast<PhaseStatus>(0);
    PhaseStatus phase2 = static_cast<PhaseStatus>(1);

    TS_ASSERT_EQUALS(mock->getNextFeasibleCase(), phase1);
    mock->updatePhaseStatusScore(phase2, 1);
    mock->updatePhaseStatusScore(phase1, 0.5);
    TS_ASSERT_EQUALS(mock->getNextFeasibleCase(), phase2);
    TS_ASSERT_THROWS_NOTHING(mock->markInfeasiblePhase(phase2));
    TS_ASSERT_EQUALS(mock->getNextFeasibleCase(), phase1);
    delete mock;
  }
};
