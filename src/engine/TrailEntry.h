#ifndef __TrailEntry_h__
#define __TrailEntry_h__

#include "PLConstraint.h"

// Struct representing a subquery
struct TrailEntry {
  TrailEntry(PLConstraint *constraint, PhaseStatus phase)
      : _constraint(constraint), _phase(phase) {}

  PLConstraint *_constraint;
  PhaseStatus _phase;
};

#endif  // __TrailEntry_h__
