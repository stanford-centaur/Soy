#ifndef __MockEngine_h__
#define __MockEngine_h__

#include "Engine.h"

using CVC4::context::Context;

class MockEngine : public Engine {
 public:
  PiecewiseLinearCaseSplit _lastSplitApplied;

  virtual void applySplit(const PiecewiseLinearCaseSplit &split) override {
    _lastSplitApplied = split;
  }

  PLConstraint *_constraintToSplit;
  virtual PLConstraint *pickSplitPLConstraint(
      DivideStrategy /*strategy*/) override {
    return _constraintToSplit;
  }

  virtual void postContextPopHook() override {}
  virtual void preContextPushHook() override {}
};

#endif  // __MockEngine_h__
