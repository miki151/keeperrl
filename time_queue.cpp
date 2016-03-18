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
  ar& SVAR(creatures)
    & SVAR(queue)
    & SVAR(dead);
}

SERIALIZABLE(TimeQueue);


template <class Archive> 
void TimeQueue::QElem::serialize(Archive& ar, const unsigned int version) {
  ar& BOOST_SERIALIZATION_NVP(creature)
    & BOOST_SERIALIZATION_NVP(time);
}

SERIALIZABLE(TimeQueue::QElem);

TimeQueue::TimeQueue() : queue([](QElem e1, QElem e2) {
    return e1.time > e2.time || (e1.time == e2.time && e1.creature->getUniqueId() > e2.creature->getUniqueId());
}) {
}

void TimeQueue::addCreature(PCreature c) {
  queue.push({c.get(), c->getLocalTime()});
  dead.erase(c.get());
  creatures.push_back(std::move(c));
}
  
PCreature TimeQueue::removeCreature(Creature* cRef) {
  int ind = -1;
  for (int i : All(creatures))
    if (creatures[i].get() == cRef) {
      ind = i;
      break;
    }
  CHECK(ind > -1) << "Creature not found";
  PCreature ret = std::move(creatures[ind]);
  creatures.erase(creatures.begin() + ind);
  dead.insert(ret.get());
  return ret;
}

vector<Creature*> TimeQueue::getAllCreatures() const {
  vector<Creature*> ret;
  for (const PCreature& c : creatures)
    ret.push_back(c.get());
  return ret;
}

void TimeQueue::removeDead() {
  while (!queue.empty() && dead.count(queue.top().creature))
    queue.pop();
}

Creature* TimeQueue::getMinCreature() {
  if (creatures.empty())
    return nullptr;
  removeDead();
  QElem elem = queue.top();
  while (elem.time != elem.creature->getLocalTime()) {
    CHECK(elem.time < elem.creature->getLocalTime());
    queue.pop();
    removeDead();
    queue.push({elem.creature, elem.creature->getLocalTime()});
    elem = queue.top();
  }
  return elem.creature;
}

Creature* TimeQueue::getNextCreature() {
  Creature* c = getMinCreature();
  return c;
}

