#ifndef __Conflict_h__
#define __Conflict_h__

#include "Debug.h"
#include "Map.h"
#include "SoyError.h"
#include "PLConstraint.h"
#include "Vector.h"

// Struct representing a Conflict, which is a map from PLConstraint to phase.
// We require different phases of the same PLConstraint to be mutually exclusive
// so a conflict never contains two phases of the same PLConstraint.
struct Conflict {
  Conflict() {}

  void addLiteral(PLConstraint *constraint, PhaseStatus phase) {
    if (_literals.exists(constraint))
      throw SoyError(
          SoyError::CONFLICT_HAS_PHASES_OF_SAME_PLCONSTRAINT);
    _literals[constraint] = phase;
    _constraints.append(constraint);
  }

  Map<PLConstraint *, PhaseStatus> _literals;
  Vector<PLConstraint *> _constraints;
};

#endif  // __Conflict_h__
