#include <cxxtest/TestSuite.h>
#include <string.h>

#include "DisjunctionConstraint.h"
#include "InputQuery.h"
#include "MockEngine.h"
#include "MockErrno.h"
#include "Options.h"
#include "PLConstraint.h"
#include "SmtCore.h"

class MockForSmtCore {
 public:
};

class SmtCoreTestSuite : public CxxTest::TestSuite {
 public:
  MockForSmtCore *mock;

  void setUp() { TS_ASSERT(mock = new MockForSmtCore); }

  void tearDown() { TS_ASSERT_THROWS_NOTHING(delete mock); }

  void test_need_to_split() {
    MockEngine *engine = new MockEngine();
    DisjunctionConstraint *constraint1 =
        new DisjunctionConstraint({0, 1, 2, 3, 4});
    DisjunctionConstraint *constraint2 = new DisjunctionConstraint({5, 6, 7});

    CVC4::context::Context &ctx = engine->getContext();
    SmtCore &smtCore = *(engine->getSmtCore());

    constraint1->initializeCDOs(&ctx);
    constraint2->initializeCDOs(&ctx);

    TS_ASSERT_THROWS_NOTHING(
        smtCore.setBranchingHeuristics(DivideStrategy::PseudoImpact));
    TS_ASSERT_THROWS_NOTHING(
        smtCore.initializeScoreTrackerIfNeeded({constraint1, constraint2}));

    engine->_constraintToSplit = constraint1;

    for (unsigned i = 0; i < (unsigned)Options::get()->getInt(
                                 Options::DEEP_SOI_REJECTION_THRESHOLD) -
                                 1;
         ++i) {
      smtCore.reportRejectedPhasePatternProposal();
      TS_ASSERT(!smtCore.needToSplit());
    }

    smtCore.reportRejectedPhasePatternProposal();
    TS_ASSERT(smtCore.needToSplit());

    delete constraint1;
    delete constraint2;
    delete engine;
  }

