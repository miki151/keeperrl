#pragma once

#include "util.h"
#include "t_string.h"

class Creature;

class NODISCARD CreatureAction {
  public:
  typedef function<void(Creature*)> ActionFun;

  CreatureAction(const Creature*, ActionFun);
  CreatureAction(ActionFun);
  CreatureAction();
  CreatureAction(TString failedReason);
  CreatureAction prepend(ActionFun);
  CreatureAction append(ActionFun);
  CreatureAction prepend(CreatureAction);
  CreatureAction append(CreatureAction);
  void perform(Creature*);
  const optional<TString>& getFailedReason() const;
  explicit operator bool() const;

  private:
  ActionFun action;
  optional<TString> failedMessage;
  const Creature* performer = nullptr;
};


