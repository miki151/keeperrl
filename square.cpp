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
#include "tribe.h"
#include "creature_name.h"
#include "movement_type.h"
#include "movement_set.h"
#include "view.h"
#include "sound.h"
#include "creature_attributes.h"
#include "event_listener.h"
#include "fire.h"

template <class Archive> 
void Square::serialize(Archive& ar, const unsigned int version) { 
  ar& SUBCLASS(Renderable);
  serializeAll(ar, inventory);
  serializeAll(ar, name, creature, triggers, vision, landingLink, poisonGas);
  serializeAll(ar, movementSet, lastViewer, viewIndex);
  serializeAll(ar, forbiddenTribe);
  if (progressMeter)
    progressMeter->addProgress();
}

ProgressMeter* Square::progressMeter = nullptr;

SERIALIZABLE(Square);

SERIALIZATION_CONSTRUCTOR_IMPL(Square);

Square::Square(const ViewObject& obj, Params p)
  : Renderable(obj), name(p.name), vision(p.vision), movementSet(p.movementSet),
    viewIndex(new ViewIndex()) {
}

Square::~Square() {
}

void Square::putCreature(Creature* c) {
  //CHECK(canEnter(c)) << c->getName().bare() << " " << getName();
  setCreature(c);
  onEnter(c);
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

void Square::setCreature(Creature* c) {
  creature = c;
}

void Square::onAddedToLevel(Position pos) const {
  if (!inventory->isEmpty())
    pos.getLevel()->addTickingSquare(pos.getCoord());
}

void Square::tick(Position pos) {
  setDirty(pos);
  if (!inventory->isEmpty())
    for (Item* item : getInventory().getItems()) {
      item->tick(pos);
      if (item->isDiscarded())
        getInventory().removeItem(item);
    }
  poisonGas->tick(pos);
  if (creature && poisonGas->getAmount() > 0.2) {
    creature->poisonWithGas(min(1.0, poisonGas->getAmount()));
  }
  for (Trigger* t : extractRefs(triggers))
    t->tick();
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
  if (!inventory->isEmpty())
    for (Item* it : getInventory().getItems())
      fireSize = max(fireSize, it->getFireSize());
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
  getInventory().addItems(std::move(items));
}

void Square::dropItems(Position pos, vector<PItem> items) {
  setDirty(pos);
  pos.getLevel()->addTickingSquare(pos.getCoord());
  dropItemsLevelGen(std::move(items));
}

void Square::addTrigger(Position pos, PTrigger t) {
  setDirty(pos);
  pos.getLevel()->addTickingSquare(pos.getCoord());
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
  FATAL << "Trigger not found";
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

Item* Square::getTopItem() const {
  if (inventory->isEmpty())
    return nullptr;
  else
    return inventory->getItems().back();
}

vector<Item*> Square::getItems(function<bool (Item*)> predicate) const {
 return inventory->getItems(predicate);
}

const vector<Item*>& Square::getItems() const {
  return inventory->getItems();
}

const vector<Item*>& Square::getItems(ItemIndex index) const {
  return inventory->getItems(index);
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

MovementSet& Square::getMovementSet() {
  return *movementSet;
}

const MovementSet& Square::getMovementSet() const {
  return *movementSet;
}

void Square::forbidMovementForTribe(Position pos, TribeId tribe) {
  CHECK(!forbiddenTribe || forbiddenTribe == tribe);
  forbiddenTribe = tribe;
  pos.updateConnectivity();
  setDirty(pos);
}

void Square::allowMovementForTribe(Position pos, TribeId tribe) {
  CHECK(!forbiddenTribe || forbiddenTribe == tribe);
  forbiddenTribe = none;
  pos.updateConnectivity();
  setDirty(pos);
}

bool Square::isTribeForbidden(TribeId tribe) const {
  return forbiddenTribe == tribe;
}

optional<TribeId> Square::getForbiddenTribe() const {
  return forbiddenTribe;
}

Inventory& Square::getInventory() {
  return *inventory;
}

const Inventory& Square::getInventory() const {
  return *inventory;
}

void Square::clearItemIndex(ItemIndex index) {
  inventory->clearIndex(index);
}
