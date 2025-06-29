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
#include "game_event.h"
#include "level.h"
#include "furniture_usage.h"
#include "furniture_entry.h"
#include "furniture_dropped_items.h"
#include "furniture_click.h"
#include "furniture_tick.h"
#include "movement_set.h"
#include "fx_info.h"
#include "furniture_on_built.h"
#include "content_factory.h"
#include "item_list.h"
#include "collective.h"

/*Furniture::Furniture(const string& n, const optional<ViewObject>& o, FurnitureType t, TribeId id)
    : viewObject(o), name(n), pluralName(makePlural(name)), type(t), movementSet(id) {
  movementSet->addTrait(MovementTrait::WALK);
}*/

Furniture::Furniture(const Furniture&) = default;
Furniture::Furniture(Furniture&&) noexcept = default;
Furniture& Furniture::operator=(Furniture&&) = default;
Furniture& Furniture::operator=(const Furniture&) = default;

Furniture::Furniture() {
  movementSet->addTrait(MovementTrait::WALK);
}

Furniture::~Furniture() {}

template<typename Archive>
void Furniture::serializeImpl(Archive& ar, const unsigned version) {
  ar(OPTION(viewObject), OPTION(removeNonFriendly), OPTION(canBuildOutsideOfTerritory));
  ar(NAMED(name), OPTION(pluralName), OPTION(type), OPTION(movementSet), OPTION(fire), OPTION(burntRemains), OPTION(destroyedRemains));
  ar(OPTION(destroyedInfo), OPTION(itemDrop), OPTION(wall), SKIP(creator), NAMED(createdTime), OPTION(canSilentlyReplace));
  ar(OPTION(blockVision), NAMED(usageType), NAMED(clickType), NAMED(tickType), OPTION(usageTime), OPTION(overrideMovement));
  ar(NAMED(constructMessage), OPTION(layer), OPTION(entryType), OPTION(lightEmission), OPTION(canHideHere), OPTION(warning));
  ar(NAMED(summonedElement), OPTION(droppedItems), OPTION(xForgetAfterBuilding), OPTION(requiredSupport), OPTION(builtOver));
  ar(OPTION(defaultBridge), OPTION(noProjectiles), OPTION(clearFogOfWar), OPTION(removeWithCreaturePresent), OPTION(upgrade));
  ar(OPTION(luxury), NAMED(onBuilt), OPTION(burnsDownMessage), OPTION(maxTraining), OPTION(bridge));
  ar(OPTION(bedType), OPTION(requiresLight), OPTION(populationIncrease), OPTION(destroyFX), OPTION(tryDestroyFX), OPTION(walkOverFX));
  ar(OPTION(walkIntoFX), OPTION(usageFX), OPTION(hostileSpell), OPTION(lastingEffect), NAMED(meltInfo), NAMED(dissolveTo));
  ar(OPTION(bloodCountdown), SKIP(bloodTime), NAMED(destroyedEffect), NAMED(freezeTo), NAMED(fillPit), NAMED(itemsRemovedEffect));
  ar(OPTION(eyeball), OPTION(storageIds), OPTION(hidesItems), NAMED(emptyViewId), OPTION(diningFurniture));
  ar(OPTION(usagePredicate), OPTION(minedAchievement), OPTION(removeInstantly), OPTION(buildingFloor), OPTION(usageSound));
  ar(OPTION(walkIntoSound), OPTION(destroySound));
  if (version >= 1)
    ar(NAMED(otherStairs));
  if (version >= 2)
    ar(OPTION(workshopSpeedBoost));
}

template <class Archive>
void Furniture::serialize(Archive& ar1, const unsigned int v) {
  if (Archive::is_loading::value)
    movementSet->clearTraits();
  serializeImpl(ar1, v);
}

SERIALIZABLE(Furniture)

const heap_optional<ViewObject>& Furniture::getViewObject() const {
  return viewObject;
}

heap_optional<ViewObject>& Furniture::getViewObject() {
  return viewObject;
}

void Furniture::updateViewObject() {
  if (viewObject) {
    double minHealth = 1;
    for (auto action : ENUM_ALL(DestroyAction::Type))
      if (auto& info = destroyedInfo[action])
        minHealth = min(minHealth, info->health);
    if (minHealth < 1) {
      viewObject->setAttribute(ViewObjectAttribute::HEALTH, minHealth);
      if (isWall())
        viewObject->setModifier(ViewObjectModifier::FURNITURE_CRACKS);
    }
  }
}

const TString& Furniture::getName(int count) const {
  if (count > 1)
    return pluralName;
  else
    return name;
}

