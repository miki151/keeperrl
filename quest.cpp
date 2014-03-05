#include "stdafx.h"

#include "quest.h"
#include "event.h"
#include "tribe.h"
#include "message_buffer.h"
#include "location.h"

template <class Archive> 
void Quest::serialize(Archive& ar, const unsigned int version) {
  ar& BOOST_SERIALIZATION_NVP(adventurers)
    & BOOST_SERIALIZATION_NVP(location);
}

SERIALIZABLE(Quest);

class KillTribeQuest : public Quest, public EventListener {
  public:
  KillTribeQuest(Tribe* _tribe, string msg, bool onlyImp = false)
      : tribe(_tribe), message(msg), onlyImportant(onlyImp) {
  }

  virtual bool isFinished() const override {
    if (onlyImportant)
      return NOTNULL(tribe->getLeader())->isDead();
    for (const Creature* c : tribe->getMembers())
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

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(Quest)
      & SUBCLASS(EventListener) 
      & BOOST_SERIALIZATION_NVP(tribe)
      & BOOST_SERIALIZATION_NVP(message)
      & BOOST_SERIALIZATION_NVP(onlyImportant)
      & BOOST_SERIALIZATION_NVP(notified);
  }

  SERIALIZATION_CONSTRUCTOR(KillTribeQuest);

  private:
  Tribe* tribe;
  string message;
  bool onlyImportant;
  bool notified = false;
};

template <class Archive>
void Quest::registerTypes(Archive& ar) {
  REGISTER_TYPE(ar, KillTribeQuest);
}

REGISTER_TYPES(Quest);

Quest* Quest::killTribeQuest(Tribe* tribe, string message, bool onlyImp) {
  return new KillTribeQuest(tribe, message, onlyImp);
}

Quest* Quest::dragon;
Quest* Quest::castleCellar;
Quest* Quest::bandits;
Quest* Quest::goblins;
Quest* Quest::dwarves;
  
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

