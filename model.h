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
#include "map_memory.h"
#include "stair_key.h"
#include "position.h"
#include "tribe.h"
#include "enum_variant.h"
#include "event_generator.h"

class Level;
class ProgressMeter;
class LevelMaker;
class LevelBuilder;
class TimeQueue;
class StairKey;
class Game;
class ExternalEnemies;
class Options;

/**
  * Main class that holds all game logic.
  */
class Model {
  public:
  ~Model();
  
  /** Makes an update to the game. This method is repeatedly called to make the game run.
    Returns the total logical time elapsed.*/
  void update(double totalTime);

  /** Returns the level that the stairs lead to. */
  Level* getLinkedLevel(Level* from, StairKey) const;

  optional<Position> getStairs(const Level* from, const Level* to);

  void addCreature(PCreature);
  void addCreature(PCreature, double delay);
  void landHeroPlayer(PCreature);
  void addExternalEnemies(const ExternalEnemies&);

  bool isTurnBased();

  double getLocalTime() const;
  void increaseLocalTime(WCreature, double diff);
  double getLocalTime(WConstCreature);

  void setGame(WGame);
  WGame getGame() const;
  void tick(double time);
  vector<WCollective> getCollectives() const;
  vector<WCreature> getAllCreatures() const;
  vector<Level*> getLevels() const;

  Level* getTopLevel() const;

  void addWoodCount(int);
  int getWoodCount() const;

  int getSaveProgressCount() const;

  void killCreature(WCreature victim);
  void updateSunlightMovement();

  PCreature extractCreature(WCreature);
  void transferCreature(PCreature, Vec2 travelDir);
  bool canTransferCreature(WCreature, Vec2 travelDir);

  Model();

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version);

  void lockSerialization();

  void addEvent(const GameEvent&);

  private:

  friend class ModelBuilder;

  PCreature makePlayer(int handicap);
  Level* buildLevel(LevelBuilder&&, PLevelMaker);
  Level* buildTopLevel(LevelBuilder&&, PLevelMaker);

  vector<PLevel> SERIAL(levels);
  PLevel SERIAL(cemetery);
  vector<PCollective> SERIAL(collectives);
  WGame SERIAL(game) = nullptr;
  double SERIAL(lastTick) = 0;
  HeapAllocated<TimeQueue> SERIAL(timeQueue);
  vector<PCreature> SERIAL(deadCreatures);
  double SERIAL(currentTime) = 0;
  int SERIAL(woodCount) = 0;
  void calculateStairNavigation();
  optional<StairKey> getStairsBetween(const Level* from, const Level* to);
  map<pair<const Level*, const Level*>, StairKey> SERIAL(stairNavigation);
  bool serializationLocked = false;
  Level* SERIAL(topLevel) = nullptr;
  template <typename>
  friend class EventListener;
  OwnerPointer<EventGenerator> SERIAL(eventGenerator);
  void checkCreatureConsistency();
  HeapAllocated<optional<ExternalEnemies>> SERIAL(externalEnemies);
};

