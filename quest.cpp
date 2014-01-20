#include "stdafx.h"

#include "quest.h"
#include "event.h"
#include "tribe.h"
#include "message_buffer.h"

using namespace std;

class KillTribeQuest : public Quest, public EventListener {
  public:
  KillTribeQuest(Tribe* _tribe, string msg, bool onlyImp = false)
      : tribe(_tribe), message(msg), onlyImportant(onlyImp) {
    EventListener::addListener(this);
  }

  virtual bool isFinished() const override {
    for (const Creature* c : (onlyImportant ? tribe->getImportantMembers() : tribe->getMembers()))
      if (!c->isDead())
        return false;
    return true;
  }

  virtual void onKillEvent(const Creature* member, const Creature* attacker) {
    if (isFinished() && !notified) {
      notified = true;
      for (auto c : adventurers)
        c->privateMessage(MessageBuffer::important("Your quest is finished."));
    }
  }

  virtual string getMessage() const override {
    return message;
  }

  private:
  Tribe* tribe;
  string message;
  bool onlyImportant;
  bool notified = false;
};

Quest* Quest::killTribeQuest(Tribe* tribe, string message, bool onlyImp) {
  return new KillTribeQuest(tribe, message, onlyImp);
}

Quest* Quest::dragon;
Quest* Quest::castleCellar;
Quest* Quest::bandits;

void Quest::initialize() {
}
  
void Quest::addAdventurer(const Creature* c) {
  adventurers.insert(c);
}

void Quest::setLocation(const Location* loc) {
  CHECK(location == nullptr) << "Attempted to set quest location for a second time";
  location = loc;
}

const Location* Quest::getLocation() const {
  return location;
}

