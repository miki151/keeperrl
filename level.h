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

#ifndef _LEVEL_H
#define _LEVEL_H

#include "util.h"
#include "debug.h"
#include "unique_entity.h"
#include "movement_type.h"
#include "sectors.h"
#include "stair_key.h"
#include "entity_set.h"
#include "vision_id.h"


class Model;
class Square;
class Player;
class LevelMaker;
class Location;
class Attack;
class CollectiveBuilder;
class ProgressMeter;
class Sectors;
class Tribe;
class Attack;
class PlayerMessage;
class CreatureBucketMap;
class Position;
class Game;
class SquareArray;
class FieldOfView;

/** A class representing a single level of the dungeon or the overworld. All events occuring on the level are performed by this class.*/
class Level {
  public:

  ~Level();
  static Rectangle getMaxBounds();
  static Rectangle getSplashBounds();
  static Rectangle getSplashVisibleBounds();

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
  bool landCreature(StairKey key, PCreature, Vec2 travelDir);
  //@}

  /** Lands the creature on the level randomly choosing one of the given squares.
      Returns the position of the stairs that were used.*/
  bool landCreature(vector<Position> landing, PCreature);
  bool landCreature(vector<Position> landing, Creature*);

  /** Returns the landing squares for given direction and stair key. See Square::getLandingLink() */
  vector<Position> getLandingSquares(StairKey) const;
  Position getLandingSquare(StairKey, Vec2 travelDir) const;

  vector<StairKey> getAllStairKeys() const;
  bool hasStairKey(StairKey) const;

  optional<Position> getStairsTo(const Level*) const;

  /** Removes the creature from \paramname{position} from the level and model. The creature object is retained.*/
  void killCreature(Creature* victim);

  void removeCreature(Creature*);

  /** Recalculates visibility data assuming that \paramname{changedSquare} has changed
      its obstructing/non-obstructing attribute. */
  void updateVisibility(Vec2 changedSquare);

  /** Returns width of the level.*/
  int getWidth() const;

  /** Returns height of the level.*/
  int getHeight() const;

  /** Checks \paramname{pos} lies within the level's boundaries.*/
  bool inBounds(Vec2 pos) const;

  /** Returns the level's boundaries.*/
  Rectangle getBounds() const;

  /** Returns the name of the level. */
  const string& getName() const;

  //@{
  /** Returns the given square. \paramname{pos} must lie within the boundaries. */
  Position getPosition(Vec2) const;
  vector<Position> getAllPositions() const;
  //@}

  void replaceSquare(Position, PSquare square, bool storePrevious = true);
  void removeSquare(Position, PSquare defaultSquare);

  /** The given square's method Square::tick() will be called every turn. */
  void addTickingSquare(Vec2 pos);
  void addTickingFurniture(Vec2 pos);

  /** Ticks all squares that must be ticked. */
  void tick();

  /** Moves the creature to a different level according to \paramname{direction}. */
  void changeLevel(StairKey key, Creature* c);

  /** Moves the creature to a given level. */
  void changeLevel(Position destination, Creature* c);

  /** Performs a throw of the item, with all consequences of the event.*/
  void throwItem(PItem item, const Attack& attack, int maxDist, Vec2 position, Vec2 direction, VisionId);
  void throwItem(vector<PItem> item, const Attack& attack, int maxDist, Vec2 position, Vec2 direction, VisionId);

  /** Sets the level to be rendered in the background with given offset.*/
  void setBackgroundLevel(const Level*, Vec2 offset);

  //@{
  /** Returns all creatures on this level. */
  const vector<Creature*>& getAllCreatures() const;
  vector<Creature*>& getAllCreatures();
  vector<Creature*> getAllCreatures(Rectangle bounds) const;
  //@}

  bool containsCreature(UniqueEntity<Creature>::Id) const;

  /** Checks whether the creature can see the square.*/
  bool canSee(const Creature* c, Vec2 to) const;

  /** Returns if it's possible to see the given square.*/
  bool canSee(Vec2 from, Vec2 to, VisionId) const;

  /** Returns all tiles visible by a creature.*/
  vector<Vec2> getVisibleTiles(Vec2 pos, VisionId) const;

  /** Checks if the player can see a given square.*/
  bool playerCanSee(Vec2 pos) const;

  /** Checks if the player can see a given creature.*/
  bool playerCanSee(const Creature*) const;

  /** Displays \paramname{playerCanSee} message if the player can see position \paramname{pos},
    and \paramname{cannot} otherwise.*/
  void globalMessage(Vec2 position, const PlayerMessage& playerCanSee, const PlayerMessage& cannot) const;
  void globalMessage(Vec2 position, const PlayerMessage& playerCanSee) const;

