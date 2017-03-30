#pragma once

#include "position.h"
#include "renderable.h"
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

class Furniture : public Renderable {
  public:
  enum BlockType { BLOCKING, NON_BLOCKING, BLOCKING_ENEMIES };

  static const string& getName(FurnitureType, int count = 1);
  static FurnitureLayer getLayer(FurnitureType);

  Furniture(const string& name, const ViewObject&, FurnitureType, BlockType, TribeId);
  Furniture(const Furniture&);
  const string& getName(int count = 1) const;
  FurnitureType getType() const;
  bool canEnter(const MovementType&) const;
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
  void onConstructedBy(WCreature) const;
  FurnitureLayer getLayer() const;
  double getLightEmission() const;
  bool canHide() const;

  enum ConstructMessage { /*default*/BUILD, FILL_UP, REINFORCE };

  Furniture& setConstructMessage(ConstructMessage);
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
  Furniture& setEntryType(FurnitureEntryType);
  Furniture& setFireInfo(const Fire&);
  Furniture& setIsWall();
  Furniture& setOverrideMovement();
  Furniture& setLayer(FurnitureLayer);
  Furniture& setLightEmission(double);
  Furniture& setCanHide();

  SERIALIZATION_DECL(Furniture)

  ~Furniture();

  private:
  string SERIAL(name);
  string SERIAL(pluralName);
  FurnitureType SERIAL(type);
  FurnitureLayer SERIAL(layer) = FurnitureLayer::MIDDLE;
  BlockType SERIAL(blockType);
  HeapAllocated<TribeId> SERIAL(tribe);
  HeapAllocated<optional<Fire>> SERIAL(fire);
  optional<FurnitureType> SERIAL(burntRemains);
  optional<FurnitureType> SERIAL(destroyedRemains);
  EnumMap<DestroyAction::Type, optional<double>> SERIAL(destroyActions);
  HeapAllocated<optional<ItemFactory>> SERIAL(itemDrop);
  EnumSet<VisionId> SERIAL(blockVision);
  optional<FurnitureUsageType> SERIAL(usageType);
  optional<FurnitureClickType> SERIAL(clickType);
  optional<FurnitureTickType> SERIAL(tickType);
  optional<FurnitureEntryType> SERIAL(entryType);
  int SERIAL(usageTime) = 1;
  bool SERIAL(overrideMovement) = false;
  bool SERIAL(wall) = false;
  ConstructMessage SERIAL(constructMessage) = BUILD;
  double SERIAL(lightEmission) = 0;
  bool SERIAL(canHideHere) = false;
};
