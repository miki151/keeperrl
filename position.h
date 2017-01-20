#pragma once

#include "util.h"
#include "stair_key.h"

class Square;
class Level;
class PlayerMessage;
class Location;
class Tribe;
class ViewIndex;
class SquareType;
class MovementType;
struct CoverInfo;
class Attack;
class Game;
class TribeId;
class Sound;
class Fire;
class DestroyAction;
class Inventory;

class Position {
  public:
  Position(Vec2, Level*);
  static vector<Position> getAll(Level*, Rectangle);
  Model* getModel() const;
  Game* getGame() const;
  int dist8(const Position&) const;
  bool isSameLevel(const Position&) const;
  bool isSameLevel(const Level*) const;
  bool isSameModel(const Position&) const;
  Vec2 getDir(const Position&) const;
  Creature* getCreature() const;
  void removeCreature();
  void putCreature(Creature*);
  string getName() const;
  Position withCoord(Vec2 newCoord) const;
  Vec2 getCoord() const;
  Level* getLevel() const;
  optional<StairKey> getLandingLink() const;
 
  bool isValid() const;
  bool operator == (const Position&) const;
  bool operator != (const Position&) const;
  Position& operator = (const Position&);
  Position plus(Vec2) const;
  Position minus(Vec2) const;
  bool operator < (const Position&) const;
  void globalMessage(const PlayerMessage& playerCanSee, const PlayerMessage& cannot) const;
  void globalMessage(const PlayerMessage& playerCanSee) const;
  void globalMessage(const Creature*, const PlayerMessage& playerCanSee, const PlayerMessage& cannot) const;
  vector<Position> neighbors8() const;
  vector<Position> neighbors4() const;
  vector<Position> neighbors8(RandomGen&) const;
  vector<Position> neighbors4(RandomGen&) const;
  vector<Position> getRectangle(Rectangle) const;
  void addCreature(PCreature, double delay);
  void addCreature(PCreature);
  const Location* getLocation() const;
  bool canEnter(const Creature*) const;
  bool canEnter(const MovementType&) const;
  bool canEnterEmpty(const Creature*) const;
  bool canEnterEmpty(const MovementType&) const;
  bool canEnterSquare(const MovementType&) const;
  void onEnter(Creature*);
  optional<FurnitureClickType> getClickType() const;
  void addSound(const Sound&) const;
  bool canHide() const;
  void getViewIndex(ViewIndex&, const Creature* viewer) const;
  vector<Trigger*> getTriggers() const;
  PTrigger removeTrigger(Trigger*);
  vector<PTrigger> removeTriggers();
  void addTrigger(PTrigger);
  const vector<Item*>& getItems() const;
  vector<Item*> getItems(function<bool (Item*)> predicate) const;
  const vector<Item*>& getItems(ItemIndex) const;
  PItem removeItem(Item*);
  Inventory& modInventory() const;
  const Inventory& getInventory() const;
  vector<PItem> removeItems(vector<Item*>);
  bool canConstruct(FurnitureType) const;
  bool isWall() const;
  void removeFurniture(const Furniture*) const;
  void addFurniture(PFurniture) const;
  void replaceFurniture(const Furniture*, PFurniture) const;
  bool isUnavailable() const;
  void dropItem(PItem);
  void dropItems(vector<PItem>);
  void construct(FurnitureType, Creature*);
  bool construct(FurnitureType, TribeId);
  bool isActiveConstruction(FurnitureLayer) const;
  bool isBurning() const;
  void fireDamage(double amount);
  bool needsRenderUpdate() const;
  void setNeedsRenderUpdate(bool) const;
  bool needsMemoryUpdate() const;
  void setNeedsMemoryUpdate(bool) const;
  const ViewObject& getViewObject() const;
  ViewObject& modViewObject();
  void forbidMovementForTribe(TribeId);
  void allowMovementForTribe(TribeId);
  bool isTribeForbidden(TribeId) const;
  optional<TribeId> getForbiddenTribe() const;
  void addPoisonGas(double amount);
  double getPoisonGasAmount() const;
  bool isCovered() const;
  bool sunlightBurns() const;
  double getLightEmission() const;
  void throwItem(PItem item, const Attack& attack, int maxDist, Vec2 direction, VisionId);
  void throwItem(vector<PItem> item, const Attack& attack, int maxDist, Vec2 direction, VisionId);
  bool canNavigate(const MovementType&) const;
  vector<Position> getVisibleTiles(VisionId);
  void updateConnectivity() const;
  void updateVisibility() const;
  bool canSeeThru(VisionId) const;
  bool isVisibleBy(const Creature*);
  void clearItemIndex(ItemIndex);
  bool isChokePoint(const MovementType&) const;
  bool isConnectedTo(Position, const MovementType&) const;
  void updateMovement();
  vector<Creature*> getAllCreatures(int range) const;
  void moveCreature(Vec2 direction);
  void moveCreature(Position);
  bool canMoveCreature(Vec2 direction) const;
  bool canMoveCreature(Position) const;
  void swapCreatures(Creature*);
  double getLight() const;
  optional<Position> getStairsTo(Position) const;
  const Furniture* getFurniture(FurnitureLayer) const;
  const Furniture* getFurniture(FurnitureType) const;
  vector<const Furniture*> getFurniture() const;
  Furniture* modFurniture(FurnitureLayer) const;
  Furniture* modFurniture(FurnitureType) const;
  vector<Furniture*> modFurniture() const;

  SERIALIZATION_DECL(Position);
  int getHash() const;

  private:
  Square* modSquare() const;
  const Square* getSquare() const;
  Vec2 SERIAL(coord);
  Level* SERIAL(level) = nullptr;
};

template <>
inline string toString(const Position& t) {
	stringstream ss;
	ss << toString(t.getCoord());
	return ss.str();
}

