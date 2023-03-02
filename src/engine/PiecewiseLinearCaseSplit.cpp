#include "PiecewiseLinearCaseSplit.h"

#include <cstdio>

#include "MStringf.h"

void PiecewiseLinearCaseSplit::storeBoundTightening(
    const Tightening &tightening) {
  _bounds.append(tightening);
}

const List<Tightening> &PiecewiseLinearCaseSplit::getBoundTightenings() const {
  return _bounds;
}

void PiecewiseLinearCaseSplit::dump(String &output) const {
  output = String("\nDumping piecewise linear case split\n");
  output += String("\tBounds are:\n");
  for (const auto &bound : _bounds) {
    output += Stringf("\t\tVariable: %u. New bound: %.2lf. Bound type: %s\n",
                      bound._variable, bound._value,
                      bound._type == Tightening::LB ? "lower" : "upper");
  }
}

void PiecewiseLinearCaseSplit::dump() const {
  String output;
  dump(output);
  printf("%s", output.ascii());
}

bool PiecewiseLinearCaseSplit::operator==(
    const PiecewiseLinearCaseSplit &other) const {
  return _bounds == other._bounds;
}

void PiecewiseLinearCaseSplit::updateVariableIndex(unsigned oldIndex,
                                                   unsigned newIndex) {
  for (auto &bound : _bounds) {
    if (bound._variable == oldIndex) bound._variable = newIndex;
  }
}