  void test_perform_split() {
    MockEngine *engine = new MockEngine();
    DisjunctionConstraint *constraint1 = new DisjunctionConstraint({0, 1, 2});
    DisjunctionConstraint *constraint2 = new DisjunctionConstraint({3, 4, 5});
    DisjunctionConstraint *constraint3 = new DisjunctionConstraint({6, 7});

    CVC4::context::Context &ctx = engine->getContext();
    SmtCore &smtCore = *(engine->getSmtCore());

    constraint1->initializeCDOs(&ctx);
    constraint2->initializeCDOs(&ctx);
    constraint3->initializeCDOs(&ctx);
    List<PhaseStatus> phases1 = constraint1->getAllCases();
    List<PhaseStatus> phases2 = constraint2->getAllCases();
    List<PhaseStatus> phases3 = constraint3->getAllCases();

    TS_ASSERT_THROWS_NOTHING(
        smtCore.setBranchingHeuristics(DivideStrategy::PseudoImpact));
    TS_ASSERT_THROWS_NOTHING(smtCore.initializeScoreTrackerIfNeeded(
        {constraint1, constraint2, constraint3}));

    // Perform a split
    engine->_constraintToSplit = constraint1;
    auto it1 = phases1.begin();
    for (unsigned i = 0; i < (unsigned)Options::get()->getInt(
                                 Options::DEEP_SOI_REJECTION_THRESHOLD);
         ++i) {
      smtCore.reportRejectedPhasePatternProposal();
    }

    TS_ASSERT(smtCore.needToSplit());
    TS_ASSERT_THROWS_NOTHING(smtCore.performSplit());
    TS_ASSERT(!smtCore.needToSplit());

    TS_ASSERT_EQUALS(engine->_lastSplitApplied,
                     constraint1->getCaseSplit(*it1));

    TS_ASSERT(!constraint1->isActive());
    TS_ASSERT(constraint2->isActive());
    TS_ASSERT_EQUALS(smtCore.getTrailLength(), 1u);

    // Perform another split
    engine->_constraintToSplit = constraint2;
    auto it2 = phases2.begin();
    for (unsigned i = 0; i < (unsigned)Options::get()->getInt(
                                 Options::DEEP_SOI_REJECTION_THRESHOLD);
         ++i) {
      smtCore.reportRejectedPhasePatternProposal();
    }

    TS_ASSERT(smtCore.needToSplit());
    TS_ASSERT_THROWS_NOTHING(smtCore.performSplit());
    TS_ASSERT(!smtCore.needToSplit());
    TS_ASSERT_EQUALS(engine->_lastSplitApplied,
                     constraint2->getCaseSplit(*it2));

    TS_ASSERT(!constraint1->isActive());
    TS_ASSERT(!constraint2->isActive());
    TS_ASSERT_EQUALS(smtCore.getTrailLength(), 2u);

    // Pop and try alternative of C2
    TS_ASSERT(smtCore.popSplit());
    TS_ASSERT_EQUALS(engine->_lastSplitApplied,
                     constraint2->getCaseSplit(*++it2));

    TS_ASSERT(!constraint1->isActive());
    TS_ASSERT(!constraint2->isActive());
    TS_ASSERT_EQUALS(smtCore.getTrailLength(), 2u);

    // Pop and try alternative of C2
    TS_ASSERT(smtCore.popSplit());
    TS_ASSERT_EQUALS(engine->_lastSplitApplied,
                     constraint2->getCaseSplit(*++it2));

    TS_ASSERT(!constraint1->isActive());
    TS_ASSERT(!constraint2->isActive());
    TS_ASSERT_EQUALS(smtCore.getTrailLength(), 2u);

    // Pop and try alternative of C1
    TS_ASSERT(smtCore.popSplit());
    TS_ASSERT_EQUALS(engine->_lastSplitApplied,
                     constraint1->getCaseSplit(*++it1));

    TS_ASSERT(!constraint1->isActive());
    TS_ASSERT(constraint2->isActive());
    TS_ASSERT_EQUALS(smtCore.getTrailLength(), 1u);

    // Pop and try alternative of C1
    TS_ASSERT(smtCore.popSplit());
    TS_ASSERT_EQUALS(engine->_lastSplitApplied,
                     constraint1->getCaseSplit(*++it1));

    TS_ASSERT(!constraint1->isActive());
    TS_ASSERT(constraint2->isActive());
    TS_ASSERT_EQUALS(smtCore.getTrailLength(), 1u);

    // Perform another split
    engine->_constraintToSplit = constraint2;
    it2 = phases2.begin();
    for (unsigned i = 0; i < (unsigned)Options::get()->getInt(
                                 Options::DEEP_SOI_REJECTION_THRESHOLD);
         ++i) {
      smtCore.reportRejectedPhasePatternProposal();
    }

    TS_ASSERT(smtCore.needToSplit());
    TS_ASSERT_THROWS_NOTHING(smtCore.performSplit());
    TS_ASSERT(!smtCore.needToSplit());
    TS_ASSERT_EQUALS(engine->_lastSplitApplied,
                     constraint2->getCaseSplit(*it2));

    TS_ASSERT(!constraint1->isActive());
    TS_ASSERT(!constraint2->isActive());
    TS_ASSERT_EQUALS(smtCore.getTrailLength(), 2u);

    // Pop and try alternative of C2
    TS_ASSERT(smtCore.popSplit());
    TS_ASSERT_EQUALS(engine->_lastSplitApplied,
                     constraint2->getCaseSplit(*++it2));

    TS_ASSERT(!constraint1->isActive());
    TS_ASSERT(!constraint2->isActive());
    TS_ASSERT_EQUALS(smtCore.getTrailLength(), 2u);

    // Perform yet another split
    engine->_constraintToSplit = constraint3;
    auto it3 = phases3.begin();
    for (unsigned i = 0; i < (unsigned)Options::get()->getInt(
                                 Options::DEEP_SOI_REJECTION_THRESHOLD);
         ++i) {
      smtCore.reportRejectedPhasePatternProposal();
    }

    TS_ASSERT(smtCore.needToSplit());
    TS_ASSERT_THROWS_NOTHING(smtCore.performSplit());
    TS_ASSERT(!smtCore.needToSplit());
    TS_ASSERT_EQUALS(engine->_lastSplitApplied,
                     constraint3->getCaseSplit(*it3));

    TS_ASSERT(!constraint1->isActive());
    TS_ASSERT(!constraint2->isActive());
    TS_ASSERT(!constraint3->isActive());
    TS_ASSERT_EQUALS(smtCore.getTrailLength(), 3u);

    // Pop and try alternative of C3
    TS_ASSERT(smtCore.popSplit());
    TS_ASSERT_EQUALS(engine->_lastSplitApplied,
                     constraint3->getCaseSplit(*++it3));

    TS_ASSERT(!constraint1->isActive());
    TS_ASSERT(!constraint2->isActive());
    TS_ASSERT(!constraint3->isActive());
    TS_ASSERT_EQUALS(smtCore.getTrailLength(), 3u);

    // Pop and try alternative of C2
    TS_ASSERT(smtCore.popSplit());
    TS_ASSERT_EQUALS(engine->_lastSplitApplied,
                     constraint2->getCaseSplit(*++it2));

    TS_ASSERT(!constraint1->isActive());
    TS_ASSERT(!constraint2->isActive());
    TS_ASSERT_EQUALS(smtCore.getTrailLength(), 2u);

    engine->_lastSplitApplied = PiecewiseLinearCaseSplit();
    // Pop and try alternative
    TS_ASSERT(!smtCore.popSplit());
    TS_ASSERT_EQUALS(engine->_lastSplitApplied, PiecewiseLinearCaseSplit());

    TS_ASSERT_EQUALS(smtCore.getTrailLength(), 0u);

    delete constraint1;
    delete constraint2;
    delete constraint3;
    delete engine;
  }

