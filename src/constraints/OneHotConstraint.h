#ifndef __OneHotConstraint_h__
#define __OneHotConstraint_h__

#include "LinearExpression.h"
#include "Map.h"
#include "PLConstraint.h"

class OneHotConstraint : public PLConstraint {
 public:
  /**********************************************************************/
  /*                           CONS/DESTRUCTION METHODS                 */
  /**********************************************************************/
  OneHotConstraint(const Set<unsigned> &elements);
  ~OneHotConstraint();
  PLConstraint *duplicateConstraint() const override;
  virtual void addBooleanStructure() override;

  /**********************************************************************/
  /*                           GENERAL METHODS                          */
  /**********************************************************************/
  PiecewiseLinearFunctionType getType() const override;
  bool participatingVariable(unsigned variable) const override;
  List<unsigned> getParticipatingVariables() const override;

  virtual void notifyLowerBound(unsigned variable, double value) override;
  virtual void notifyUpperBound(unsigned variable, double value) override;
  virtual void notifyBVariable(int lit, bool value) override;
  virtual bool satisfied() const override;
  bool satisfied(const Map<unsigned, double> &assignment) const;
  const Set<unsigned> &getElements() const { return _elements; };
  PhaseStatus getPhaseOfElement(unsigned variable) const {
    return _elementToPhaseStatus[variable];
  }
  unsigned getElementOfPhase(PhaseStatus phase) const {
    return _phaseStatusToElement[phase];
  }

  virtual List<PhaseStatus> getAllCases() const override;
  virtual PiecewiseLinearCaseSplit getCaseSplit(
      PhaseStatus phase) const override;
  virtual void getEntailedTightenings(
      List<Tightening> &tightenings) const override;

 private:
  void eliminateElement(unsigned variable);

  /**********************************************************************/
  /*                             SoI METHODS                            */
  /**********************************************************************/
 public:
  virtual inline bool supportSoI() const override { return true; }
  virtual void getCostFunctionComponent(LinearExpression &cost,
                                        PhaseStatus phase) const override;
  virtual PhaseStatus getPhaseStatusInAssignment() override;

  /**********************************************************************/
  /*                             DEBUG METHODS                          */
  /**********************************************************************/
  virtual void dump(String &) const override;

 private:
  Set<unsigned> _elements;
  Map<unsigned, PhaseStatus> _elementToPhaseStatus;
  Map<PhaseStatus, unsigned> _phaseStatusToElement;
};

#endif  // __OneHotConstraint_h__
