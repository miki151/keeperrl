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
#include "stair_key.h"
#include "position.h"
#include "tribe.h"
#include "enum_variant.h"
#include "event_generator.h"
#include "game_time.h"

class Level;
class ProgressMeter;
class LevelMaker;
class LevelBuilder;
class TimeQueue;
class StairKey;
class Game;
class ExternalEnemies;
class Options;
class AvatarInfo;
class GameConfig;
class ContentFactory;

/**
  * Main class that holds all game logic.
  */
class Model : public OwnedObject<Model> {
  public:
  static PModel create(ContentFactory*, optional<MusicType>);
  
  /** Makes an update to the game. This method is repeatedly called to make the game run.
    Returns the total logical time elapsed.*/
  bool update(double totalTime);

  /** Returns the level that the stairs lead to. */
  WLevel getLinkedLevel(WLevel from, StairKey) const;

  optional<Position> getStairs(WConstLevel from, WConstLevel to);

  void addCreature(PCreature);
  void addCreature(PCreature, TimeInterval delay);
  void landHeroPlayer(PCreature);
  void landWarlord(vector<PCreature>);
  void addExternalEnemies(ExternalEnemies);
  const heap_optional<ExternalEnemies>& getExternalEnemies() const;
  optional<MusicType> getDefaultMusic() const;

  bool isTurnBased();

  LocalTime getLocalTime() const;
  double getLocalTimeDouble() const;
  TimeQueue& getTimeQueue();
  int getMoveCounter() const;
  void increaseMoveCounter();

  void setGame(WGame);
  WGame getGame() const;
  void tick(LocalTime);
  vector<Collective*> getCollectives() const;
  vector<Creature*> getAllCreatures() const;
  const vector<PCreature>& getDeadCreatures() const;
  vector<WLevel> getLevels() const;
  const vector<WLevel>& getMainLevels() const;
  WLevel getTopLevel() const;
  LevelId getUniqueId() const;

  void addCollective(PCollective);

  void addWoodCount(int);
  int getWoodCount() const;

  int getSaveProgressCount() const;

  void killCreature(Creature* victim);
  void updateSunlightMovement();

  PCreature extractCreature(Creature*);
  void transferCreature(PCreature, Vec2 travelDir);
  void transferCreature(PCreature, const vector<Position>& destinations);
  bool canTransferCreature(Creature*, Vec2 travelDir);

  SERIALIZATION_DECL(Model)

  void discardForRetirement();
  void prepareForRetirement();

  void addEvent(const GameEvent&);

  WLevel buildLevel(const ContentFactory*, LevelBuilder, PLevelMaker, int depth, optional<string> name);
  WLevel buildMainLevel(const ContentFactory*, LevelBuilder, PLevelMaker);
  void calculateStairNavigation();

  private:
  struct Private {};

  public:
  Model(Private);
  ~Model();

  private:

  friend class ModelBuilder;

  PCreature makePlayer(int handicap);

  vector<PLevel> SERIAL(levels);
  vector<WLevel> SERIAL(mainLevels);
  PLevel SERIAL(cemetery);
  vector<PCollective> SERIAL(collectives);
  WGame SERIAL(game) = nullptr;
  LocalTime SERIAL(lastTick);
  HeapAllocated<TimeQueue> SERIAL(timeQueue);
  vector<PCreature> SERIAL(deadCreatures);
  double SERIAL(currentTime) = 0;
  int SERIAL(woodCount) = 0;
  optional<StairKey> getStairsBetween(WConstLevel from, WConstLevel to) const;
  map<pair<LevelId, LevelId>, StairKey> SERIAL(stairNavigation);
  bool serializationLocked = false;
  template <typename>
  friend class EventListener;
  OwnerPointer<EventGenerator> SERIAL(eventGenerator);
  void checkCreatureConsistency();
  heap_optional<ExternalEnemies> SERIAL(externalEnemies);
  int moveCounter = 0;
  optional<MusicType> SERIAL(defaultMusic);
};