  void test_perform_split_inactive_constraint() {
    MockEngine *engine = new MockEngine();
    DisjunctionConstraint *constraint1 =
        new DisjunctionConstraint({0, 1, 2, 3, 4});
    DisjunctionConstraint *constraint2 = new DisjunctionConstraint({5, 6, 7});

    CVC4::context::Context &ctx = engine->getContext();
    SmtCore &smtCore = *(engine->getSmtCore());

    constraint1->initializeCDOs(&ctx);
    constraint2->initializeCDOs(&ctx);

    TS_ASSERT_THROWS_NOTHING(
        smtCore.setBranchingHeuristics(DivideStrategy::PseudoImpact));
    TS_ASSERT_THROWS_NOTHING(
        smtCore.initializeScoreTrackerIfNeeded({constraint1, constraint2}));

    constraint1->setActive(false);
    engine->_constraintToSplit = constraint1;

    for (unsigned i = 0; i < (unsigned)Options::get()->getInt(
                                 Options::DEEP_SOI_REJECTION_THRESHOLD);
         ++i) {
      smtCore.reportRejectedPhasePatternProposal();
    }

    TS_ASSERT(smtCore.needToSplit());
    TS_ASSERT_THROWS_NOTHING(smtCore.performSplit());
    TS_ASSERT(!smtCore.needToSplit());

    // Check that no split was performed
    TS_ASSERT_EQUALS(engine->_lastSplitApplied, PiecewiseLinearCaseSplit());
    delete constraint1;
    delete constraint2;
    delete engine;
  }

  void test_todo() {
    // Reason: the inefficiency in resizing the tableau mutliple times
    TS_TRACE(
        "add support for adding multiple equations at once, not one-by-one");
  }

  void test_luby() {
    TS_ASSERT_EQUALS(SmtCore::luby(1), 1u);
    TS_ASSERT_EQUALS(SmtCore::luby(2), 1u);
    TS_ASSERT_EQUALS(SmtCore::luby(3), 2u);
    TS_ASSERT_EQUALS(SmtCore::luby(4), 1u);
    TS_ASSERT_EQUALS(SmtCore::luby(5), 1u);
    TS_ASSERT_EQUALS(SmtCore::luby(6), 2u);
    TS_ASSERT_EQUALS(SmtCore::luby(7), 4u);
    TS_ASSERT_EQUALS(SmtCore::luby(8), 1u);
    TS_ASSERT_EQUALS(SmtCore::luby(9), 1u);
    TS_ASSERT_EQUALS(SmtCore::luby(10), 2u);
    TS_ASSERT_EQUALS(SmtCore::luby(11), 1u);
    TS_ASSERT_EQUALS(SmtCore::luby(12), 1u);
    TS_ASSERT_EQUALS(SmtCore::luby(13), 2u);
    TS_ASSERT_EQUALS(SmtCore::luby(14), 4u);
    TS_ASSERT_EQUALS(SmtCore::luby(15), 8u);
  }
};
