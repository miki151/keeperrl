#include "stdafx.h"
#include "furniture.h"
#include "player_message.h"
#include "tribe.h"
#include "fire.h"
#include "view_object.h"
#include "furniture_factory.h"
#include "creature.h"
#include "creature_attributes.h"
#include "sound.h"
#include "item_factory.h"
#include "item.h"
#include "game.h"
#include "event_listener.h"
#include "level.h"

Furniture::Furniture(const string& n, const ViewObject& o, FurnitureType t, BlockType b, TribeId id)
    : Renderable(o), name(n), type(t), blockType(b), tribe(id) {}

Furniture::Furniture(const Furniture&) = default;

SERIALIZATION_CONSTRUCTOR_IMPL(Furniture);

Furniture::~Furniture() {}

DestroyAction::Value Furniture::getDefaultDestroyAction() const {
  if (canCut)
    return DestroyAction::CUT;
  else
    return DestroyAction::BASH;
}

template<typename Archive>
void Furniture::serialize(Archive& ar, const unsigned) {
  ar & SUBCLASS(Renderable) & SUBCLASS(UniqueEntity<Furniture>);
  serializeAll(ar, name, type, blockType, tribe, fire, burntRemains, destroyedRemains, strength, itemDrop, canCut);
  serializeAll(ar, blockVision, usageType, clickType, tickType, usageTime);
}

SERIALIZABLE(Furniture);


const string& Furniture::getName() const {
  return name;
}

FurnitureType Furniture::getType() const {
  return type;
}

bool Furniture::canEnter(const MovementType& movement) const {
  return blockType == NON_BLOCKING || (blockType == BLOCKING_ENEMIES && movement.isCompatible(getTribe()));
}
  
void Furniture::destroy(Position pos) {
  pos.globalMessage("The " + name + " " + DestroyAction::getIsDestroyed(getDefaultDestroyAction()));
  pos.getGame()->addEvent({EventId::FURNITURE_DESTROYED, pos});
  if (*itemDrop)
    pos.dropItems((*itemDrop)->random());
  if (destroyedRemains)
    pos.replaceFurniture(this, FurnitureFactory::get(*destroyedRemains, getTribe()));
  else
    pos.removeFurniture(this);
}

void Furniture::tryToDestroyBy(Position pos, Creature* c) {
  c->addSound(DestroyAction::getSound(getDefaultDestroyAction()));
  if (!strength)
    destroy(pos);
  else {
    *strength -= c->getAttr(AttrType::STRENGTH);
    if (*strength <= 0)
      destroy(pos);
  }
}

TribeId Furniture::getTribe() const {
  return *tribe;
}

void Furniture::tick(Position pos) {
  if (tickType)
    FurnitureTick::handle(*tickType, pos, this);
  if (auto& fire = getFire())
    if (fire->isBurning()) {
      modViewObject().setAttribute(ViewObject::Attribute::BURNING, fire->getSize());
      Debug() << getName() << " burning " << fire->getSize();
      for (Position v : pos.neighbors8(Random))
        if (fire->getSize() > Random.getDouble() * 40)
          v.fireDamage(fire->getSize() / 20);
      fire->tick();
      if (fire->isBurntOut()) {
        pos.globalMessage("The " + getName() + " burns down");
        pos.getGame()->addEvent({EventId::FURNITURE_DESTROYED, pos});
        pos.updateMovement();
        if (burntRemains)
          pos.replaceFurniture(this, FurnitureFactory::get(*burntRemains, *tribe));
        else
          pos.removeFurniture(this);
        return;
      }
      pos.fireDamage(fire->getSize());
    }
}

bool Furniture::canSeeThru(VisionId id) const {
  return !blockVision.contains(id);
}

bool Furniture::click(Position pos) {
  if (clickType) {
    FurnitureClick::handle(*clickType, pos, this);
    pos.setNeedsRenderUpdate(true);
    return true;
  } else
    return false;
}

void Furniture::use(Position pos, Creature* c) {
  if (usageType)
    FurnitureUsage::handle(*usageType, pos, this, c);
}

optional<FurnitureUsageType> Furniture::getUsageType() const {
  return usageType;
}

int Furniture::getUsageTime() const {
  return usageTime;
}

optional<FurnitureClickType> Furniture::getClickType() const {
  return clickType;
}

bool Furniture::isTicking() const {
  return !!tickType;
}

const optional<Fire>& Furniture::getFire() const {
  return *fire;
}

optional<Fire>& Furniture::getFire() {
  return *fire;
}

bool Furniture::canDestroy(const MovementType& movement) {
   return !!strength && (!getFire() || !getFire()->isBurning()) && !movement.isCompatible(getTribe());
}

bool Furniture::canDestroy(const Creature* c) {
  return canDestroy(c->getMovementType()) || (!!strength && c->getAttributes().isInvincible());
}

void Furniture::fireDamage(Position pos, double amount) {
  if (auto& fire = getFire()) {
    bool burning = fire->isBurning();
    fire->set(amount);
    if (!burning && fire->isBurning()) {
      pos.globalMessage("The " + getName() + " catches fire");
      modViewObject().setAttribute(ViewObject::Attribute::BURNING, fire->getSize());
      pos.updateMovement();
      pos.getLevel()->addTickingSquare(pos.getCoord());
    }
  }
}

Furniture& Furniture::setStrength(double s) {
  strength = s;
  return *this;
}

Furniture& Furniture::setItemDrop(ItemFactory f) {
  itemDrop->reset(f);
  return *this;
}

Furniture& Furniture::setBurntRemains(FurnitureType t) {
  burntRemains = t;
  return *this;
}

Furniture& Furniture::setDestroyedRemains(FurnitureType t) {
  destroyedRemains = t;
  return *this;
}

Furniture& Furniture::setCanCut() {
  canCut = true;
  return *this;
}

Furniture& Furniture::setBlockVision() {
  for (auto vision : ENUM_ALL(VisionId))
    blockVision.insert(vision);
  return *this;
}

Furniture& Furniture::setBlockVision(VisionId id, bool blocks) {
  blockVision.set(id, blocks);
  return *this;
}

Furniture& Furniture::setUsageType(FurnitureUsageType type) {
  usageType = type;
  return *this;
}

Furniture& Furniture::setUsageTime(int t) {
  usageTime = t;
  return *this;
}

Furniture& Furniture::setClickType(FurnitureClickType type) {
  clickType = type;
  return *this;
}

Furniture& Furniture::setTickType(FurnitureTickType type) {
  tickType = type;
  return *this;
}

Furniture& Furniture::setFireInfo(const Fire& f) {
  *fire = f;
  return *this;
}

bool Furniture::canDestroy(DestroyAction::Value value) const {
  if (value == DestroyAction::CUT)
    return canCut;
  else
    return true;
}
