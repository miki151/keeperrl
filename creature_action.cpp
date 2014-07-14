#include "stdafx.h"
#include "creature_action.h"

CreatureAction::CreatureAction(ActionFun f) : action(f) {
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
    : action(a.action), failedMessage(a.failedMessage), wasUsed(true) {
  a.wasUsed = true;
}
#endif

void CreatureAction::perform() {
  CHECK(action);
  action();
#ifndef RELEASE
  wasUsed = true;
#endif
}

CreatureAction CreatureAction::prepend(ActionFun f) {
  CHECK(action);
  ActionFun tmp = action;
  action = [=] { f(); tmp(); };
  return *this;
}

CreatureAction CreatureAction::append(ActionFun f) {
  CHECK(action);
  ActionFun tmp = action;
  action = [=] { tmp(); f(); };
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


