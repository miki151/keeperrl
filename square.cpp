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
#include "square_interaction.h"
#include "event_listener.h"

template <class Archive> 
void Square::serialize(Archive& ar, const unsigned int version) { 
  ar& SUBCLASS(Renderable);
  serializeAll(ar, inventoryPtr, name, creature, triggers, vision, hide, strength, landingLink, fire, poisonGas);
  serializeAll(ar, constructions, currentConstruction, ticking, movementSet, lastViewer, viewIndex, destroyable);
  serializeAll(ar, forbiddenTribe, applySound, applyType, applyTime, interaction, owner);
  if (progressMeter)
    progressMeter->addProgress();
}

ProgressMeter* Square::progressMeter = nullptr;

SERIALIZABLE(Square);

SERIALIZATION_CONSTRUCTOR_IMPL(Square);

Square::Square(const ViewObject& obj, Params p)
  : Renderable(obj), name(p.name), vision(p.vision), hide(p.canHide), strength(p.strength),
    fire(p.strength, p.flamability), constructions(p.constructions), ticking(p.ticking),
    movementSet(p.movementSet), viewIndex(new ViewIndex()), destroyable(p.canDestroy), owner(p.owner),
    applySound(p.applySound), applyType(p.applyType), applyTime(p.applyTime.get_value_or(1)),
    interaction(p.interaction) {
  modViewObject().setIndoors(isCovered());
}

Square::~Square() {
}

void Square::putCreature(Creature* c) {
  //CHECK(canEnter(c)) << c->getName().bare() << " " << getName();
  setCreature(c);
  onEnter(c);
  if (c->getAttributes().isStationary())
    c->getPosition().getLevel()->addTickingSquare(c->getPosition().getCoord());
  if (Game* game = c->getGame())
    game->addEvent({EventId::MOVED, c});
}

string Square::getName() const {
  return name;
}

