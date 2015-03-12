#include "stdafx.h"
#include "creature_action.h"

CreatureAction::CreatureAction(const Creature* c, ActionFun f) : action(f), performer(c) {
}

CreatureAction::CreatureAction(const string& msg)
    : action(nullptr), failedMessage(msg) {
}

#ifndef RELEASE
static bool usageCheck = false;

CreatureAction::~CreatureAction() {
  CHECK(wasUsed || !usageCheck);
}

void CreatureAction::checkUsage(bool b) {
  usageCheck = b;
}

CreatureAction::CreatureAction(const CreatureAction& a) 
    : action(a.action), failedMessage(a.failedMessage), performer(a.performer), wasUsed(true) {
  a.wasUsed = true;
}
#endif

void CreatureAction::perform(Creature* c) {
  CHECK(c == performer);
  CHECK(action);
  action(c);
#ifndef RELEASE
  wasUsed = true;
#endif
}

CreatureAction CreatureAction::prepend(ActionFun f) {
  CHECK(action);
  ActionFun tmp = action;
  action = [=] (Creature* c) { f(c); tmp(c); };
  return *this;
}

CreatureAction CreatureAction::append(ActionFun f) {
  CHECK(action);
  ActionFun tmp = action;
  action = [=] (Creature* c) { tmp(c); f(c); };
  return *this;
}

string CreatureAction::getFailedReason() const {
  return failedMessage;
}

CreatureAction::operator bool() const {
#ifndef RELEASE
  wasUsed = true;
#endif
  return action != nullptr;
}


