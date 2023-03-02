#ifndef __Watcher_h__
#define __Watcher_h__

#include "List.h"
#include "Map.h"

class Watchee {
 public:
  virtual List<unsigned> getParticipatingVariables() const {
    return List<unsigned>();
  }
  virtual void notifyLowerBound(unsigned /*variable*/, double /*value*/) {}
  virtual void notifyUpperBound(unsigned /*variable*/, double /*value*/) {}
  virtual void notifyBVariable(int /*bVariable*/, bool /*value*/) {}
};

class Watcher {
 public:
  void registerToWatchAllVariables(Watchee *watchee) {
    for (const auto &var : watchee->getParticipatingVariables()) {
      registerToWatchVariable(watchee, var);
    }
  }
  void registerToWatchVariable(Watchee *watchee, unsigned variable) {
    _variableToWatchees[variable].append(watchee);
  }

  void unregisterToWatchVariable(Watchee *watchee, unsigned variable) {
    _variableToWatchees[variable].erase(watchee);
  }

  void registerToWatchBVariable(Watchee *watchee, int bVariable) {
    _bVariableToWatchees[bVariable].append(watchee);
  }
  void unregisterToWatchBVariable(Watchee *watchee, int bVariable) {
    _bVariableToWatchees[bVariable].erase(watchee);
  }

 protected:
  Map<unsigned, List<Watchee *>> _variableToWatchees;
  Map<int, List<Watchee *>> _bVariableToWatchees;
};

#endif  // __Watcher_h__
