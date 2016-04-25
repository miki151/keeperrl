/* Copyright (C) 2013-2014 Michal Brzozowski (rusolis@poczta.fm)

   This file is part of KeeperRL.

   KeeperRL is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   KeeperRL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

#include "stdafx.h"

#include "square.h"
#include "square_factory.h"
#include "level.h"
#include "creature.h"
#include "item.h"
#include "view_object.h"
#include "trigger.h"
#include "progress_meter.h"
#include "game.h"
#include "vision.h"
#include "player_message.h"
#include "square_type.h"
#include "view_index.h"
#include "inventory.h"
#include "poison_gas.h"
#include "fire.h"
#include "tribe.h"
#include "creature_name.h"
#include "movement_type.h"
#include "movement_set.h"
#include "view.h"
#include "sound.h"
#include "creature_attributes.h"

template <class Archive> 
void Square::serialize(Archive& ar, const unsigned int version) { 
  ar& SUBCLASS(Renderable)
    & SVAR(inventoryPtr)
    & SVAR(name)
    & SVAR(level)
    & SVAR(position)
    & SVAR(creature)
    & SVAR(triggers)
    & SVAR(backgroundObject)
    & SVAR(vision)
    & SVAR(hide)
    & SVAR(strength)
    & SVAR(travelDir)
    & SVAR(landingLink)
    & SVAR(fire)
    & SVAR(poisonGas)
    & SVAR(constructions)
    & SVAR(ticking)
    & SVAR(movementSet)
    & SVAR(updateViewIndex)
    & SVAR(updateMemory)
    & SVAR(viewIndex)
    & SVAR(destroyable)
    & SVAR(owner)
    & SVAR(forbiddenTribe)
    & SVAR(unavailable)
    & SVAR(applySound);
  if (progressMeter)
    progressMeter->addProgress();
  updateViewIndex = true;
  updateMemory = true;
}

ProgressMeter* Square::progressMeter = nullptr;

SERIALIZABLE(Square);

SERIALIZATION_CONSTRUCTOR_IMPL(Square);

Square::Square(const ViewObject& obj, Params p)
  : Renderable(obj), name(p.name), vision(p.vision), hide(p.canHide), strength(p.strength),
    fire(p.strength, p.flamability), constructions(p.constructions), ticking(p.ticking),
    movementSet(p.movementSet), viewIndex(new ViewIndex()), destroyable(p.canDestroy), owner(p.owner),
    applySound(p.applySound) {
}

Square::~Square() {
}

void Square::putCreature(Creature* c) {
  //CHECK(canEnter(c)) << c->getName().bare() << " " << getName();
  setCreature(c);
  onEnter(c);
  if (c->getAttributes().isStationary())
    level->addTickingSquare(position);
  c->onMoved();
}

void Square::setPosition(Vec2 v) {
  position = v;
  modViewObject().setPosition(v);
}

Vec2 Square::getPosition() const {
  return position;
}

Position Square::getPosition2() const {
  return Position(position, level);
}

string Square::getName() const {
  return name;
}

void Square::setName(const string& s) {
  setDirty();
  name = s;
}

void Square::setLandingLink(StairKey key) {
  landingLink = key;
}

optional<StairKey> Square::getLandingLink() const {
  return landingLink;
}

double Square::getLightEmission() const {
  double sum = 0;
  for (auto& elem : triggers)
    sum += elem->getLightEmission();
  return sum;
}

void Square::addTravelDir(Vec2 dir) {
  if (!findElement(travelDir, dir))
    travelDir.push_back(dir);
}

bool Square::canConstruct(const SquareType& type) const {
  return constructions.count(type.getId());
}

bool Square::construct(const SquareType& type) {
  setDirty();
  CHECK(canConstruct(type));
  if (--constructions[type.getId()] <= 0) {
    level->replaceSquare(position, SquareFactory::get(type));
    return true;
  } else
    return false;
}

bool Square::canDestroy(TribeId tribe) const {
  return isDestroyable() && owner != tribe && !fire->isBurning();
}

bool Square::isDestroyable() const {
  return destroyable && !unavailable;
}

void Square::destroy() {
  CHECK(isDestroyable());
  setDirty();
  getLevel()->globalMessage(getPosition(), "The " + getName() + " is destroyed.");
  level->getGame()->getView()->addSound(SoundId::REMOVE_CONSTRUCTION);
  level->getGame()->onSquareDestroyed(getPosition2());
  getLevel()->removeSquare(getPosition(), SquareFactory::get(SquareId::FLOOR));
}

bool Square::canDestroy(const Creature* c) const {
  return canDestroy(c->getTribeId())
    || (isDestroyable() && c->getAttributes().isInvincible()); // so that boulders destroy keeper doors
}

void Square::destroyBy(Creature* c) {
  destroy();
}

void Square::burnOut() {
  setDirty();
  getLevel()->globalMessage(getPosition(), "The " + getName() + " burns down.");
  level->getGame()->onSquareDestroyed(getPosition2());
  getLevel()->removeSquare(getPosition(), SquareFactory::get(SquareId::FLOOR));
}

const vector<Vec2>& Square::getTravelDir() const {
  return travelDir;
}

void Square::setCreature(Creature* c) {
  creature = c;
}

void Square::setLevel(Level* l) {
  level = l;
  if (ticking || !inventoryEmpty())
    level->addTickingSquare(position);
}

Level* Square::getLevel() {
  return level;
}

const Level* Square::getLevel() const {
  return level;
}

bool Square::isUnavailable() const {
  return unavailable;
}

void Square::setUnavailable() {
  unavailable = true;
  constructions.clear();
  movementSet->clear();
}

bool Square::sunlightBurns() const {
  return level->isInSunlight(position);
}

void Square::updateSunlightMovement(bool isSunlight) {
  movementSet->setSunlight(isSunlight);
}

void Square::updateMovement() {
  if (fire->isBurning()) {
    if (!movementSet->isOnFire()) {
      movementSet->setOnFire(true);
      level->updateConnectivity(position);
    }
  } else
  if (movementSet->isOnFire()) {
    movementSet->setOnFire(false);
    level->updateConnectivity(position);
  }
}

void Square::tick() {
  setDirty();
  if (!inventoryEmpty())
    for (Item* item : getInventory().getItems()) {
      item->tick(Position(position, level));
      if (item->isDiscarded())
        getInventory().removeItem(item);
    }
  poisonGas->tick(getPosition2());
  if (creature && poisonGas->getAmount() > 0.2) {
    creature->poisonWithGas(min(1.0, poisonGas->getAmount()));
  }
  if (fire->isBurning()) {
    modViewObject().setAttribute(ViewObject::Attribute::BURNING, fire->getSize());
    Debug() << getName() << " burning " << fire->getSize();
    for (Position v : getPosition2().neighbors8(Random))
      if (fire->getSize() > Random.getDouble() * 40)
        v.setOnFire(fire->getSize() / 20);
    fire->tick();
    if (fire->isBurntOut()) {
      level->globalMessage(position, "The " + getName() + " burns out");
      updateMovement();
      burnOut();
      return;
    }
    if (creature)
      creature->setOnFire(fire->getSize());
    for (Item* it : getItems())
      it->setOnFire(fire->getSize(), Position(position, level));
    for (Trigger* t : extractRefs(triggers))
      t->setOnFire(fire->getSize());
  }
  for (Trigger* t : extractRefs(triggers))
    t->tick();
  if (creature && creature->getAttributes().isStationary())
    level->updateConnectivity(position);
  tickSpecial();
}

bool Square::itemLands(vector<Item*> item, const Attack& attack) const {
  if (creature) {
    if (!creature->dodgeAttack(attack))
      return true;
    else {
      if (item.size() > 1)
        creature->you(MsgType::MISS_THROWN_ITEM_PLURAL, item[0]->getPluralTheName(item.size()));
      else
        creature->you(MsgType::MISS_THROWN_ITEM, item[0]->getTheName());
      return false;
    }
  }
  for (const PTrigger& t : triggers)
    if (t->interceptsFlyingItem(item[0]))
      return true;
  return false;
}

bool Square::itemBounces(Item* item, VisionId v) const {
  return !unavailable && !canSeeThru(v);
}

void Square::onItemLands(vector<PItem> item, const Attack& attack, int remainingDist, Vec2 dir, VisionId vision) {
  setDirty();
  if (creature) {
    item[0]->onHitCreature(creature, attack, item.size());
    if (!item[0]->isDiscarded())
      dropItems(std::move(item));
    return;
  }
  for (PTrigger& t : triggers)
    if (t->interceptsFlyingItem(item[0].get())) {
      t->onInterceptFlyingItem(std::move(item), attack, remainingDist, dir, vision);
      return;
    }

  item[0]->onHitSquareMessage(getPosition2(), item.size());
  if (!item[0]->isDiscarded())
    dropItems(std::move(item));
}

bool Square::canNavigate(const MovementType& type1) const {
  MovementType type(type1);
  return canEnterEmpty(type) || 
    ((isDestroyable() && (!type.getTribe() || canDestroy(*type.getTribe()))) && !canEnterEmpty(type.setForced())) || 
    (creature && creature->getAttributes().isStationary() && type.getTribe() != creature->getTribeId());
}

bool Square::canEnter(const MovementType& movement) const {
  return creature == nullptr && canEnterEmpty(movement);
}

bool Square::canEnterEmpty(const MovementType& movement) const {
  if (creature && creature->getAttributes().isStationary())
    return false;
  if (!movement.isForced() && forbiddenTribe && forbiddenTribe == movement.getTribe())
    return false;
  return movementSet->canEnter(movement);
}

bool Square::canEnter(const Creature* c) const {
  return creature == nullptr && canEnterEmpty(c);
}

bool Square::canEnterEmpty(const Creature* c) const {
  return canEnterEmpty(c->getMovementType());
}

void Square::setOnFire(double amount) {
  setDirty();
  bool burning = fire->isBurning();
  fire->set(amount);
  if (!burning && fire->isBurning()) {
    level->addTickingSquare(position);
    level->globalMessage(position, "The " + getName() + " catches fire");
    modViewObject().setAttribute(ViewObject::Attribute::BURNING, fire->getSize());
    updateMovement();
  }
  if (creature)
    creature->setOnFire(amount);
  for (Item* it : getItems())
    it->setOnFire(amount, Position(position, level));
}

void Square::addPoisonGas(double amount) {
  setDirty();
  if (canSeeThru()) {
    poisonGas->addAmount(amount);
    level->addTickingSquare(position);
  }
}

double Square::getPoisonGasAmount() const {
  return poisonGas->getAmount();
}

bool Square::isBurning() const {
  return fire->isBurning();
}

void Square::setBackground(const Square* square) {
  setDirty();
  if (getViewObject().layer() != ViewLayer::FLOOR_BACKGROUND) {
    const ViewObject& obj = square->backgroundObject ? (*square->backgroundObject.get()) : square->getViewObject();
    if (obj.layer() == ViewLayer::FLOOR_BACKGROUND) {
      backgroundObject.reset(new ViewObject(obj));
    }
  }
}

void Square::getViewIndex(ViewIndex& ret, TribeId tribe) const {
  if (!updateViewIndex) {
    ret = *viewIndex;
    return;
  }
  double fireSize = 0;
  if (!inventoryEmpty())
    for (Item* it : getInventory().getItems())
    fireSize = max(fireSize, it->getFireSize());
  fireSize = max(fireSize, fire->getSize());
  if (backgroundObject)
    ret.insert(*backgroundObject.get());
  ret.insert(getViewObject());
  for (const PTrigger& t : triggers)
    if (auto obj = t->getViewObject(tribe))
      ret.insert(copyOf(*obj).setAttribute(ViewObject::Attribute::BURNING, fireSize));
  if (Item* it = getTopItem())
    ret.insert(copyOf(it->getViewObject()).setAttribute(ViewObject::Attribute::BURNING, fireSize));
  if (poisonGas->getAmount() > 0)
    ret.setHighlight(HighlightType::POISON_GAS, min(1.0, poisonGas->getAmount()));
  if (unavailable)
    ret.setHighlight(HighlightType::UNAVAILABLE);
  updateViewIndex = false;
  *viewIndex = ret;
}

void Square::onEnter(Creature* c) {
  setDirty();
  for (Trigger* t : extractRefs(triggers))
    t->onCreatureEnter(c);
  onEnterSpecial(c);
}

void Square::dropItem(PItem item) {
  dropItems(makeVec<PItem>(std::move(item)));
}

void Square::dropItems(vector<PItem> items) {
  setDirty();
  if (level)  // if level == null, then it's being constructed, square will be added later
    level->addTickingSquare(getPosition());
  for (PItem& it : items)
    getInventory().addItem(std::move(it));
}

bool Square::hasItem(Item* it) const {
  return !inventoryEmpty() && getInventory().hasItem(it);
}

Creature* Square::getCreature() {
  return creature;
}

void Square::addTrigger(PTrigger t) {
  setDirty();
  level->addTickingSquare(position);
  Trigger* ref = t.get();
  level->addLightSource(position, t->getLightEmission());
  triggers.push_back(std::move(t));
}

vector<Trigger*> Square::getTriggers() const {
  return extractRefs(triggers);
}

PTrigger Square::removeTrigger(Trigger* trigger) {
  CHECK(trigger);
  setDirty();
  for (PTrigger& t : triggers)
    if (t.get() == trigger) {
      PTrigger ret = std::move(t);
      removeElement(triggers, t);
      level->removeLightSource(position, ret->getLightEmission());
      return ret;
    }
  FAIL << "Trigger not found";
  return nullptr;
}

vector<PTrigger> Square::removeTriggers() {
  vector<PTrigger> ret;
  for (Trigger* t : extractRefs(triggers))
    ret.push_back(removeTrigger(t));
  return ret;
}

const Creature* Square::getCreature() const {
  return creature;
}

void Square::removeCreature() {
  setDirty();
  CHECK(creature);
  Creature* tmp = creature;
  creature = nullptr;
  if (tmp->getAttributes().isStationary())
    level->updateConnectivity(position);
}

bool Square::canSeeThru(VisionId v) const {
  return vision && (v == *vision || Vision::get(v)->getInheritedFov() == Vision::get(*vision));
}

bool Square::canSeeThru() const {
  return canSeeThru(VisionId::NORMAL);
}

void Square::setVision(VisionId v) {
  vision = v;
  getLevel()->updateVisibility(getPosition());
}

bool Square::canHide() const {
  return hide;
}

int Square::getStrength() const {
  return strength;
}

Item* Square::getTopItem() const {
  if (!inventoryEmpty())
    return getInventory().getItems().back();
  else
    return nullptr;
}

vector<Item*> Square::getItems(function<bool (Item*)> predicate) const {
  if (!inventoryEmpty())
    return getInventory().getItems(predicate);
  else
    return {};
}

static vector<Item*> empty;

const vector<Item*>& Square::getItems() const {
  if (!inventoryEmpty())
    return getInventory().getItems();
  else
    return empty;
}

const vector<Item*>& Square::getItems(ItemIndex index) const {
  if (!inventoryEmpty())
    return getInventory().getItems(index);
  else
    return empty;
}

PItem Square::removeItem(Item* it) {
  setDirty();
  return getInventory().removeItem(it);
}

vector<PItem> Square::removeItems(vector<Item*> it) {
  setDirty();
  return getInventory().removeItems(it);
}

void Square::setDirty() {
  updateMemory = true;
  updateViewIndex = true;
}

void Square::setMemoryUpdated() {
  updateMemory = false;
}

bool Square::needsMemoryUpdate() const {
  return updateMemory;
}

void Square::addTraitForTribe(TribeId tribe, MovementTrait trait) {
  movementSet->addTraitForTribe(tribe, trait);
  level->updateConnectivity(position);
}

void Square::removeTraitForTribe(TribeId tribe, MovementTrait trait) {
  movementSet->removeTraitForTribe(tribe, trait);
  level->updateConnectivity(position);
}

void Square::forbidMovementForTribe(TribeId tribe) {
  CHECK(!forbiddenTribe || forbiddenTribe == tribe);
  forbiddenTribe = tribe;
  level->updateConnectivity(position);
  setDirty();
}

void Square::allowMovementForTribe(TribeId tribe) {
  CHECK(!forbiddenTribe || forbiddenTribe == tribe);
  forbiddenTribe = none;
  level->updateConnectivity(position);
  setDirty();
}

bool Square::isTribeForbidden(TribeId tribe) const {
  return forbiddenTribe == tribe;
}

optional<TribeId> Square::getForbiddenTribe() const {
  return forbiddenTribe;
}

optional<SquareApplyType> Square::getApplyType(const Creature* c) const {
  if (auto ret = getApplyType())
    if (canApply(c))
      return ret;
  return none;
}

Inventory& Square::getInventory() {
  if (!inventoryPtr)
    inventoryPtr.reset(new Inventory());
  return *inventoryPtr;
}

const Inventory& Square::getInventory() const {
  if (!inventoryPtr)
    inventoryPtr.reset(new Inventory());
  return *inventoryPtr;
}

bool Square::inventoryEmpty() const {
  return !inventoryPtr || inventoryPtr->isEmpty();
}

void Square::clearItemIndex(ItemIndex index) {
  if (!inventoryEmpty())
    getInventory().clearIndex(index);
}

void Square::apply(Creature* c) {
  if (applySound)
    c->addSound(*applySound);
  onApply(c);
}
