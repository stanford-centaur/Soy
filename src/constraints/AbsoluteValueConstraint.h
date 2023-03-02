#ifndef __AbsoluteValueConstraint_h__
#define __AbsoluteValueConstraint_h__

#include "PLConstraint.h"

class AbsoluteValueConstraint : public PLConstraint {
  /**********************************************************************/
  /*                           CONS/DESTRUCTION METHODS                 */
  /**********************************************************************/
 public:
  /*
    The f variable is the absolute value of the b variable:
    f = | b |
  */
  AbsoluteValueConstraint(unsigned b, unsigned f);

  virtual void addBooleanStructure() override;
  virtual PLConstraint *duplicateConstraint() const;
  virtual void transformToUseAuxVariables(InputQuery &) override;

  /**********************************************************************/
  /*                           GENERAL METHODS                          */
  /**********************************************************************/
 public:
  virtual PiecewiseLinearFunctionType getType() const override {
    return PiecewiseLinearFunctionType::ABSOLUTE_VALUE;
  };

  unsigned getB() const { return _b; };
  unsigned getF() const { return _f; };
  unsigned getPosAux() const { return _posAux; };
  unsigned getNegAux() const { return _negAux; };
  bool auxVariablesInUse() const { return _auxVarsInUse; };

  virtual bool participatingVariable(unsigned variable) const override;
  virtual List<unsigned> getParticipatingVariables() const override;

  virtual bool satisfied() const override;
  virtual void setPhaseStatus(PhaseStatus phase) override;
  virtual bool phaseFixed() const override;
  virtual List<PhaseStatus> getAllCases() const override;
  virtual PiecewiseLinearCaseSplit getCaseSplit(
      PhaseStatus caseId) const override;

 private:
  unsigned _b, _f;
  unsigned _posAux, _negAux;
  bool _auxVarsInUse;
  bool _haveEliminatedVariables;

  void fixPhaseIfNeeded();
  PiecewiseLinearCaseSplit getPositiveSplit() const;
  PiecewiseLinearCaseSplit getNegativeSplit() const;

  /**********************************************************************/
  /*                             SoI METHODS                            */
  /**********************************************************************/
 public:
  virtual bool supportSoI() const { return true; };

  virtual void getCostFunctionComponent(LinearExpression & /* cost */,
                                        PhaseStatus /* phase */) const override;

  virtual PhaseStatus getPhaseStatusInAssignment() override;

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
 public:
  virtual void dump(String &) const override;

 private:
  static String phaseToString(PhaseStatus phase);
};

#endif  // __AbsoluteValueConstraint_h__
