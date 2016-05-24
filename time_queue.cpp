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

#include "time_queue.h"
#include "creature.h"

template <class Archive> 
void TimeQueue::serialize(Archive& ar, const unsigned int version) { 
  serializeAll(ar, creatures, queue);
}

SERIALIZABLE(TimeQueue);

TimeQueue::TimeQueue() : queue([](const Creature* c1, const Creature* c2) {
    return c1->getLocalTime() < c2->getLocalTime() ||
    (c1->getLocalTime() == c2->getLocalTime() && c1->getUniqueId() > c2->getUniqueId());}) {
}

void TimeQueue::addCreature(PCreature c) {
  queue.insert(c.get());
  creatures.push_back(std::move(c));
}
  
PCreature TimeQueue::removeCreature(Creature* cRef) {
  for (int i : All(creatures))
    if (creatures[i].get() == cRef) {
      queue.erase(cRef);
      PCreature ret = std::move(creatures[i]);
      creatures.erase(creatures.begin() + i);
      return ret;
    }
  FAIL << "Creature not found";
  return nullptr;
}

vector<Creature*> TimeQueue::getAllCreatures() const {
  return extractRefs(creatures);
}

Creature* TimeQueue::getNextCreature() {
  if (creatures.empty())
    return nullptr;
  else
    return *queue.begin();
}

void TimeQueue::beforeUpdateTime(Creature* c) {
  queue.erase(c);
}

void TimeQueue::afterUpdateTime(Creature* c) {
  queue.insert(c);
}