  /** Displays \paramname{playerCanSee} message if the player can see the creature, 
    and \paramname{cannot} otherwise.*/
  void globalMessage(const Creature*, const PlayerMessage& ifPlayerCanSee, const PlayerMessage& cannot) const;

  /** Returns the player creature.*/
  Creature* getPlayer() const;

  const vector<Location*> getAllLocations() const;
  void addMarkedLocation(Rectangle bounds);
  void clearLocations();

  const Model* getModel() const;
  Model* getModel();
  Game* getGame() const;

  void addLightSource(Vec2, double radius);
  void removeLightSource(Vec2, double radius);

  /** Returns the amount of light in the square, capped within (0, 1).*/
  double getLight(Vec2) const;

  /** Returns whether the square is in direct sunlight.*/
  bool isInSunlight(Vec2 pos) const;

  /** Returns if two squares are connected assuming given movement.*/
  bool areConnected(Vec2, Vec2, const MovementType&) const;

  bool isChokePoint(Vec2, const MovementType&) const;

  void updateSunlightMovement();

  const optional<ViewObject>& getBackgroundObject(Vec2) const;
  int getNumModifiedSquares() const;
  bool isUnavailable(Vec2) const;

  void setNeedsMemoryUpdate(Vec2, bool);
  bool needsMemoryUpdate(Vec2) const;
  bool needsRenderUpdate(Vec2) const;
  void setNeedsRenderUpdate(Vec2, bool);

  LevelId getUniqueId() const;
  void setFurniture(Vec2, const Furniture&);

  SERIALIZATION_DECL(Level);

  private:
  friend class Position;
  const Square* getSafeSquare(Vec2) const;
  Square* modSafeSquare(Vec2);
  Vec2 transform(Vec2);
  HeapAllocated<SquareArray> SERIAL(squares);
  Table<PSquare> SERIAL(oldSquares);
  class FurnitureInfo {
    friend class Level;
    public:
    void reset();
    Furniture* get();
    const Furniture* get() const;
    struct FurnitureConstruction {
      FurnitureType SERIAL(type);
      int SERIAL(time);
      SERIALIZE_ALL(type, time);
    };

    optional<FurnitureConstruction> SERIAL(construction);
    SERIALIZE_ALL(furniture, construction);

    private:
    HeapAllocated<optional<Furniture>> SERIAL(furniture);
  };
  Table<FurnitureInfo> SERIAL(furnitureInfo);
  HeapAllocated<Table<optional<ViewObject>>> SERIAL(background);
  Table<bool> SERIAL(memoryUpdates);
  Table<bool> renderUpdates = Table<bool>(getMaxBounds(), true);
  Table<bool> SERIAL(unavailable);
  unordered_map<StairKey, vector<Position>> SERIAL(landingSquares);
  vector<Location*> SERIAL(locations);
  set<Vec2> SERIAL(tickingSquares);
  set<Vec2> SERIAL(tickingFurniture);
  void eraseCreature(Creature*, Vec2 coord);
  void placeCreature(Creature*, Vec2 pos);
  void unplaceCreature(Creature*, Vec2 pos);
  vector<Creature*> SERIAL(creatures);
  EntitySet<Creature> SERIAL(creatureIds);
  Model* SERIAL(model) = nullptr;
  mutable HeapAllocated<EnumMap<VisionId, FieldOfView>> SERIAL(fieldOfView);
  string SERIAL(name);
  const Level* SERIAL(backgroundLevel) = nullptr;
  Vec2 SERIAL(backgroundOffset);
  Table<double> SERIAL(sunlight);
  HeapAllocated<CreatureBucketMap> SERIAL(bucketMap);
  Table<double> SERIAL(lightAmount);
  Table<double> SERIAL(lightCapAmount);
  mutable unordered_map<MovementType, Sectors> SERIAL(sectors);
  Sectors& getSectors(const MovementType&) const;
  
  friend class LevelBuilder;
  Level(SquareArray, Model*, vector<Location*>, const string& name, Table<double> sunlight, LevelId);

  void addLightSource(Vec2 pos, double radius, int numLight);
  void addDarknessSource(Vec2 pos, double radius, int numLight);
  FieldOfView& getFieldOfView(VisionId vision) const;
  vector<Vec2> getVisibleTilesNoDarkness(Vec2 pos, VisionId vision) const;
  bool isWithinVision(Vec2 from, Vec2 to, VisionId) const;
  LevelId SERIAL(levelId) = 0;
  bool SERIAL(noDiagonalPassing) = false;
};

#endif
