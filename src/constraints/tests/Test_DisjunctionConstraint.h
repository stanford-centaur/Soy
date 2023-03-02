/*********************                                                        */
/*! \file Test_DisjunctionConstraint.h
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
#include "DisjunctionConstraint.h"
#include "GurobiWrapper.h"
#include "MockErrno.h"
#include "Statistics.h"
#include "context/context.h"

class DisjunctionConstraintTestSuite : public CxxTest::TestSuite {
 public:
  MockErrno *mockErrno;
  PhaseStatus phase1;
  PhaseStatus phase2;
  PhaseStatus phase3;
  PhaseStatus phase4;
  PhaseStatus phase5;

  void setUp() {
    TS_ASSERT(mockErrno = new MockErrno);
    phase1 = static_cast<PhaseStatus>(0);
    phase2 = static_cast<PhaseStatus>(1);
    phase3 = static_cast<PhaseStatus>(2);
    phase4 = static_cast<PhaseStatus>(3);
    phase5 = static_cast<PhaseStatus>(4);
  }

  void tearDown() { TS_ASSERT_THROWS_NOTHING(delete mockErrno); }

  void test_onehot_constraint() {
    Set<unsigned> elements = {0, 1, 3, 5};
    DisjunctionConstraint *disj = new DisjunctionConstraint(elements);

    TS_ASSERT_EQUALS(disj->getType(), DISJUNCT);

    List<unsigned> vars = {0, 1, 3, 5};
    TS_ASSERT_EQUALS(disj->getParticipatingVariables(), vars);
    TS_ASSERT(disj->participatingVariable(0));
    TS_ASSERT(disj->participatingVariable(1));
    TS_ASSERT(!disj->participatingVariable(2));
    TS_ASSERT(disj->participatingVariable(3));
    TS_ASSERT(!disj->participatingVariable(4));

    TS_ASSERT_EQUALS(disj->getElementOfPhase(phase1), 0u);
    TS_ASSERT_EQUALS(disj->getElementOfPhase(phase2), 1u);
    TS_ASSERT_EQUALS(disj->getElementOfPhase(phase3), 3u);
    TS_ASSERT_EQUALS(disj->getElementOfPhase(phase4), 5u);
    TS_ASSERT_THROWS_ANYTHING(disj->getElementOfPhase(phase5));

    TS_ASSERT_EQUALS(disj->getPhaseOfElement(0), phase1);
    TS_ASSERT_EQUALS(disj->getPhaseOfElement(1), phase2);
    TS_ASSERT_THROWS_ANYTHING(disj->getPhaseOfElement(2));
    TS_ASSERT_EQUALS(disj->getPhaseOfElement(3), phase3);
    TS_ASSERT_EQUALS(disj->getPhaseOfElement(5), phase4);

    delete disj;
  }

  void test_add_boolean_structure() {
    Set<unsigned> elements = {0, 1, 3, 5};
    DisjunctionConstraint *disj = new DisjunctionConstraint(elements);

    CadicalWrapper *cadical = new CadicalWrapper();
    disj->registerSatSolver(cadical);
    TS_ASSERT_THROWS_NOTHING(disj->addBooleanStructure());

    cadical->assumeLiteral(disj->getLiteralOfPhaseStatus(phase1));
    TS_ASSERT_THROWS_NOTHING(cadical->solve());
    TS_ASSERT(cadical->getAssignment(disj->getLiteralOfPhaseStatus(phase1)));

    delete disj;
    delete cadical;
  }

  void test_duplicate_constraint() {
    Set<unsigned> elements = {0, 1, 3, 5};
    DisjunctionConstraint *disj = new DisjunctionConstraint(elements);

    DisjunctionConstraint *duplicate =
        static_cast<DisjunctionConstraint *>(disj->duplicateConstraint());

    TS_ASSERT_EQUALS(disj->getElements(), duplicate->getElements());
    TS_ASSERT_EQUALS(disj->getAllCases(), duplicate->getAllCases());

    delete disj;
    delete duplicate;
  }

  void test_notify_bounds_preprocessing() {
    // No context, no bound manager
    {
      Set<unsigned> elements = {0, 1, 3, 5};
      DisjunctionConstraint *disj = new DisjunctionConstraint(elements);
      List<Tightening> tightenings;
      TS_ASSERT_THROWS_NOTHING(disj->getEntailedTightenings(tightenings));
      TS_ASSERT_EQUALS(tightenings.size(), 0u);

      disj->notifyLowerBound(0, 0);
      disj->notifyLowerBound(1, 0);
      disj->notifyLowerBound(3, 0);
      disj->notifyLowerBound(5, 0);
      disj->notifyUpperBound(0, 1);
      disj->notifyUpperBound(1, 1);
      disj->notifyUpperBound(3, 1);
      disj->notifyUpperBound(5, 1);
      // No element eliminated yet.
      TS_ASSERT_EQUALS(disj->getElements(), elements);
      TS_ASSERT_THROWS_NOTHING(disj->getEntailedTightenings(tightenings));
      for (const auto &e : elements) {
        TS_ASSERT(tightenings.exists(Tightening(e, 0, Tightening::LB)));
        TS_ASSERT(tightenings.exists(Tightening(e, 1, Tightening::UB)));
      }
      delete disj;
    }

    {
      Set<unsigned> elements = {0, 1, 3, 5};
      DisjunctionConstraint *disj = new DisjunctionConstraint(elements);
      disj->notifyLowerBound(0, 0);
      disj->notifyLowerBound(1, 0);
      disj->notifyLowerBound(3, 0);
      disj->notifyLowerBound(5, 0);
      disj->notifyUpperBound(0, 1);
      disj->notifyUpperBound(1, 1);
      disj->notifyUpperBound(3, 1);
      disj->notifyUpperBound(5, 1);
      disj->notifyLowerBound(5, 0.1);
      // All but one element eliminated
      Set<unsigned> elementsLeft = {5};
      TS_ASSERT_EQUALS(disj->getElements(), elementsLeft);

      List<Tightening> tightenings;
      TS_ASSERT_THROWS_NOTHING(disj->getEntailedTightenings(tightenings));
      TS_ASSERT(tightenings.exists(Tightening(0, 0, Tightening::LB)));
      TS_ASSERT(tightenings.exists(Tightening(0, 0, Tightening::UB)));
      TS_ASSERT(tightenings.exists(Tightening(1, 0, Tightening::LB)));
      TS_ASSERT(tightenings.exists(Tightening(1, 0, Tightening::UB)));
      TS_ASSERT(tightenings.exists(Tightening(3, 0, Tightening::LB)));
      TS_ASSERT(tightenings.exists(Tightening(3, 0, Tightening::UB)));
      TS_ASSERT(tightenings.exists(Tightening(5, 1, Tightening::LB)));
      TS_ASSERT(tightenings.exists(Tightening(5, 1, Tightening::UB)));
      delete disj;
    }

    {
      Set<unsigned> elements = {0, 1, 3, 5};
      DisjunctionConstraint *disj = new DisjunctionConstraint(elements);
      disj->notifyLowerBound(0, 0);
      disj->notifyLowerBound(1, 0);
      disj->notifyLowerBound(3, 0);
      disj->notifyLowerBound(5, 0);
      disj->notifyUpperBound(0, 1);
      disj->notifyUpperBound(1, 1);
      disj->notifyUpperBound(3, 1);
      disj->notifyUpperBound(5, 1);

      disj->notifyLowerBound(5, 1);
      Set<unsigned> elementsLeft = {5};
      TS_ASSERT_EQUALS(disj->getElements(), elementsLeft);

      List<Tightening> tightenings;
      TS_ASSERT_THROWS_NOTHING(disj->getEntailedTightenings(tightenings));
      TS_ASSERT(tightenings.exists(Tightening(0, 0, Tightening::LB)));
      TS_ASSERT(tightenings.exists(Tightening(0, 0, Tightening::UB)));
      TS_ASSERT(tightenings.exists(Tightening(1, 0, Tightening::LB)));
      TS_ASSERT(tightenings.exists(Tightening(1, 0, Tightening::UB)));
      TS_ASSERT(tightenings.exists(Tightening(3, 0, Tightening::LB)));
      TS_ASSERT(tightenings.exists(Tightening(3, 0, Tightening::UB)));
      TS_ASSERT(tightenings.exists(Tightening(5, 1, Tightening::LB)));
      TS_ASSERT(tightenings.exists(Tightening(5, 1, Tightening::UB)));
      delete disj;
    }

    {
      Set<unsigned> elements = {0, 1, 3, 5};
      DisjunctionConstraint *disj = new DisjunctionConstraint(elements);
      disj->notifyLowerBound(0, 0);
      disj->notifyLowerBound(1, 0);
      disj->notifyLowerBound(3, 0);
      disj->notifyLowerBound(5, 0);
      disj->notifyUpperBound(0, 1);
      disj->notifyUpperBound(1, 1);
      disj->notifyUpperBound(3, 1);
      disj->notifyUpperBound(5, 1);
      disj->notifyUpperBound(5, 0.5);
      Set<unsigned> elementsLeft = {0, 1, 3};
      TS_ASSERT_EQUALS(disj->getElements(), elementsLeft);

      List<Tightening> tightenings;
      TS_ASSERT_THROWS_NOTHING(disj->getEntailedTightenings(tightenings));
      TS_ASSERT(tightenings.exists(Tightening(0, 0, Tightening::LB)));
      TS_ASSERT(tightenings.exists(Tightening(0, 1, Tightening::UB)));
      TS_ASSERT(tightenings.exists(Tightening(1, 0, Tightening::LB)));
      TS_ASSERT(tightenings.exists(Tightening(1, 1, Tightening::UB)));
      TS_ASSERT(tightenings.exists(Tightening(3, 0, Tightening::LB)));
      TS_ASSERT(tightenings.exists(Tightening(3, 1, Tightening::UB)));
      TS_ASSERT(tightenings.exists(Tightening(5, 0, Tightening::LB)));
      TS_ASSERT(tightenings.exists(Tightening(5, 0, Tightening::UB)));
      delete disj;
    }

    {
      Set<unsigned> elements = {0, 1, 3, 5};
      DisjunctionConstraint *disj = new DisjunctionConstraint(elements);
      disj->notifyLowerBound(0, 0);
      disj->notifyLowerBound(1, 0);
      disj->notifyLowerBound(3, 0);
      disj->notifyLowerBound(5, 0);
      disj->notifyUpperBound(0, 1);
      disj->notifyUpperBound(1, 1);
      disj->notifyUpperBound(3, 1);
      disj->notifyUpperBound(5, 1);
      disj->notifyUpperBound(5, 0);
      Set<unsigned> elementsLeft = {0, 1, 3};
      TS_ASSERT_EQUALS(disj->getElements(), elementsLeft);

      List<Tightening> tightenings;
      TS_ASSERT_THROWS_NOTHING(disj->getEntailedTightenings(tightenings));
      TS_ASSERT(tightenings.exists(Tightening(0, 0, Tightening::LB)));
      TS_ASSERT(tightenings.exists(Tightening(0, 1, Tightening::UB)));
      TS_ASSERT(tightenings.exists(Tightening(1, 0, Tightening::LB)));
      TS_ASSERT(tightenings.exists(Tightening(1, 1, Tightening::UB)));
      TS_ASSERT(tightenings.exists(Tightening(3, 0, Tightening::LB)));
      TS_ASSERT(tightenings.exists(Tightening(3, 1, Tightening::UB)));
      TS_ASSERT(tightenings.exists(Tightening(5, 0, Tightening::LB)));
      TS_ASSERT(tightenings.exists(Tightening(5, 0, Tightening::UB)));

      CVC4::context::Context context;
      disj->initializeCDOs(&context);
      TS_ASSERT_EQUALS(disj->getAllCases().size(), 3u);
      TS_ASSERT_EQUALS(disj->numberOfFeasiblePhases(), 3u);
      delete disj;
    }
  }

  void test_notify_bounds_during_search() {
    // No context, no bound manager
    {
      CVC4::context::Context context;
      BoundManager bm(context);
      bm.initialize(6);
      for (unsigned i = 0; i < 6; ++i) {
        bm.setLowerBound(i, 0);
        bm.setUpperBound(i, 1);
      }
      Set<unsigned> elements = {0, 1, 3, 5};
      DisjunctionConstraint *disj = new DisjunctionConstraint(elements);
      disj->initializeCDOs(&context);
      disj->registerBoundManager(&bm);

      TS_ASSERT_EQUALS(disj->getElements(), elements);
      TS_ASSERT_EQUALS(disj->getAllCases().size(), 4u);
      TS_ASSERT(disj->hasFeasiblePhases());
      TS_ASSERT(!disj->phaseFixed());

      context.push();
      disj->notifyLowerBound(0, 0.1);

      TS_ASSERT_EQUALS(disj->getElements(), elements);
      TS_ASSERT_EQUALS(disj->getAllCases().size(), 4u);
      TS_ASSERT_EQUALS(disj->numberOfFeasiblePhases(), 1u);
      TS_ASSERT(disj->hasFeasiblePhases());
      TS_ASSERT(disj->phaseFixed());
      TS_ASSERT_EQUALS(disj->getValidCaseSplit(), disj->getCaseSplit(phase1));

      context.pop();

      TS_ASSERT_EQUALS(disj->getElements(), elements);
      TS_ASSERT_EQUALS(disj->getAllCases().size(), 4u);
      TS_ASSERT_EQUALS(disj->numberOfFeasiblePhases(), 4u);
      TS_ASSERT(disj->hasFeasiblePhases());
      TS_ASSERT(!disj->phaseFixed());
      TS_ASSERT_EQUALS(disj->getValidCaseSplit(), PiecewiseLinearCaseSplit());

      delete disj;
    }

    {
      CVC4::context::Context context;
      BoundManager bm(context);
      bm.initialize(6);
      for (unsigned i = 0; i < 6; ++i) {
        bm.setLowerBound(i, 0);
        bm.setUpperBound(i, 1);
      }
      Set<unsigned> elements = {0, 1, 3, 5};
      DisjunctionConstraint *disj = new DisjunctionConstraint(elements);
      disj->initializeCDOs(&context);
      disj->registerBoundManager(&bm);

      TS_ASSERT_EQUALS(disj->getElements(), elements);
      TS_ASSERT_EQUALS(disj->getAllCases().size(), 4u);
      TS_ASSERT(disj->hasFeasiblePhases());
      TS_ASSERT(!disj->phaseFixed());

      context.push();
      disj->notifyUpperBound(0, 0.5);

      TS_ASSERT_EQUALS(disj->getElements(), elements);
      TS_ASSERT_EQUALS(disj->getAllCases().size(), 4u);
      TS_ASSERT_EQUALS(disj->numberOfFeasiblePhases(), 3u);
      TS_ASSERT(disj->hasFeasiblePhases());
      TS_ASSERT(!disj->phaseFixed());
      TS_ASSERT(!disj->isFeasible(phase1));
      context.pop();

      TS_ASSERT_EQUALS(disj->getElements(), elements);
      TS_ASSERT_EQUALS(disj->getAllCases().size(), 4u);
      TS_ASSERT_EQUALS(disj->numberOfFeasiblePhases(), 4u);
      TS_ASSERT(disj->hasFeasiblePhases());
      TS_ASSERT(!disj->phaseFixed());
      TS_ASSERT_EQUALS(disj->getValidCaseSplit(), PiecewiseLinearCaseSplit());
      delete disj;
    }
  }

  void test_case_splits() {
    Set<unsigned> elements = {0, 1, 3, 5};
    DisjunctionConstraint *disj = new DisjunctionConstraint(elements);
    CVC4::context::Context context;
    TS_ASSERT_THROWS_NOTHING(disj->initializeCDOs(&context));

    PiecewiseLinearCaseSplit split1;
    split1.storeBoundTightening(Tightening(0, 1, Tightening::LB));
    split1.storeBoundTightening(Tightening(1, 0, Tightening::UB));
    split1.storeBoundTightening(Tightening(3, 0, Tightening::UB));
    split1.storeBoundTightening(Tightening(5, 0, Tightening::UB));

    TS_ASSERT_EQUALS(split1, disj->getCaseSplit(phase1));

    context.push();
    TS_ASSERT_THROWS_NOTHING(disj->setPhaseStatus(phase1));
    TS_ASSERT(disj->phaseFixed());
    TS_ASSERT_EQUALS(disj->getValidCaseSplit(), split1);

    context.pop();
    TS_ASSERT(!disj->phaseFixed());
    TS_ASSERT_EQUALS(disj->getValidCaseSplit(), PiecewiseLinearCaseSplit());

    delete disj;
  }

  void test_satisfied() {
    CVC4::context::Context context;
    BoundManager bm(context);
    AssignmentManager am(bm);
    Set<unsigned> elements = {0, 1, 3, 5};
    DisjunctionConstraint *disj = new DisjunctionConstraint(elements);
    disj->registerAssignmentManager(&am);

    bm.initialize(6);
    am.initialize();

    am.setAssignment(0, 1);
    am.setAssignment(1, 1);
    am.setAssignment(3, 1);
    am.setAssignment(5, 1);
    TS_ASSERT(disj->satisfied());

    am.setAssignment(0, 0);
    am.setAssignment(1, 1);
    am.setAssignment(3, 1);
    am.setAssignment(5, 1);
    TS_ASSERT(disj->satisfied());

    am.setAssignment(0, 0);
    am.setAssignment(1, 0);
    am.setAssignment(3, 0);
    am.setAssignment(5, 1);
    TS_ASSERT(disj->satisfied());

    am.setAssignment(0, 0);
    am.setAssignment(1, 0);
    am.setAssignment(3, 0);
    am.setAssignment(5, 0);
    TS_ASSERT(!disj->satisfied());

    am.setAssignment(0, 0.0001);
    am.setAssignment(1, 0.0001);
    am.setAssignment(3, 0.0001);
    am.setAssignment(5, 0.9999);
    TS_ASSERT(!disj->satisfied());

    am.setAssignment(0, 0.001);
    am.setAssignment(1, 0.001);
    am.setAssignment(3, 0.001);
    am.setAssignment(5, 1);
    TS_ASSERT(disj->satisfied());

    am.setAssignment(0, -0.001);
    am.setAssignment(1, 0.001);
    am.setAssignment(3, 0.001);
    am.setAssignment(5, 1);
    TS_ASSERT(!disj->satisfied());
    delete disj;
  }

  void test_soi() {
    Set<unsigned> elements = {0, 1, 3, 5};
    DisjunctionConstraint *disj = new DisjunctionConstraint(elements);
    TS_ASSERT(disj->supportSoI());

    CVC4::context::Context context;
    TS_ASSERT_THROWS_NOTHING(disj->initializeCDOs(&context));

    LinearExpression cost;
    LinearExpression costResult;
    TS_ASSERT_THROWS_NOTHING(disj->getCostFunctionComponent(cost, phase1));
    costResult._constant = 1;
    costResult._addends[0] = -1;
    TS_ASSERT_EQUALS(cost, costResult);

    TS_ASSERT_THROWS_NOTHING(disj->getCostFunctionComponent(cost, phase1));
    costResult._constant = 2;
    costResult._addends[0] = -2;
    TS_ASSERT_EQUALS(cost, costResult);

    TS_ASSERT_THROWS_NOTHING(disj->getCostFunctionComponent(cost, phase4));
    costResult._constant = 3;
    costResult._addends[5] = -1;
    TS_ASSERT_EQUALS(cost, costResult);

    context.push();
    // If the phase is infeasible, do not add to the cost.
    TS_ASSERT_THROWS_NOTHING(disj->markInfeasiblePhase(phase3));
    TS_ASSERT_THROWS_NOTHING(disj->getCostFunctionComponent(cost, phase3));
    TS_ASSERT_EQUALS(cost, costResult);

    context.pop();
    TS_ASSERT_THROWS_NOTHING(disj->getCostFunctionComponent(cost, phase3));
    costResult._constant = 4;
    costResult._addends[3] = -1;
    TS_ASSERT_EQUALS(cost, costResult);

    BoundManager bm(context);
    bm.initialize(6);
    bm.setLowerBound(0, 0);
    bm.setUpperBound(0, 1);
    bm.setLowerBound(1, 0);
    bm.setUpperBound(1, 1);
    bm.setLowerBound(3, 0);
    bm.setUpperBound(3, 1);
    bm.setLowerBound(5, 0);
    bm.setUpperBound(5, 1);

    AssignmentManager assignmentManager(bm);
    disj->registerAssignmentManager(&assignmentManager);

    assignmentManager.setAssignment(0, 0.1);
    assignmentManager.setAssignment(1, 0.1);
    assignmentManager.setAssignment(3, 0.1);
    assignmentManager.setAssignment(5, 0.7);
    TS_ASSERT_EQUALS(disj->getPhaseStatusInAssignment(), phase4);

    delete disj;
  }
};
