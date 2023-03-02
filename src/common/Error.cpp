/*********************                                                        */
/*! \file Error.cpp
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

#include "Error.h"

#include <stdlib.h>
#include <string.h>

#include "T/Errno.h"

Error::Error(const char *errorClass, int code) : _code(code) {
  memset(_errorClass, 0, sizeof(_userMessage));
  memset(_userMessage, 0, sizeof(_userMessage));

  strncpy(_errorClass, errorClass, BUFFER_SIZE - 1);
  _errno = T::errorNumber();
}

Error::Error(const char *errorClass, int code, const char *userMessage)
    : _code(code) {
  memset(_errorClass, 0, sizeof(_userMessage));
  memset(_userMessage, 0, sizeof(_userMessage));

  strncpy(_errorClass, errorClass, BUFFER_SIZE - 1);
  setUserMessage(userMessage);

  _errno = T::errorNumber();
}

int Error::getErrno() const { return _errno; }

int Error::getCode() const { return _code; }

void Error::setUserMessage(const char *userMessage) {
  strncpy(_userMessage, userMessage, BUFFER_SIZE - 1);
}

const char *Error::getErrorClass() const { return _errorClass; }

const char *Error::getUserMessage() const { return _userMessage; }
