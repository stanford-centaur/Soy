/*********************                                                        */
/*! \file PLConstraintScoreTracker.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Haoze Wu
 ** This file is part of the Soy project.
 ** Copyright (c) 2023 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** A general class that maintains a heap from PLConstraint to a score.

**/

#ifndef __PLConstraintScoreTracker_h__
#define __PLConstraintScoreTracker_h__

#include <set>

#include "Debug.h"
#include "List.h"
#include "MStringf.h"
#include "PLConstraint.h"

#define SCORE_TRACKER_LOG(x, ...)                 \
  LOG(GlobalConfiguration::SCORE_TRACKER_LOGGING, \
      "PLConstraintScoreTracker: %s\n", x)

struct ScoreEntry {
  ScoreEntry(PLConstraint *constraint, double score)
      : _constraint(constraint), _score(score){};

  bool operator<(const ScoreEntry &other) const {
    if (_score == other._score)
      return _constraint > other._constraint;
    else
      return _score > other._score;
  }

  PLConstraint *_constraint;
  double _score;
};

typedef std::set<ScoreEntry> Scores;

class PLConstraintScoreTracker {
 public:
  virtual ~PLConstraintScoreTracker() = default;

  /*
    Initialize the scores for all constraints to 0.
  */
  void initialize(const List<PLConstraint *> &plConstraints);

  /*
    Empty the local variables.
  */
  void reset();

  /*
    Update the score of a constraint.
  */
  virtual void updateScore(PLConstraint *constraint, double score) = 0;

  void decayScores();

  /*
    Set the score of a constraint.
  */
  void setScore(PLConstraint *constraint, double score);

  /*
    Among active and unfixed constraints, return the one with the largest
    score.
  */
  PLConstraint *topUnfixed();

  /*
    Return the constraint with the largest score.
  */
  inline PLConstraint *top() { return _scores.begin()->_constraint; }

  /*
    Get the score of the PLConstraint
  */
  inline double getScore(PLConstraint *constraint) {
    DEBUG({
      ASSERT(_plConstraintToScore.exists(constraint));
      ASSERT(_scores.find(
                 ScoreEntry(constraint, _plConstraintToScore[constraint])) !=
             _scores.end());
    });
    return _plConstraintToScore[constraint];
  }

 protected:
  Scores _scores;
  Map<PLConstraint *, double> _plConstraintToScore;
  List<PLConstraint *> _plConstraints;
};

#endif  // __PLConstraintScoreTracker_h__
