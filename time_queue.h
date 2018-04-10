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

#pragma once

#include "util.h"
#include "entity_set.h"
#include "entity_map.h"
#include "indexed_vector.h"
#include "game_time.h"

class Creature;

class TimeQueue {
  public:
  TimeQueue();
  WCreature getNextCreature(double maxTime);
  vector<WCreature> getAllCreatures() const;
  void addCreature(PCreature, LocalTime time);
  PCreature removeCreature(WCreature);
  LocalTime getTime(WConstCreature);
  void increaseTime(WCreature, TimeInterval);
  void makeExtraMove(WCreature);
  bool hasExtraMove(WCreature);
  void postponeMove(WCreature);
  void moveNow(WCreature);
  bool willMoveThisTurn(WConstCreature);
  bool compareOrder(WConstCreature, WConstCreature);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);

  private:
  bool contains(WCreature) const;

  vector<PCreature> SERIAL(creatures);
  struct Queue {
    void push(WCreature);
    void pushFront(WCreature);
    bool empty();
    WCreature front();
    void popFront();
    void erase(WCreature);
    deque<WCreature> SERIAL(players);
    deque<WCreature> SERIAL(nonPlayers);
    EntityMap<Creature, int> SERIAL(orderMap);
    SERIALIZE_ALL(players, nonPlayers, orderMap)

    private:
    void clearNull();
  };
  struct ExtendedTime {
    ExtendedTime();
    ExtendedTime(LocalTime);
    double getDouble() const;
    bool operator < (ExtendedTime) const;
    LocalTime SERIAL(time);
    bool SERIAL(extraTurn) = false;
    SERIALIZE_ALL(time, extraTurn)
  };
  map<ExtendedTime, Queue> SERIAL(queue);
  EntityMap<Creature, ExtendedTime> SERIAL(timeMap);
};

