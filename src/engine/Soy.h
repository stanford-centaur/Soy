#ifndef __Soy_h__
#define __Soy_h__

#include "DnCManager.h"
#include "InputQuery.h"
#include "Options.h"

class Soy {
 public:
  Soy();

  /*
    Entry point of this class
  */
  void run();

 private:
  std::unique_ptr<DnCManager> _dncManager;
  InputQuery _inputQuery;
  /*
    Display the results
  */
  void displayResults(unsigned long long microSecondsElapsed) const;
};

#endif  // __Soy_h__
