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
#include "furniture_dropped_items.h"
#include "furniture_click.h"
#include "furniture_tick.h"
#include "movement_set.h"

static string makePlural(const string& s) {
  if (s.empty())
    return "";
  if (s.back() == 'y')
    return s.substr(0, s.size() - 1) + "ies";
  if (s.back() == 'h')
    return s + "es";
  if (s.back() == 's')
    return s;
  if (endsWith(s, "shelf"))
    return s.substr(0, s.size() - 5) + "shelves";
  return s + "s";
}

Furniture::Furniture(const string& n, const optional<ViewObject>& o, FurnitureType t, TribeId id)
    : viewObject(o), name(n), pluralName(makePlural(name)), type(t), movementSet(id) {
  movementSet->addTrait(MovementTrait::WALK);
}

Furniture::Furniture(const Furniture&) = default;

SERIALIZATION_CONSTRUCTOR_IMPL(Furniture)

Furniture::~Furniture() {}

template<typename Archive>
void Furniture::serialize(Archive& ar, const unsigned) {
  ar(SUBCLASS(OwnedObject<Furniture>), viewObject, removeNonFriendly);
  ar(name, pluralName, type, movementSet, fire, burntRemains, destroyedRemains, destroyActions, itemDrop);
  ar(blockVision, usageType, clickType, tickType, usageTime, overrideMovement, wall, creator, createdTime);
  ar(constructMessage, layer, entryType, lightEmission, canHideHere, warning, summonedElement, droppedItems);
  ar(canBuildBridge, noProjectiles, clearFogOfWar, removeWithCreaturePresent, xForgetAfterBuilding, showEfficiency);
}

SERIALIZABLE(Furniture)

const optional<ViewObject>& Furniture::getViewObject() const {  PROFILE
  return *viewObject;
}

optional<ViewObject>& Furniture::getViewObject() {
  PROFILE;
  return *viewObject;
}

const string& Furniture::getName(FurnitureType type, int count) {
  static EnumMap<FurnitureType, string> names(
      [] (FurnitureType type) { return FurnitureFactory::get(type, TribeId::getHostile())->getName(1); });
  static EnumMap<FurnitureType, string> pluralNames(
      [] (FurnitureType type) { return FurnitureFactory::get(type, TribeId::getHostile())->getName(2); });
  if (count == 1)
    return names[type];
  else
    return pluralNames[type];
}

FurnitureLayer Furniture::getLayer(FurnitureType type) {
  static EnumMap<FurnitureType, FurnitureLayer> layers(
      [] (FurnitureType type) { return FurnitureFactory::get(type, TribeId::getHostile())->getLayer(); });
  return layers[type];
}

const string& Furniture::getName(int count) const {
  if (count > 1)
    return pluralName;
  else
    return name;
}

bool Furniture::isWall(FurnitureType type) {
  static EnumMap<FurnitureType, bool> layers(
      [] (FurnitureType type) { return FurnitureFactory::get(type, TribeId::getHostile())->isWall(); });
  return layers[type];
}

pair<double, optional<int>> getPopulationIncreaseInfo(FurnitureType type) {
  switch (type) {
    case FurnitureType::PIGSTY:
      return {0.25, 4};
    case FurnitureType::MINION_STATUE:
      return {1, none};
    case FurnitureType::STONE_MINION_STATUE:
      return {1, 4};
    case FurnitureType::THRONE:
      return {10, none};
    default:
      return {0, none};
  }
}

optional<string> Furniture::getPopulationIncreaseDescription(FurnitureType type) {
  auto info = getPopulationIncreaseInfo(type);
  if (info.first > 0) {
    auto ret = "Increases population limit by " + toString(info.first);
    if (auto limit = info.second)
      ret += ", up to " + toString(*limit);
    ret += ".";
    return ret;
  }
  return none;
}

int Furniture::getPopulationIncrease(FurnitureType type, int numBuilt) {
  auto info = getPopulationIncreaseInfo(type);
  return min(int(numBuilt * info.first), info.second.value_or(1000000));
}

FurnitureType Furniture::getType() const {
  return type;
}

bool Furniture::isVisibleTo(WConstCreature c) const {
  PROFILE;
  if (entryType)
    return entryType->isVisibleTo(this, c);
  else
    return true;
}

const MovementSet& Furniture::getMovementSet() const {
  return *movementSet;
}

