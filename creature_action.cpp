#include "stdafx.h"
#include "creature_action.h"

CreatureAction::CreatureAction(WConstCreature c, ActionFun f) : action(f), performer(c) {
}

CreatureAction::CreatureAction(CreatureAction::ActionFun f) : action(f) {
}

CreatureAction::CreatureAction(const string& msg)
    : action(nullptr), failedMessage(msg) {
}

void CreatureAction::perform(WCreature c) {
  CHECK(action);
  CHECK(!performer || c == performer);
  action(c);
}

CreatureAction CreatureAction::prepend(ActionFun f) {
  CHECK(action);
  ActionFun tmp = action;
  action = [=] (WCreature c) { f(c); tmp(c); };
  return *this;
}

CreatureAction CreatureAction::append(ActionFun f) {
  CHECK(action);
  ActionFun tmp = action;
  action = [=] (WCreature c) { tmp(c); f(c); };
  return *this;
}

string CreatureAction::getFailedReason() const {
  return failedMessage;
}

CreatureAction::operator bool() const {
  return action != nullptr;
}


