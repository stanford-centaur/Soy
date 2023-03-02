

#ifndef __InputQuery_h__
#define __InputQuery_h__

#include "Equation.h"
#include "List.h"
#include "MString.h"
#include "Map.h"
#include "PLConstraint.h"

class InputQuery {
 public:
  /**********************************************************************/
  /*                           CONS/DESTRUCTION METHODS                 */
  /**********************************************************************/

  InputQuery();
  ~InputQuery();

  /*
    Assignment operator and copy constructor, duplicate the constraints.
  */
  InputQuery &operator=(const InputQuery &other);
  InputQuery(const InputQuery &other);
  bool operator==(const InputQuery &ipq2);

  /**********************************************************************/
  /*                         Methods to add constraints                 */
  /**********************************************************************/
  void setNumberOfVariables(unsigned numberOfVariables);
  void setLowerBound(unsigned variable, double bound);
  void setUpperBound(unsigned variable, double bound);

  unsigned getNumberOfVariables() const;
  double getLowerBound(unsigned variable) const;
  double getUpperBound(unsigned variable) const;
  const Map<unsigned, double> &getLowerBounds() const;
  const Map<unsigned, double> &getUpperBounds() const;
  void clearBounds();

  void addEquation(const Equation &equation);
  void removeEquation(const Equation &equation);
  const List<Equation> &getEquations() const;
  List<Equation> &getEquations();

  void addPLConstraint(PLConstraint *constraint);
  const List<PLConstraint *> &getPLConstraints() const;
  List<PLConstraint *> &getPLConstraints();

  void markVariableToStep(unsigned variable, unsigned step);
  unsigned getStepOfVariable(unsigned variable) const;
  Set<unsigned> getVariablesOfStep(unsigned step) const;
  List<unsigned> getStepsOfPLConstraint(const PLConstraint *constraint) const;

  /*
    Methods for setting and getting the solution.
  */
  void setSolutionValue(unsigned variable, double value);
  double getSolutionValue(unsigned variable) const;

  /**********************************************************************/
  /*                         Helper functions                           */
  /**********************************************************************/
  bool equationBelongsToStep(const Equation &equation,
                             const List<unsigned> &steps) const;

  bool constraintBelongsToStep(const PLConstraint *constraint,
                               const List<unsigned> &steps) const;

  /**********************************************************************/
  /*                             Debug Methods                          */
  /**********************************************************************/
  void dump() const;
  void printAllBounds() const;
  unsigned countInfiniteBounds();
  void checkSolutionCompliance(const Map<unsigned, double> &assignment) const;

 private:
  unsigned _numberOfVariables;
  List<Equation> _equations;
  Map<unsigned, double> _lowerBounds;
  Map<unsigned, double> _upperBounds;
  List<PLConstraint *> _plConstraints;
  Map<unsigned, unsigned> _variableToStep;
  Map<unsigned, Set<unsigned>> _stepToVariables;

  Map<unsigned, double> _solution;

  /*
    Free any stored pl constraints.
  */
  void freeConstraintsIfNeeded();
};

#endif  // __InputQuery_h__
