#pragma once

#include "position.h"
#include "renderable.h"
#include "destroy_action.h"
#include "fire.h"
#include "item_factory.h"
#include "vision_id.h"
#include "furniture_click.h"
#include "furniture_usage.h"
#include "furniture_tick.h"
#include "event_listener.h"

class TribeId;
class Creature;
class MovementType;
class Fire;
class ItemFactory;
class GameEvent;

class Furniture : public Renderable {
  public:
  enum BlockType { BLOCKING, NON_BLOCKING, BLOCKING_ENEMIES };
  Furniture(const string& name, const ViewObject&, FurnitureType, BlockType, TribeId);
  Furniture(const Furniture&);
  const string& getName() const;
  FurnitureType getType() const;
  bool canEnter(const MovementType&) const;
  bool canDestroy(const Creature*) const;
  bool canDestroy(const MovementType&) const;
  bool canDestroy(DestroyAction::Value) const;
  void destroy(Position);
  void tryToDestroyBy(Position, Creature*);
  TribeId getTribe() const;
  const optional<Fire>& getFire() const;
  optional<Fire>& getFire();
  void fireDamage(Position, double amount);
  void tick(Position);
  bool canSeeThru(VisionId) const;
  void click(Position) const;
  bool isClickable() const;
  bool overridesMovement() const;
  void use(Position, Creature*) const;
  optional<FurnitureUsageType> getUsageType() const;
  int getUsageTime() const;
  optional<FurnitureClickType> getClickType() const;
  bool isTicking() const;

  Furniture& setDestroyable(double);
  Furniture& setItemDrop(ItemFactory);
  Furniture& setBurntRemains(FurnitureType);
  Furniture& setDestroyedRemains(FurnitureType);
  Furniture& setCanCut();
  Furniture& setBlockVision();
  Furniture& setBlockVision(VisionId, bool);
  Furniture& setUsageType(FurnitureUsageType);
  Furniture& setUsageTime(int);
  Furniture& setClickType(FurnitureClickType);
  Furniture& setTickType(FurnitureTickType);
  Furniture& setFireInfo(const Fire&);
  Furniture& setOverrideMovement();

  SERIALIZATION_DECL(Furniture)

  ~Furniture();

  private:
  DestroyAction::Value getDefaultDestroyAction() const;
  string SERIAL(name);
  FurnitureType SERIAL(type);
  BlockType SERIAL(blockType);
  HeapAllocated<TribeId> SERIAL(tribe);
  HeapAllocated<optional<Fire>> SERIAL(fire);
  optional<FurnitureType> SERIAL(burntRemains);
  optional<FurnitureType> SERIAL(destroyedRemains);
  optional<double> SERIAL(strength);
  HeapAllocated<optional<ItemFactory>> SERIAL(itemDrop);
  bool SERIAL(canCut) = false;
  EnumSet<VisionId> SERIAL(blockVision);
  optional<FurnitureUsageType> SERIAL(usageType);
  optional<FurnitureClickType> SERIAL(clickType);
  optional<FurnitureTickType> SERIAL(tickType);
  int SERIAL(usageTime) = 1;
  bool SERIAL(overrideMovement) = false;
};
