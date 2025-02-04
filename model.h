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
#include "biome_id.h"
#include "movement_type.h"

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
class Portals;

/**
  * Main class that holds all game logic.
  */
class Model : public OwnedObject<Model> {
  public:
  static PModel create(ContentFactory*, optional<MusicType>, BiomeId);

  /** Makes an update to the game. This method is repeatedly called to make the game run.
    Returns the total logical time elapsed.*/
  bool update(double totalTime);

  /** Returns the level that the stairs lead to. */
  Level* getLinkedLevel(Level* from, StairKey) const;
  bool areConnected(StairKey, StairKey, const MovementType&);

  void addCreature(PCreature);
  void addCreature(PCreature, TimeInterval delay);
  void landWarlord(vector<PCreature>);
  bool landCreature(vector<Position> landing, PCreature);
  void addExternalEnemies(ExternalEnemies);
  const heap_optional<ExternalEnemies>& getExternalEnemies() const;
  optional<MusicType> getDefaultMusic() const;

  LocalTime getLocalTime() const;
  double getLocalTimeDouble() const;
  TimeQueue& getTimeQueue();
  int getMoveCounter() const;
  void increaseMoveCounter();

  void setGame(Game*);
  Game* getGame() const;
  void tick(LocalTime);
  vector<Collective*> getCollectives() const;
  vector<Creature*> getAllCreatures() const;
  const vector<PCreature>& getDeadCreatures() const;
  vector<Level*> getLevels() const;
  Level* getMainLevel(int depth) const;
  optional<int> getMainLevelDepth(const Level*) const;
  Range getMainLevelsDepth() const;
  vector<Level*> getDungeonBranch(Level* current, const MapMemory&) const;
  Level* getGroundLevel() const;
  LevelId getUniqueId() const;

  void addCollective(PCollective);

  int getSaveProgressCount() const;

  void killCreature(Creature* victim);
  void killCreature(PCreature victim);
  void updateSunlightMovement();

  PCreature extractCreature(Creature*);
  void transferCreature(PCreature, Vec2 travelDir);
  void transferCreature(PCreature, const vector<Position>& destinations);
  bool canTransferCreature(Creature*, Vec2 travelDir);

  SERIALIZATION_DECL(Model)

  void discardForRetirement();
  void prepareForRetirement();

  void addEvent(const GameEvent&);

  Level* buildLevel(const ContentFactory*, LevelBuilder, PLevelMaker, int depth, TString name);
  Level* buildMainLevel(const ContentFactory*, LevelBuilder, PLevelMaker);
  Level* buildUpLevel(const ContentFactory*, LevelBuilder, PLevelMaker);
  void calculateStairNavigation();

  BiomeId getBiomeId() const;

  HeapAllocated<Portals> SERIAL(portals);
  Vec2 SERIAL(position);

  bool serializationLocked = false;

  private:
  struct Private {};

  public:
  Model(Private);
  ~Model();

  private:

  friend class ModelBuilder;

  PCreature makePlayer(int handicap);

  vector<PLevel> SERIAL(levels);
  vector<Level*> SERIAL(mainLevels);
  vector<Level*> SERIAL(upLevels);
  PLevel SERIAL(cemetery);
  vector<PCollective> SERIAL(collectives);
  Game* SERIAL(game) = nullptr;
  LocalTime SERIAL(lastTick);
  HeapAllocated<TimeQueue> SERIAL(timeQueue);
  vector<PCreature> SERIAL(deadCreatures);
  double SERIAL(currentTime) = 0;
  using StairConnections = HashMap<StairKey, int>;
  StairConnections createStairConnections(const MovementType&) const;
  HashMap<MovementType, StairConnections> SERIAL(stairNavigation);
  template <typename>
  friend class EventListener;
  OwnerPointer<EventGenerator> SERIAL(eventGenerator);
  void checkCreatureConsistency();
  heap_optional<ExternalEnemies> SERIAL(externalEnemies);
  int moveCounter = 0;
  optional<MusicType> SERIAL(defaultMusic);
  BiomeId SERIAL(biomeId);
};

