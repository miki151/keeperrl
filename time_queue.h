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
  Creature* getNextCreature(double maxTime);
  vector<Creature*> getAllCreatures() const;
  void addCreature(PCreature, LocalTime time);
  PCreature removeCreature(Creature*);
  LocalTime getTime(const Creature*);
  void increaseTime(Creature*, TimeInterval);
  void makeExtraMove(Creature*);
  bool hasExtraMove(Creature*);
  void postponeMove(Creature*);
  void moveNow(Creature*);
  bool willMoveThisTurn(const Creature*);
  bool compareOrder(const Creature*, const Creature*);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);

  private:
  bool contains(Creature*) const;

  vector<PCreature> SERIAL(creatures);
  struct Queue {
    void push(Creature*);
    void pushFront(Creature*);
    bool empty();
    Creature* front();
    void popFront();
    void erase(Creature*);
    deque<Creature*> SERIAL(players);
    deque<Creature*> SERIAL(nonPlayers);
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

