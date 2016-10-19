#pragma once

#include "util.h"

class Creature;

class CreatureAction {
  public:
  typedef function<void(Creature*)> ActionFun;

  CreatureAction(const Creature*, ActionFun);
  CreatureAction(const string& failedReason = "");
#ifndef RELEASE
  // This stuff is so that you don't forget to perform() an action or check if it failed
  static void checkUsage(bool);
  CreatureAction(const CreatureAction&);
  ~CreatureAction();
#endif
  CreatureAction prepend(ActionFun);
  CreatureAction append(ActionFun);
  void perform(Creature*);
  string getFailedReason() const;
  operator bool() const;

  private:
  ActionFun action;
  string failedMessage;
  const Creature* performer;
#ifndef RELEASE
  mutable bool wasUsed = false;
#endif
};


