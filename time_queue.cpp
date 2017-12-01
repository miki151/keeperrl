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

LocalTime TimeQueue::getTime(WConstCreature c) {
  return timeMap.getOrFail(c).time;
}

void TimeQueue::Queue::push(WCreature c) {
  if (c->isPlayer())
    players.push_back(c);
  else
    nonPlayers.push_back(c);
}

bool TimeQueue::Queue::empty() const {
  return players.empty() && nonPlayers.empty();
}

WCreature TimeQueue::Queue::front() {
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

void TimeQueue::Queue::erase(WCreature c) {
  auto eraseFrom = [&c] (deque<WCreature>& queue) {
    for (int i : All(queue))
      if (queue[i] == c) {
        queue[i] = nullptr;
        break;
      }
  };
  eraseFrom(players);
  eraseFrom(nonPlayers);
}

void TimeQueue::increaseTime(WCreature c, TimeInterval diff) {
  auto& time = timeMap.getOrFail(c);
  queue.at(time).erase(c);
  time.time += diff;
  time.extraTurn = false;
  queue[time].push(c);
}

void TimeQueue::makeExtraMove(WCreature c) {
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

bool TimeQueue::hasExtraMove(WCreature c) {
  return timeMap.getOrFail(c).extraTurn;
}

void TimeQueue::postponeMove(WCreature c) {
  auto time = timeMap.getOrFail(c);
  queue.at(time).erase(c);
  queue.at(time).push(c);
}

TimeQueue::TimeQueue() {}

PCreature TimeQueue::removeCreature(WCreature cRef) {
  for (int i : All(creatures))
    if (creatures[i].get() == cRef) {
      queue.at(timeMap.getOrFail(cRef)).erase(cRef);
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

WCreature TimeQueue::getNextCreature(double maxTime) {
  if (creatures.empty())
    return nullptr;
  while (1) {
    while (1) {
      CHECK(!queue.empty());
      if (!queue.begin()->second.empty())
        break;
      queue.erase(queue.begin());
    }
    if (queue.begin()->first.getDouble() > maxTime)
      return nullptr;
    auto& q = queue.begin()->second;
    while (!q.empty() && q.front() == nullptr)
      q.popFront();
    if (!q.empty())
      return q.front();
  }
}

TimeQueue::ExtendedTime::ExtendedTime() {}

TimeQueue::ExtendedTime::ExtendedTime(LocalTime t) : time(t) {}

double TimeQueue::ExtendedTime::getDouble() const {
  double ret = time.getDouble();
  if (extraTurn)
    ret += 0.5;
  return ret;
}

bool TimeQueue::ExtendedTime::operator <(TimeQueue::ExtendedTime o) const {
  return time < o.time || (time == o.time && !extraTurn && o.extraTurn);
}
