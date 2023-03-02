#include <cxxtest/TestSuite.h>
#include <string.h>

#include "AbsoluteValueConstraint.h"
#include "Engine.h"
#include "FloatUtils.h"
#include "InputQuery.h"
#include "SoyError.h"
#include "MockErrno.h"
#include "MockFileFactory.h"

class MockForInputQuery : public MockFileFactory, public MockErrno {
 public:
};

class InputQueryTestSuite : public CxxTest::TestSuite {
 public:
  MockForInputQuery *mock;
  MockFile *file;

  void setUp() {
    TS_ASSERT(mock = new MockForInputQuery);

    file = &(mock->mockFile);
  }

  void tearDown() { TS_ASSERT_THROWS_NOTHING(delete mock); }

  void test_lower_bounds() {
    InputQuery inputQuery;

    TS_ASSERT_THROWS_NOTHING(inputQuery.setNumberOfVariables(5));
    TS_ASSERT_EQUALS(inputQuery.getLowerBound(3),
                     FloatUtils::negativeInfinity());
    TS_ASSERT_THROWS_NOTHING(inputQuery.setLowerBound(3, -3));
    TS_ASSERT_EQUALS(inputQuery.getLowerBound(3), -3);
    TS_ASSERT_THROWS_NOTHING(inputQuery.setLowerBound(3, 5));
    TS_ASSERT_EQUALS(inputQuery.getLowerBound(3), 5);

    TS_ASSERT_EQUALS(inputQuery.getLowerBound(2),
                     FloatUtils::negativeInfinity());
    TS_ASSERT_THROWS_NOTHING(inputQuery.setLowerBound(2, 4));
    TS_ASSERT_EQUALS(inputQuery.getLowerBound(2), 4);

    TS_ASSERT_THROWS_EQUALS(inputQuery.getLowerBound(5), const SoyError &e,
                            e.getCode(),
                            SoyError::VARIABLE_INDEX_OUT_OF_RANGE);

    TS_ASSERT_THROWS_EQUALS(inputQuery.setLowerBound(6, 1),
                            const SoyError &e, e.getCode(),
                            SoyError::VARIABLE_INDEX_OUT_OF_RANGE);
  }

  void test_upper_bounds() {
    InputQuery inputQuery;

    TS_ASSERT_THROWS_NOTHING(inputQuery.setNumberOfVariables(5));
    TS_ASSERT_EQUALS(inputQuery.getUpperBound(2), FloatUtils::infinity());
    TS_ASSERT_THROWS_NOTHING(inputQuery.setUpperBound(2, -4));
    TS_ASSERT_EQUALS(inputQuery.getUpperBound(2), -4);
    TS_ASSERT_THROWS_NOTHING(inputQuery.setUpperBound(2, 55));
    TS_ASSERT_EQUALS(inputQuery.getUpperBound(2), 55);

    TS_ASSERT_EQUALS(inputQuery.getUpperBound(0), FloatUtils::infinity());
    TS_ASSERT_THROWS_NOTHING(inputQuery.setUpperBound(0, 1));
    TS_ASSERT_EQUALS(inputQuery.getUpperBound(0), 1);

    TS_ASSERT_THROWS_EQUALS(inputQuery.getUpperBound(5), const SoyError &e,
                            e.getCode(),
                            SoyError::VARIABLE_INDEX_OUT_OF_RANGE);

    TS_ASSERT_THROWS_EQUALS(inputQuery.setUpperBound(6, 1),
                            const SoyError &e, e.getCode(),
                            SoyError::VARIABLE_INDEX_OUT_OF_RANGE);
  }

  void test_equality_operator() {
    AbsoluteValueConstraint *abs1 = new AbsoluteValueConstraint(3, 5);

    InputQuery *inputQuery = new InputQuery;

    inputQuery->setNumberOfVariables(5);
    inputQuery->setLowerBound(2, -4);
    inputQuery->setUpperBound(2, 55);
    inputQuery->addPLConstraint(abs1);

    InputQuery inputQuery2 = *inputQuery;

    TS_ASSERT_EQUALS(inputQuery2.getNumberOfVariables(), 5U);
    TS_ASSERT_EQUALS(inputQuery2.getLowerBound(2), -4);
    TS_ASSERT_EQUALS(inputQuery2.getUpperBound(2), 55);

    auto constraints = inputQuery2.getPLConstraints();

    TS_ASSERT_EQUALS(constraints.size(), 1U);
    AbsoluteValueConstraint *constraint =
        (AbsoluteValueConstraint *)*constraints.begin();

    TS_ASSERT_DIFFERS(constraint, abs1);  // Different pointers

    TS_ASSERT(constraint->participatingVariable(3));
    TS_ASSERT(constraint->participatingVariable(5));

    inputQuery2 = *inputQuery;  // Repeat the assignment

    delete inputQuery;

    TS_ASSERT_EQUALS(inputQuery2.getNumberOfVariables(), 5U);
    TS_ASSERT_EQUALS(inputQuery2.getLowerBound(2), -4);
    TS_ASSERT_EQUALS(inputQuery2.getUpperBound(2), 55);

    constraints = inputQuery2.getPLConstraints();

    TS_ASSERT_EQUALS(constraints.size(), 1U);
    constraint = (AbsoluteValueConstraint *)*constraints.begin();

    TS_ASSERT_DIFFERS(constraint, abs1);  // Different pointers

    TS_ASSERT(constraint->participatingVariable(3));
    TS_ASSERT(constraint->participatingVariable(5));
  }

  void test_infinite_bounds() {
    InputQuery *inputQuery = new InputQuery;

    inputQuery->setNumberOfVariables(5);
    inputQuery->setLowerBound(2, -4);
    inputQuery->setUpperBound(2, 55);
    inputQuery->setUpperBound(3, FloatUtils::infinity());

    TS_ASSERT_EQUALS(inputQuery->countInfiniteBounds(), 8U);

    delete inputQuery;
  }

  void test_equation_belongs_to_steps() {
    InputQuery *inputQuery = new InputQuery;

    inputQuery->setNumberOfVariables(3);
    TS_ASSERT_THROWS_NOTHING(inputQuery->markVariableToStep(0, 0));
    TS_ASSERT_THROWS_NOTHING(inputQuery->markVariableToStep(1, 1));
    TS_ASSERT_THROWS_NOTHING(inputQuery->markVariableToStep(2, 2));

    Equation e1;
    e1.addAddend(1, 0);
    e1.addAddend(1, 1);

    TS_ASSERT(inputQuery->equationBelongsToStep(e1, {0}));
    TS_ASSERT(inputQuery->equationBelongsToStep(e1, {1}));
    TS_ASSERT(inputQuery->equationBelongsToStep(e1, {0, 1}));
    TS_ASSERT(inputQuery->equationBelongsToStep(e1, {0, 2}));
    TS_ASSERT(inputQuery->equationBelongsToStep(e1, {0, 3}));
    TS_ASSERT(!inputQuery->equationBelongsToStep(e1, {2, 3}));

    OneHotConstraint oneHot({0, 1});
    List<unsigned> steps = {0, 1};
    TS_ASSERT_EQUALS(inputQuery->getStepsOfPLConstraint(&oneHot), steps);

    delete inputQuery;
  }
};
