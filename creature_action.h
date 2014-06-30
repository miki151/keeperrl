#ifndef _CREATURE_ACTION_H
#define _CREATURE_ACTION_H

#include "util.h"

class CreatureAction {
  public:
  CreatureAction(function<void()>);
  CreatureAction(const string& failedReason);
#ifndef RELEASE
  // This stuff is so that you don't forget to perform() an action or check if it failed
  static void checkUsage(bool);
  CreatureAction(const CreatureAction&);
  ~CreatureAction();
#endif
  CreatureAction prepend(function<void()>);
  CreatureAction append(function<void()>);
  void perform();
  string getFailedReason() const;
  operator bool() const;

  private:
  function<void()> action;
  function<void()> before;
  function<void()> after;
  string failedMessage;
#ifndef RELEASE
  mutable bool wasUsed = false;
#endif
};


#endif
