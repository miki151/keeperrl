#include "stdafx.h"
#include "creature_action.h"

CreatureAction::CreatureAction(function<void()> f) : action(f) {
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

CreatureAction::CreatureAction(const CreatureAction& a) : action(a.action), before(a.before), after(a.after),
  failedMessage(a.failedMessage), wasUsed(true) {
  a.wasUsed = true;
}
#endif

void CreatureAction::perform() {
  if (before)
    before();
  CHECK(action);
  action();
  if (after)
    after();
#ifndef RELEASE
  wasUsed = true;
#endif
}

CreatureAction CreatureAction::prepend(function<void()> f) {
  if (!before)
    before = f;
  else
    before = [=] { f(); before(); };
  return *this;
}

CreatureAction CreatureAction::append(function<void()> f) {
  if (!after)
    after = f;
  else
    after = [=] { after(); f(); };
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


