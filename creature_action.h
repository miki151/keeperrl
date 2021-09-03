#pragma once

#include "util.h"

class Creature;

class NODISCARD CreatureAction {
  public:
  typedef function<void(Creature*)> ActionFun;

  CreatureAction(const Creature*, ActionFun);
  CreatureAction(ActionFun);
  CreatureAction(const string& failedReason = "");
  CreatureAction prepend(ActionFun);
  CreatureAction append(ActionFun);
  CreatureAction prepend(CreatureAction);
  CreatureAction append(CreatureAction);
  void perform(Creature*);
  string getFailedReason() const;
  explicit operator bool() const;

  private:
  ActionFun action;
  string failedMessage;
  const Creature* performer = nullptr;
};