void Square::setName(Position pos, const string& s) {
  setDirty(pos);
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

static optional<short int> getConstructionTime(ConstructionsId id, SquareId square) {
  switch (id) {
    case ConstructionsId::BRIDGE:
      return square == SquareId::BRIDGE ? 20 : optional<short int>(none);
    case ConstructionsId::CUT_TREE:
      return square == SquareId::TREE_TRUNK ? 20 : optional<short int>(none);
    case ConstructionsId::BED:
      return square == SquareId::BED ? 10 : optional<short int>(none);
    case ConstructionsId::BEAST_CAGE:
      return square == SquareId::BEAST_CAGE ? 10 : optional<short int>(none);
    case ConstructionsId::GRAVE:
      return square == SquareId::GRAVE ? 10 : optional<short int>(none);
    case ConstructionsId::MINING:
      return square == SquareId::FLOOR ? Random.get(3, 8) : optional<short int>(none);
    case ConstructionsId::MINING_ORE:
      return square == SquareId::FLOOR ? Random.get(25, 50) : optional<short int>(none);
    case ConstructionsId::OUTDOOR_INSTALLATIONS:
      switch (square) {
        case SquareId::IMPALED_HEAD:
        case SquareId::EYEBALL: return 5;
        case SquareId::KEEPER_BOARD: return 15;
        default: return none;
      }
    case ConstructionsId::MOUNTAIN_GEN_ORES:
      switch (square) {
        case SquareId::FLOOR: return Random.get(3, 8);
        case SquareId::IRON_ORE:
        case SquareId::STONE:
        case SquareId::GOLD_ORE: return 1;
        default: return none;
      }
    case ConstructionsId::DUNGEON_ROOMS:
      switch (square) {
        case SquareId::TREASURE_CHEST: return 10;
        case SquareId::DORM: return 10;
        case SquareId::TRIBE_DOOR: return 10;
        case SquareId::TRAINING_ROOM: return 10;
        case SquareId::LIBRARY: return 10;
        case SquareId::HATCHERY: return 10;
        case SquareId::STOCKPILE: return 1;
        case SquareId::STOCKPILE_EQUIP: return 1;
        case SquareId::STOCKPILE_RES: return 1;
        case SquareId::CEMETERY: return 10;
        case SquareId::WORKSHOP: return 10;
        case SquareId::FORGE: return 10;
        case SquareId::LABORATORY: return 10;
        case SquareId::JEWELER: return 10;
        case SquareId::PRISON: return 10;
        case SquareId::TORTURE_TABLE: return 10;
        case SquareId::BEAST_LAIR: return 10;
        case SquareId::IMPALED_HEAD: return 5;
        case SquareId::WHIPPING_POST: return 5;
        case SquareId::BARRICADE: return 20;
        case SquareId::TORCH: return 5;
        case SquareId::EYEBALL: return 5;
        case SquareId::CREATURE_ALTAR: return 35;
        case SquareId::MINION_STATUE: return 35;
        case SquareId::THRONE: return 100;
        case SquareId::MOUNTAIN: return 15;
        case SquareId::RITUAL_ROOM: return 10;
        case SquareId::KEEPER_BOARD: return 15;
        default: return none;
      }
  }
}

bool Square::canConstruct(const SquareType& type) const {
  return constructions && getConstructionTime(*constructions, type.getId());
}

bool Square::construct(Position position, const SquareType& type) {
  setDirty(position);
  CHECK(canConstruct(type));
  if (!currentConstruction || currentConstruction->id != type.getId())
    currentConstruction = CurrentConstruction{type.getId(), *getConstructionTime(*constructions, type.getId())};
  if (--currentConstruction->turnsRemaining <= 0) {
    position.getLevel()->replaceSquare(position, SquareFactory::get(type));
    return true;
  } else
    return false;
}

bool Square::canDestroy(const MovementType& movement) const {
  return isDestroyable() && (!owner || !movement.isCompatible(*owner)) && !fire->isBurning();
}

bool Square::isDestroyable() const {
  return destroyable;
}

void Square::destroy(Position position) {
  CHECK(isDestroyable());
  setDirty(position);
  position.globalMessage("The " + getName() + " is destroyed.");
  position.getGame()->getView()->addSound(SoundId::REMOVE_CONSTRUCTION);
  position.getGame()->addEvent({EventId::SQUARE_DESTROYED, position});
  position.getLevel()->removeSquare(position, SquareFactory::get(SquareId::FLOOR));
}

bool Square::canDestroy(const Creature* c) const {
  return canDestroy(c->getMovementType())
    || (isDestroyable() && c->getAttributes().isInvincible()); // so that boulders destroy keeper doors
}

void Square::destroyBy(Position pos, Creature* c) {
  destroy(pos);
}

void Square::burnOut(Position position) {
  setDirty(position);
  position.globalMessage("The " + getName() + " burns down.");
  position.getGame()->addEvent({EventId::SQUARE_DESTROYED, position});
  position.getLevel()->removeSquare(position, SquareFactory::get(SquareId::FLOOR));
}

void Square::setCreature(Creature* c) {
  creature = c;
}

void Square::onAddedToLevel(Position pos) const {
  if (ticking || !inventoryEmpty())
    pos.getLevel()->addTickingSquare(pos.getCoord());
}

void Square::setCovered(bool covered) {
  movementSet->setCovered(covered);
  modViewObject().setIndoors(covered);
}

bool Square::isCovered() const {
  return movementSet->isCovered();
}

void Square::updateMovement(Position pos) {
  if (fire->isBurning()) {
    if (!movementSet->isOnFire()) {
      movementSet->setOnFire(true);
      pos.getLevel()->updateConnectivity(pos.getCoord());
    }
  } else
  if (movementSet->isOnFire()) {
    movementSet->setOnFire(false);
    pos.getLevel()->updateConnectivity(pos.getCoord());
  }
}

void Square::tick(Position pos) {
  setDirty(pos);
  if (!inventoryEmpty())
    for (Item* item : getInventory().getItems()) {
      item->tick(pos);
      if (item->isDiscarded())
        getInventory().removeItem(item);
    }
  poisonGas->tick(pos);
  if (creature && poisonGas->getAmount() > 0.2) {
    creature->poisonWithGas(min(1.0, poisonGas->getAmount()));
  }
  if (fire->isBurning()) {
    modViewObject().setAttribute(ViewObject::Attribute::BURNING, fire->getSize());
    Debug() << getName() << " burning " << fire->getSize();
    for (Position v : pos.neighbors8(Random))
      if (fire->getSize() > Random.getDouble() * 40)
        v.setOnFire(fire->getSize() / 20);
    fire->tick();
    if (fire->isBurntOut()) {
      pos.globalMessage("The " + getName() + " burns out");
      updateMovement(pos);
      burnOut(pos);
      return;
    }
    if (creature)
      creature->setOnFire(fire->getSize());
    for (Item* it : getItems())
      it->setOnFire(fire->getSize(), pos);
    for (Trigger* t : extractRefs(triggers))
      t->setOnFire(fire->getSize());
  }
  for (Trigger* t : extractRefs(triggers))
    t->tick();
  if (creature && creature->getAttributes().isStationary())
    pos.getLevel()->updateConnectivity(pos.getCoord());
  tickSpecial(pos);
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
  return !canSeeThru(v);
}

void Square::onItemLands(Position pos, vector<PItem> item, const Attack& attack, int remainingDist, Vec2 dir,
    VisionId vision) {
  setDirty(pos);
  if (creature) {
    item[0]->onHitCreature(creature, attack, item.size());
    if (!item[0]->isDiscarded())
      dropItems(pos, std::move(item));
    return;
  }
  for (PTrigger& t : triggers)
    if (t->interceptsFlyingItem(item[0].get())) {
      t->onInterceptFlyingItem(std::move(item), attack, remainingDist, dir, vision);
      return;
    }

  item[0]->onHitSquareMessage(pos, item.size());
  if (!item[0]->isDiscarded())
    dropItems(pos, std::move(item));
}

bool Square::canNavigate(const MovementType& type) const {
  MovementType typeForced(type);
  typeForced.setForced();
  return canEnterEmpty(type) || 
    // for destroying doors, etc, but not entering forbidden zone
    (canDestroy(type) && !canEnterEmpty(typeForced)) ||
    // for navigating through hostile boulders
    (creature && creature->getAttributes().isStationary() && !type.isCompatible(creature->getTribeId()));
}

bool Square::canEnter(const MovementType& movement) const {
  return creature == nullptr && canEnterEmpty(movement);
}

bool Square::canEnterEmpty(const MovementType& movement) const {
  if (creature && creature->getAttributes().isStationary())
    return false;
  if (!movement.isForced() && forbiddenTribe && movement.isCompatible(*forbiddenTribe))
    return false;
  return movementSet->canEnter(movement);
}

bool Square::canEnter(const Creature* c) const {
  return creature == nullptr && canEnterEmpty(c);
}

bool Square::canEnterEmpty(const Creature* c) const {
  return canEnterEmpty(c->getMovementType());
}

void Square::setOnFire(Position pos, double amount) {
  setDirty(pos);
  bool burning = fire->isBurning();
  fire->set(amount);
  if (!burning && fire->isBurning()) {
    pos.getLevel()->addTickingSquare(pos.getCoord());
    pos.globalMessage("The " + getName() + " catches fire");
    modViewObject().setAttribute(ViewObject::Attribute::BURNING, fire->getSize());
    updateMovement(pos);
  }
  if (creature)
    creature->setOnFire(amount);
  for (Item* it : getItems())
    it->setOnFire(amount, pos);
}

void Square::addPoisonGas(Position pos, double amount) {
  setDirty(pos);
  if (canSeeThru()) {
    poisonGas->addAmount(amount);
    pos.getLevel()->addTickingSquare(pos.getCoord());
  }
}

double Square::getPoisonGasAmount() const {
  return poisonGas->getAmount();
}

bool Square::isBurning() const {
  return fire->isBurning();
}

optional<ViewObject> Square::extractBackground() const {
  const ViewObject& obj = getViewObject();
  if (obj.layer() == ViewLayer::FLOOR_BACKGROUND)
    return obj;
  else
    return none;
}

void Square::getViewIndex(ViewIndex& ret, const Creature* viewer) const {
  if ((!viewer && lastViewer) || (viewer && lastViewer == viewer->getUniqueId())) {
    ret = *viewIndex;
    return;
  }
  // viewer is null only in Spectator mode, so setting a random id to lastViewer is ok
  lastViewer = viewer ? viewer->getUniqueId() : Creature::Id();
  double fireSize = 0;
  if (!inventoryEmpty())
    for (Item* it : getInventory().getItems())
    fireSize = max(fireSize, it->getFireSize());
  fireSize = max(fireSize, fire->getSize());
  ret.insert(getViewObject());
  for (const PTrigger& t : triggers)
    if (auto obj = t->getViewObject(viewer))
      ret.insert(copyOf(*obj).setAttribute(ViewObject::Attribute::BURNING, fireSize));
  if (Item* it = getTopItem())
    ret.insert(copyOf(it->getViewObject()).setAttribute(ViewObject::Attribute::BURNING, fireSize));
  if (poisonGas->getAmount() > 0)
    ret.setHighlight(HighlightType::POISON_GAS, min(1.0, poisonGas->getAmount()));
  *viewIndex = ret;
}

void Square::onEnter(Creature* c) {
  setDirty(c->getPosition());
  for (Trigger* t : extractRefs(triggers))
    t->onCreatureEnter(c);
  onEnterSpecial(c);
}

void Square::dropItem(Position pos, PItem item) {
  dropItems(pos, makeVec<PItem>(std::move(item)));
}

void Square::dropItemsLevelGen(vector<PItem> items) {
  for (PItem& it : items)
    getInventory().addItem(std::move(it));
}

void Square::dropItems(Position pos, vector<PItem> items) {
  setDirty(pos);
  pos.getLevel()->addTickingSquare(pos.getCoord());
  dropItemsLevelGen(std::move(items));
}

bool Square::hasItem(Item* it) const {
  return !inventoryEmpty() && getInventory().hasItem(it);
}

void Square::addTrigger(Position pos, PTrigger t) {
  setDirty(pos);
  pos.getLevel()->addTickingSquare(pos.getCoord());
  Trigger* ref = t.get();
  pos.getLevel()->addLightSource(pos.getCoord(), t->getLightEmission());
  triggers.push_back(std::move(t));
}

vector<Trigger*> Square::getTriggers() const {
  return extractRefs(triggers);
}

PTrigger Square::removeTrigger(Position pos, Trigger* trigger) {
  CHECK(trigger);
  setDirty(pos);
  for (PTrigger& t : triggers)
    if (t.get() == trigger) {
      PTrigger ret = std::move(t);
      removeElement(triggers, t);
      pos.getLevel()->removeLightSource(pos.getCoord(), ret->getLightEmission());
      return ret;
    }
  FAIL << "Trigger not found";
  return nullptr;
}

vector<PTrigger> Square::removeTriggers(Position pos) {
  vector<PTrigger> ret;
  for (Trigger* t : extractRefs(triggers))
    ret.push_back(removeTrigger(pos, t));
  return ret;
}

Creature* Square::getCreature() const {
  return creature;
}

void Square::removeCreature(Position pos) {
  setDirty(pos);
  CHECK(creature);
  Creature* tmp = creature;
  creature = nullptr;
  if (tmp->getAttributes().isStationary())
    pos.getLevel()->updateConnectivity(pos.getCoord());
}

bool Square::canSeeThru(VisionId v) const {
  return vision && (v == *vision || Vision::get(v)->getInheritedFov() == Vision::get(*vision));
}

bool Square::canSeeThru() const {
  return canSeeThru(VisionId::NORMAL);
}

void Square::setVision(Position pos, VisionId v) {
  vision = v;
  pos.getLevel()->updateVisibility(pos.getCoord());
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

PItem Square::removeItem(Position pos, Item* it) {
  setDirty(pos);
  return getInventory().removeItem(it);
}

vector<PItem> Square::removeItems(Position pos, vector<Item*> it) {
  setDirty(pos);
  return getInventory().removeItems(it);
}

void Square::setDirty(Position pos) {
  pos.getLevel()->setNeedsMemoryUpdate(pos.getCoord(), true);
  pos.getLevel()->setNeedsRenderUpdate(pos.getCoord(), true);
  lastViewer.reset();
}

void Square::addTraitForTribe(Position pos, TribeId tribe, MovementTrait trait) {
  movementSet->addTraitForTribe(tribe, trait);
  pos.getLevel()->updateConnectivity(pos.getCoord());
}

void Square::removeTraitForTribe(Position pos, TribeId tribe, MovementTrait trait) {
  movementSet->removeTraitForTribe(tribe, trait);
  pos.getLevel()->updateConnectivity(pos.getCoord());
}

void Square::forbidMovementForTribe(Position pos, TribeId tribe) {
  CHECK(!forbiddenTribe || forbiddenTribe == tribe);
  forbiddenTribe = tribe;
  pos.getLevel()->updateConnectivity(pos.getCoord());
  setDirty(pos);
}

void Square::allowMovementForTribe(Position pos, TribeId tribe) {
  CHECK(!forbiddenTribe || forbiddenTribe == tribe);
  forbiddenTribe = none;
  pos.getLevel()->updateConnectivity(pos.getCoord());
  setDirty(pos);
}

optional<SquareInteraction> Square::getInteraction() const {
  return interaction;
}

bool Square::isTribeForbidden(TribeId tribe) const {
  return forbiddenTribe == tribe;
}

optional<TribeId> Square::getForbiddenTribe() const {
  return forbiddenTribe;
}

optional<SquareApplyType> Square::getApplyType() const {
  return applyType;
}

double Square::getApplyTime() const {
  return applyTime;
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
  if (interaction)
    SquareInteractions::apply(*interaction, c->getPosition(), c);
  else
    onApply(c);
}

void Square::apply(Position pos) {
  if (applySound)
    pos.addSound(*applySound);
  onApply(pos);
}
