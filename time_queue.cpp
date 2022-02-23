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
  queue[time].push(c.get());
  creatures.push_back(std::move(c));
}

LocalTime TimeQueue::getTime(const Creature* c) {
  return timeMap.getOrFail(c).time;
}

static void clearNull(deque<Creature*>& q) {
  while (!q.empty() && q.back() == nullptr)
    q.pop_back();
  while (!q.empty() && q.front() == nullptr)
    q.pop_front();
}

void TimeQueue::Queue::clearNull() {
  ::clearNull(players);
  ::clearNull(nonPlayers);
}

void TimeQueue::Queue::push(Creature* c) {
  clearNull();
  if (c->isPlayer()) {
    int order = players.empty() ? 0 : orderMap.getOrFail(players.back()) + 1;
    orderMap.set(c, order);
    players.push_back(c);
  } else {
    int order = nonPlayers.empty() ? 1000000000 : orderMap.getOrFail(nonPlayers.back()) + 1;
    orderMap.set(c, order);
    nonPlayers.push_back(c);
  }
}

void TimeQueue::Queue::pushFront(Creature* c) {
  clearNull();
  if (c->isPlayer()) {
    int order = players.empty() ? 0 : orderMap.getOrFail(players.front()) - 1;
    orderMap.set(c, order);
    players.push_front(c);
  } else {
    int order = nonPlayers.empty() ? 1000000000 : orderMap.getOrFail(nonPlayers.front()) - 1;
    orderMap.set(c, order);
    nonPlayers.push_front(c);
  }
}

bool TimeQueue::Queue::empty() {
  clearNull();
  return players.empty() && nonPlayers.empty();
}

Creature* TimeQueue::Queue::front() {
  clearNull();
  if (!players.empty())
    return players.front();
  else
    return nonPlayers.front();
}

void TimeQueue::Queue::popFront() {
  if (!players.empty())
    return players.pop_front();
  else
    return nonPlayers.pop_front();
}

void TimeQueue::Queue::erase(Creature* c) {
  auto eraseFrom = [&c] (deque<Creature*>& queue) {
    for (int i : All(queue))
      if (queue[i] == c) {
        queue[i] = nullptr;
        return true;
      }
    return false;
  };
  CHECK(eraseFrom(players) || eraseFrom(nonPlayers));
}

void TimeQueue::increaseTime(Creature* c, TimeInterval diff) {
  auto& time = timeMap.getOrFail(c);
  queue.at(time).erase(c);
  time.time += diff;
  time.extraTurn = false;
  queue[time].push(c);
}

void TimeQueue::makeExtraMove(Creature* c) {
  auto& time = timeMap.getOrFail(c);
  queue.at(time).erase(c);
  if (!time.extraTurn)
    time.extraTurn = true;
  else {
    time.time += 1_visible;
    time.extraTurn = false;
  }
  queue[time].push(c);
}

bool TimeQueue::hasExtraMove(Creature* c) {
  return timeMap.getOrFail(c).extraTurn;
}

void TimeQueue::postponeMove(Creature* c) {
  CHECK(contains(c));
  auto time = timeMap.getOrFail(c);
  queue.at(time).erase(c);
  queue.at(time).push(c);
}

void TimeQueue::moveNow(Creature* c) {
  CHECK(contains(c));
  auto time = timeMap.getOrFail(c);
  queue.at(time).erase(c);
  queue.at(time).pushFront(c);
}

bool TimeQueue::willMoveThisTurn(const Creature* c) {
  auto hisTime = timeMap.getOrFail(c);
  auto curTime = queue.begin()->first;
  return hisTime.time == curTime.time && (!hisTime.extraTurn || curTime.extraTurn);
}

bool TimeQueue::compareOrder(const Creature* c1, const Creature* c2) {
  if (!willMoveThisTurn(c1) && willMoveThisTurn(c2))
    return true;
  else if (willMoveThisTurn(c1) && !willMoveThisTurn(c2))
    return false;
  if (!willMoveThisTurn(c1))
    return c1->getLastMoveCounter() < c2->getLastMoveCounter();
  auto time1 = timeMap.getOrFail(c1);
  auto time2 = timeMap.getOrFail(c2);
  if (time1 < time2)
    return true;
  if (time2 < time1)
    return false;
  auto& orderMap = queue.at(time1).orderMap;
  return orderMap.getOrFail(c1) < orderMap.getOrFail(c2);
}

bool TimeQueue::contains(Creature* c) const {
  for (auto& creature : creatures)
    if (creature.get() == c)
      return true;
  return false;
}

TimeQueue::TimeQueue() {}

PCreature TimeQueue::removeCreature(Creature* cRef) {
  for (int i : All(creatures))
    if (creatures[i].get() == cRef) {
      queue.at(timeMap.getOrFail(cRef)).erase(cRef);
      PCreature ret = std::move(creatures[i]);
      creatures.removeIndexPreserveOrder(i);
      return ret;
    }
  FATAL << "Creature not found " << cRef->identify();
  return nullptr;
}

vector<Creature*> TimeQueue::getAllCreatures() const {
  return getWeakPointers(creatures);
}

Creature* TimeQueue::getNextCreature(double maxTime) {
  if (creatures.empty())
    return nullptr;
  while (1) {
    CHECK(!queue.empty());
    if (!queue.begin()->second.empty())
      break;
    queue.erase(queue.begin());
  }
  auto nowTime = queue.begin()->first;
  if (nowTime.getDouble() > maxTime)
    return nullptr;
  auto& q = queue.begin()->second;
  if (!nowTime.extraTurn) {
    auto nextQueue = ++queue.begin();
    if (nextQueue != queue.end()) {
      if (nextQueue->first.time == nowTime.time && !nextQueue->second.empty() && nextQueue->second.front()->isPlayer())
        return nextQueue->second.front();
    }
  }
  return q.front();
}

TimeQueue::ExtendedTime::ExtendedTime() {}

TimeQueue::ExtendedTime::ExtendedTime(LocalTime t) : time(t) {}

double TimeQueue::ExtendedTime::getDouble() const {
  double ret = time.getDouble();
  if (extraTurn)
    ret += 0.5;
  return ret;
}

bool TimeQueue::ExtendedTime::operator < (TimeQueue::ExtendedTime o) const {
  return time < o.time || (time == o.time && !extraTurn && o.extraTurn);
}
