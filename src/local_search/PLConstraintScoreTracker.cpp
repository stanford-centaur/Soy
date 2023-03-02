/*********************                                                        */
/*! \file PLConstraintScoreTracker.cpp
 ** \verbatim
 ** Top contributors (to current version):
 **   Haoze Wu
 ** This file is part of the Soy project.
 ** Copyright (c) 2023 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#include "PLConstraintScoreTracker.h"

void PLConstraintScoreTracker::reset() {
  _scores.clear();
  _plConstraintToScore.clear();
  _plConstraints.clear();
}

void PLConstraintScoreTracker::initialize(
    const List<PLConstraint *> &plConstraints) {
  reset();
  for (const auto &constraint : plConstraints) {
    _scores.insert({constraint, 0});
    _plConstraintToScore[constraint] = 0;
  }
  _plConstraints = plConstraints;
}

void PLConstraintScoreTracker::decayScores() {
  for (const auto &constraint : _plConstraints) {
    double oldScore = _plConstraintToScore[constraint];
    double newScore = oldScore / 10;
    _scores.erase(ScoreEntry(constraint, oldScore));
    _scores.insert(ScoreEntry(constraint, newScore));
    _plConstraintToScore[constraint] = newScore;
  }
}

void PLConstraintScoreTracker::setScore(PLConstraint *constraint,
                                        double score) {
  ASSERT(_plConstraintToScore.exists(constraint));

  double oldScore = _plConstraintToScore[constraint];

  ASSERT(_scores.find(ScoreEntry(constraint, oldScore)) != _scores.end());

  _scores.erase(ScoreEntry(constraint, oldScore));
  _scores.insert(ScoreEntry(constraint, score));
  _plConstraintToScore[constraint] = score;
}

PLConstraint *PLConstraintScoreTracker::topUnfixed() {
  for (const auto &entry : _scores) {
    if (entry._constraint->isActive() && !entry._constraint->phaseFixed()) {
      SCORE_TRACKER_LOG(
          Stringf("Score of top unfixed plConstraint: %.2f", entry._score)
              .ascii());
      return entry._constraint;
    }
  }
  return NULL;
}
