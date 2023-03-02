/*********************                                                        */
/*! \file Test_OneHotConstraint.h
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
#include "MockErrno.h"
#include "OneHotConstraint.h"
#include "Statistics.h"
#include "context/context.h"

class OneHotConstraintTestSuite : public CxxTest::TestSuite {
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
    OneHotConstraint *oneHot = new OneHotConstraint(elements);

    TS_ASSERT_EQUALS(oneHot->getType(), ONE_HOT);

    List<unsigned> vars = {0, 1, 3, 5};
    TS_ASSERT_EQUALS(oneHot->getParticipatingVariables(), vars);
    TS_ASSERT(oneHot->participatingVariable(0));
    TS_ASSERT(oneHot->participatingVariable(1));
    TS_ASSERT(!oneHot->participatingVariable(2));
    TS_ASSERT(oneHot->participatingVariable(3));
    TS_ASSERT(!oneHot->participatingVariable(4));

    TS_ASSERT_EQUALS(oneHot->getElementOfPhase(phase1), 0u);
    TS_ASSERT_EQUALS(oneHot->getElementOfPhase(phase2), 1u);
    TS_ASSERT_EQUALS(oneHot->getElementOfPhase(phase3), 3u);
    TS_ASSERT_EQUALS(oneHot->getElementOfPhase(phase4), 5u);
    TS_ASSERT_THROWS_ANYTHING(oneHot->getElementOfPhase(phase5));

    TS_ASSERT_EQUALS(oneHot->getPhaseOfElement(0), phase1);
    TS_ASSERT_EQUALS(oneHot->getPhaseOfElement(1), phase2);
    TS_ASSERT_THROWS_ANYTHING(oneHot->getPhaseOfElement(2));
    TS_ASSERT_EQUALS(oneHot->getPhaseOfElement(3), phase3);
    TS_ASSERT_EQUALS(oneHot->getPhaseOfElement(5), phase4);

    delete oneHot;
  }

  void test_add_boolean_structure() {
    Set<unsigned> elements = {0, 1, 3, 5};
    OneHotConstraint *oneHot = new OneHotConstraint(elements);

    CadicalWrapper *cadical = new CadicalWrapper();
    oneHot->registerSatSolver(cadical);
    TS_ASSERT_THROWS_NOTHING(oneHot->addBooleanStructure());

    cadical->assumeLiteral(oneHot->getLiteralOfPhaseStatus(phase1));
    TS_ASSERT_THROWS_NOTHING(cadical->solve());
    TS_ASSERT(cadical->getAssignment(oneHot->getLiteralOfPhaseStatus(phase1)));
    TS_ASSERT(!cadical->getAssignment(oneHot->getLiteralOfPhaseStatus(phase2)));
    TS_ASSERT(!cadical->getAssignment(oneHot->getLiteralOfPhaseStatus(phase3)));
    TS_ASSERT(!cadical->getAssignment(oneHot->getLiteralOfPhaseStatus(phase4)));

    cadical->addConstraint({oneHot->getLiteralOfPhaseStatus(phase2)});
    TS_ASSERT_THROWS_NOTHING(cadical->preprocess());
    TS_ASSERT_EQUALS(
        cadical->getLiteralStatus(oneHot->getLiteralOfPhaseStatus(phase1)),
        FALSE);
    TS_ASSERT_EQUALS(
        cadical->getLiteralStatus(oneHot->getLiteralOfPhaseStatus(phase2)),
        TRUE);
    TS_ASSERT_EQUALS(
        cadical->getLiteralStatus(oneHot->getLiteralOfPhaseStatus(phase3)),
        FALSE);
    TS_ASSERT_EQUALS(
        cadical->getLiteralStatus(oneHot->getLiteralOfPhaseStatus(phase4)),
        FALSE);

    delete oneHot;
    delete cadical;
  }

  void test_duplicate_constraint() {
    Set<unsigned> elements = {0, 1, 3, 5};
    OneHotConstraint *oneHot = new OneHotConstraint(elements);

    OneHotConstraint *duplicate =
        static_cast<OneHotConstraint *>(oneHot->duplicateConstraint());

    TS_ASSERT_EQUALS(oneHot->getElements(), duplicate->getElements());
    TS_ASSERT_EQUALS(oneHot->getAllCases(), duplicate->getAllCases());

    delete oneHot;
    delete duplicate;
  }

  void test_notify_bounds_preprocessing() {
    // No context, no bound manager
    {
      Set<unsigned> elements = {0, 1, 3, 5};
      OneHotConstraint *oneHot = new OneHotConstraint(elements);
      List<Tightening> tightenings;
      TS_ASSERT_THROWS_NOTHING(oneHot->getEntailedTightenings(tightenings));
      TS_ASSERT_EQUALS(tightenings.size(), 0u);

      oneHot->notifyLowerBound(0, 0);
      oneHot->notifyLowerBound(1, 0);
      oneHot->notifyLowerBound(3, 0);
      oneHot->notifyLowerBound(5, 0);
      oneHot->notifyUpperBound(0, 1);
      oneHot->notifyUpperBound(1, 1);
      oneHot->notifyUpperBound(3, 1);
      oneHot->notifyUpperBound(5, 1);
      // No element eliminated yet.
      TS_ASSERT_EQUALS(oneHot->getElements(), elements);
      TS_ASSERT_THROWS_NOTHING(oneHot->getEntailedTightenings(tightenings));
      for (const auto &e : elements) {
        TS_ASSERT(tightenings.exists(Tightening(e, 0, Tightening::LB)));
        TS_ASSERT(tightenings.exists(Tightening(e, 1, Tightening::UB)));
      }
      delete oneHot;
    }

    {
      Set<unsigned> elements = {0, 1, 3, 5};
      OneHotConstraint *oneHot = new OneHotConstraint(elements);
      oneHot->notifyLowerBound(0, 0);
      oneHot->notifyLowerBound(1, 0);
      oneHot->notifyLowerBound(3, 0);
      oneHot->notifyLowerBound(5, 0);
      oneHot->notifyUpperBound(0, 1);
      oneHot->notifyUpperBound(1, 1);
      oneHot->notifyUpperBound(3, 1);
      oneHot->notifyUpperBound(5, 1);
      oneHot->notifyLowerBound(5, 0.1);
      // All but one element eliminated
      Set<unsigned> elementsLeft = {5};
      TS_ASSERT_EQUALS(oneHot->getElements(), elementsLeft);

      List<Tightening> tightenings;
      TS_ASSERT_THROWS_NOTHING(oneHot->getEntailedTightenings(tightenings));
      TS_ASSERT(tightenings.exists(Tightening(0, 0, Tightening::LB)));
      TS_ASSERT(tightenings.exists(Tightening(0, 0, Tightening::UB)));
      TS_ASSERT(tightenings.exists(Tightening(1, 0, Tightening::LB)));
      TS_ASSERT(tightenings.exists(Tightening(1, 0, Tightening::UB)));
      TS_ASSERT(tightenings.exists(Tightening(3, 0, Tightening::LB)));
      TS_ASSERT(tightenings.exists(Tightening(3, 0, Tightening::UB)));
      TS_ASSERT(tightenings.exists(Tightening(5, 1, Tightening::LB)));
      TS_ASSERT(tightenings.exists(Tightening(5, 1, Tightening::UB)));
      delete oneHot;
    }

    {
      Set<unsigned> elements = {0, 1, 3, 5};
      OneHotConstraint *oneHot = new OneHotConstraint(elements);
      oneHot->notifyLowerBound(0, 0);
      oneHot->notifyLowerBound(1, 0);
      oneHot->notifyLowerBound(3, 0);
      oneHot->notifyLowerBound(5, 0);
      oneHot->notifyUpperBound(0, 1);
      oneHot->notifyUpperBound(1, 1);
      oneHot->notifyUpperBound(3, 1);
      oneHot->notifyUpperBound(5, 1);

      oneHot->notifyLowerBound(5, 1);
      Set<unsigned> elementsLeft = {5};
      TS_ASSERT_EQUALS(oneHot->getElements(), elementsLeft);

      List<Tightening> tightenings;
      TS_ASSERT_THROWS_NOTHING(oneHot->getEntailedTightenings(tightenings));
      TS_ASSERT(tightenings.exists(Tightening(0, 0, Tightening::LB)));
      TS_ASSERT(tightenings.exists(Tightening(0, 0, Tightening::UB)));
      TS_ASSERT(tightenings.exists(Tightening(1, 0, Tightening::LB)));
      TS_ASSERT(tightenings.exists(Tightening(1, 0, Tightening::UB)));
      TS_ASSERT(tightenings.exists(Tightening(3, 0, Tightening::LB)));
      TS_ASSERT(tightenings.exists(Tightening(3, 0, Tightening::UB)));
      TS_ASSERT(tightenings.exists(Tightening(5, 1, Tightening::LB)));
      TS_ASSERT(tightenings.exists(Tightening(5, 1, Tightening::UB)));
      delete oneHot;
    }

    {
      Set<unsigned> elements = {0, 1, 3, 5};
      OneHotConstraint *oneHot = new OneHotConstraint(elements);
      oneHot->notifyLowerBound(0, 0);
      oneHot->notifyLowerBound(1, 0);
      oneHot->notifyLowerBound(3, 0);
      oneHot->notifyLowerBound(5, 0);
      oneHot->notifyUpperBound(0, 1);
      oneHot->notifyUpperBound(1, 1);
      oneHot->notifyUpperBound(3, 1);
      oneHot->notifyUpperBound(5, 1);
      oneHot->notifyUpperBound(5, 0.5);
      Set<unsigned> elementsLeft = {0, 1, 3};
      TS_ASSERT_EQUALS(oneHot->getElements(), elementsLeft);

      List<Tightening> tightenings;
      TS_ASSERT_THROWS_NOTHING(oneHot->getEntailedTightenings(tightenings));
      TS_ASSERT(tightenings.exists(Tightening(0, 0, Tightening::LB)));
      TS_ASSERT(tightenings.exists(Tightening(0, 1, Tightening::UB)));
      TS_ASSERT(tightenings.exists(Tightening(1, 0, Tightening::LB)));
      TS_ASSERT(tightenings.exists(Tightening(1, 1, Tightening::UB)));
      TS_ASSERT(tightenings.exists(Tightening(3, 0, Tightening::LB)));
      TS_ASSERT(tightenings.exists(Tightening(3, 1, Tightening::UB)));
      TS_ASSERT(tightenings.exists(Tightening(5, 0, Tightening::LB)));
      TS_ASSERT(tightenings.exists(Tightening(5, 0, Tightening::UB)));
      delete oneHot;
    }

    {
      Set<unsigned> elements = {0, 1, 3, 5};
      OneHotConstraint *oneHot = new OneHotConstraint(elements);
      oneHot->notifyLowerBound(0, 0);
      oneHot->notifyLowerBound(1, 0);
      oneHot->notifyLowerBound(3, 0);
      oneHot->notifyLowerBound(5, 0);
      oneHot->notifyUpperBound(0, 1);
      oneHot->notifyUpperBound(1, 1);
      oneHot->notifyUpperBound(3, 1);
      oneHot->notifyUpperBound(5, 1);
      oneHot->notifyUpperBound(5, 0);
      Set<unsigned> elementsLeft = {0, 1, 3};
      TS_ASSERT_EQUALS(oneHot->getElements(), elementsLeft);

      List<Tightening> tightenings;
      TS_ASSERT_THROWS_NOTHING(oneHot->getEntailedTightenings(tightenings));
      TS_ASSERT(tightenings.exists(Tightening(0, 0, Tightening::LB)));
      TS_ASSERT(tightenings.exists(Tightening(0, 1, Tightening::UB)));
      TS_ASSERT(tightenings.exists(Tightening(1, 0, Tightening::LB)));
      TS_ASSERT(tightenings.exists(Tightening(1, 1, Tightening::UB)));
      TS_ASSERT(tightenings.exists(Tightening(3, 0, Tightening::LB)));
      TS_ASSERT(tightenings.exists(Tightening(3, 1, Tightening::UB)));
      TS_ASSERT(tightenings.exists(Tightening(5, 0, Tightening::LB)));
      TS_ASSERT(tightenings.exists(Tightening(5, 0, Tightening::UB)));

      CVC4::context::Context context;
      oneHot->initializeCDOs(&context);
      TS_ASSERT_EQUALS(oneHot->getAllCases().size(), 3u);
      TS_ASSERT_EQUALS(oneHot->numberOfFeasiblePhases(), 3u);
      delete oneHot;
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
      OneHotConstraint *oneHot = new OneHotConstraint(elements);
      oneHot->initializeCDOs(&context);
      oneHot->registerBoundManager(&bm);

      TS_ASSERT_EQUALS(oneHot->getElements(), elements);
      TS_ASSERT_EQUALS(oneHot->getAllCases().size(), 4u);
      TS_ASSERT(oneHot->hasFeasiblePhases());
      TS_ASSERT(!oneHot->phaseFixed());

      context.push();
      oneHot->notifyLowerBound(0, 0.1);

      TS_ASSERT_EQUALS(oneHot->getElements(), elements);
      TS_ASSERT_EQUALS(oneHot->getAllCases().size(), 4u);
      TS_ASSERT_EQUALS(oneHot->numberOfFeasiblePhases(), 1u);
      TS_ASSERT(oneHot->hasFeasiblePhases());
      TS_ASSERT(oneHot->phaseFixed());
      TS_ASSERT_EQUALS(oneHot->getValidCaseSplit(),
                       oneHot->getCaseSplit(phase1));

      context.pop();

      TS_ASSERT_EQUALS(oneHot->getElements(), elements);
      TS_ASSERT_EQUALS(oneHot->getAllCases().size(), 4u);
      TS_ASSERT_EQUALS(oneHot->numberOfFeasiblePhases(), 4u);
      TS_ASSERT(oneHot->hasFeasiblePhases());
      TS_ASSERT(!oneHot->phaseFixed());
      TS_ASSERT_EQUALS(oneHot->getValidCaseSplit(), PiecewiseLinearCaseSplit());

      delete oneHot;
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
      OneHotConstraint *oneHot = new OneHotConstraint(elements);
      oneHot->initializeCDOs(&context);
      oneHot->registerBoundManager(&bm);

      TS_ASSERT_EQUALS(oneHot->getElements(), elements);
      TS_ASSERT_EQUALS(oneHot->getAllCases().size(), 4u);
      TS_ASSERT(oneHot->hasFeasiblePhases());
      TS_ASSERT(!oneHot->phaseFixed());

      context.push();
      oneHot->notifyUpperBound(0, 0.5);

      TS_ASSERT_EQUALS(oneHot->getElements(), elements);
      TS_ASSERT_EQUALS(oneHot->getAllCases().size(), 4u);
      TS_ASSERT_EQUALS(oneHot->numberOfFeasiblePhases(), 3u);
      TS_ASSERT(oneHot->hasFeasiblePhases());
      TS_ASSERT(!oneHot->phaseFixed());
      TS_ASSERT(!oneHot->isFeasible(phase1));
      context.pop();

      TS_ASSERT_EQUALS(oneHot->getElements(), elements);
      TS_ASSERT_EQUALS(oneHot->getAllCases().size(), 4u);
      TS_ASSERT_EQUALS(oneHot->numberOfFeasiblePhases(), 4u);
      TS_ASSERT(oneHot->hasFeasiblePhases());
      TS_ASSERT(!oneHot->phaseFixed());
      TS_ASSERT_EQUALS(oneHot->getValidCaseSplit(), PiecewiseLinearCaseSplit());
      delete oneHot;
    }
  }

  void test_case_splits() {
    Set<unsigned> elements = {0, 1, 3, 5};
    OneHotConstraint *oneHot = new OneHotConstraint(elements);
    CVC4::context::Context context;
    TS_ASSERT_THROWS_NOTHING(oneHot->initializeCDOs(&context));

    PiecewiseLinearCaseSplit split1;
    split1.storeBoundTightening(Tightening(0, 1, Tightening::LB));
    split1.storeBoundTightening(Tightening(1, 0, Tightening::UB));
    split1.storeBoundTightening(Tightening(3, 0, Tightening::UB));
    split1.storeBoundTightening(Tightening(5, 0, Tightening::UB));

    TS_ASSERT_EQUALS(split1, oneHot->getCaseSplit(phase1));

    context.push();
    TS_ASSERT_THROWS_NOTHING(oneHot->setPhaseStatus(phase1));
    TS_ASSERT(oneHot->phaseFixed());
    TS_ASSERT_EQUALS(oneHot->getValidCaseSplit(), split1);

    context.pop();
    TS_ASSERT(!oneHot->phaseFixed());
    TS_ASSERT_EQUALS(oneHot->getValidCaseSplit(), PiecewiseLinearCaseSplit());

    delete oneHot;
  }

  void test_satisfied() {
    CVC4::context::Context context;
    BoundManager bm(context);
    AssignmentManager am(bm);
    Set<unsigned> elements = {0, 1, 3, 5};
    OneHotConstraint *oneHot = new OneHotConstraint(elements);
    oneHot->registerAssignmentManager(&am);

    bm.initialize(6);
    am.initialize();

    am.setAssignment(0, 1);
    am.setAssignment(1, 1);
    am.setAssignment(3, 1);
    am.setAssignment(5, 1);
    TS_ASSERT(!oneHot->satisfied());

    am.setAssignment(0, 0);
    am.setAssignment(1, 1);
    am.setAssignment(3, 1);
    am.setAssignment(5, 1);
    TS_ASSERT(!oneHot->satisfied());

    am.setAssignment(0, 0);
    am.setAssignment(1, 0);
    am.setAssignment(3, 0);
    am.setAssignment(5, 1);
    TS_ASSERT(oneHot->satisfied());

    am.setAssignment(0, 0);
    am.setAssignment(1, 0);
    am.setAssignment(3, 0);
    am.setAssignment(5, 0);
    TS_ASSERT(!oneHot->satisfied());

    am.setAssignment(0, 0.0001);
    am.setAssignment(1, 0.0001);
    am.setAssignment(3, 0.0001);
    am.setAssignment(5, 0.9999);
    TS_ASSERT(!oneHot->satisfied());

    delete oneHot;
  }

  void test_soi() {
    Set<unsigned> elements = {0, 1, 3, 5};
    OneHotConstraint *oneHot = new OneHotConstraint(elements);
    TS_ASSERT(oneHot->supportSoI());

    CVC4::context::Context context;
    TS_ASSERT_THROWS_NOTHING(oneHot->initializeCDOs(&context));

    LinearExpression cost;
    LinearExpression costResult;
    TS_ASSERT_THROWS_NOTHING(oneHot->getCostFunctionComponent(cost, phase1));
    costResult._constant = 1;
    costResult._addends[0] = -1;
    TS_ASSERT_EQUALS(cost, costResult);

    TS_ASSERT_THROWS_NOTHING(oneHot->getCostFunctionComponent(cost, phase1));
    costResult._constant = 2;
    costResult._addends[0] = -2;
    TS_ASSERT_EQUALS(cost, costResult);

    TS_ASSERT_THROWS_NOTHING(oneHot->getCostFunctionComponent(cost, phase4));
    costResult._constant = 3;
    costResult._addends[5] = -1;
    TS_ASSERT_EQUALS(cost, costResult);

    context.push();
    // If the phase is infeasible, do not add to the cost.
    TS_ASSERT_THROWS_NOTHING(oneHot->markInfeasiblePhase(phase3));
    TS_ASSERT_THROWS_NOTHING(oneHot->getCostFunctionComponent(cost, phase3));
    TS_ASSERT_EQUALS(cost, costResult);

    context.pop();
    TS_ASSERT_THROWS_NOTHING(oneHot->getCostFunctionComponent(cost, phase3));
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
    oneHot->registerAssignmentManager(&assignmentManager);

    assignmentManager.setAssignment(0, 0.1);
    assignmentManager.setAssignment(1, 0.1);
    assignmentManager.setAssignment(3, 0.1);
    assignmentManager.setAssignment(5, 0.7);
    TS_ASSERT_EQUALS(oneHot->getPhaseStatusInAssignment(), phase4);

    delete oneHot;
  }
};
