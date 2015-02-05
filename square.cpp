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

template <class Archive> 
void Square::serialize(Archive& ar, const unsigned int version) { 
  ar& SUBCLASS(Renderable)
    & SVAR(inventory)
    & SVAR(name)
    & SVAR(level)
    & SVAR(position)
    & SVAR(creature)
    & SVAR(triggers)
    & SVAR(backgroundObject)
    & SVAR(vision)
    & SVAR(hide)
    & SVAR(strength)
    & SVAR(height)
    & SVAR(travelDir)
    & SVAR(landingLink)
    & SVAR(fire)
    & SVAR(poisonGas)
    & SVAR(constructions)
    & SVAR(ticking)
    & SVAR(fog)
    & SVAR(movementType)
    & SVAR(updateViewIndex)
    & SVAR(updateMemory)
    & SVAR(viewIndex)
    & SVAR(canDestroySquare);
  CHECK_SERIAL;
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
    movementType(p.movementType), canDestroySquare(p.canDestroy) {
}

Square::~Square() {
}

void Square::putCreature(Creature* c) {
  CHECK(canEnter(c));
  creature = c;
  onEnter(c);
}

void Square::setPosition(Vec2 v) {
  position = v;
  modViewObject().setPosition(v);
}

Vec2 Square::getPosition() const {
  return position;
}

string Square::getName() const {
  return name;
}

void Square::setName(const string& s) {
  setDirty();
  name = s;
}

void Square::setLandingLink(StairDirection direction, StairKey key) {
  landingLink = make_pair(direction, key);
}

bool Square::isLandingSquare(StairDirection direction, StairKey key) {
  return landingLink == make_pair(direction, key);
}

optional<pair<StairDirection, StairKey>> Square::getLandingLink() const {
  return landingLink;
}

double Square::getLightEmission() const {
  double sum = 0;
  for (auto& elem : triggers)
    sum += elem->getLightEmission();
  return sum;
}

void Square::setHeight(double h) {
  modViewObject().setAttribute(ViewObject::Attribute::HEIGHT, h);
  height = h;
}

void Square::addTravelDir(Vec2 dir) {
  if (!findElement(travelDir, dir))
    travelDir.push_back(dir);
}

bool Square::canConstruct(SquareType type) const {
  return constructions.count(type.getId());
}

bool Square::construct(SquareType type) {
  setDirty();
  CHECK(canConstruct(type));
  if (--constructions[type.getId()] <= 0) {
    PSquare newSquare = PSquare(SquareFactory::get(type));
    level->replaceSquare(position, std::move(newSquare));
    return true;
  } else
    return false;
}

bool Square::canDestroy() const {
  return canDestroySquare;
}

void Square::destroy() {
  CHECK(canDestroy());
  setDirty();
  getLevel()->globalMessage(getPosition(), "The " + getName() + " is destroyed.");
  GlobalEvents.addSquareReplacedEvent(getLevel(), getPosition());
  getLevel()->replaceSquare(getPosition(), PSquare(SquareFactory::get(SquareId::FLOOR)));
}

bool Square::canDestroyBy(const Creature* c) const {
  return canDestroy();
}

void Square::destroyBy(Creature* c) {
  destroy();
}

void Square::burnOut() {
  setDirty();
  getLevel()->globalMessage(getPosition(), "The " + getName() + " burns down.");
  GlobalEvents.addSquareReplacedEvent(getLevel(), getPosition());
  getLevel()->replaceSquare(getPosition(), PSquare(SquareFactory::get(SquareId::FLOOR)));
}

const vector<Vec2>& Square::getTravelDir() const {
  return travelDir;
}

void Square::putCreatureSilently(Creature* c) {
  creature = c;
}

void Square::setLevel(Level* l) {
  level = l;
  if (ticking || !inventory.isEmpty())
    level->addTickingSquare(position);
}

const Level* Square::getConstLevel() const {
  return level;
}

Level* Square::getLevel() {
  return level;
}

const Level* Square::getLevel() const {
  return level;
}

void Square::setFog(double val) {
  setDirty();
  fog = val;
}

void Square::tick(double time) {
  setDirty();
  if (!inventory.isEmpty())
    for (Item* item : inventory.getItems()) {
      item->tick(time, level, position);
      if (item->isDiscarded())
        inventory.removeItem(item);
    }
  poisonGas.tick(level, position);
  if (creature && poisonGas.getAmount() > 0.2) {
    creature->poisonWithGas(min(1.0, poisonGas.getAmount()));
  }
  if (fire.isBurning()) {
    modViewObject().setAttribute(ViewObject::Attribute::BURNING, fire.getSize());
    Debug() << getName() << " burning " << fire.getSize();
    for (Square* s : level->getSquares(position.neighbors8(true)))
      if (fire.getSize() > Random.getDouble() * 40)
        s->setOnFire(fire.getSize() / 20);
    fire.tick(level, position);
    if (fire.isBurntOut()) {
      level->globalMessage(position, "The " + getName() + " burns out");
      burnOut();
      return;
    }
    if (creature)
      creature->setOnFire(fire.getSize());
    for (Item* it : getItems())
      it->setOnFire(fire.getSize(), level, position);
    for (Trigger* t : extractRefs(triggers))
      t->setOnFire(fire.getSize());
  }
  for (Trigger* t : extractRefs(triggers))
    t->tick(time);
  tickSpecial(time);
}

bool Square::itemLands(vector<Item*> item, const Attack& attack) const {
  if (creature) {
    if (!creature->dodgeAttack(attack))
      return true;
    else {
      if (item.size() > 1)
        creature->you(MsgType::MISS_THROWN_ITEM_PLURAL, item[0]->getTheName(true));
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

void Square::onItemLands(vector<PItem> item, const Attack& attack, int remainingDist, Vec2 dir, VisionId vision) {
  setDirty();
  if (creature) {
    item[0]->onHitCreature(creature, attack, item.size() > 1);
    if (!item[0]->isDiscarded())
      dropItems(std::move(item));
    return;
  }
  for (PTrigger& t : triggers)
    if (t->interceptsFlyingItem(item[0].get())) {
      t->onInterceptFlyingItem(std::move(item), attack, remainingDist, dir, vision);
      return;
    }

  item[0]->onHitSquareMessage(position, this, item.size() > 1);
  if (!item[0]->isDiscarded())
    dropItems(std::move(item));
}

bool Square::canEnter(MovementType movement) const {
  return creature == nullptr && canEnterEmpty(movement);
}

bool Square::canEnterEmpty(MovementType movement) const {
  return movementType.canEnter(movement);
}

const MovementType& Square::getMovementType() const {
  return movementType;
}

bool Square::canEnter(const Creature* c) const {
  return creature == nullptr && canEnterEmpty(c);
}

bool Square::canEnterEmpty(const Creature* c) const {
  return c->canEnter(movementType) || canEnterSpecial(c);
}

bool Square::canEnterSpecial(const Creature* c) const {
  return false;
}

void Square::setOnFire(double amount) {
  setDirty();
  bool burning = fire.isBurning();
  fire.set(amount);
  if (!burning && fire.isBurning()) {
    level->addTickingSquare(position);
    level->globalMessage(position, "The " + getName() + " catches fire.");
    modViewObject().setAttribute(ViewObject::Attribute::BURNING, fire.getSize());
  }
  if (creature)
    creature->setOnFire(amount);
  for (Item* it : getItems())
    it->setOnFire(amount, level, position);
}

void Square::addPoisonGas(double amount) {
  setDirty();
  if (canSeeThru()) {
    poisonGas.addAmount(amount);
    level->addTickingSquare(position);
  }
}

double Square::getPoisonGasAmount() const {
  return poisonGas.getAmount();
}

bool Square::isBurning() const {
  return fire.isBurning();
}

optional<ViewObject> Square::getBackgroundObject() const {
  return backgroundObject;
}

void Square::setBackground(const Square* square) {
  setDirty();
  if (getViewObject().layer() != ViewLayer::FLOOR_BACKGROUND) {
    const ViewObject& obj = square->backgroundObject ? (*square->backgroundObject) : square->getViewObject();
    if (obj.layer() == ViewLayer::FLOOR_BACKGROUND)
      backgroundObject = obj;
  }
}

void Square::getViewIndex(ViewIndex& ret, const Tribe* tribe) const {
  if (!updateViewIndex) {
    ret = viewIndex;
    return;
  }
  double fireSize = 0;
  for (Item* it : inventory.getItems())
    fireSize = max(fireSize, it->getFireSize());
  fireSize = max(fireSize, fire.getSize());
  if (backgroundObject)
    ret.insert(*backgroundObject);
  ret.insert(getViewObject());
  for (const PTrigger& t : triggers)
    if (auto obj = t->getViewObject(tribe))
      ret.insert(copyOf(*obj).setAttribute(ViewObject::Attribute::BURNING, fireSize));
  if (Item* it = getTopItem())
    ret.insert(copyOf(it->getViewObject()).setAttribute(ViewObject::Attribute::BURNING, fireSize));
  ret.setHighlight(HighlightType::NIGHT, 1.0 - level->getLight(position));
  if (poisonGas.getAmount() > 0)
    ret.setHighlight(HighlightType::POISON_GAS, min(1.0, poisonGas.getAmount()));
  if (fog)
    ret.setHighlight(HighlightType::FOG, fog);
  updateViewIndex = false;
  viewIndex = ret;
}

void Square::onEnter(Creature* c) {
  setDirty();
  for (Trigger* t : extractRefs(triggers))
    t->onCreatureEnter(c);
  onEnterSpecial(c);
}

void Square::dropItem(PItem item) {
  setDirty();
  if (level)  // if level == null, then it's being constructed, square will be added later
    level->addTickingSquare(getPosition());
  inventory.addItem(std::move(item));
}

void Square::dropItems(vector<PItem> items) {
  for (PItem& it : items)
    dropItem(std::move(it));
}

bool Square::hasItem(Item* it) const {
  return inventory.hasItem(it);
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

const vector<Trigger*> Square::getTriggers() const {
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
  creature = 0;
}

bool Square::canSeeThru(VisionId v) const {
  return vision && (v == *vision || Vision::get(v)->getInheritedFov() == Vision::get(*vision));
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
  Item* last = nullptr;
  if (!inventory.isEmpty())
  for (Item* it : inventory.getItems()) {
    last = it;
    if (it->getViewObject().layer() == ViewLayer::LARGE_ITEM)
      return it;
  }
  return last;
}

vector<Item*> Square::getItems(function<bool (Item*)> predicate) const {
  return inventory.getItems(predicate);
}

const vector<Item*>& Square::getItems() const {
  return inventory.getItems();
}

const vector<Item*>& Square::getItems(ItemIndex index) const {
  return inventory.getItems(index);
}

PItem Square::removeItem(Item* it) {
  setDirty();
  return inventory.removeItem(it);
}

vector<PItem> Square::removeItems(vector<Item*> it) {
  setDirty();
  return inventory.removeItems(it);
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

void Square::setMovementType(MovementType t) {
  movementType = t;
}

