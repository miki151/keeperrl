#pragma once

#include "util.h"

class Creature;

class NODISCARD CreatureAction {
  public:
  typedef function<void(Creature*)> ActionFun;

  CreatureAction(const Creature*, ActionFun);
  CreatureAction(const string& failedReason = "");
  CreatureAction prepend(ActionFun);
  CreatureAction append(ActionFun);
  void perform(Creature*);
  string getFailedReason() const;
  operator bool() const;

  private:
  ActionFun action;
  string failedMessage;
  const Creature* performer;
};


