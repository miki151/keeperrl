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
class Sectors;
class ContentFactory;
struct FurnitureEffectInfo;

class Position {
  public:
  Position(Vec2, WLevel);
  struct IsValid{};
  Position(Vec2, WLevel, IsValid);
  static vector<Position> getAll(WLevel, Rectangle);
  WModel getModel() const;
  WGame getGame() const;
  optional<int> dist8(const Position&) const;
  bool isSameLevel(const Position&) const;
  bool isSameLevel(WConstLevel) const;
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
  void setLandingLink(StairKey) const;
  void removeLandingLink() const;
  const vector<Position>& getLandingAtNextLevel(StairKey);

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
  void addCreature(PCreature, TimeInterval delay) const;
  // will crash if it's not possible to place creature here
  void addCreature(PCreature) const;
  // will try to place creature somewhere else close on the level if it's not possible
  bool landCreature(PCreature) const;
  bool canEnter(const Creature*) const;
  bool canEnter(const MovementType&) const;
  bool canEnterEmpty(const Creature*) const;
  bool canEnterEmpty(const MovementType&) const;
  bool canEnterEmptyCalc(const MovementType&, optional<FurnitureLayer> ignore = none) const;
  void onEnter(Creature*) const;
  optional<FurnitureClickType> getClickType() const;
  void addSound(const Sound&) const;
  void getViewIndex(ViewIndex&, const Creature* viewer) const;
  const vector<Item*>& getItems() const;
  const vector<Item*>& getItems(ItemIndex) const;
  const vector<Item*>& getItems(CollectiveResourceId) const;
  PItem removeItem(Item*) const;
  const Inventory& getInventory() const;
  vector<PItem> removeItems(vector<Item*>);
  bool canConstruct(FurnitureType) const;
  bool isWall() const;
  bool isBuildingSupport() const;
  void removeFurniture(const Furniture*) const;
  void removeFurniture(const Furniture*, PFurniture replace, Creature* destroyedBy = nullptr) const;
  void removeFurniture(FurnitureLayer) const;
  void addFurniture(PFurniture) const;
  bool isUnavailable() const;
  void dropItem(PItem);
  void dropItems(vector<PItem>);
  void construct(FurnitureType, Creature*);
  bool construct(FurnitureType, TribeId);
  bool isActiveConstruction(FurnitureLayer) const;
  bool isBurning() const;
  bool fireDamage(double amount) const;
  bool iceDamage() const;
  bool acidDamage() const;
  bool needsRenderUpdate() const;
  void setNeedsRenderUpdate(bool) const;
  bool needsMemoryUpdate() const;
  void setNeedsMemoryUpdate(bool) const;
  void setNeedsRenderAndMemoryUpdate(bool) const;
  ViewId getTopViewId() const;
  void forbidMovementForTribe(TribeId);
  void allowMovementForTribe(TribeId);
  bool isTribeForbidden(TribeId) const;
  optional<TribeId> getForbiddenTribe() const;
  void addGas(TileGasType, double amount);
  double getGasAmount(TileGasType) const;
  bool isCovered() const;
  bool sunlightBurns() const;
  double getLightEmission() const;
  void addCreatureLight(bool darkness);
  void addSwarmer();
  void removeSwarmer();
  void removeCreatureLight(bool darkness);
  void throwItem(vector<PItem> item, const Attack& attack, int maxDist, Position target, VisionId);
  bool canNavigateCalc(const MovementType&) const;
  bool canNavigate(const MovementType& type) const;
  bool canNavigateToOrNeighbor(Position, const MovementType&) const;
  bool canNavigateTo(Position, const MovementType&) const;
  double getNavigationCost(const MovementType&, const Sectors& onltMovementSectors) const;
  optional<DestroyAction> getBestDestroyAction(const MovementType&) const;
  vector<Position> getVisibleTiles(const Vision&);
  void updateConnectivity() const;
  void updateVisibility() const;
  bool canSeeThruIgnoringGas(VisionId) const;
  bool canSeeThru(VisionId) const;
  bool canSeeThru(VisionId, const ContentFactory*) const;
  bool stopsProjectiles(VisionId) const;
  bool isVisibleBy(const Creature*) const;
  bool canSee(Position, const Vision&) const;
  void clearItemIndex(ItemIndex) const;
  bool isConnectedTo(Position, const MovementType&) const;
  void updateMovementDueToFire() const;
  vector<Creature*> getAllCreatures(int range) const;
  void moveCreature(Vec2 direction);
  void moveCreature(Position, bool teleportEffect = false);
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
  optional<short> getDistanceToNearestPortal() const;
  optional<Position> getOtherPortal() const;
  void registerPortal();
  void removePortal();
  optional<int> getPortalIndex() const;
  double getLightingEfficiency() const;
  bool isDirEffectBlocked(VisionId) const;
  void addFurnitureEffect(TribeId, const FurnitureEffectInfo&) const;
  void removeFurnitureEffect(TribeId, const FurnitureEffectInfo&) const;
  int countSwarmers() const;
  Collective* getCollective() const;

  SERIALIZATION_DECL(Position)
  int getHash() const;

  private:
  Square* modSquare() const;
  const Square* getSquare() const;
  Vec2 SERIAL(coord);
  Level* SERIAL(level) = nullptr;
  bool SERIAL(valid) = false;
  void updateSupport() const;
  void updateBuildingSupport() const;
  void addFurnitureImpl(PFurniture) const;
  void updateSupportViewId(Furniture*) const;
};

template <>
inline string toString(const Position& t) {
  stringstream ss;
  ss << toString(t.getCoord());
  return ss.str();
}

using PositionSet = unordered_set<Position, CustomHash<Position>>;
