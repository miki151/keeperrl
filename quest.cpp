/* Copyright (C) 2013-2014 Michal Brzozowski (rusolis@poczta.fm)

   This file is part of KeeperRL.

   KeeperRL is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   KeeperRL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

#include "stdafx.h"

#include "quest.h"
#include "event.h"
#include "tribe.h"
#include "message_buffer.h"
#include "location.h"
#include "creature.h"

template <class Archive> 
void Quest::serialize(Archive& ar, const unsigned int version) {
  ar& SUBCLASS(Singleton)
    & SVAR(adventurers)
    & SVAR(location)
    & SVAR(startMessage);
  CHECK_SERIAL;
}

SERIALIZABLE(Quest);
SERIALIZATION_CONSTRUCTOR_IMPL(Quest);

Quest::Quest(const string& message) : startMessage(message) {}

class KillTribeQuest : public Quest {
  public:
  KillTribeQuest(Tribe* _tribe, string msg, bool onlyImp = false)
      : Quest(msg), tribe(_tribe), onlyImportant(onlyImp) {
  }

  virtual bool isFinished() const override {
    if (onlyImportant)
      return NOTNULL(tribe->getLeader())->isDead();
    for (const Creature* c : tribe->getMembers())
      if (!c->isDead())
        return false;
    return true;
  }

  REGISTER_HANDLER(KillEvent,const Creature* member, const Creature* attacker) {
    if (isFinished() && !notified) {
      notified = true;
      for (auto c : adventurers) {
        c->playerMessage(MessageBuffer::important("Your quest is finished."));
      }
    }
  }

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar& SUBCLASS(Quest)
      & SVAR(tribe)
      & SVAR(onlyImportant)
      & SVAR(notified);
    CHECK_SERIAL;
  }

  SERIALIZATION_CONSTRUCTOR(KillTribeQuest);

  private:
  Tribe* SERIAL(tribe);
  bool SERIAL(onlyImportant);
  bool SERIAL2(notified, false);
};

template <class Archive>
void Quest::registerTypes(Archive& ar) {
  REGISTER_TYPE(ar, KillTribeQuest);
}

REGISTER_TYPES(Quest);

Quest* Quest::killTribeQuest(Tribe* tribe, string message, bool onlyImp) {
  return new KillTribeQuest(tribe, message, onlyImp);
}

void Quest::addAdventurer(Creature* c) {
  string text = MessageBuffer::important(startMessage);
  if (location) {
    text += " You need to head " + 
      getCardinalName((location->getBounds().middle() - c->getPosition()).getBearing().getCardinalDir()) + 
      " to find it.";
    c->learnLocation(location);
  }
  c->playerMessage(text);
  adventurers.insert(c);
}

void Quest::setLocation(const Location* loc) {
  CHECK(location == nullptr) << "Attempted to set quest location for a second time";
  location = loc;
}

