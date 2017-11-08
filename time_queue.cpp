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
#include "view_object.h"

template <class Archive> 
void TimeQueue::serialize(Archive& ar, const unsigned int version) { 
  ar(creatures, timeMap, queue);
}

SERIALIZABLE(TimeQueue);

void TimeQueue::addCreature(PCreature c, LocalTime time) {
  timeMap.set(c.get(), time);
  queue[time].push_back(c.get());
  creatures.push_back(std::move(c));
}

LocalTime TimeQueue::getTime(WConstCreature c) {
  return timeMap.getOrFail(c);
}

void TimeQueue::eraseFromQueue(deque<WCreature>& q, WCreature c) {
  for (int i : All(q))
    if (q[i] == c) {
      q[i] = nullptr;
      break;
    }
}

void TimeQueue::increaseTime(WCreature c, TimeInterval diff) {
  LocalTime& time = timeMap.getOrFail(c);
  eraseFromQueue(queue.at(time), c);
  time += diff;
  queue[time].push_back(c);
}

// Queue is initialized in a lazy manner because during deserialization the comparator doesn't 
// work, as the Creatures are still being deserialized.
TimeQueue::TimeQueue() {}
  
PCreature TimeQueue::removeCreature(WCreature cRef) {
  for (int i : All(creatures))
    if (creatures[i].get() == cRef) {
      eraseFromQueue(queue.at(timeMap.getOrFail(cRef)), cRef);
      PCreature ret = std::move(creatures[i]);
      creatures.removeIndexPreserveOrder(i);
      return ret;
    }
  FATAL << "Creature not found";
  return nullptr;
}

vector<WCreature> TimeQueue::getAllCreatures() const {
  return getWeakPointers(creatures);
}

WCreature TimeQueue::getNextCreature() {
  if (creatures.empty())
    return nullptr;
  while (1) {
    while (1) {
      CHECK(!queue.empty());
      if (!queue.begin()->second.empty())
        break;
      queue.erase(queue.begin());
    }
    auto& q = queue.begin()->second;
    while (!q.empty() && q.front() == nullptr)
      q.pop_front();
    if (!q.empty())
      return q.front();
  }
}

