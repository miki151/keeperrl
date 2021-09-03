#include "stdafx.h"
#include "creature_action.h"

CreatureAction::CreatureAction(const Creature* c, ActionFun f) : action(f), performer(c) {
}

CreatureAction::CreatureAction(CreatureAction::ActionFun f) : action(f) {
}

CreatureAction::CreatureAction(const string& msg)
    : action(nullptr), failedMessage(msg) {
}

void CreatureAction::perform(Creature* c) {
  CHECK(action);
  CHECK(!performer || c == performer);
  action(c);
}

CreatureAction CreatureAction::prepend(CreatureAction f) {
  return prepend(f.action);
}

CreatureAction CreatureAction::append(CreatureAction f) {
  return append(f.action);
}

CreatureAction CreatureAction::prepend(ActionFun f) {
  if (!!action) {
    ActionFun tmp = action;
    action = [=] (Creature* c) { f(c); tmp(c); };
  }
  return *this;
}

CreatureAction CreatureAction::append(ActionFun f) {
  if (!!action) {
    ActionFun tmp = action;
    action = [=] (Creature* c) { tmp(c); f(c); };
  }
  return *this;
}

string CreatureAction::getFailedReason() const {
  return failedMessage;
}

CreatureAction::operator bool() const {
  return action != nullptr;
}


