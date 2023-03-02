/*********************                                                        */
/*! \file Debug.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Duligur Ibeling
 ** This file is part of the Soy project.
 ** Copyright (c) 2023 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#ifndef __Debug_h__
#define __Debug_h__

#include <stdio.h>

#include <cstdlib>

#ifndef NDEBUG
#define DEBUG(x) x
#else
#define DEBUG(x)
#endif

#ifndef NDEBUG
#define LOG(x, f, y, ...) \
  {                       \
    if ((x)) {            \
      printf(f, y);       \
    }                     \
  }
#else
#define LOG(x, f, y, ...) \
  {}
#endif

#define TRACK(x, f, y, ...) \
  {                         \
    if ((x)) {              \
      printf(f, y);         \
    }                       \
  }

#ifndef NDEBUG
#define ASSERTM(x, y, ...) \
  {                        \
    if (!(x)) {            \
      printf(y);           \
      exit(1);             \
    }                      \
  }
#else
#define ASSERTM(x, y, ...)
#endif

#ifndef NDEBUG
#define ASSERT(x)                                                            \
  {                                                                          \
    if (!(x)) {                                                              \
      printf("Assertion violation! File %s, line %d\n", __FILE__, __LINE__); \
      exit(1);                                                               \
    }                                                                        \
  }
#else
#define ASSERT(x)
#endif

#endif  // __Debug_h__
