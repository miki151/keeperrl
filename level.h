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
#include "debug.h"
#include "unique_entity.h"
#include "movement_type.h"
#include "sectors.h"
#include "stair_key.h"
#include "entity_set.h"
#include "vision_id.h"
#include "furniture_layer.h"

class Model;
class Square;
class Player;
class LevelMaker;
class Attack;
class ProgressMeter;
class Sectors;
class Tribe;
class Attack;
class PlayerMessage;
class CreatureBucketMap;
class Position;
class Game;
class SquareArray;
class FurnitureArray;
class Vision;
class FieldOfView;
class Portals;
class RoofSupport;
class ContentFactory;
struct PhylacteryInfo;

/** A class representing a single level of the dungeon or the overworld. All events occuring on the level are performed by this class.*/
class Level : public OwnedObject<Level> {
  public:

  ~Level();
  static Rectangle getMaxBounds();
  static double getCreatureLightRadius();

  /** Moves the creature. Updates the creature's position.*/
  void moveCreature(Creature*, Vec2 direction);

  /** Swaps positions of two creatures. */
  void swapCreatures(Creature*, Creature*);

  /** Puts the \paramname{creature} on \paramname{position}. */
  void putCreature(Vec2 position, Creature*);

  //@{
  /** Finds an appropriate square for the \paramname{creature} changing level from \paramname{direction}.
    The square's method Square::isLandingSquare must return true for \paramname{direction}. 
    Returns the position of the stairs that were used. */
  bool landCreature(StairKey key, Creature*);
  bool landCreature(StairKey key, PCreature);
  bool landCreature(StairKey key, Creature*, Vec2 travelDir);
  bool landCreature(StairKey key, PCreature, Vec2 travelDir);
  //@}

  /** Lands the creature on the level randomly choosing one of the given squares.
      Returns the position of the stairs that were used.*/
  bool landCreature(vector<Position> landing, PCreature);
  bool landCreature(vector<Position> landing, Creature*);

  optional<Position> getClosestLanding(vector<Position> landing, Creature*) const;

  /** Returns the landing squares for given direction and stair key. See Square::getLandingLink() */
  const vector<Position>& getLandingSquares(StairKey) const;
  Position getLandingSquare(StairKey, Vec2 travelDir) const;

  vector<StairKey> getAllStairKeys() const;
  bool hasStairKey(StairKey) const;

  optional<Position> getStairsTo(WConstLevel);

  void killCreature(Creature* victim);

  void removeCreature(Creature*);

  void updateVisibility(Vec2 changedSquare);

  bool inBounds(Vec2 pos) const;

  const Rectangle& getBounds() const;

  vector<Position> getAllPositions() const;
  vector<Position> getAllLandingPositions() const;

  void addTickingSquare(Vec2 pos);
  void addTickingFurniture(Vec2 pos);

  void tick();

  void changeLevel(StairKey key, Creature* c);

  void changeLevel(Position destination, Creature* c);

  const vector<Creature*>& getAllCreatures() const;
  vector<Creature*>& getAllCreatures();
  vector<Creature*> getAllCreatures(Rectangle bounds) const;
  vector<PhylacteryInfo> getPhylacteries();


  bool containsCreature(UniqueEntity<Creature>::Id) const;

  bool canSee(Vec2 from, Vec2 to, const Vision&) const;

  vector<Vec2> getVisibleTiles(Vec2 pos, const Vision&) const;

  vector<Creature*> getPlayers() const;

  WModel getModel() const;
  WGame getGame() const;

  void addLightSource(Vec2, double radius);
  void removeLightSource(Vec2, double radius);

  /** Returns the amount of light in the square, capped within (0, 1).*/
  double getLight(Vec2) const;

  /** Returns whether the square is in direct sunlight.*/
  bool isInSunlight(Vec2 pos) const;

  void updateSunlightMovement();

  int getNumGeneratedSquares() const;
  int getNumTotalSquares() const;
  bool isUnavailable(Vec2) const;