void Furniture::setName(const TString& n) {
  name = n;
  viewObject->setDescription(capitalFirst(name));
}

FurnitureType Furniture::getType() const {
  return type;
}

bool Furniture::isVisibleTo(const Creature* c) const {
  PROFILE;
  if (entryType)
    return entryType->isVisibleTo(this, c);
  else
    return true;
}

const MovementSet& Furniture::getMovementSet() const {
  return *movementSet;
}

void Furniture::onEnter(Creature* c) const {
  if (entryType) {
    auto f = c->getPosition().modFurniture(layer);
    f->entryType->handle(f, c);
  }
}

void Furniture::onItemsRemoved(Position pos) const {
  if (itemsRemovedEffect)
    itemsRemovedEffect->apply(pos);
}

const heap_optional<ItemList>& Furniture::getItemDrop() const {
  return itemDrop;
}

const vector<StorageId>& Furniture::getStorageId() const {
  return storageIds;
}

void Furniture::destroy(Position pos, const DestroyAction& action, Creature* destroyedBy) {
  if (!destroyedEffect)
    pos.globalMessage(TSentence(action.getIsDestroyed(), name));
  if (itemDrop)
    pos.dropItems(itemDrop->random(pos.getGame()->getContentFactory(), pos.getModelDifficulty()));
  if (destroyFX)
    pos.getGame()->addEvent(EventInfo::FX{pos, *destroyFX});
  auto effect = destroyedEffect;
  pos.removeFurniture(
      this,
      destroyedRemains ? pos.getGame()->getContentFactory()->furniture.getFurniture(*destroyedRemains, getTribe()) : nullptr,
      destroyedBy);
  if (effect)
    effect->apply(pos);
  if (destroySound)
    pos.addSound(*destroySound);
}

void Furniture::tryToDestroyBy(Position pos, Creature* c, const DestroyAction& action) {
  if (auto& info = destroyedInfo[action.getType()]) {
    double damage = action.getDamage(c);
    info->health -= damage / info->strength;
    updateViewObject();
    pos.setNeedsRenderAndMemoryUpdate(true);
    if (tryDestroyFX)
      pos.getGame()->addEvent(EventInfo::FX{pos, *tryDestroyFX});
    if (info->health <= 0)
      destroy(pos, action, c);
    else
      if (auto s = action.getSound())
        pos.addSound(*s);
  }
}

TribeId Furniture::getTribe() const {
  return movementSet->getTribe();
}

void Furniture::setTribe(TribeId id) {
  movementSet->setTribe(id);
}

bool Furniture::hasRequiredSupport(Position pos) const {
  return requiredSupport.empty() || !!getSupportInfo(pos);
}

bool Furniture::doesHideItems() const {
  return hidesItems;
}

optional<FurnitureType> Furniture::getDiningFurnitureType() const {
  return diningFurniture;
}

const optional<CreaturePredicate>& Furniture::getUsagePredicate() const {
  return usagePredicate;
}

const optional<ViewId>& Furniture::getEmptyViewId() const {
  return emptyViewId;
}

const optional<AchievementId>& Furniture::getMinedAchievement() const {
  return minedAchievement;
}

bool Furniture::canRemoveInstantly() const {
  return removeInstantly;
}

bool Furniture::isBuildingFloor() const {
  return buildingFloor;
}

optional<FurnitureType> Furniture::getOtherStairs() const {
  return otherStairs;
}

optional<ViewId> Furniture::getSupportViewId(Position pos) const {
  if (auto ret = getSupportInfo(pos))
    return ret->viewId;
  return none;
}

const Furniture::SupportInfo* Furniture::getSupportInfo(Position pos) const {
  auto hasSupport = [&](const vector<Dir>& dirs) {
    for (auto dir : dirs)
      if (!pos.plus(Vec2(dir)).isWall())
        return false;
    return true;
  };
  for (int i : All(requiredSupport)) {
    if (hasSupport(requiredSupport[i].dirs))
      return &requiredSupport[i];
  }
  return nullptr;
}

