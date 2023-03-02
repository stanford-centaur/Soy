/*********************                                                        */
/*! \file MStringf.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Soy project.
 ** Copyright (c) 2023 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#ifndef __Stringf_h__
#define __Stringf_h__

#include <cstdarg>
#include <cstdio>

#include "MString.h"

class Stringf : public String {
 public:
  enum {
    MAX_STRING_LENGTH = 10000,
  };

  Stringf(const char *format, ...) {
    va_list argList;
    va_start(argList, format);

    char buffer[MAX_STRING_LENGTH];

    vsprintf(buffer, format, argList);

    va_end(argList);

    _super = Super(buffer);
  }
};

#ifdef CXXTEST_RUNNING
#include <cxxtest/ValueTraits.h>
#include <stdio.h>
namespace CxxTest {
CXXTEST_TEMPLATE_INSTANTIATION
class ValueTraits<Stringf> {
 public:
  ValueTraits(const Stringf &stringf) : _stringf(stringf) {}

  const char *asString() const { return _stringf.ascii(); }

 private:
  const Stringf &_stringf;
};
}  // namespace CxxTest

#endif  // CXXTEST_RUNNING

#endif  // __Stringf_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
