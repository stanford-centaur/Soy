/*********************                                                        */
/*! \file InputParserError.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz, Rachel Lim
 ** This file is part of the Soy project.
 ** Copyright (c) 2023 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

 **/

#ifndef __InputParserError_h__
#define __InputParserError_h__

#include "Error.h"

class InputParserError : public Error {
 public:
  enum Code {
    VARIABLE_INDEX_OUT_OF_RANGE = 0,
    UNEXPECTED_INPUT = 1,
    FILE_DOESNT_EXIST = 2,
    UNSUPPORTED_BOUND_TYPE = 3,
    MULTIPLE_OBJECTIVES = 4,
    UNSUPPORT_PIECEWISE_LINEAR_CONSTRAINT = 5,
  };

  InputParserError(InputParserError::Code code)
      : Error("InputParserError", (int)code) {}

  InputParserError(InputParserError::Code code, const char *userMessage)
      : Error("InputParserError", (int)code, userMessage) {}
};

#endif  // __InputParserError_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
