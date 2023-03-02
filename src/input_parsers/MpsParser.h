#ifndef __MpsParser_h__
#define __MpsParser_h__

#include "Equation.h"
#include "File.h"
#include "Map.h"
#include "Set.h"

#define MPS_LOG(x, ...) \
  LOG(GlobalConfiguration::MPS_PARSER_LOGGING, "MpsParser: %s\n", x)

class InputQuery;
class String;

class MpsParser {
 public:
  enum RowType {
    EQ = 0,
    LE,
    GE,
    OBJ,
  };

  MpsParser(const String &path);

  // Extract an input query from the input file
  void generateQuery(InputQuery &inputQuery);

  // Getters
  unsigned getNumVars() const;
  unsigned getNumEquations() const;
  String getEquationName(unsigned index) const;
  String getVarName(unsigned index) const;
  double getUpperBound(unsigned index) const;
  double getLowerBound(unsigned index) const;

  Map<String, unsigned> getVariableNameToVariableIndex() const;

 private:
  // Helpers for parsing the various section of the file
  void parse(const String &path);
  void parseRow(const String &line);
  void parseColumn(const String &line, bool &markingInteger);
  void parseRhs(const String &line);
  void parseBounds(const String &line);
  void setRemainingBounds();
  void parseGeneralConstraint(const String &line, File &file);

  // Helpers for preparing the input query
  void populateBounds(InputQuery &inputQuery);
  void populateEquations(InputQuery &inputQuery);
  void populateEquation(Equation &equation, unsigned index, bool &isOneHot,
                        bool &isDisjunct) const;
  void addPLConstraints(InputQuery &inputQuery) const;

  static unsigned variableToStep(String name);

  // Number of equations and variables
  unsigned _numRows;
  unsigned _numVars;
  // The row of the objective function.
  int _indexOfObjective;

  // Various mappings
  Map<String, unsigned> _equationNameToIndex;
  Map<unsigned, String> _equationIndexToName;
  Map<unsigned, unsigned> _equationIndexToRowType;
  Map<unsigned, Map<unsigned, double>> _equationIndexToCoefficients;
  Map<unsigned, double> _equationIndexToRhs;
  Map<String, unsigned> _variableNameToIndex;
  Map<unsigned, String> _variableIndexToName;
  Map<unsigned, double> _varToUpperBounds;
  Map<unsigned, double> _varToLowerBounds;

  // Piecewise-linear constraints
  Set<unsigned> _integerVariables;

  // From input variable to output variable of the absolute function
  Map<unsigned, unsigned> _absoluteValues;
};

#endif  // __MpsParser_h__