void Furniture::updateFire(Position pos, FurnitureLayer supposedLayer) {
  PROFILE_BLOCK("Furniture::updateFire");
  PROFILE_BLOCK(type.data());
  if (fire && fire->isBurning()) {
    {
      auto otherF = pos.getFurniture(layer);
      auto otherFMod = pos.modFurniture(layer);
      CHECK(otherF == this);
    }
    auto burnState = fire->getBurnState();
    if (viewObject)
      viewObject->setAttribute(ViewObject::Attribute::BURNING, min(1.0, double(burnState) / 50));
//    INFO << getName() << " burning ";
    for (Position v : pos.neighbors8())
      v.fireDamage(burnState / 5, nullptr);
    pos.fireDamage(burnState, nullptr);
    fire->tick();
    if (fire->isBurntOut()) {
      switch (burnsDownMessage) {
        case BurnsDownMessage::BURNS_DOWN:
          pos.globalMessage(TSentence("FURNITURE_BURNS_DOWN", getName()));
          break;
        case BurnsDownMessage::STOPS_BURNING:
          pos.globalMessage(TSentence("FURNITURE_STOPS_BURNING", getName()));
          break;
      }
      pos.updateMovementDueToFire();
      pos.removeCreatureLight(false);
      pos.removeFurniture(this, burntRemains ?
          pos.getGame()->getContentFactory()->furniture.getFurniture(*burntRemains, getTribe()) : nullptr);
      return;
    }
  }
  if (bloodTime && *bloodTime <= pos.getModel()->getLocalTime())
    spreadBlood(pos);
}

void Furniture::tick(Position pos, FurnitureLayer supposedLayer) {
  PROFILE_BLOCK("Furniture::tick");
  PROFILE_BLOCK(type.data());
  if (tickType)
    tickType->handle(pos, this); // this function can delete this
}

bool Furniture::blocksAnyVision() const {
  return !blockVision.isEmpty();
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
    pos.setNeedsRenderAndMemoryUpdate(true);
  }
}

void Furniture::use(Position pos, Creature* c) const {
  if (usageType)
    FurnitureUsage::handle(*usageType, pos, this, c);
  if (usageSound)
    pos.addSound(*usageSound);
}

bool Furniture::canUse(const Creature* c) const {
  if (usageType)
    return FurnitureUsage::canHandle(*usageType, c);
  else
    return true;
}

optional<FurnitureUsageType> Furniture::getUsageType() const {
  return usageType;
}

bool Furniture::hasUsageType(BuiltinUsageId id) const {
  return usageType && usageType->getReferenceMaybe<BuiltinUsageId>() == id;
}

TimeInterval Furniture::getUsageTime() const {
  return usageTime;
}

optional<FurnitureClickType> Furniture::getClickType() const {
  return clickType;
}

optional<FurnitureTickType> Furniture::getTickType() const {
  return tickType;
}

const heap_optional<FurnitureEntry>& Furniture::getEntryType() const {
  return entryType;
}

bool Furniture::isTicking() const {
  return !!tickType;
}

bool Furniture::isWall() const {
  return wall;
}

void Furniture::onConstructedBy(Position pos, Creature* c) {
  if (c) {
    creator = c;
    createdTime = c->getLocalTime();
    if (constructMessage)
      switch (*constructMessage) {
        case ConstructMessage::BUILD:
          c->verb(TStringId("YOU_BUILD"), TStringId("HE_BUILDS"), TSentence("A_ARTICLE", getName()));
          break;
        case ConstructMessage::FILL_UP:
          c->verb(TStringId("YOU_FILL_UP_TUNNEL"), TStringId("HE_FILLS_UP_TUNNEL"));
          break;
        case ConstructMessage::REINFORCE:
          c->verb(TStringId("YOU_REINFORCE_THE_WALL"), TStringId("HE_REINFORCES_THE_WALL"));
          break;
        case ConstructMessage::SET_UP:
          c->verb(TStringId("YOU_SET_UP"), TStringId("HE_SETS_UP"), TSentence("A_ARTICLE", getName()));
          break;
      }
  }
  if (onBuilt)
    handleOnBuilt(pos, this, *onBuilt);
}

void Furniture::beforeRemoved(Position pos) const {
  if (onBuilt)
    handleBeforeRemoved(pos, this, *onBuilt);
}

bool Furniture::isStairs() const {
  return onBuilt && !!getStairDirection(*onBuilt);
}

