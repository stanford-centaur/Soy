/*********************                                                        */
/*! \file Test_List.h
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

#include "AbsoluteValueConstraint.h"
#include "BoundManager.h"
#include "CadicalWrapper.h"
#include "GurobiWrapper.h"
#include "InputQuery.h"
#include "MockErrno.h"
#include "OneHotConstraint.h"
#include "Statistics.h"
#include "context/context.h"

class AbsoluteConstraintTestSuite : public CxxTest::TestSuite {
 public:
  MockErrno *mockErrno;

  void setUp() { TS_ASSERT(mockErrno = new MockErrno); }

  void tearDown() { TS_ASSERT_THROWS_NOTHING(delete mockErrno); }

  void test_abs_constraint() {
    unsigned b = 1;
    unsigned f = 4;
    AbsoluteValueConstraint *abs = new AbsoluteValueConstraint(b, f);

    TS_ASSERT_EQUALS(abs->getType(), ABSOLUTE_VALUE);
    TS_ASSERT_EQUALS(abs->getB(), 1u);
    TS_ASSERT_EQUALS(abs->getF(), 4u);
    TS_ASSERT(!abs->auxVariablesInUse());

    List<unsigned> vars = {1, 4};
    TS_ASSERT_EQUALS(abs->getParticipatingVariables(), vars);

    InputQuery ipq;
    ipq.setNumberOfVariables(5);

    TS_ASSERT_THROWS_NOTHING(abs->transformToUseAuxVariables(ipq));

    TS_ASSERT(abs->auxVariablesInUse());
    TS_ASSERT_EQUALS(abs->getPosAux(), 5u);
    TS_ASSERT_EQUALS(abs->getNegAux(), 6u);

    vars = {1, 4, 5, 6};
    TS_ASSERT_EQUALS(abs->getParticipatingVariables(), vars);

    TS_ASSERT(abs->participatingVariable(1));
    TS_ASSERT(abs->participatingVariable(4));
    TS_ASSERT(!abs->participatingVariable(0));
    TS_ASSERT(!abs->participatingVariable(2));

    delete abs;
  }

  void test_add_boolean_structure() {}

  void test_duplicate_constraint() {
    unsigned b = 1;
    unsigned f = 4;
    AbsoluteValueConstraint *abs = new AbsoluteValueConstraint(b, f);

    AbsoluteValueConstraint *duplicate =
        static_cast<AbsoluteValueConstraint *>(abs->duplicateConstraint());

    TS_ASSERT_EQUALS(abs->getAllCases(), duplicate->getAllCases());

    delete abs;
    delete duplicate;
  }

  void assert_lower_upper_bound(unsigned f, unsigned b, double fLower,
                                double bLower, double fUpper, double bUpper,
                                List<Tightening> entailedTightenings) {
    TS_ASSERT_EQUALS(entailedTightenings.size(), 8U);

    Tightening f_lower(f, fLower, Tightening::LB);
    Tightening f_upper(f, fUpper, Tightening::UB);
    Tightening b_lower(b, bLower, Tightening::LB);
    Tightening b_upper(b, bUpper, Tightening::UB);
    for (const auto &t : {f_lower, f_upper, b_lower, b_upper}) {
      TS_ASSERT(entailedTightenings.exists(t));
      if (!entailedTightenings.exists(t)) {
        std::cout << " Cannot find tightening (" << fLower << bLower << fUpper
                  << bUpper << ") : " << std::endl;
        t.dump();

        std::cout << "Entailed tightenings: " << std::endl;
        for (auto ent : entailedTightenings) ent.dump();
      }
    }
  }

  void assert_tightenings_match(List<Tightening> a, List<Tightening> b) {
    TS_ASSERT_EQUALS(a.size(), b.size());

    for (const auto &it : a) {
      TS_ASSERT(b.exists(it));
      if (!b.exists(it)) {
        std::cout << " Cannot find tightening " << std::endl;
        it.dump();

        std::cout << "Entailed tightenings: " << std::endl;
        for (auto ent : a) ent.dump();

        std::cout << "Expected tightenings: " << std::endl;
        for (auto ent : b) ent.dump();
      }
    }
  }

  void test_notify_bounds_preprocessing() {
    // No context, no bound manager
    {
      unsigned b = 1;
      unsigned f = 4;
      unsigned posAux = 5;
      unsigned negAux = 6;

      AbsoluteValueConstraint *abs = new AbsoluteValueConstraint(b, f);

      InputQuery ipq;
      ipq.setNumberOfVariables(5);

      TS_ASSERT_THROWS_NOTHING(abs->transformToUseAuxVariables(ipq));

      abs->notifyLowerBound(b, 1);
      abs->notifyLowerBound(f, 2);
      abs->notifyUpperBound(b, 7);
      abs->notifyUpperBound(f, 7);

      List<Tightening> entailedTightenings;

      // 1 < x_b < 7 , 2 < x_f < 7 -> 2 < x_b
      // 0 <= posAux <= 0, 0 <= negAux < 14
      abs->getEntailedTightenings(entailedTightenings);
      assert_lower_upper_bound(f, b, 2, 1, 7, 7, entailedTightenings);
      assert_lower_upper_bound(posAux, negAux, 0, 0, FloatUtils::infinity(), 14,
                               entailedTightenings);

      abs->notifyLowerBound(b, 3);
      entailedTightenings.clear();
      abs->getEntailedTightenings(entailedTightenings);
      assert_lower_upper_bound(f, b, 2, 3, 7, 7, entailedTightenings);

      abs->notifyLowerBound(f, 3);
      abs->notifyUpperBound(b, 6);
      entailedTightenings.clear();
      abs->getEntailedTightenings(entailedTightenings);
      assert_lower_upper_bound(f, b, 3, 3, 6, 6, entailedTightenings);

      abs->notifyUpperBound(f, 6);
      abs->notifyUpperBound(b, 7);
      entailedTightenings.clear();
      abs->getEntailedTightenings(entailedTightenings);
      assert_lower_upper_bound(f, b, 3, 3, 6, 6, entailedTightenings);

      abs->notifyLowerBound(f, -3);
      entailedTightenings.clear();
      abs->getEntailedTightenings(entailedTightenings);
      assert_lower_upper_bound(f, b, 3, 3, 6, 6, entailedTightenings);

      abs->notifyLowerBound(b, 5);
      abs->notifyUpperBound(f, 5);
      // 5 < x_b < 6 , 3 < x_f < 5
      entailedTightenings.clear();
      abs->getEntailedTightenings(entailedTightenings);
      assert_lower_upper_bound(f, b, 3, 5, 5, 5, entailedTightenings);

      delete abs;
    }

    {
      unsigned b = 1;
      unsigned f = 4;
      AbsoluteValueConstraint *abs = new AbsoluteValueConstraint(b, f);

      InputQuery ipq;
      ipq.setNumberOfVariables(5);

      TS_ASSERT_THROWS_NOTHING(abs->transformToUseAuxVariables(ipq));

      List<Tightening> entailedTightenings;

      // 8 < b < 18, 48 < f < 64
      abs->notifyUpperBound(b, 18);
      abs->notifyUpperBound(f, 64);
      abs->notifyLowerBound(b, 8);
      abs->notifyLowerBound(f, 48);
      abs->getEntailedTightenings(entailedTightenings);
      assert_lower_upper_bound(f, b, 48, 8, 64, 18, entailedTightenings);

      delete abs;
    }

    {
      unsigned b = 1;
      unsigned f = 4;
      AbsoluteValueConstraint *abs = new AbsoluteValueConstraint(b, f);

      InputQuery ipq;
      ipq.setNumberOfVariables(5);

      TS_ASSERT_THROWS_NOTHING(abs->transformToUseAuxVariables(ipq));

      List<Tightening> entailedTightenings;

      // 3 < b < 4, 1 < f < 2
      abs->notifyUpperBound(b, 4);
      abs->notifyUpperBound(f, 2);
      abs->notifyLowerBound(b, 3);
      abs->notifyLowerBound(f, 1);
      abs->getEntailedTightenings(entailedTightenings);
      assert_lower_upper_bound(f, b, 1, 3, 2, 2, entailedTightenings);

      delete abs;
    }

    {
      unsigned b = 1;
      unsigned f = 4;
      AbsoluteValueConstraint *abs = new AbsoluteValueConstraint(b, f);

      InputQuery ipq;
      ipq.setNumberOfVariables(5);

      TS_ASSERT_THROWS_NOTHING(abs->transformToUseAuxVariables(ipq));

      List<Tightening> entailedTightenings;
      List<Tightening>::iterator it;

      abs->notifyUpperBound(b, 7);
      abs->notifyUpperBound(f, 6);
      abs->notifyLowerBound(b, 0);
      abs->notifyLowerBound(f, 0);

      // 0 < x_b < 7 ,0 < x_f < 6
      abs->getEntailedTightenings(entailedTightenings);
      assert_lower_upper_bound(f, b, 0, 0, 6, 6, entailedTightenings);

      abs->notifyUpperBound(b, 5);
      // 0 < x_b < 5 ,0 < x_f < 6
      entailedTightenings.clear();
      abs->getEntailedTightenings(entailedTightenings);
      assert_lower_upper_bound(f, b, 0, 0, 5, 5, entailedTightenings);

      abs->notifyLowerBound(b, 1);
      // 1 < x_b < 5 ,0 < x_f < 6
      entailedTightenings.clear();
      abs->getEntailedTightenings(entailedTightenings);
      assert_lower_upper_bound(f, b, 0, 1, 5, 5, entailedTightenings);

      abs->notifyUpperBound(f, 4);
      // 1 < x_b < 5 ,0 < x_f < 4
      entailedTightenings.clear();
      abs->getEntailedTightenings(entailedTightenings);
      assert_lower_upper_bound(f, b, 0, 1, 4, 4, entailedTightenings);

      // Non overlap
      abs->notifyUpperBound(f, 2);
      abs->notifyLowerBound(b, 3);

      // 3 < x_b < 5 ,0 < x_f < 2
      entailedTightenings.clear();
      abs->getEntailedTightenings(entailedTightenings);
      TS_ASSERT_EQUALS(entailedTightenings.size(), 8U);
      assert_lower_upper_bound(f, b, 0, 3, 2, 2, entailedTightenings);
      TS_ASSERT(abs->phaseFixed());
      delete abs;
    }

    {
      unsigned b = 1;
      unsigned f = 4;
      AbsoluteValueConstraint *abs = new AbsoluteValueConstraint(b, f);

      InputQuery ipq;
      ipq.setNumberOfVariables(5);

      TS_ASSERT_THROWS_NOTHING(abs->transformToUseAuxVariables(ipq));

      List<Tightening> entailedTightenings;

      abs->notifyUpperBound(b, 6);
      abs->notifyUpperBound(f, 5);
      abs->notifyLowerBound(b, 4);
      abs->notifyLowerBound(f, 3);

      // 4 < x_b < 6 ,3 < x_f < 5
      entailedTightenings.clear();
      abs->getEntailedTightenings(entailedTightenings);
      assert_lower_upper_bound(b, f, 4, 3, 5, 5, entailedTightenings);
      TS_ASSERT(abs->phaseFixed());
      delete abs;
    }

    {
      unsigned b = 1;
      unsigned f = 4;
      AbsoluteValueConstraint *abs = new AbsoluteValueConstraint(b, f);

      List<Tightening> entailedTightenings;
      List<Tightening>::iterator it;

      abs->notifyLowerBound(b, 5);
      abs->notifyUpperBound(b, 10);
      abs->notifyLowerBound(f, -1);
      abs->notifyUpperBound(f, 10);

      TS_ASSERT_EQUALS(abs->getAllCases().size(), 1u);
      abs->getEntailedTightenings(entailedTightenings);

      assert_tightenings_match(entailedTightenings,
                               List<Tightening>({
                                   Tightening(b, 5, Tightening::LB),
                                   Tightening(b, 10, Tightening::UB),
                                   Tightening(f, 0, Tightening::LB),
                                   Tightening(f, 10, Tightening::UB),
                               }));
      TS_ASSERT(abs->phaseFixed());
      delete abs;
    }

    {
      unsigned b = 1;
      unsigned f = 4;
      AbsoluteValueConstraint *abs = new AbsoluteValueConstraint(b, f);

      List<Tightening> entailedTightenings;
      List<Tightening>::iterator it;

      abs->notifyLowerBound(b, -6);
      abs->notifyUpperBound(b, 3);
      abs->notifyLowerBound(f, 2);
      abs->notifyUpperBound(f, 4);

      // -6 < x_b < 3 ,2 < x_f < 4
      abs->getEntailedTightenings(entailedTightenings);

      assert_tightenings_match(entailedTightenings,
                               List<Tightening>({
                                   Tightening(b, -4, Tightening::LB),
                                   Tightening(b, 3, Tightening::UB),
                                   Tightening(f, 2, Tightening::LB),
                                   Tightening(f, 4, Tightening::UB),
                               }));
      TS_ASSERT(!abs->phaseFixed());

      entailedTightenings.clear();

      // -6 < x_b < 2 ,2 < x_f < 4
      abs->notifyUpperBound(b, 2);
      abs->getEntailedTightenings(entailedTightenings);

      assert_tightenings_match(entailedTightenings,
                               List<Tightening>({
                                   Tightening(b, -4, Tightening::LB),
                                   Tightening(b, 2, Tightening::UB),
                                   Tightening(f, 2, Tightening::LB),
                                   Tightening(f, 4, Tightening::UB),
                               }));

      TS_ASSERT(!abs->phaseFixed());

      entailedTightenings.clear();

      // -6 < x_b < 1 ,2 < x_f < 4, now stuck in negative phase
      abs->notifyUpperBound(b, 1);
      abs->getEntailedTightenings(entailedTightenings);

      assert_tightenings_match(entailedTightenings,
                               List<Tightening>({
                                   Tightening(b, -4, Tightening::LB),
                                   Tightening(b, 1, Tightening::UB),
                                   Tightening(f, 2, Tightening::LB),
                                   Tightening(f, 4, Tightening::UB),
                               }));

      TS_ASSERT(abs->phaseFixed());
      delete abs;
    }

    {
      unsigned b = 1;
      unsigned f = 4;

      AbsoluteValueConstraint abs(b, f);
      List<Tightening> entailedTightenings;
      List<Tightening>::iterator it;

      abs.notifyLowerBound(b, -5);
      abs.notifyUpperBound(b, 10);
      abs.notifyLowerBound(f, 3);
      abs.notifyUpperBound(f, 7);

      // -5 < x_b < 7 ,3 < x_f < 7
      abs.getEntailedTightenings(entailedTightenings);
      assert_tightenings_match(entailedTightenings,
                               List<Tightening>({
                                   Tightening(b, -5, Tightening::LB),
                                   Tightening(b, 7, Tightening::UB),
                                   Tightening(f, 3, Tightening::LB),
                                   Tightening(f, 7, Tightening::UB),
                               }));

      TS_ASSERT(!abs.phaseFixed());
      entailedTightenings.clear();

      // -5 < x_b < 7 ,6 < x_f < 7, positive phase
      abs.notifyLowerBound(f, 6);

      abs.getEntailedTightenings(entailedTightenings);
      assert_tightenings_match(entailedTightenings,
                               List<Tightening>({
                                   Tightening(b, -5, Tightening::LB),
                                   Tightening(b, 7, Tightening::UB),
                                   Tightening(f, 6, Tightening::LB),
                                   Tightening(f, 7, Tightening::UB),
                               }));

      TS_ASSERT(abs.phaseFixed());
      entailedTightenings.clear();

      // -5 < x_b < 3 ,6 < x_f < 7

      // Extreme case, disjoint ranges

      abs.notifyUpperBound(b, 3);

      abs.getEntailedTightenings(entailedTightenings);
      assert_tightenings_match(entailedTightenings,
                               List<Tightening>({
                                   Tightening(b, -5, Tightening::LB),
                                   Tightening(b, 3, Tightening::UB),
                                   Tightening(f, 6, Tightening::LB),
                                   Tightening(f, 5, Tightening::UB),
                               }));
    }

    {
      unsigned b = 1;
      unsigned f = 4;

      AbsoluteValueConstraint abs(b, f);
      List<Tightening> entailedTightenings;
      List<Tightening>::iterator it;

      abs.notifyLowerBound(b, -1);
      abs.notifyUpperBound(b, 1);
      abs.notifyLowerBound(f, 2);
      abs.notifyUpperBound(f, 4);

      // -1 < x_b < 1 ,2 < x_f < 4
      abs.getEntailedTightenings(entailedTightenings);
      assert_tightenings_match(entailedTightenings,
                               List<Tightening>({
                                   Tightening(b, -1, Tightening::LB),
                                   Tightening(b, 1, Tightening::UB),
                                   Tightening(f, 2, Tightening::LB),
                                   Tightening(f, 1, Tightening::UB),
                               }));
    }

    {
      unsigned b = 1;
      unsigned f = 4;

      AbsoluteValueConstraint abs(b, f);
      List<Tightening> entailedTightenings;
      List<Tightening>::iterator it;

      abs.notifyLowerBound(b, -7);
      abs.notifyUpperBound(b, 7);
      abs.notifyLowerBound(f, 0);
      abs.notifyUpperBound(f, 6);

      // -7 < x_b < 7 ,0 < x_f < 6
      abs.getEntailedTightenings(entailedTightenings);
      assert_tightenings_match(entailedTightenings,
                               List<Tightening>({
                                   Tightening(b, -6, Tightening::LB),
                                   Tightening(b, 6, Tightening::UB),
                                   Tightening(f, 0, Tightening::LB),
                                   Tightening(f, 6, Tightening::UB),
                               }));

      entailedTightenings.clear();

      // -7 < x_b < 5 ,0 < x_f < 6
      abs.notifyUpperBound(b, 5);
      abs.getEntailedTightenings(entailedTightenings);
      assert_tightenings_match(entailedTightenings,
                               List<Tightening>({
                                   Tightening(b, -6, Tightening::LB),
                                   Tightening(b, 5, Tightening::UB),
                                   Tightening(f, 0, Tightening::LB),
                                   Tightening(f, 6, Tightening::UB),
                               }));

      entailedTightenings.clear();
      // 0 < x_b < 5 ,0 < x_f < 6
      abs.notifyLowerBound(b, 0);
      abs.getEntailedTightenings(entailedTightenings);
      assert_tightenings_match(entailedTightenings,
                               List<Tightening>({
                                   Tightening(b, 0, Tightening::LB),
                                   Tightening(b, 5, Tightening::UB),
                                   Tightening(f, 0, Tightening::LB),
                                   Tightening(f, 6, Tightening::UB),
                               }));

      TS_ASSERT(abs.phaseFixed());
      entailedTightenings.clear();

      // 3 < x_b < 5 ,0 < x_f < 6
      abs.notifyLowerBound(b, 3);
      abs.getEntailedTightenings(entailedTightenings);
      assert_tightenings_match(entailedTightenings,
                               List<Tightening>({
                                   Tightening(b, 3, Tightening::LB),
                                   Tightening(b, 5, Tightening::UB),
                                   Tightening(f, 0, Tightening::LB),
                                   Tightening(f, 6, Tightening::UB),
                               }));
    }

    {
      unsigned b = 1;
      unsigned f = 4;

      AbsoluteValueConstraint abs(b, f);
      List<Tightening> entailedTightenings;
      List<Tightening>::iterator it;

      abs.notifyLowerBound(b, -7);
      abs.notifyUpperBound(b, 7);
      abs.notifyLowerBound(f, 0);
      abs.notifyUpperBound(f, 6);

      // -7 < x_b < 7 ,0 < x_f < 6
      abs.getEntailedTightenings(entailedTightenings);
      assert_tightenings_match(entailedTightenings,
                               List<Tightening>({
                                   Tightening(b, -6, Tightening::LB),
                                   Tightening(b, 6, Tightening::UB),
                                   Tightening(f, 0, Tightening::LB),
                                   Tightening(f, 6, Tightening::UB),
                               }));

      entailedTightenings.clear();

      // -7 < x_b < 5 ,0 < x_f < 6
      abs.notifyUpperBound(b, 5);
      abs.getEntailedTightenings(entailedTightenings);
      assert_tightenings_match(entailedTightenings,
                               List<Tightening>({
                                   Tightening(b, -6, Tightening::LB),
                                   Tightening(b, 5, Tightening::UB),
                                   Tightening(f, 0, Tightening::LB),
                                   Tightening(f, 6, Tightening::UB),
                               }));

      entailedTightenings.clear();
      // 0 < x_b < 5 ,0 < x_f < 6
      abs.notifyLowerBound(b, 0);
      abs.getEntailedTightenings(entailedTightenings);
      assert_tightenings_match(entailedTightenings,
                               List<Tightening>({
                                   Tightening(b, 0, Tightening::LB),
                                   Tightening(b, 5, Tightening::UB),
                                   Tightening(f, 0, Tightening::LB),
                                   Tightening(f, 6, Tightening::UB),
                               }));

      TS_ASSERT(abs.phaseFixed());

      entailedTightenings.clear();

      // 3 < x_b < 5 ,0 < x_f < 6
      abs.notifyLowerBound(b, 3);
      abs.getEntailedTightenings(entailedTightenings);
      assert_tightenings_match(entailedTightenings,
                               List<Tightening>({
                                   Tightening(b, 3, Tightening::LB),
                                   Tightening(b, 5, Tightening::UB),
                                   Tightening(f, 0, Tightening::LB),
                                   Tightening(f, 6, Tightening::UB),
                               }));
      TS_ASSERT(abs.phaseFixed());
    }

    {
      unsigned b = 1;
      unsigned f = 4;

      AbsoluteValueConstraint abs(b, f);
      List<Tightening> entailedTightenings;
      List<Tightening>::iterator it;

      abs.notifyLowerBound(b, -20);
      abs.notifyUpperBound(b, -2);
      abs.notifyLowerBound(f, 0);
      abs.notifyUpperBound(f, 15);

      // -20 < x_b < -2 ,0 < x_f < 15
      abs.getEntailedTightenings(entailedTightenings);
      assert_tightenings_match(entailedTightenings,
                               List<Tightening>({
                                   Tightening(b, -15, Tightening::LB),
                                   Tightening(b, -2, Tightening::UB),
                                   Tightening(f, 0, Tightening::LB),
                                   Tightening(f, 15, Tightening::UB),
                               }));

      TS_ASSERT(abs.phaseFixed());

      entailedTightenings.clear();

      // -20 < x_b < -2 ,7 < x_f < 15
      abs.notifyLowerBound(f, 7);
      abs.getEntailedTightenings(entailedTightenings);
      assert_tightenings_match(entailedTightenings,
                               List<Tightening>({
                                   Tightening(b, -15, Tightening::LB),
                                   Tightening(b, -7, Tightening::UB),
                                   Tightening(f, 7, Tightening::LB),
                                   Tightening(f, 15, Tightening::UB),
                               }));

      TS_ASSERT(abs.phaseFixed());

      entailedTightenings.clear();

      // -12 < x_b < -2 ,7 < x_f < 15
      abs.notifyLowerBound(b, -12);
      abs.getEntailedTightenings(entailedTightenings);
      assert_tightenings_match(entailedTightenings,
                               List<Tightening>({
                                   Tightening(b, -12, Tightening::LB),
                                   Tightening(b, -7, Tightening::UB),
                                   Tightening(f, 7, Tightening::LB),
                                   Tightening(f, 12, Tightening::UB),
                               }));

      entailedTightenings.clear();

      // -12 < x_b < -8 ,7 < x_f < 15
      abs.notifyUpperBound(b, -8);
      abs.getEntailedTightenings(entailedTightenings);
      assert_tightenings_match(entailedTightenings,
                               List<Tightening>({
                                   Tightening(b, -12, Tightening::LB),
                                   Tightening(b, -8, Tightening::UB),
                                   Tightening(f, 7, Tightening::LB),
                                   Tightening(f, 12, Tightening::UB),
                               }));
    }

    {
      unsigned b = 1;
      unsigned f = 4;

      AbsoluteValueConstraint abs(b, f);
      List<Tightening> entailedTightenings;
      List<Tightening>::iterator it;

      abs.notifyLowerBound(b, -20);
      abs.notifyUpperBound(b, -2);
      abs.notifyLowerBound(f, 25);
      abs.notifyUpperBound(f, 30);

      // -20 < x_b < -2 ,25 < x_f < 30
      abs.getEntailedTightenings(entailedTightenings);
      assert_tightenings_match(entailedTightenings,
                               List<Tightening>({
                                   Tightening(b, -20, Tightening::LB),
                                   Tightening(b, -25, Tightening::UB),
                                   Tightening(f, 25, Tightening::LB),
                                   Tightening(f, 30, Tightening::UB),
                               }));

      TS_ASSERT(abs.phaseFixed());
    }
  }

  void assert_lower_upper_bound_bm(unsigned f, unsigned b, double fLower,
                                   double bLower, double fUpper, double bUpper,
                                   BoundManager &boundManager) {
    TS_ASSERT_EQUALS(boundManager.getLowerBound(b), bLower);
    TS_ASSERT_EQUALS(boundManager.getLowerBound(f), fLower);
    TS_ASSERT_EQUALS(boundManager.getUpperBound(b), bUpper);
    TS_ASSERT_EQUALS(boundManager.getUpperBound(f), fUpper);
  }

  void test_notify_bounds_during_search() {
    {
      unsigned b = 1;
      unsigned f = 4;
      // unsigned posAux = 5;
      // unsigned negAux = 6;

      AbsoluteValueConstraint *abs = new AbsoluteValueConstraint(b, f);

      InputQuery ipq;
      ipq.setNumberOfVariables(5);

      TS_ASSERT_THROWS_NOTHING(abs->transformToUseAuxVariables(ipq));

      CVC4::context::Context context;
      TS_ASSERT_THROWS_NOTHING(abs->initializeCDOs(&context));
      BoundManager boundManager(context);
      abs->registerBoundManager(&boundManager);
      boundManager.initialize(7);

      abs->notifyLowerBound(b, 1);
      abs->notifyLowerBound(f, 2);
      abs->notifyUpperBound(b, 7);
      abs->notifyUpperBound(f, 7);

      TS_ASSERT(abs->phaseFixed());

      // 1 < x_b < 7 , 2 < x_f < 7 -> 2 < x_b
      // 0 <= posAux <= 0, 0 <= negAux < 14
      assert_lower_upper_bound_bm(f, b, 2, 1, 7, 7, boundManager);

      abs->notifyLowerBound(b, 3);
      assert_lower_upper_bound_bm(f, b, 2, 3, 7, 7, boundManager);

      abs->notifyLowerBound(f, 3);
      abs->notifyUpperBound(b, 6);
      assert_lower_upper_bound_bm(f, b, 3, 3, 6, 6, boundManager);

      abs->notifyUpperBound(f, 6);
      abs->notifyUpperBound(b, 7);
      assert_lower_upper_bound_bm(f, b, 3, 3, 6, 6, boundManager);

      abs->notifyLowerBound(f, -3);
      assert_lower_upper_bound_bm(f, b, 3, 3, 6, 6, boundManager);

      abs->notifyLowerBound(b, 5);
      abs->notifyUpperBound(f, 5);
      // 5 < x_b < 6 , 3 < x_f < 5
      assert_lower_upper_bound_bm(f, b, 3, 5, 5, 5, boundManager);

      delete abs;
    }

    {
      unsigned b = 1;
      unsigned f = 4;
      // unsigned posAux = 5;
      // unsigned negAux = 6;

      AbsoluteValueConstraint *abs = new AbsoluteValueConstraint(b, f);

      InputQuery ipq;
      ipq.setNumberOfVariables(5);

      TS_ASSERT_THROWS_NOTHING(abs->transformToUseAuxVariables(ipq));

      CVC4::context::Context context;
      TS_ASSERT_THROWS_NOTHING(abs->initializeCDOs(&context));
      BoundManager boundManager(context);
      abs->registerBoundManager(&boundManager);
      boundManager.initialize(7);

      // 8 < b < 18, 48 < f < 64
      abs->notifyUpperBound(b, 18);
      abs->notifyUpperBound(f, 64);
      abs->notifyLowerBound(b, 8);
      abs->notifyLowerBound(f, 48);
      assert_lower_upper_bound_bm(f, b, 48, 8, 64, 18, boundManager);

      delete abs;
    }

    {
      unsigned b = 1;
      unsigned f = 4;
      // unsigned posAux = 5;
      // unsigned negAux = 6;

      AbsoluteValueConstraint *abs = new AbsoluteValueConstraint(b, f);

      InputQuery ipq;
      ipq.setNumberOfVariables(5);

      TS_ASSERT_THROWS_NOTHING(abs->transformToUseAuxVariables(ipq));

      CVC4::context::Context context;
      TS_ASSERT_THROWS_NOTHING(abs->initializeCDOs(&context));
      BoundManager boundManager(context);
      abs->registerBoundManager(&boundManager);
      boundManager.initialize(7);

      // 3 < b < 4, 1 < f < 2
      abs->notifyUpperBound(b, 4);
      abs->notifyUpperBound(f, 2);
      assert_lower_upper_bound_bm(f, b, FloatUtils::negativeInfinity(), -2, 2,
                                  2, boundManager);
      TS_ASSERT_THROWS_ANYTHING(abs->notifyLowerBound(b, 3));

      delete abs;
    }

    {
      unsigned b = 1;
      unsigned f = 4;
      // unsigned posAux = 5;
      // unsigned negAux = 6;

      AbsoluteValueConstraint *abs = new AbsoluteValueConstraint(b, f);

      InputQuery ipq;
      ipq.setNumberOfVariables(5);

      TS_ASSERT_THROWS_NOTHING(abs->transformToUseAuxVariables(ipq));

      CVC4::context::Context context;
      TS_ASSERT_THROWS_NOTHING(abs->initializeCDOs(&context));
      BoundManager boundManager(context);
      abs->registerBoundManager(&boundManager);
      boundManager.initialize(7);

      abs->notifyUpperBound(b, 7);
      abs->notifyUpperBound(f, 6);
      abs->notifyLowerBound(b, 0);
      abs->notifyLowerBound(f, 0);

      // 0 < x_b < 7 ,0 < x_f < 6
      assert_lower_upper_bound_bm(f, b, 0, 0, 6, 6, boundManager);
      TS_ASSERT(abs->phaseFixed());

      abs->notifyUpperBound(b, 5);
      // 0 < x_b < 5 ,0 < x_f < 6
      assert_lower_upper_bound_bm(f, b, 0, 0, 5, 5, boundManager);

      abs->notifyLowerBound(b, 1);
      // 1 < x_b < 5 ,0 < x_f < 6
      assert_lower_upper_bound_bm(f, b, 0, 1, 5, 5, boundManager);

      abs->notifyUpperBound(f, 4);
      // 1 < x_b < 5 ,0 < x_f < 4
      assert_lower_upper_bound_bm(f, b, 0, 1, 4, 4, boundManager);

      // Non overlap
      TS_ASSERT_THROWS_NOTHING(abs->notifyUpperBound(f, 2));
      TS_ASSERT_THROWS_ANYTHING(abs->notifyLowerBound(b, 3));

      // 3 < x_b < 5 ,0 < x_f < 2
      delete abs;
    }

    {
      unsigned b = 1;
      unsigned f = 4;
      // unsigned posAux = 5;
      // unsigned negAux = 6;

      AbsoluteValueConstraint *abs = new AbsoluteValueConstraint(b, f);

      InputQuery ipq;
      ipq.setNumberOfVariables(5);

      TS_ASSERT_THROWS_NOTHING(abs->transformToUseAuxVariables(ipq));

      CVC4::context::Context context;
      TS_ASSERT_THROWS_NOTHING(abs->initializeCDOs(&context));
      BoundManager boundManager(context);
      abs->registerBoundManager(&boundManager);
      boundManager.initialize(7);

      abs->notifyUpperBound(b, 6);
      abs->notifyUpperBound(f, 5);
      abs->notifyLowerBound(b, 4);
      abs->notifyLowerBound(f, 3);

      // 4 < x_b < 6 ,3 < x_f < 5
      assert_lower_upper_bound_bm(b, f, 4, 3, 5, 5, boundManager);
      TS_ASSERT(abs->phaseFixed());
      delete abs;
    }

    {
      unsigned b = 1;
      unsigned f = 4;
      // unsigned posAux = 5;
      // unsigned negAux = 6;

      AbsoluteValueConstraint *abs = new AbsoluteValueConstraint(b, f);

      InputQuery ipq;
      ipq.setNumberOfVariables(5);

      TS_ASSERT_THROWS_NOTHING(abs->transformToUseAuxVariables(ipq));

      CVC4::context::Context context;
      TS_ASSERT_THROWS_NOTHING(abs->initializeCDOs(&context));
      BoundManager boundManager(context);
      abs->registerBoundManager(&boundManager);
      boundManager.initialize(7);

      abs->notifyLowerBound(b, 5);
      abs->notifyUpperBound(b, 10);
      abs->notifyLowerBound(f, -1);
      abs->notifyUpperBound(f, 10);

      TS_ASSERT_EQUALS(abs->getAllCases().size(), 2u);
      TS_ASSERT(abs->phaseFixed());
      delete abs;
    }

    {
      unsigned b = 1;
      unsigned f = 4;
      // unsigned posAux = 5;
      // unsigned negAux = 6;

      AbsoluteValueConstraint *abs = new AbsoluteValueConstraint(b, f);

      InputQuery ipq;
      ipq.setNumberOfVariables(5);

      TS_ASSERT_THROWS_NOTHING(abs->transformToUseAuxVariables(ipq));

      CVC4::context::Context context;
      TS_ASSERT_THROWS_NOTHING(abs->initializeCDOs(&context));
      BoundManager boundManager(context);
      abs->registerBoundManager(&boundManager);
      boundManager.initialize(7);

      abs->notifyLowerBound(b, -6);
      abs->notifyUpperBound(b, 3);
      abs->notifyLowerBound(f, 2);
      abs->notifyUpperBound(f, 4);

      // -6 < x_b < 3 ,2 < x_f < 4
      TS_ASSERT(!abs->phaseFixed());

      // -6 < x_b < 2 ,2 < x_f < 4
      abs->notifyUpperBound(b, 2);
      TS_ASSERT(!abs->phaseFixed());

      // -6 < x_b < 1 ,2 < x_f < 4, now stuck in negative phase
      abs->notifyUpperBound(b, 1);
      TS_ASSERT(abs->phaseFixed());
      delete abs;
    }

    {
      unsigned b = 1;
      unsigned f = 4;
      // unsigned posAux = 5;
      // unsigned negAux = 6;

      AbsoluteValueConstraint abs = AbsoluteValueConstraint(b, f);

      InputQuery ipq;
      ipq.setNumberOfVariables(5);

      TS_ASSERT_THROWS_NOTHING(abs.transformToUseAuxVariables(ipq));

      CVC4::context::Context context;
      TS_ASSERT_THROWS_NOTHING(abs.initializeCDOs(&context));
      BoundManager boundManager(context);
      abs.registerBoundManager(&boundManager);
      boundManager.initialize(7);

      abs.notifyLowerBound(b, -5);
      abs.notifyUpperBound(b, 10);
      abs.notifyLowerBound(f, 3);
      abs.notifyUpperBound(f, 7);

      TS_ASSERT(!abs.phaseFixed());

      // -5 < x_b < 7 ,6 < x_f < 7, positive phase
      abs.notifyLowerBound(f, 6);

      TS_ASSERT(abs.phaseFixed());

      // -5 < x_b < 3 ,6 < x_f < 7

      // Extreme case, disjoint ranges

      TS_ASSERT_THROWS_ANYTHING(abs.notifyUpperBound(b, 3));
    }

    {
      unsigned b = 1;
      unsigned f = 4;
      // unsigned posAux = 5;
      // unsigned negAux = 6;

      AbsoluteValueConstraint abs = AbsoluteValueConstraint(b, f);

      InputQuery ipq;
      ipq.setNumberOfVariables(5);

      TS_ASSERT_THROWS_NOTHING(abs.transformToUseAuxVariables(ipq));

      CVC4::context::Context context;
      TS_ASSERT_THROWS_NOTHING(abs.initializeCDOs(&context));
      BoundManager boundManager(context);
      abs.registerBoundManager(&boundManager);
      boundManager.initialize(7);

      abs.notifyLowerBound(b, -1);
      abs.notifyUpperBound(b, 1);
      TS_ASSERT_THROWS_ANYTHING(abs.notifyLowerBound(f, 2));
    }

    {
      unsigned b = 1;
      unsigned f = 4;
      // unsigned posAux = 5;
      // unsigned negAux = 6;

      AbsoluteValueConstraint abs = AbsoluteValueConstraint(b, f);

      InputQuery ipq;
      ipq.setNumberOfVariables(5);

      TS_ASSERT_THROWS_NOTHING(abs.transformToUseAuxVariables(ipq));

      CVC4::context::Context context;
      TS_ASSERT_THROWS_NOTHING(abs.initializeCDOs(&context));
      BoundManager boundManager(context);
      abs.registerBoundManager(&boundManager);
      boundManager.initialize(7);

      abs.notifyLowerBound(b, -7);
      abs.notifyUpperBound(b, 7);
      abs.notifyLowerBound(f, 0);
      abs.notifyUpperBound(f, 6);

      TS_ASSERT(!abs.phaseFixed());

      context.push();
      // -7 < x_b < 5 ,0 < x_f < 6
      abs.notifyUpperBound(b, 5);
      // 0 < x_b < 5 ,0 < x_f < 6
      abs.notifyLowerBound(b, 0);

      TS_ASSERT(abs.phaseFixed());
      abs.notifyLowerBound(b, 3);
      TS_ASSERT(abs.phaseFixed());

      context.pop();
      TS_ASSERT(!abs.phaseFixed());
    }

    {
      unsigned b = 1;
      unsigned f = 4;
      // unsigned posAux = 5;
      // unsigned negAux = 6;

      AbsoluteValueConstraint abs = AbsoluteValueConstraint(b, f);

      InputQuery ipq;
      ipq.setNumberOfVariables(5);

      TS_ASSERT_THROWS_NOTHING(abs.transformToUseAuxVariables(ipq));

      CVC4::context::Context context;
      TS_ASSERT_THROWS_NOTHING(abs.initializeCDOs(&context));
      BoundManager boundManager(context);
      abs.registerBoundManager(&boundManager);
      boundManager.initialize(7);

      abs.notifyLowerBound(b, -7);
      abs.notifyUpperBound(b, 7);
      abs.notifyLowerBound(f, 0);
      abs.notifyUpperBound(f, 6);

      // -7 < x_b < 7 ,0 < x_f < 6

      context.push();

      abs.notifyUpperBound(b, 5);
      TS_ASSERT(!abs.phaseFixed());
      context.push();
      // 0 < x_b < 5 ,0 < x_f < 6
      abs.notifyLowerBound(b, 0);
      TS_ASSERT(abs.phaseFixed());

      // 3 < x_b < 5 ,0 < x_f < 6
      abs.notifyLowerBound(b, 3);
      TS_ASSERT(abs.phaseFixed());
    }

    {
      unsigned b = 1;
      unsigned f = 4;
      // unsigned posAux = 5;
      // unsigned negAux = 6;

      AbsoluteValueConstraint abs = AbsoluteValueConstraint(b, f);

      InputQuery ipq;
      ipq.setNumberOfVariables(5);

      TS_ASSERT_THROWS_NOTHING(abs.transformToUseAuxVariables(ipq));

      CVC4::context::Context context;
      TS_ASSERT_THROWS_NOTHING(abs.initializeCDOs(&context));
      BoundManager boundManager(context);
      abs.registerBoundManager(&boundManager);
      boundManager.initialize(7);

      abs.notifyLowerBound(b, -20);
      abs.notifyUpperBound(b, -2);
      abs.notifyLowerBound(f, 0);
      abs.notifyUpperBound(f, 15);

      // -20 < x_b < -2 ,0 < x_f < 15
      TS_ASSERT(abs.phaseFixed());

      context.push();
      // -20 < x_b < -2 ,7 < x_f < 15
      abs.notifyLowerBound(f, 7);
      TS_ASSERT(abs.phaseFixed());

      abs.notifyLowerBound(b, -12);

      // -12 < x_b < -8 ,7 < x_f < 15
      abs.notifyUpperBound(b, -8);
    }

    {
      unsigned b = 1;
      unsigned f = 4;
      // unsigned posAux = 5;
      // unsigned negAux = 6;

      AbsoluteValueConstraint abs = AbsoluteValueConstraint(b, f);

      InputQuery ipq;
      ipq.setNumberOfVariables(5);

      TS_ASSERT_THROWS_NOTHING(abs.transformToUseAuxVariables(ipq));

      CVC4::context::Context context;
      TS_ASSERT_THROWS_NOTHING(abs.initializeCDOs(&context));
      BoundManager boundManager(context);
      abs.registerBoundManager(&boundManager);
      boundManager.initialize(7);

      abs.notifyLowerBound(b, -20);
      abs.notifyUpperBound(b, -2);
      TS_ASSERT(abs.phaseFixed());
      TS_ASSERT_THROWS_ANYTHING(abs.notifyLowerBound(f, 25));
    }
  }

  void test_case_splits() {
    unsigned b = 1;
    unsigned f = 4;
    AbsoluteValueConstraint *abs = new AbsoluteValueConstraint(b, f);
    TS_ASSERT_THROWS_ANYTHING(abs->getCaseSplit(PHASE_NOT_FIXED));
    TS_ASSERT(!abs->phaseFixed());

    CVC4::context::Context context;
    TS_ASSERT_THROWS_NOTHING(abs->initializeCDOs(&context));

    TS_ASSERT_THROWS_ANYTHING(abs->getCaseSplit(PHASE_NOT_FIXED));

    InputQuery ipq;
    ipq.setNumberOfVariables(5);
    TS_ASSERT_THROWS_NOTHING(abs->transformToUseAuxVariables(ipq));

    PiecewiseLinearCaseSplit splitPos;
    splitPos.storeBoundTightening(Tightening(1, 0.0, Tightening::LB));
    splitPos.storeBoundTightening(Tightening(5, 0.0, Tightening::UB));
    PiecewiseLinearCaseSplit splitNeg;
    splitNeg.storeBoundTightening(Tightening(1, 0.0, Tightening::UB));
    splitNeg.storeBoundTightening(Tightening(6, 0.0, Tightening::UB));

    TS_ASSERT_EQUALS(abs->getCaseSplit(ABS_PHASE_POSITIVE), splitPos);
    TS_ASSERT_EQUALS(abs->getCaseSplit(ABS_PHASE_NEGATIVE), splitNeg);

    context.push();
    TS_ASSERT_THROWS_NOTHING(abs->setPhaseStatus(ABS_PHASE_POSITIVE));
    TS_ASSERT(abs->phaseFixed());
    TS_ASSERT_EQUALS(abs->getValidCaseSplit(), splitPos);

    context.pop();
    TS_ASSERT(!abs->phaseFixed());
    TS_ASSERT_EQUALS(abs->getValidCaseSplit(), PiecewiseLinearCaseSplit());

    delete abs;
  }

  void test_satisfied() {
    CVC4::context::Context context;
    BoundManager bm(context);
    AssignmentManager am(bm);
    AbsoluteValueConstraint *abs = new AbsoluteValueConstraint(0, 1);
    abs->registerAssignmentManager(&am);

    bm.initialize(2);
    am.initialize();

    am.setAssignment(0, 0.5);
    am.setAssignment(1, 1);
    TS_ASSERT(!abs->satisfied());

    am.setAssignment(0, -1);
    am.setAssignment(1, -1);
    TS_ASSERT(!abs->satisfied());

    am.setAssignment(0, 1);
    am.setAssignment(1, -1);
    TS_ASSERT(!abs->satisfied());

    am.setAssignment(0, 0);
    am.setAssignment(1, 0);
    TS_ASSERT(abs->satisfied());

    am.setAssignment(0, 1);
    am.setAssignment(1, 1);
    TS_ASSERT(abs->satisfied());

    am.setAssignment(0, -1);
    am.setAssignment(1, 1);
    TS_ASSERT(abs->satisfied());

    delete abs;
  }

  void test_soi() {
    unsigned b = 1;
    unsigned f = 4;
    AbsoluteValueConstraint *abs = new AbsoluteValueConstraint(b, f);
    CVC4::context::Context context;
    TS_ASSERT_THROWS_NOTHING(abs->initializeCDOs(&context));
    TS_ASSERT(abs->supportSoI());

    LinearExpression cost1;
    LinearExpression costResult;
    TS_ASSERT_THROWS_NOTHING(
        abs->getCostFunctionComponent(cost1, ABS_PHASE_NEGATIVE));
    costResult._addends[1] = 1;
    costResult._addends[4] = 1;
    TS_ASSERT_EQUALS(cost1, costResult);

    LinearExpression cost2;
    TS_ASSERT_THROWS_NOTHING(
        abs->getCostFunctionComponent(cost2, ABS_PHASE_POSITIVE));
    costResult._addends[1] = -1;
    costResult._addends[4] = 1;
    TS_ASSERT_EQUALS(cost2, costResult);

    context.push();
    // If the phase is infeasible, do not add to the cost.
    TS_ASSERT_THROWS_NOTHING(abs->markInfeasiblePhase(ABS_PHASE_POSITIVE));
    TS_ASSERT_THROWS_NOTHING(
        abs->getCostFunctionComponent(cost2, ABS_PHASE_POSITIVE));
    TS_ASSERT_EQUALS(cost2, costResult);

    context.pop();
    TS_ASSERT_THROWS_NOTHING(
        abs->getCostFunctionComponent(cost1, ABS_PHASE_POSITIVE));
    costResult._addends[1] = 0;
    costResult._addends[4] = 2;
    TS_ASSERT_EQUALS(cost1, costResult);

    BoundManager bm(context);
    bm.initialize(5);
    bm.setLowerBound(1, -5);
    bm.setUpperBound(1, 5);
    bm.setLowerBound(4, 0);
    bm.setUpperBound(4, 5);

    AssignmentManager assignmentManager(bm);
    abs->registerAssignmentManager(&assignmentManager);

    assignmentManager.setAssignment(1, 0.1);
    assignmentManager.setAssignment(4, 0.4);
    TS_ASSERT_EQUALS(abs->getPhaseStatusInAssignment(), ABS_PHASE_POSITIVE);

    assignmentManager.setAssignment(1, -0.1);
    assignmentManager.setAssignment(4, 0.4);
    TS_ASSERT_EQUALS(abs->getPhaseStatusInAssignment(), ABS_PHASE_NEGATIVE);

    delete abs;
  }
};
