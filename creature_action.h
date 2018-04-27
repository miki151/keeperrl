#pragma once

#include "util.h"

class Creature;

class NODISCARD CreatureAction {
  public:
  typedef function<void(WCreature)> ActionFun;

  CreatureAction(WConstCreature, ActionFun);
  CreatureAction(ActionFun);
  CreatureAction(const string& failedReason = "");
  CreatureAction prepend(ActionFun);
  CreatureAction append(ActionFun);
  void perform(WCreature);
  string getFailedReason() const;
  explicit operator bool() const;

  private:
  ActionFun action;
  string failedMessage;
  WConstCreature performer;
};