void Furniture::onEnter(WCreature c) const {
  if (entryType) {
    auto f = c->getPosition().modFurniture(layer);
    f->entryType->handle(f, c);
  }
}

void Furniture::destroy(Position pos, const DestroyAction& action) {
  pos.globalMessage("The " + name + " " + action.getIsDestroyed());
  auto myLayer = layer;
  auto myType = type;
  if (itemDrop)
    pos.dropItems(itemDrop->random());
  pos.removeFurniture(this, destroyedRemains ? FurnitureFactory::get(*destroyedRemains, getTribe()) : nullptr);
  pos.getGame()->addEvent(EventInfo::FurnitureDestroyed{pos, myType, myLayer});
}

void Furniture::tryToDestroyBy(Position pos, WCreature c, const DestroyAction& action) {
  if (auto& strength = destroyActions[action.getType()]) {
    c->addSound(action.getSound());
    double damage = c->getAttr(AttrType::DAMAGE);
    if (auto skill = action.getDestroyingSkillMultiplier())
      damage = damage * c->getAttributes().getSkills().getValue(*skill);
    *strength -= damage;
    if (*strength <= 0)
      destroy(pos, action);
  }
}

TribeId Furniture::getTribe() const {
  return movementSet->getTribe();
}

void Furniture::setTribe(TribeId id) {
  movementSet->setTribe(id);
}

