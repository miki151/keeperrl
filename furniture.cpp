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
#include "furniture_usage.h"
#include "furniture_entry.h"
#include "furniture_click.h"
#include "furniture_tick.h"

Furniture::Furniture(const string& n, const ViewObject& o, FurnitureType t, BlockType b, TribeId id)
    : Renderable(o), name(n), type(t), blockType(b), tribe(id) {}

Furniture::Furniture(const Furniture&) = default;

SERIALIZATION_CONSTRUCTOR_IMPL(Furniture);

Furniture::~Furniture() {}

template<typename Archive>
void Furniture::serialize(Archive& ar, const unsigned) {
  ar & SUBCLASS(Renderable);
  serializeAll(ar, name, type, blockType, tribe, fire, burntRemains, destroyedRemains, destroyActions, itemDrop);
  serializeAll(ar, blockVision, usageType, clickType, tickType, usageTime, overrideMovement, canSupport);
  serializeAll(ar, constructMessage, layer, entryType);
}

SERIALIZABLE(Furniture);

const string& Furniture::getName(FurnitureType type) {
  static EnumMap<FurnitureType, string> names(
      [] (FurnitureType type) { return FurnitureFactory::get(type, TribeId::getHostile())->getName(); });
  return names[type];
}

FurnitureLayer Furniture::getLayer(FurnitureType type) {
  static EnumMap<FurnitureType, FurnitureLayer> layers(
      [] (FurnitureType type) { return FurnitureFactory::get(type, TribeId::getHostile())->getLayer(); });
  return layers[type];
}

const string& Furniture::getName() const {
  return name;
}

FurnitureType Furniture::getType() const {
  return type;
}

bool Furniture::canEnter(const MovementType& movement) const {
  return blockType == NON_BLOCKING || (blockType == BLOCKING_ENEMIES && movement.isCompatible(getTribe()));
}

void Furniture::onEnter(Creature* c) const {
  if (entryType)
    FurnitureEntry::handle(*entryType, this, c);
}

void Furniture::destroy(Position pos, const DestroyAction& action) {
  pos.globalMessage("The " + name + " " + action.getIsDestroyed());
  pos.getGame()->addEvent({EventId::FURNITURE_DESTROYED, EventInfo::FurnitureEvent{pos, getLayer()}});
  if (*itemDrop)
    pos.dropItems((*itemDrop)->random());
  if (destroyedRemains)
    pos.replaceFurniture(this, FurnitureFactory::get(*destroyedRemains, getTribe()));
  else
    pos.removeFurniture(this);
}

void Furniture::tryToDestroyBy(Position pos, Creature* c, const DestroyAction& action) {
  if (auto& strength = destroyActions[action.getType()]) {
    c->addSound(action.getSound());
    *strength -= c->getAttr(AttrType::STRENGTH);
    if (*strength <= 0)
      destroy(pos, action);
  }
}

TribeId Furniture::getTribe() const {
  return *tribe;
}

void Furniture::tick(Position pos) {
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
        pos.getGame()->addEvent({EventId::FURNITURE_DESTROYED, EventInfo::FurnitureEvent{pos, getLayer()}});
        pos.updateMovement();
        if (burntRemains)
          pos.replaceFurniture(this, FurnitureFactory::get(*burntRemains, *tribe));
        else
          pos.removeFurniture(this);
        return;
      }
      pos.fireDamage(fire->getSize());
    }
  if (tickType)
    FurnitureTick::handle(*tickType, pos, this); // this function can delete this
}

bool Furniture::canSeeThru(VisionId id) const {
  return !blockVision.contains(id);
}

bool Furniture::isClickable() const {
  return !!clickType;
}

bool Furniture::overridesMovement() const {
  return overrideMovement;
}

void Furniture::click(Position pos) const {
  if (clickType) {
    FurnitureClick::handle(*clickType, pos, this);
    pos.setNeedsRenderUpdate(true);
  }
}

void Furniture::use(Position pos, Creature* c) const {
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

bool Furniture::canSupportDoor() const {
  return canSupport;
}

void Furniture::onConstructedBy(Creature* c) const {
  switch (constructMessage) {
    case BUILD:
      c->monsterMessage(c->getName().the() + " builds a " + getName());
      c->playerMessage("You build a " + getName());
      break;
    case FILL_UP:
      c->monsterMessage(c->getName().the() + " fills up the tunnel");
      c->playerMessage("You fill up the tunnel");
      break;
    case REINFORCE:
      c->monsterMessage(c->getName().the() + " reinforces the wall");
      c->playerMessage("You reinforce the wall");
      break;
  }
}

FurnitureLayer Furniture::getLayer() const {
  return layer;
}

Furniture& Furniture::setConstructMessage(Furniture::ConstructMessage msg) {
  constructMessage = msg;
  return *this;
}

const optional<Fire>& Furniture::getFire() const {
  return *fire;
}

optional<Fire>& Furniture::getFire() {
  return *fire;
}

bool Furniture::canDestroy(const MovementType& movement, const DestroyAction& action) const {
   return canDestroy(action) &&
       (!getFire() || !getFire()->isBurning()) &&
       (!movement.isCompatible(getTribe()) || action.canDestroyFriendly());
}

void Furniture::fireDamage(Position pos, double amount) {
  if (auto& fire = getFire()) {
    bool burning = fire->isBurning();
    fire->set(amount);
    if (!burning && fire->isBurning()) {
      pos.globalMessage("The " + getName() + " catches fire");
      modViewObject().setAttribute(ViewObject::Attribute::BURNING, fire->getSize());
      pos.updateMovement();
      pos.getLevel()->addTickingFurniture(pos.getCoord());
    }
  }
}

Furniture& Furniture::setDestroyable(double s) {
  setDestroyable(s, DestroyAction::Type::BOULDER);
  setDestroyable(s, DestroyAction::Type::BASH);
  return *this;
}

Furniture& Furniture::setDestroyable(double s, DestroyAction::Type type) {
  destroyActions[type] = s;
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

Furniture& Furniture::setEntryType(FurnitureEntryType type) {
  entryType = type;
  return *this;
}

Furniture& Furniture::setFireInfo(const Fire& f) {
  *fire = f;
  return *this;
}

Furniture& Furniture::setCanSupportDoor() {
  canSupport = true;
  return *this;
}

Furniture& Furniture::setOverrideMovement() {
  overrideMovement = true;
  return *this;
}

Furniture& Furniture::setLayer(FurnitureLayer l) {
  layer = l;
  return *this;
}

bool Furniture::canDestroy(const DestroyAction& action) const {
  return !!destroyActions[action.getType()];
}

optional<double> Furniture::getStrength(const DestroyAction& action) const {
  return destroyActions[action.getType()];
}