optional<Position> Furniture::getSecondPart(Position pos) const {
  if (onBuilt)
    if (auto dir = getStairDirection(*onBuilt)) {
      int myIndex = *pos.getModel()->getMainLevelDepth(pos.getLevel());
      int nextIndex = pos.getModel()->getMainLevelsDepth().clamp(myIndex + *dir);
      if (nextIndex != myIndex)
        return Position(pos.getCoord(), pos.getModel()->getMainLevel(nextIndex));
    }
  return none;
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

bool Furniture::emitsWarning(const Creature*) const {
  return warning;
}

bool Furniture::canRemoveWithCreaturePresent() const {
  return removeWithCreaturePresent && !wall;
}

bool Furniture::canRemoveNonFriendly() const {
  return removeNonFriendly;
}

Creature* Furniture::getCreator() const {
  if (auto c = creator.get())
    if (!c->isDead())
      return c;
  return nullptr;
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
  return xForgetAfterBuilding;
}

void Furniture::onCreatureWalkedOver(Position pos, Vec2 direction) const {
  if (walkOverFX)
    pos.getGame()->addEvent((EventInfo::FX{pos, *walkOverFX, direction}));
}

void Furniture::onCreatureWalkedInto(Position pos, Vec2 direction) const {
  auto game = pos.getGame();
  if (walkIntoFX) {
    game->addEvent((EventInfo::FX{pos, *walkIntoFX, direction}));
  }
  if (walkIntoSound)
    pos.addSound(*walkIntoSound);
}

bool Furniture::onBloodNear(Position pos) {
  if (bloodCountdown)
    if (--*bloodCountdown == 0)
      return true;
  return false;
}

bool Furniture::hasBlood() const {
  return !!bloodTime;
}

void Furniture::spreadBlood(Position pos) {
  if (!!bloodCountdown) {
    bloodCountdown = none;
    viewObject->setModifier(ViewObjectModifier::BLOODY);
    setName(TStringId("BLOODY_WATER"));
    for (auto v : pos.neighbors4())
      if (auto f = v.getFurniture(layer))
        if (!!f->bloodCountdown) {
          v.modFurniture(layer)->bloodTime = pos.getModel()->getLocalTime() + 1_visible;
          v.getLevel()->addBurningFurniture(v.getCoord(), layer);
        }
  }
}

int Furniture::getMaxTraining(AttrType t) const {
  return getValueMaybe(maxTraining, t).value_or(0);
}

const HashMap<AttrType, int>& Furniture::getMaxTraining() const {
  return maxTraining;
}

optional<FurnitureType> Furniture::getUpgrade() const {
  return upgrade;
}

optional<FXVariantName> Furniture::getUsageFX() const {
  return usageFX;
}


vector<PItem> Furniture::dropItems(Position pos, vector<PItem> v) const {
  if (droppedItems) {
    return droppedItems->handle(pos, this, std::move(v));
  } else
    return v;
}

optional<FurnitureType> Furniture::getDefaultBridge() const {
  return defaultBridge;
}

optional<FurnitureType> Furniture::getFillPit() const {
  return fillPit;
}

double Furniture::getLuxury() const {
  return luxury;
}

const Furniture::PopulationInfo& Furniture::getPopulationIncrease() const {
  return populationIncrease;
}

const vector<FurnitureType>& Furniture::getBuiltOver() const {
  return builtOver;
}

bool Furniture::isBridge() const {
  return bridge;
}

bool Furniture::silentlyReplace() const {
  return canSilentlyReplace;
}

void Furniture::setType(FurnitureType t) {
  type = t;
}

bool Furniture::buildOutsideOfTerritory() const {
  return canBuildOutsideOfTerritory;
}

bool Furniture::isRequiresLight() const {
  return requiresLight;
}

bool Furniture::isHostileSpell() const {
  return hostileSpell;
}

bool Furniture::isEyeball() const {
  return eyeball;
}

optional<BedType> Furniture::getBedType() const {
  return bedType;
}

const optional<FurnitureEffectInfo>& Furniture::getLastingEffectInfo() const {
  return lastingEffect;
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

const heap_optional<Fire>& Furniture::getFire() const {
  return fire;
}

bool Furniture::canDestroy(const Position& pos, const MovementType& movement, const DestroyAction& action) const {
  auto outsideTerritory = [&] {
    auto col = pos.getCollective();
    return !col || col->getTribeId() != getTribe();
  };
  return canDestroy(action) &&
      (!fire || !fire->isBurning()) &&
      (!movement.isCompatible(getTribe()) || action.canDestroyFriendly() || outsideTerritory());
}

bool Furniture::acidDamage(Position pos) {
  if (dissolveTo) {
    pos.globalMessage(TSentence("FURNITURE_DISSOLVES", getName()));
    PFurniture replace = pos.getGame()->getContentFactory()->furniture.getFurniture(*dissolveTo, getTribe());
    pos.removeFurniture(this, std::move(replace));
    return true;
  }
  else if (destroyedInfo[DestroyAction::Type::BASH]) {
    destroy(pos, DestroyAction(DestroyAction::Type::BASH));
    return true;
  }
  return false;
}

bool Furniture::fireDamage(Position pos, bool withMessage) {
  if (meltInfo) {
    pos.globalMessage(TSentence("FURNITURE_MELTS", getName()));
    PFurniture replace;
    if (meltInfo->meltTo)
      replace = pos.getGame()->getContentFactory()->furniture.getFurniture(*meltInfo->meltTo, getTribe());
    pos.removeFurniture(this, std::move(replace));
    return true;
  }
  else if (fire) {
    bool burning = fire->isBurning();
    fire->set();
    if (!burning && fire->isBurning()) {
      if (withMessage)
        pos.globalMessage(TSentence("FURNITURE_CATCHES_FIRE", getName()));
      if (viewObject)
        viewObject->setAttribute(ViewObject::Attribute::BURNING, 0.3);
      pos.updateMovementDueToFire();
      pos.getLevel()->addBurningFurniture(pos.getCoord(), layer);
      pos.addCreatureLight(false);
      return true;
    }
  }
  return false;
}

bool Furniture::iceDamage(Position pos) {
  if (freezeTo) {
    pos.globalMessage(TSentence("FURNITURE_FREEZES", getName()));
    pos.removeFurniture(this, pos.getGame()->getContentFactory()->furniture.getFurniture(*freezeTo, getTribe()));
    return true;
  }
  return false;
}

Furniture& Furniture::setDestroyable(double s) {
  setDestroyable(s, DestroyAction::Type::BOULDER);
  setDestroyable(s, DestroyAction::Type::BASH);
  return *this;
}

Furniture& Furniture::setDestroyable(double s, DestroyAction::Type type) {
  destroyedInfo[type] = DestroyedInfo{ 1.0, s };
  return *this;
}

bool Furniture::canDestroy(const DestroyAction& action) const {
  return !!destroyedInfo[action.getType()];
}

optional<double> Furniture::getStrength(const DestroyAction& action) const {
  if (auto info = destroyedInfo[action.getType()]) {
    CHECK(info->health > 0) << info->health;
    CHECK(info->strength > 0) << info->health;
    return info->strength * info->health;
  }
  return none;
}

static ViewLayer getViewLayer(FurnitureLayer layer) {
  switch (layer) {
    case FurnitureLayer::FLOOR:
    case FurnitureLayer::GROUND:
      return ViewLayer::FLOOR_BACKGROUND;
    case FurnitureLayer::MIDDLE:
      return ViewLayer::FLOOR;
    case FurnitureLayer::CEILING:
      return ViewLayer::TORCH1;
  }
}

#include "pretty_archive.h"
template <>
void Furniture::serialize(PrettyInputArchive& ar1, unsigned int v) {
  optional<ViewId> viewId;
  if (viewObject)
    viewId = viewObject->id();
  ar1(NAMED(viewId));
  optional<ViewLayer> viewLayer;
  if (viewObject)
    viewLayer = viewObject->layer();
  ar1(NAMED(viewLayer));
  PrettyFlag blockMovement;
  ar1(OPTION(blockMovement));
  optional_no_none<int> strength;
  ar1(NAMED(strength));
  optional_no_none<vector<pair<int, DestroyAction::Type>>> strength2;
  ar1(NAMED(strength2));
  optional_no_none<Dir> attachmentDir;
  if (viewObject)
    if (auto dir = viewObject->getAttachmentDir())
      attachmentDir = *dir;
  ar1(NAMED(attachmentDir));
  PrettyFlag blockingEnemies;
  ar1(OPTION(blockingEnemies));
  PrettyFlag blockAllVision;
  ar1(OPTION(blockAllVision));
  optional_no_none<double> waterDepth;
  if (viewObject)
    if (auto depth = viewObject->getAttribute(ViewObjectAttribute::WATER_DEPTH))
      waterDepth = *depth;
  ar1(NAMED(waterDepth));
  serializeImpl(ar1, v);
  ar1(endInput());
  if (blockMovement.value)
    setBlocking();
  if (strength)
    setDestroyable(*strength);
  if (strength2)
    for (auto& elem : *strength2)
      setDestroyable(elem.first, elem.second);
  if (viewId)
    viewObject = ViewObject(*viewId, viewLayer.value_or(getViewLayer(layer)), capitalFirst(getName()));
  if (clearFogOfWar)
    viewObject->setModifier(ViewObjectModifier::DOUBLE_CLICK_RESOURCE);
  if (attachmentDir)
    viewObject->setAttachmentDir(*attachmentDir);
  if (waterDepth)
    viewObject->setAttribute(ViewObjectAttribute::WATER_DEPTH, *waterDepth);
  if (blockingEnemies.value)
    setBlockingEnemies();
  if (blockAllVision.value)
    blockVision = EnumSet<VisionId>::fullSet();
  if (pluralName.empty())
    pluralName = TStringId((name.data() + "_PLURAL"_s).data());
}