void Furniture::tick(Position pos) {
  PROFILE;
  if (fire && fire->isBurning()) {
    if (viewObject)
      viewObject->setAttribute(ViewObject::Attribute::BURNING, fire->getSize());
    INFO << getName() << " burning " << fire->getSize();
    for (Position v : pos.neighbors8(Random))
      if (fire->getSize() > Random.getDouble() * 40)
        v.fireDamage(fire->getSize() / 20);
    fire->tick();
    if (fire->isBurntOut()) {
      pos.globalMessage("The " + getName() + " burns down");
      pos.updateMovementDueToFire();
      pos.removeCreatureLight(false);
      auto myLayer = layer;
      auto myType = type;
      pos.removeFurniture(this, burntRemains ? FurnitureFactory::get(*burntRemains, getTribe()) : nullptr);
      pos.getGame()->addEvent(EventInfo::FurnitureDestroyed{pos, myType, myLayer});
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

bool Furniture::stopsProjectiles(VisionId id) const {
  return !canSeeThru(id) || noProjectiles;
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

void Furniture::use(Position pos, WCreature c) const {
  if (usageType)
    FurnitureUsage::handle(*usageType, pos, this, c);
}

bool Furniture::canUse(WConstCreature c) const {
  if (usageType)
    return FurnitureUsage::canHandle(*usageType, c);
  else
    return true;
}

optional<FurnitureUsageType> Furniture::getUsageType() const {
  return usageType;
}

TimeInterval Furniture::getUsageTime() const {
  return usageTime;
}

optional<FurnitureClickType> Furniture::getClickType() const {
  return clickType;
}

bool Furniture::isTicking() const {
  return !!tickType;
}

bool Furniture::isWall() const {
  return wall;
}

void Furniture::onConstructedBy(WCreature c) {
  creator = c;
  createdTime = c->getLocalTime();
  if (constructMessage)
    switch (*constructMessage) {
      case BUILD:
        c->thirdPerson(c->getName().the() + " builds " + addAParticle(getName()));
        c->secondPerson("You build " + addAParticle(getName()));
        break;
      case FILL_UP:
        c->thirdPerson(c->getName().the() + " fills up the tunnel");
        c->secondPerson("You fill up the tunnel");
        break;
      case REINFORCE:
        c->thirdPerson(c->getName().the() + " reinforces the wall");
        c->secondPerson("You reinforce the wall");
        break;
      case SET_UP:
        c->thirdPerson(c->getName().the() + " sets up " + addAParticle(getName()));
        c->secondPerson("You set up " + addAParticle(getName()));
        break;
    }
}

FurnitureLayer Furniture::getLayer() const {
  return layer;
}

double Furniture::getLightEmission() const {
  if (fire && fire->isBurning())
    return Level::getCreatureLightRadius();
  else
    return lightEmission;
}

bool Furniture::canHide() const {
  return canHideHere;
}

bool Furniture::emitsWarning(WConstCreature) const {
  return warning;
}

bool Furniture::canRemoveWithCreaturePresent() const {
  return removeWithCreaturePresent && !wall;
}

bool Furniture::canRemoveNonFriendly() const {
  return removeNonFriendly;
}

WCreature Furniture::getCreator() const {
  return creator;
}

optional<LocalTime> Furniture::getCreatedTime() const {
  return createdTime;
}

optional<CreatureId> Furniture::getSummonedElement() const {
  return summonedElement;
}

bool Furniture::isClearFogOfWar() const {
  return clearFogOfWar;
}

bool Furniture::forgetAfterBuilding() const {
  return isWall() || xForgetAfterBuilding;
}

bool Furniture::isShowEfficiency() const {
  return showEfficiency;
}

vector<PItem> Furniture::dropItems(Position pos, vector<PItem> v) const {
  if (droppedItems) {
    return droppedItems->handle(pos, this, std::move(v));
  } else
    return v;
}

bool Furniture::canBuildBridgeOver() const {
  return canBuildBridge;
}

Furniture& Furniture::setBlocking() {
  movementSet->clearTraits();
  return *this;
}

Furniture& Furniture::setBlockingEnemies() {
  movementSet->addTrait(MovementTrait::WALK);
  movementSet->setBlockingEnemies();
  return *this;
}

MovementSet& Furniture::modMovementSet() {
  return *movementSet;
}

Furniture& Furniture::setConstructMessage(optional<ConstructMessage> msg) {
  constructMessage = msg;
  return *this;
}

const optional<Fire>& Furniture::getFire() const {
  return *fire;
}

bool Furniture::canDestroy(const MovementType& movement, const DestroyAction& action) const {
   return canDestroy(action) &&
       (!fire || !fire->isBurning()) &&
       (!movement.isCompatible(getTribe()) || action.canDestroyFriendly());
}

void Furniture::fireDamage(Position pos, double amount) {
  if (fire) {
    bool burning = fire->isBurning();
    fire->set(amount);
    if (!burning && fire->isBurning()) {
      pos.globalMessage("The " + getName() + " catches fire");
      if (viewObject)
        viewObject->setAttribute(ViewObject::Attribute::BURNING, fire->getSize());
      pos.updateMovementDueToFire();
      pos.getLevel()->addTickingFurniture(pos.getCoord());
      pos.addCreatureLight(false);
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
  itemDrop = f;
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

Furniture& Furniture::setUsageTime(TimeInterval t) {
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

Furniture& Furniture::setEntryType(FurnitureEntry type) {
  entryType = type;
  return *this;
}

Furniture& Furniture::setDroppedItems(FurnitureDroppedItems t) {
  droppedItems = t;
  return *this;
}

Furniture& Furniture::setFireInfo(const Fire& f) {
  fire = f;
  return *this;
}

Furniture& Furniture::setIsWall() {
  wall = true;
  return *this;
}

Furniture& Furniture::setOverrideMovement() {
  overrideMovement = true;
  return *this;
}

Furniture& Furniture::setCanRemoveWithCreaturePresent(bool s) {
  removeWithCreaturePresent = s;
  return *this;
}

Furniture& Furniture::setCanRemoveNonFriendly(bool s) {
  removeNonFriendly = s;
  return *this;
}

Furniture& Furniture::setForgetAfterBuilding() {
  xForgetAfterBuilding = true;
  return *this;
}

Furniture& Furniture::setShowEfficiency() {
  showEfficiency = true;
  return *this;
}

Furniture& Furniture::setLayer(FurnitureLayer l) {
  layer = l;
  return *this;
}

Furniture& Furniture::setLightEmission(double v) {
  lightEmission = v;
  return *this;
}

Furniture& Furniture::setCanHide() {
  canHideHere = true;
  return *this;
}

Furniture& Furniture::setEmitsWarning() {
  warning = true;
  return *this;
}

Furniture& Furniture::setSummonedElement(CreatureId id) {
  summonedElement = id;
  return *this;
}

Furniture& Furniture::setCanBuildBridgeOver() {
  canBuildBridge = true;
  return *this;
}

Furniture&Furniture::setStopProjectiles() {
  noProjectiles = true;
  return *this;
}

Furniture&Furniture::setClearFogOfWar() {
  clearFogOfWar = true;
  return *this;
}

bool Furniture::canDestroy(const DestroyAction& action) const {
  return !!destroyActions[action.getType()];
}

optional<double> Furniture::getStrength(const DestroyAction& action) const {
  return destroyActions[action.getType()];
}