  void setNeedsMemoryUpdate(Vec2, bool);
  bool needsMemoryUpdate(Vec2) const;
  bool needsRenderUpdate(Vec2) const;
  void setNeedsRenderUpdate(Vec2, bool);

  LevelId getUniqueId() const;
  void setFurniture(Vec2, PFurniture);

  Sectors& getSectors(const MovementType&) const;
  struct EffectSet {
    vector<LastingEffect> SERIAL(friendly);
    vector<LastingEffect> SERIAL(hostile);
    SERIALIZE_ALL(friendly, hostile)
  };
  using EffectsTable = Table<EffectSet>;

  SERIALIZATION_DECL(Level)

  optional<string> SERIAL(name);
  int SERIAL(depth) = 0;
  bool canTranfer = true;
  Table<Collective*> SERIAL(territory);
  int sightRange = 100;

  private:
  friend class Position;
  const Square* getSafeSquare(Vec2) const;
  Square* modSafeSquare(Vec2);
  HeapAllocated<SquareArray> SERIAL(squares);
  HeapAllocated<FurnitureArray> SERIAL(furniture);
  Table<bool> SERIAL(memoryUpdates);
  Table<bool> renderUpdates = Table<bool>(getMaxBounds(), true);
  Table<bool> SERIAL(unavailable);
  unordered_map<StairKey, vector<Position>> SERIAL(landingSquares);
  set<Vec2> SERIAL(tickingSquares);
  set<Vec2> SERIAL(tickingFurniture);
  void eraseCreature(Creature*, Vec2 coord);
  void placeCreature(Creature*, Vec2 pos);
  void unplaceCreature(Creature*, Vec2 pos);
  vector<Creature*> SERIAL(creatures);
  EntitySet<Creature> SERIAL(creatureIds);
  WModel SERIAL(model) = nullptr;
  mutable HeapAllocated<EnumMap<VisionId, FieldOfView>> SERIAL(fieldOfView);
  Table<double> SERIAL(sunlight);
  Table<bool> SERIAL(covered);
  HeapAllocated<RoofSupport> SERIAL(roofSupport);
  HeapAllocated<CreatureBucketMap> SERIAL(bucketMap);
  vector<pair<int, CreatureBucketMap>> SERIAL(swarmMaps);
  Table<double> SERIAL(lightAmount);
  Table<double> SERIAL(lightCapAmount);
  EnumMap<TribeId::KeyType, unique_ptr<EffectsTable>> SERIAL(furnitureEffects);
  mutable unordered_map<MovementType, Sectors, CustomHash<MovementType>> sectors;
  Sectors& getSectorsDontCreate(const MovementType&) const;

  friend class LevelBuilder;
  struct Private {};

  static PLevel create(SquareArray s, FurnitureArray f, WModel m, Table<double> sun, LevelId id,
      Table<bool> cover, Table<bool> unavailable, const ContentFactory*);

  public:
  Level(Private, SquareArray, FurnitureArray, WModel, Table<double> sunlight, LevelId);

  private:
  void addLightSource(Vec2 pos, double radius, int numLight);
  void addDarknessSource(Vec2 pos, double radius, int numLight);
  FieldOfView& getFieldOfView(VisionId vision) const;
  const vector<SVec2>& getVisibleTilesNoDarkness(Vec2 pos, VisionId vision) const;
  bool isWithinVision(Vec2 from, Vec2 to, const Vision&) const;
  LevelId SERIAL(levelId) = 0;
  bool SERIAL(noDiagonalPassing) = false;
  void updateCreatureLight(Vec2, int diff);
  HeapAllocated<Portals> SERIAL(portals);
  bool isCovered(Vec2) const;
  template<typename Fun>
  void forEachEffect(Vec2, TribeId, Fun);
  void placeSwarmer(Vec2, Creature*);
  void unplaceSwarmer(Vec2, Creature*);
};

