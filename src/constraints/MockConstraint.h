#ifndef __MockConstraint_h__
#define __MockConstraint_h__

#include "PLConstraint.h"

class MockConstraint : public PLConstraint {
  /**********************************************************************/
  /*                           CONS/DESTRUCTION METHODS                 */
  /**********************************************************************/
 public:
  MockConstraint(unsigned numCases) : PLConstraint(), _numCases(numCases) {}

  unsigned _numCases;

  virtual ~MockConstraint() {}

  virtual void addBooleanStructure(){};

  virtual MockConstraint *duplicateConstraint() const override { return NULL; };

  /**********************************************************************/
  /*                           GENERAL METHODS                          */
  /**********************************************************************/
  virtual PiecewiseLinearFunctionType getType() const override {
    return (PiecewiseLinearFunctionType)999;
  };

  virtual bool participatingVariable(unsigned) const override { return false; };
  virtual List<unsigned> getParticipatingVariables() const override {
    return List<unsigned>();
  };

  virtual bool satisfied() const override { return false; };

  virtual List<PhaseStatus> getAllCases() const override {
    List<PhaseStatus> cases;
    for (unsigned i = 0; i < _numCases; ++i)
      cases.append(static_cast<PhaseStatus>(i));
    return cases;
  }

  /**********************************************************************/
  /*                         BOUND WRAPPER METHODS                      */
  /**********************************************************************/
  /* These methods prefer using BoundManager over local bound arrays.   */

 public:
  virtual void notifyLowerBound(unsigned, double){};
  virtual void notifyUpperBound(unsigned, double){};
  virtual void getEntailedTightenings(List<Tightening> &) const {};
};

#endif  // __MockConstraint_h__
