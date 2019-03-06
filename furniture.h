#pragma once

#include "destroy_action.h"
#include "fire.h"
#include "item_factory.h"
#include "vision_id.h"
#include "event_listener.h"
#include "furniture_layer.h"
#include "luxury_info.h"

class TribeId;
class Creature;
class MovementType;
class Fire;
class ItemFactory;
class GameEvent;
class Position;
class FurnitureEntry;
class FurnitureDroppedItems;
class ViewObject;
class MovementSet;

class Furniture : public OwnedObject<Furniture> {
  public:
  static const string& getName(FurnitureType, int count = 1);
  static FurnitureLayer getLayer(FurnitureType);
  static bool isWall(FurnitureType);
  static LuxuryInfo getLuxuryInfo(FurnitureType);
  static int getPopulationIncrease(FurnitureType, int numBuilt);
  static optional<std::string> getPopulationIncreaseDescription(FurnitureType);

  Furniture(const string& name, const optional<ViewObject>&, FurnitureType, TribeId);
  Furniture(const Furniture&);
  const heap_optional<ViewObject>& getViewObject() const;
  heap_optional<ViewObject>& getViewObject();
  const string& getName(int count = 1) const;
  FurnitureType getType() const;
  bool isVisibleTo(const Creature*) const;
  const MovementSet& getMovementSet() const;
  void onEnter(Creature*) const;
  bool canDestroy(const MovementType&, const DestroyAction&) const;
  bool canDestroy(const DestroyAction&) const;
  optional<double> getStrength(const DestroyAction&) const;
  void destroy(Position, const DestroyAction&);
  void tryToDestroyBy(Position, Creature*, const DestroyAction&);
  TribeId getTribe() const;
  void setTribe(TribeId);
  const heap_optional<Fire>& getFire() const;
  void fireDamage(Position, double amount);
  void tick(Position);
  bool canSeeThru(VisionId) const;
  bool stopsProjectiles(VisionId) const;
  void click(Position) const;
  bool overridesMovement() const;
  void use(Position, Creature*) const;
  bool canUse(const Creature*) const;
  optional<FurnitureUsageType> getUsageType() const;
  TimeInterval getUsageTime() const;
  optional<FurnitureClickType> getClickType() const;
  optional<FurnitureTickType> getTickType() const;
  bool isTicking() const;
  bool isWall() const;
  bool isBuildingSupport() const;
  void onConstructedBy(Position, Creature*);
  FurnitureLayer getLayer() const;
  double getLightEmission() const;
  bool canHide() const;
  bool emitsWarning(const Creature*) const;
  bool canRemoveWithCreaturePresent() const;
  bool canRemoveNonFriendly() const;
  Creature* getCreator() const;
  optional<LocalTime> getCreatedTime() const;
  optional<CreatureId> getSummonedElement() const;
  bool isClearFogOfWar() const;
  bool forgetAfterBuilding() const;
  void onCreatureWalkedOver(Position, Vec2 direction) const;
  void onCreatureWalkedInto(Position, Vec2 direction) const;
  /**
   * @brief Calls special functionality to handle dropped items, if any.
   * @return possibly empty subset of the items that weren't consumned and can be dropped normally.
   */
  vector<PItem> dropItems(Position, vector<PItem>) const;
  bool canBuildBridgeOver() const;
  const LuxuryInfo& getLuxuryInfo() const;

  enum ConstructMessage { /*default*/BUILD, FILL_UP, REINFORCE, SET_UP };

  Furniture& setBlocking();
  Furniture& setBlockingEnemies();
  Furniture& setConstructMessage(optional<ConstructMessage>);
  Furniture& setDestroyable(double);
  Furniture& setDestroyable(double, DestroyAction::Type);
  Furniture& setItemDrop(ItemFactory);
  Furniture& setBurntRemains(FurnitureType);
  Furniture& setDestroyedRemains(FurnitureType);
  Furniture& setBlockVision();
  Furniture& setBlockVision(VisionId, bool);
  Furniture& setUsageType(FurnitureUsageType);
  Furniture& setUsageTime(TimeInterval);
  Furniture& setClickType(FurnitureClickType);
  Furniture& setTickType(FurnitureTickType);
  Furniture& setEntryType(FurnitureEntry);
  Furniture& setDroppedItems(FurnitureDroppedItems);
  Furniture& setFireInfo(const Fire&);
  Furniture& setIsWall();
  Furniture& setIsBuildingSupport();
  Furniture& setOverrideMovement();
  Furniture& setLayer(FurnitureLayer);
  Furniture& setLightEmission(double);
  Furniture& setCanHide();
  Furniture& setEmitsWarning();
  Furniture& setPlacementMessage(MsgType);
  Furniture& setSummonedElement(CreatureId);
  Furniture& setCanBuildBridgeOver();
  Furniture& setStopProjectiles();
  Furniture& setClearFogOfWar();
  Furniture& setCanRemoveWithCreaturePresent(bool state);
  Furniture& setCanRemoveNonFriendly(bool state);
  Furniture& setForgetAfterBuilding();
  Furniture& setLuxury(double luxury);
  Furniture& setOnBuilt(FurnitureOnBuilt);
  MovementSet& modMovementSet();

  SERIALIZATION_DECL(Furniture)

  ~Furniture();

  private:
  heap_optional<ViewObject> SERIAL(viewObject);
  string SERIAL(name);
  string SERIAL(pluralName);
  FurnitureType SERIAL(type);
  FurnitureLayer SERIAL(layer) = FurnitureLayer::MIDDLE;
  HeapAllocated<MovementSet> SERIAL(movementSet);
  heap_optional<Fire> SERIAL(fire);
  optional<FurnitureType> SERIAL(burntRemains);
  optional<FurnitureType> SERIAL(destroyedRemains);
  struct DestroyedInfo {
    double SERIAL(health);
    double SERIAL(strength);
    SERIALIZE_ALL(health, strength);
  };
  EnumMap<DestroyAction::Type, optional<DestroyedInfo>> SERIAL(destroyedInfo);
  heap_optional<ItemFactory> SERIAL(itemDrop);
  EnumSet<VisionId> SERIAL(blockVision);
  optional<FurnitureUsageType> SERIAL(usageType);
  optional<FurnitureClickType> SERIAL(clickType);
  optional<FurnitureTickType> SERIAL(tickType);
  optional<FurnitureOnBuilt> SERIAL(onBuilt);
  heap_optional<FurnitureEntry> SERIAL(entryType);
  heap_optional<FurnitureDroppedItems> SERIAL(droppedItems);
  TimeInterval SERIAL(usageTime) = 1_visible;
  bool SERIAL(overrideMovement) = false;
  bool SERIAL(removeWithCreaturePresent) = true;
  bool SERIAL(removeNonFriendly) = false;
  bool SERIAL(wall) = false;
  bool SERIAL(buildingSupport) = false;
  optional<ConstructMessage> SERIAL(constructMessage) = BUILD;
  double SERIAL(lightEmission) = 0;
  bool SERIAL(canHideHere) = false;
  bool SERIAL(warning) = false;
  WeakPointer<Creature> SERIAL(creator);
  optional<LocalTime> SERIAL(createdTime);
  optional<CreatureId> SERIAL(summonedElement);
  bool SERIAL(canBuildBridge) = false;
  bool SERIAL(noProjectiles) = false;
  bool SERIAL(clearFogOfWar) = false;
  bool SERIAL(xForgetAfterBuilding) = false;
  LuxuryInfo SERIAL(luxuryInfo);
  void updateViewObject();
};
