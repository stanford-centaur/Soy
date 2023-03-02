#ifndef __IntegerConstraint_h__
#define __IntegerConstraint_h__

#include "LinearExpression.h"
#include "Map.h"
#include "PLConstraint.h"

class IntegerConstraint : public PLConstraint {
 public:
  /**********************************************************************/
  /*                           CONS/DESTRUCTION METHODS                 */
  /**********************************************************************/
  IntegerConstraint(unsigned variable);
  ~IntegerConstraint();
  virtual void addBooleanStructure() override;
  PLConstraint *duplicateConstraint() const override;

  /**********************************************************************/
  /*                           GENERAL METHODS                          */
  /**********************************************************************/
  PiecewiseLinearFunctionType getType() const override;
  bool participatingVariable(unsigned variable) const override;
  List<unsigned> getParticipatingVariables() const override;

  unsigned getVariable() const;

  virtual bool satisfied() const override;
  virtual bool supportCaseSplit() const { return false; };

  void setPhaseStatus(PhaseStatus) {
    throw SoyError(SoyError::FEATURE_NOT_YET_SUPPORTED,
                       "Trying to set phase status of IntegerConstraint");
  }

  virtual bool hasFeasiblePhases() const {
    ASSERT(_boundManager);
    double lb = _boundManager->getLowerBound(_variable);
    double ub = _boundManager->getUpperBound(_variable);
    return FloatUtils::lte(FloatUtils::roundUp(lb), FloatUtils::roundDown(ub));
  }

  virtual bool isFeasible(PhaseStatus) const override {
    throw SoyError(SoyError::FEATURE_NOT_YET_SUPPORTED,
                       "Trying to check phase status of IntegerConstraint");
  }

  unsigned numberOfFeasiblePhases() const override {
    double lb = _boundManager->getLowerBound(_variable);
    double ub = _boundManager->getUpperBound(_variable);
    if (FloatUtils::lt(ub, lb))
      return 0;
    else
      return (unsigned)(FloatUtils::roundDown(ub) - FloatUtils::roundUp(lb) +
                        1);
  }

  virtual void markInfeasiblePhase(PhaseStatus) {
    throw SoyError(SoyError::FEATURE_NOT_YET_SUPPORTED,
                       "Trying to mark phase status of IntegerConstraint");
  }

  PiecewiseLinearCaseSplit getCaseSplitForInt(int value) const;

  virtual PiecewiseLinearCaseSplit getValidCaseSplit() const override {
    ASSERT(_context);
    double lb = FloatUtils::roundUp(_boundManager->getLowerBound(_variable));
    double ub = FloatUtils::roundDown(_boundManager->getUpperBound(_variable));
    if (FloatUtils::areEqual(lb, ub))
      return getCaseSplitForInt(lb);
    else if (FloatUtils::lt(lb, ub))
      return PiecewiseLinearCaseSplit();
    else
      throw SoyError(SoyError::REQUESTED_NONEXISTENT_CASE_SPLIT,
                         "No feasible case split left.");
  }

  virtual List<PhaseStatus> getAllCases() const override {
    return List<PhaseStatus>();
  };

  /**********************************************************************/
  /*                             SoI METHODS                            */
  /**********************************************************************/
 public:
  virtual inline bool supportSoI() const override { return false; }

  /**********************************************************************/
  /*                         BOUND WRAPPER METHODS                      */
  /**********************************************************************/
  /* These methods prefer using BoundManager over local bound arrays.   */

 public:
  virtual void notifyLowerBound(unsigned variable, double bound) override;
  virtual void notifyUpperBound(unsigned variable, double bound) override;
  virtual void getEntailedTightenings(
      List<Tightening> &tightenings) const override;

  /**********************************************************************/
  /*                             DEBUG METHODS                          */
  /**********************************************************************/
  virtual void dump(String &) const override;

 private:
  unsigned _variable;
};

#endif  // __IntegerConstraint_h__
