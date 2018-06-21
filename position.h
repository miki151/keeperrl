#pragma once

#include "util.h"
#include "stair_key.h"
#include "game_time.h"

class Square;
class Level;
class PlayerMessage;
class Tribe;
class ViewIndex;
class MovementType;
struct CoverInfo;
class Attack;
class Game;
class TribeId;
class Sound;
class Fire;
class DestroyAction;
class Inventory;
class Vision;

class Position {
  public:
  Position(Vec2, WLevel);
  static vector<Position> getAll(WLevel, Rectangle);
  WModel getModel() const;
  WGame getGame() const;
  int dist8(const Position&) const;
  bool isSameLevel(const Position&) const;
  bool isSameLevel(WConstLevel) const;
  bool isSameModel(const Position&) const;
  Vec2 getDir(const Position&) const;
  WCreature getCreature() const;
  void removeCreature();
  void putCreature(WCreature);
  string getName() const;
  Position withCoord(Vec2 newCoord) const;
  Vec2 getCoord() const;
  WLevel getLevel() const;
  optional<StairKey> getLandingLink() const;
 
  bool isValid() const;
  bool operator == (const Position&) const;
  bool operator != (const Position&) const;
  Position plus(Vec2) const;
  Position minus(Vec2) const;
  void unseenMessage(const PlayerMessage&) const;
  void globalMessage(const PlayerMessage&) const;
  vector<Position> neighbors8() const;
  vector<Position> neighbors4() const;
  vector<Position> neighbors8(RandomGen&) const;
  vector<Position> neighbors4(RandomGen&) const;
  vector<Position> getRectangle(Rectangle) const;
  void addCreature(PCreature, TimeInterval delay);
  void addCreature(PCreature);
  bool canEnter(WConstCreature) const;
  bool canEnter(const MovementType&) const;
  bool canEnterEmpty(WConstCreature) const;
  bool canEnterEmpty(const MovementType&, optional<FurnitureLayer> ignore = none) const;
  void onEnter(WCreature);
  optional<FurnitureClickType> getClickType() const;
  void addSound(const Sound&) const;
  void getViewIndex(ViewIndex&, WConstCreature viewer) const;
  const vector<WItem>& getItems() const;
  const vector<WItem>& getItems(ItemIndex) const;
  PItem removeItem(WItem);
  Inventory& modInventory() const;
  const Inventory& getInventory() const;
  vector<PItem> removeItems(vector<WItem>);
  bool canConstruct(FurnitureType) const;
  bool isWall() const;
  void removeFurniture(WConstFurniture) const;
  void addFurniture(PFurniture) const;
  void replaceFurniture(WConstFurniture, PFurniture) const;
  bool isUnavailable() const;
  void dropItem(PItem);
  void dropItems(vector<PItem>);
  void construct(FurnitureType, WCreature);
  bool construct(FurnitureType, TribeId);
  bool isActiveConstruction(FurnitureLayer) const;
  bool isBurning() const;
  void fireDamage(double amount);
  bool needsRenderUpdate() const;
  void setNeedsRenderUpdate(bool) const;
  bool needsMemoryUpdate() const;
  void setNeedsMemoryUpdate(bool) const;
  const ViewObject& getViewObject() const;
  void forbidMovementForTribe(TribeId);
  void allowMovementForTribe(TribeId);
  bool isTribeForbidden(TribeId) const;
  optional<TribeId> getForbiddenTribe() const;
  void addPoisonGas(double amount);
  double getPoisonGasAmount() const;
  bool isCovered() const;
  bool sunlightBurns() const;
  double getLightEmission() const;
  void addCreatureLight(bool darkness);
  void removeCreatureLight(bool darkness);
  void throwItem(PItem item, const Attack& attack, int maxDist, Vec2 direction, VisionId);
  void throwItem(vector<PItem> item, const Attack& attack, int maxDist, Vec2 direction, VisionId);
  bool canNavigate(const MovementType&) const;
  bool canNavigateToOrNeighbor(Position from, const MovementType&) const;
  optional<double> getNavigationCost(const MovementType&) const;
  optional<DestroyAction> getBestDestroyAction(const MovementType&) const;
  vector<Position> getVisibleTiles(const Vision&);
  void updateConnectivity() const;
  void updateVisibility() const;
  bool canSeeThru(VisionId) const;
  bool stopsProjectiles(VisionId) const;
  bool isVisibleBy(WConstCreature) const;
  void clearItemIndex(ItemIndex) const;
  bool isChokePoint(const MovementType&) const;
  bool isConnectedTo(Position, const MovementType&) const;
  void updateMovement();
  vector<WCreature> getAllCreatures(int range) const;
  void moveCreature(Vec2 direction);
  void moveCreature(Position);
  bool canMoveCreature(Vec2 direction) const;
  bool canMoveCreature(Position) const;
  void swapCreatures(WCreature);
  double getLight() const;
  optional<Position> getStairsTo(Position) const;
  WConstFurniture getFurniture(FurnitureLayer) const;
  WConstFurniture getFurniture(FurnitureType) const;
  vector<WConstFurniture> getFurniture() const;
  WFurniture modFurniture(FurnitureLayer) const;
  WFurniture modFurniture(FurnitureType) const;
  vector<WFurniture> modFurniture() const;

  SERIALIZATION_DECL(Position)
  int getHash() const;

  private:
  WSquare modSquare() const;
  WConstSquare getSquare() const;
  Vec2 SERIAL(coord);
  Level* level = nullptr;
  bool SERIAL(valid) = false;
  void updateSupport() const;
};

template <>
inline string toString(const Position& t) {
	stringstream ss;
	ss << toString(t.getCoord());
	return ss.str();
}

using PositionSet = unordered_set<Position, CustomHash<Position>>;
