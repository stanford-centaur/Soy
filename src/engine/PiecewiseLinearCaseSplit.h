#ifndef __PiecewiseLinearCaseSplit_h__
#define __PiecewiseLinearCaseSplit_h__

#include "MString.h"
#include "Pair.h"
#include "Tightening.h"

class PiecewiseLinearCaseSplit {
 public:
  /*
    Store information regarding a bound tightening.
  */
  void storeBoundTightening(const Tightening &tightening);
  const List<Tightening> &getBoundTightenings() const;

  /*
    Dump the case split - for debugging purposes.
  */
  void dump() const;
  void dump(String &output) const;

  /*
    Equality operator.
  */
  bool operator==(const PiecewiseLinearCaseSplit &other) const;

  /*
    Change the index of a variable that appears in this case split
  */
  void updateVariableIndex(unsigned oldIndex, unsigned newIndex);

 private:
  /*
    Bound tightening information.
  */
  List<Tightening> _bounds;
};

#endif  // __PiecewiseLinearCaseSplit_h__
