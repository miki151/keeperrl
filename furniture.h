#pragma once

#include "destroy_action.h"
#include "fire.h"
#include "item_factory.h"
#include "vision_id.h"
#include "event_listener.h"
#include "furniture_layer.h"

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

  Furniture(const string& name, const optional<ViewObject>&, FurnitureType, TribeId);
  Furniture(const Furniture&);
  const optional<ViewObject>& getViewObject() const;
  optional<ViewObject>& getViewObject();
  const string& getName(int count = 1) const;
  FurnitureType getType() const;
  bool isVisibleTo(WConstCreature) const;
  const MovementSet& getMovementSet() const;
  void onEnter(WCreature) const;
  bool canDestroy(const MovementType&, const DestroyAction&) const;
  bool canDestroy(const DestroyAction&) const;
  optional<double> getStrength(const DestroyAction&) const;
  void destroy(Position, const DestroyAction&);
  void tryToDestroyBy(Position, WCreature, const DestroyAction&);
  TribeId getTribe() const;
  void setTribe(TribeId);
  const optional<Fire>& getFire() const;
  optional<Fire>& getFire();
  void fireDamage(Position, double amount);
  void tick(Position);
  bool canSeeThru(VisionId) const;
  bool stopsProjectiles(VisionId) const;
  void click(Position) const;
  bool isClickable() const;
  bool overridesMovement() const;
  void use(Position, WCreature) const;
  bool canUse(WConstCreature) const;
  optional<FurnitureUsageType> getUsageType() const;
  int getUsageTime() const;
  optional<FurnitureClickType> getClickType() const;
  bool isTicking() const;
  bool isWall() const;
  void onConstructedBy(WCreature);
  FurnitureLayer getLayer() const;
  double getLightEmission() const;
  bool canHide() const;
  bool emitsWarning(WConstCreature) const;
  WCreature getCreator() const;
  optional<double> getCreatedTime() const;
  optional<CreatureId> getSummonedElement() const;
  /**
   * @brief Calls special functionality to handle dropped items, if any.
   * @return possibly empty subset of the items that weren't consumned and can be dropped normally.
   */
  vector<PItem> dropItems(Position, vector<PItem>) const;
  bool canBuildBridgeOver() const;

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
  Furniture& setUsageTime(int);
  Furniture& setClickType(FurnitureClickType);
  Furniture& setTickType(FurnitureTickType);
  Furniture& setEntryType(FurnitureEntry);
  Furniture& setDroppedItems(FurnitureDroppedItems);
  Furniture& setFireInfo(const Fire&);
  Furniture& setIsWall();
  Furniture& setOverrideMovement();
  Furniture& setLayer(FurnitureLayer);
  Furniture& setLightEmission(double);
  Furniture& setCanHide();
  Furniture& setEmitsWarning();
  Furniture& setPlacementMessage(MsgType);
  Furniture& setSummonedElement(CreatureId);
  Furniture& setCanBuildBridgeOver();
  Furniture& setStopProjectiles();
  MovementSet& modMovementSet();

  SERIALIZATION_DECL(Furniture)

  ~Furniture();

  private:
  HeapAllocated<optional<ViewObject>> SERIAL(viewObject);
  string SERIAL(name);
  string SERIAL(pluralName);
  FurnitureType SERIAL(type);
  FurnitureLayer SERIAL(layer) = FurnitureLayer::MIDDLE;
  HeapAllocated<MovementSet> SERIAL(movementSet);
  HeapAllocated<optional<Fire>> SERIAL(fire);
  optional<FurnitureType> SERIAL(burntRemains);
  optional<FurnitureType> SERIAL(destroyedRemains);
  EnumMap<DestroyAction::Type, optional<double>> SERIAL(destroyActions);
  HeapAllocated<optional<ItemFactory>> SERIAL(itemDrop);
  EnumSet<VisionId> SERIAL(blockVision);
  optional<FurnitureUsageType> SERIAL(usageType);
  optional<FurnitureClickType> SERIAL(clickType);
  optional<FurnitureTickType> SERIAL(tickType);
  HeapAllocated<optional<FurnitureEntry>> SERIAL(entryType);
  HeapAllocated<optional<FurnitureDroppedItems>> SERIAL(droppedItems);
  int SERIAL(usageTime) = 1;
  bool SERIAL(overrideMovement) = false;
  bool SERIAL(wall) = false;
  optional<ConstructMessage> SERIAL(constructMessage) = BUILD;
  double SERIAL(lightEmission) = 0;
  bool SERIAL(canHideHere) = false;
  bool SERIAL(warning) = false;
  WCreature SERIAL(creator);
  optional<double> SERIAL(createdTime);
  optional<CreatureId> SERIAL(summonedElement);
  bool SERIAL(canBuildBridge) = false;
  bool SERIAL(noProjectiles) = false;
};
