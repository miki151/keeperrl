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

#include "creature.h"
#include "creature_factory.h"
#include "level.h"
#include "enemy_check.h"
#include "ranged_weapon.h"
#include "statistics.h"
#include "options.h"
#include "model.h"
#include "effect.h"
#include "item_factory.h"
#include "square.h"

template <class Archive> 
void Creature::MoraleOverride::serialize(Archive& ar, const unsigned int version) {
}

SERIALIZABLE(Creature::MoraleOverride);

template <class Archive> 
void Creature::serialize(Archive& ar, const unsigned int version) { 
  ar
    & SUBCLASS(CreatureAttributes)
    & SUBCLASS(Renderable)
    & SUBCLASS(UniqueEntity)
    & SVAR(level)
    & SVAR(position)
    & SVAR(time)
    & SVAR(equipment)
    & SVAR(shortestPath)
    & SVAR(knownHiding)
    & SVAR(tribe)
    & SVAR(enemyChecks)
    & SVAR(health)
    & SVAR(morale)
    & SVAR(dead)
    & SVAR(lastTick)
    & SVAR(collapsed)
    & SVAR(injuredBodyParts)
    & SVAR(lostBodyParts)
    & SVAR(hidden)
    & SVAR(lastAttacker)
    & SVAR(swapPositionCooldown)
    & SVAR(lastingEffects)
    & SVAR(unknownAttacker)
    & SVAR(privateEnemies)
    & SVAR(holding)
    & SVAR(controller)
    & SVAR(controllerStack)
    & SVAR(creatureVisions)
    & SVAR(kills)
    & SVAR(difficultyPoints)
    & SVAR(points)
    & SVAR(numAttacksThisTurn)
    & SVAR(moraleOverrides)
    & SVAR(attrIncrease)
    & SVAR(visibleEnemies)
    & SVAR(vision)
    & SVAR(personalEvents);
  CHECK_SERIAL;
}

SERIALIZABLE(Creature);

SERIALIZATION_CONSTRUCTOR_IMPL(Creature);

Creature::Creature(const ViewObject& object, Tribe* t, const CreatureAttributes& attributes, ControllerFactory f)
    : CreatureAttributes(attributes), Renderable(object), tribe(t), controller(f.get(this)) {
  if (tribe)
    tribe->addMember(this);
  for (auto id : ENUM_ALL(AttrType))
    CHECK(attr[id] > 0);
  CHECK(getUniqueId() != 0);
  modViewObject().setCreatureId(getUniqueId());
}

Creature::Creature(Tribe* t, const CreatureAttributes& attr, ControllerFactory f)
    : Creature(ViewObject(*attr.viewId, ViewLayer::CREATURE, (*attr.name).bare()), t, attr, f) {
  updateVision();    
}

Creature::~Creature() {
  if (tribe)
    tribe->removeMember(this);
}

bool Creature::isFireResistant() const {
  return fireCreature || isAffected(LastingEffect::FIRE_RESISTANT);
}

void Creature::addSpell(Spell* spell) {
  spells.add(spell);
}

vector<Spell*> Creature::getSpells() const {
  return spells.getAll();
}

double Creature::getSpellDelay(Spell* spell) const {
  CHECK(!isReady(spell));
  return spells.getReadyTime(spell) - getTime();
}

bool Creature::isReady(Spell* spell) const {
  return spells.getReadyTime(spell) < getTime();
}

static double getWillpowerMult(double sorcerySkill) {
  return 2 * pow(0.25, sorcerySkill); 
}

CreatureAction Creature::castSpell(Spell* spell) const {
  CHECK(spells.contains(spell));
  CHECK(!spell->isDirected());
  if (!isReady(spell))
    return CreatureAction("You can't cast this spell yet.");
  return CreatureAction(this, [=] (Creature *c) {
    monsterMessage(getName().the() + " casts a spell");
    playerMessage("You cast " + spell->getName());
    Effect::applyToCreature(c, spell->getEffectType(), EffectStrength::NORMAL);
    c->level->getModel()->getStatistics().add(StatId::SPELL_CAST);
    c->spells.setReadyTime(spell, getTime() + spell->getDifficulty()
        * getWillpowerMult(getSkillValue(Skill::get(SkillId::SORCERY))));
    c->spendTime(1);
  });
}

CreatureAction Creature::castSpell(Spell* spell, Vec2 dir) const {
  CHECK(spells.contains(spell));
  CHECK(spell->isDirected());
  CHECK(dir.length8() == 1);
  if (!isReady(spell))
    return CreatureAction("You can't cast this spell yet.");
  return CreatureAction(this, [=] (Creature *c) {
    monsterMessage(getName().the() + " casts a spell");
    playerMessage("You cast " + spell->getName());
    Effect::applyDirected(c, dir, spell->getDirEffectType(), EffectStrength::NORMAL);
    level->getModel()->getStatistics().add(StatId::SPELL_CAST);
    c->spells.setReadyTime(spell, getTime() + spell->getDifficulty()
        * getWillpowerMult(getSkillValue(Skill::get(SkillId::SORCERY))));
    c->spendTime(1);
  });
}

void Creature::addCreatureVision(CreatureVision* creatureVision) {
  creatureVisions.push_back(creatureVision);
}

void Creature::removeCreatureVision(CreatureVision* vision) {
  removeElement(creatureVisions, vision);
}

void Creature::pushController(PController ctrl) {
  controllerStack.push_back(std::move(controller));
  setController(std::move(ctrl));
}

void Creature::setController(PController ctrl) {
  if (ctrl->isPlayer())
    modViewObject().setModifier(ViewObject::Modifier::PLAYER);
  controller = std::move(ctrl);
  level->updatePlayer();
}

void Creature::popController() {
  if (controller->isPlayer())
    modViewObject().removeModifier(ViewObject::Modifier::PLAYER);
  CHECK(!controllerStack.empty());
  controller = std::move(controllerStack.back());
  controllerStack.pop_back();
  level->updatePlayer();
}

bool Creature::isDead() const {
  return dead;
}

const Creature* Creature::getLastAttacker() const {
  return lastAttacker;
}

vector<const Creature*> Creature::getKills() const {
  return kills;
}

void Creature::spendTime(double t) {
  time += 100.0 * t / (double) getAttr(AttrType::SPEED);
  hidden = false;
}

CreatureAction Creature::forceMove(Vec2 dir) const {
  const_cast<Creature*>(this)->forceMovement = true;
  CreatureAction action = move(dir);
  const_cast<Creature*>(this)->forceMovement = false;
  if (action)
    return action.prepend([this] (Creature* c) { c->forceMovement = true; })
      .append([this] (Creature* c) { c->forceMovement = false; });
  else
    return action;
}

CreatureAction Creature::move(Vec2 direction) const {
  if (holding)
    return CreatureAction("You can't break free!");
  if ((direction.length8() != 1 || !level->canMoveCreature(this, direction)) && !swapPosition(direction))
    return CreatureAction();
  return CreatureAction(this, [=](Creature* c) {
    c->stationary = false;
    Debug() << getName().the() << " moving " << direction;
    if (isAffected(LastingEffect::ENTANGLED)) {
      playerMessage("You can't break free!");
      c->spendTime(1);
      return;
    }
    if (level->canMoveCreature(c, direction))
      level->moveCreature(c, direction);
    else
      c->swapPosition(direction).perform(c);
    double oldTime = getTime();
    if (collapsed) {
      you(MsgType::CRAWL, getSquare()->getName());
      c->spendTime(3);
    } else
      c->spendTime(1);
    c->modViewObject().addMovementInfo({direction, oldTime, getTime(), ViewObject::MovementInfo::MOVE});
  });
}

int Creature::getDebt(const Creature* debtor) const {
  return controller->getDebt(debtor);
}

void Creature::takeItems(vector<PItem> items, const Creature* from) {
  vector<Item*> ref = extractRefs(items);
  getSquare()->dropItems(std::move(items));
  controller->onItemsAppeared(ref, from);
}

void Creature::you(MsgType type, const string& param) const {
  controller->you(type, param);
}

void Creature::you(const string& param) const {
  controller->you(param);
}

void Creature::playerMessage(const PlayerMessage& message) const {
  controller->privateMessage(message);
}

Controller* Creature::getController() {
  return controller.get();
}

CreatureAction Creature::swapPosition(Vec2 direction, bool force) const {
  if (!getLevel()->inBounds(position + direction))
    return CreatureAction();
  const Creature* other = getSafeSquare(direction)->getCreature();
  if (!other)
    return CreatureAction();
  if (other->isAffected(LastingEffect::SLEEP) && !force)
    return CreatureAction(other->getName().the() + " is sleeping.");
  if ((swapPositionCooldown && !isPlayer()) || other->stationary || other->invincible ||
      direction.length8() != 1 || (other->isPlayer() && !force) || (other->isEnemy(this) && !force) ||
      !getSafeSquare(direction)->canEnterEmpty(this) || !getSquare()->canEnterEmpty(other))
    return CreatureAction();
  return CreatureAction(this, [=](Creature* c) {
    c->swapPositionCooldown = 4;
    if (!force)
      getSafeSquare(direction)->getCreature()->playerMessage("Excuse me!");
    playerMessage("Excuse me!");
    Creature* other = c->getSafeSquare(direction)->getCreature();
    level->swapCreatures(c, other);
    other->modViewObject().addMovementInfo({-direction, getTime(), other->getTime(),
        ViewObject::MovementInfo::MOVE});
  });
}

void Creature::makeMove() {
  numAttacksThisTurn = 0;
  CHECK(!isDead());
  if (holding && holding->isDead())
    holding = nullptr;
  if (isAffected(LastingEffect::SLEEP)) {
    controller->sleeping();
    spendTime(1);
    return;
  }
  if (isAffected(LastingEffect::STUNNED)) {
    spendTime(1);
    return;
  }
  int range = FieldOfView::sightRange;
  updateVisibleCreatures(Rectangle(getPosition() - Vec2(range, range), getPosition() + Vec2(range, range)));
  if (swapPositionCooldown)
    --swapPositionCooldown;
  MEASURE(controller->makeMove(), "creature move time");
  Debug() << getName().bare() << " morale " << getMorale();
  if (!hidden)
    modViewObject().removeModifier(ViewObject::Modifier::HIDDEN);
  unknownAttacker.clear();
  if (fireCreature && Random.roll(5))
    getSquare()->setOnFire(1);
}

Square* Creature::getSquare() {
  return level->getSafeSquare(position);
}

const Square* Creature::getSquare() const {
  return getLevel()->getSafeSquare(position);
}

vector<Square*> Creature::getSquare(Vec2 direction) {
  return getLevel()->getSquare(position + direction);
}

vector<const Square*> Creature::getSquare(Vec2 direction) const {
  return getLevel()->getSquare(position + direction);
}

Square* Creature::getSafeSquare(Vec2 direction) {
  return getLevel()->getSafeSquare(position + direction);
}

const Square* Creature::getSafeSquare(Vec2 direction) const {
  return getLevel()->getSafeSquare(position + direction);
}

vector<Square*> Creature::getSquares(const vector<Vec2>& direction) {
  return getLevel()->getSquares(transform2<Vec2>(direction, [&] (const Vec2& v) { return v + getPosition();}));
}

vector<const Square*> Creature::getSquares(const vector<Vec2>& direction) const {
  return getLevel()->getSquares(transform2<Vec2>(direction, [&] (const Vec2& v) { return v + getPosition();}));
}

CreatureAction Creature::wait() const {
  return CreatureAction(this, [=](Creature* c) {
    Debug() << getName().the() << " waiting";
    bool keepHiding = hidden;
    c->spendTime(1);
    c->hidden = keepHiding;
  });
}

const Equipment& Creature::getEquipment() const {
  return equipment;
}

Equipment& Creature::getEquipment() {
  return equipment;
}

vector<PItem> Creature::steal(const vector<Item*> items) {
  return equipment.removeItems(items);
}

Item* Creature::getAmmo() const {
  for (Item* item : equipment.getItems())
    if (item->getClass() == ItemClass::AMMO)
      return item;
  return nullptr;
}

Level* Creature::getLevel() {
  return level;
}

const Level* Creature::getLevel() const {
  return level;
}

Vec2 Creature::getPosition() const {
  return position;
}

void Creature::globalMessage(const PlayerMessage& playerCanSee, const PlayerMessage& cant) const {
  if (level)
    level->globalMessage(this, playerCanSee, cant);
}

void Creature::monsterMessage(const PlayerMessage& playerCanSee, const PlayerMessage& cant) const {
  if (level && !isPlayer())
    level->globalMessage(this, playerCanSee, cant);
}

void Creature::addSkill(Skill* skill) {
  if (!hasSkill(skill)) {
    skills.insert(skill->getId());
    playerMessage(skill->getHelpText());
  }
}

bool Creature::hasSkill(Skill* skill) const {
  return skills.hasDiscrete(skill->getId());
}

double Creature::getSkillValue(const Skill* skill) const {
  return skills.getValue(skill->getId());
}

const EnumSet<SkillId>& Creature::getDiscreteSkills() const {
  return skills.getAllDiscrete();
}

vector<Item*> Creature::getPickUpOptions() const {
  if (!isHumanoid())
    return vector<Item*>();
  else
    return getSquare()->getItems();
}

string Creature::getPluralTheName(Item* item, int num) const {
  if (num == 1)
    return item->getTheName(false, isBlind());
  else
    return toString(num) + " " + item->getTheName(true, isBlind());
}

string Creature::getPluralAName(Item* item, int num) const {
  if (num == 1)
    return item->getAName(false, isBlind());
  else
    return toString(num) + " " + item->getAName(true, isBlind());
}

CreatureAction Creature::pickUp(const vector<Item*>& items, bool spendT) const {
  if (!isHumanoid())
    return CreatureAction("You can't pick up anything!");
  double weight = getInventoryWeight();
  for (Item* it : items)
    weight += it->getWeight();
  if (weight > 2 * getModifier(ModifierType::INV_LIMIT))
    return CreatureAction("You are carrying too much to pick this up.");
  return CreatureAction(this, [=](Creature* c) {
    Debug() << getName().the() << " pickup ";
    if (spendT)
      for (auto stack : stackItems(items)) {
        monsterMessage(getName().the() + " picks up " + getPluralAName(stack[0], stack.size()));
        playerMessage("You pick up " + getPluralTheName(stack[0], stack.size()));
      }
    for (auto item : items) {
      c->equipment.addItem(c->getSquare()->removeItem(item));
    }
    if (getInventoryWeight() > getModifier(ModifierType::INV_LIMIT))
      playerMessage("You are overloaded.");
    GlobalEvents.addPickupEvent(this, items);
    if (spendT)
      c->spendTime(1);
  });
}

vector<vector<Item*>> Creature::stackItems(vector<Item*> items) const {
  map<string, vector<Item*> > stacks = groupBy<Item*, string>(items, 
      [this] (Item* const& item) { return item->getNameAndModifiers(false, isBlind()); });
  return getValues(stacks);
}

CreatureAction Creature::drop(const vector<Item*>& items) const {
  if (!isHumanoid())
    return CreatureAction("You can't drop this item!");
  return CreatureAction(this, [=](Creature* c) {
    Debug() << getName().the() << " drop";
    for (auto stack : stackItems(items)) {
      monsterMessage(getName().the() + " drops " + getPluralAName(stack[0], stack.size()));
      playerMessage("You drop " + getPluralTheName(stack[0], stack.size()));
    }
    for (auto item : items) {
      c->getSquare()->dropItem(c->equipment.removeItem(item));
    }
    GlobalEvents.addDropEvent(this, items);
    c->spendTime(1);
  });
}

void Creature::drop(vector<PItem> items) {
  getSquare()->dropItems(std::move(items));
}

bool Creature::canEquipIfEmptySlot(const Item* item, string* reason) const {
  if (!isHumanoid()) {
    if (reason)
      *reason = "Only humanoids can equip items!";
    return false;
  }
  if (numGood(BodyPart::ARM) == 0) {
    if (reason)
      *reason = "You have no healthy arms!";
    return false;
  }
  if (numGood(BodyPart::ARM) == 1 && item->isWieldedTwoHanded()) {
    if (reason)
      *reason = "You need two hands to wield " + item->getAName() + "!";
    return false;
  }
  return item->canEquip();
}

bool Creature::canEquip(const Item* item) const {
  return canEquipIfEmptySlot(item, nullptr) && equipment.canEquip(item);
}

CreatureAction Creature::equip(Item* item) const {
  string reason;
  if (!canEquipIfEmptySlot(item, &reason))
    return CreatureAction(reason);
  if (contains(equipment.getItem(item->getEquipmentSlot()), item))
    return CreatureAction();
  return CreatureAction(this, [=](Creature *c) {
    Debug() << getName().the() << " equip " << item->getName();
    EquipmentSlot slot = item->getEquipmentSlot();
    if (c->equipment.getItem(slot).size() >= c->equipment.getMaxItems(slot)) {
      Item* previousItem = c->equipment.getItem(slot)[0];
      c->equipment.unequip(previousItem);
      previousItem->onUnequip(c);
    }
    c->equipment.equip(item, slot);
    c->playerMessage("You equip " + item->getTheName(false, isBlind()));
    c->monsterMessage(c->getName().the() + " equips " + item->getAName());
    item->onEquip(c);
    GlobalEvents.addEquipEvent(c, item);
    c->spendTime(1);
  });
}

CreatureAction Creature::unequip(Item* item) const {
  if (!equipment.isEquiped(item))
    return CreatureAction("This item is not equiped.");
  if (!isHumanoid())
    return CreatureAction("You can't remove this item!");
  if (numGood(BodyPart::ARM) == 0)
    return CreatureAction("You have no healthy arms!");
  return CreatureAction(this, [=](Creature* c) {
    Debug() << getName().the() << " unequip";
    CHECK(equipment.isEquiped(item)) << "Item not equiped.";
    EquipmentSlot slot = item->getEquipmentSlot();
    c->equipment.unequip(item);
    c->playerMessage("You " + string(slot == EquipmentSlot::WEAPON ? " sheathe " : " remove ") +
        item->getTheName(false, isBlind()));
    c->monsterMessage(c->getName().the() + (slot == EquipmentSlot::WEAPON ? " sheathes " : " removes ") +
        item->getAName());
    item->onUnequip(c);
    c->spendTime(1);
  });
}

CreatureAction Creature::heal(Vec2 direction) const {
  if (!getLevel()->inBounds(position + direction))
    return CreatureAction();
  const Creature* other = getSafeSquare(direction)->getCreature();
  if (!hasSkill(Skill::get(SkillId::HEALING)) || !other || other->getHealth() >= 0.9999)
    return CreatureAction();
  return CreatureAction(this, [=](Creature *c) {
    Creature* other = c->getSafeSquare(direction)->getCreature();
    other->playerMessage("\"Let me help you my friend.\"");
    other->you(MsgType::ARE, "healed by " + getName().the());
    other->heal();
    c->spendTime(1);
  });
}

CreatureAction Creature::bumpInto(Vec2 direction) const {
  if (!getLevel()->inBounds(position + direction))
    return CreatureAction(); 
  if (const Creature* other = getSafeSquare(direction)->getCreature())
    return CreatureAction(this, [=](Creature* c) {
      other->controller->onBump(c);
      c->spendTime(1);
    });
  else
    return CreatureAction();
}

CreatureAction Creature::applySquare() const {
  if (getSquare()->getApplyType(this))
    return CreatureAction(this, [=](Creature* c) {
      Debug() << getName().the() << " applying " << getSquare()->getName();
      c->getSquare()->onApply(c);
      c->spendTime(1);
    });
  else
    return CreatureAction();
}

CreatureAction Creature::hide() const {
  if (!hasSkill(Skill::get(SkillId::AMBUSH)))
    return CreatureAction("You don't have this skill.");
  if (!getSquare()->canHide())
    return CreatureAction("You can't hide here.");
  return CreatureAction(this, [=](Creature* c) {
    playerMessage("You hide behind the " + getSquare()->getName());
    c->knownHiding.clear();
    c->modViewObject().setModifier(ViewObject::Modifier::HIDDEN);
    for (const Creature* other : getLevel()->getAllCreatures())
      if (other->canSee(this) && other->isEnemy(this)) {
        c->knownHiding.insert(other);
        if (!isBlind())
          you(MsgType::CAN_SEE_HIDING, other->getName().the());
      }
    c->spendTime(1);
    c->hidden = true;
  });
}

CreatureAction Creature::chatTo(Vec2 direction) const {
  for (const Square* s : getSquare(direction))
    if (const Creature* other = s->getCreature())
      return CreatureAction(this, [=](Creature* c) {
          playerMessage("You chat with " + other->getName().the());
          c->getSafeSquare(direction)->getCreature()->onChat(c);
          c->spendTime(1);
          });
  return CreatureAction();
}

void Creature::onChat(Creature* from) {
  if (isEnemy(from) && chatReactionHostile) {
    if (chatReactionHostile->front() == '\"')
      from->playerMessage(*chatReactionHostile);
    else
      from->playerMessage(getName().the() + " " + *chatReactionHostile);
  }
  if (!isEnemy(from) && chatReactionFriendly) {
    if (chatReactionFriendly->front() == '\"')
      from->playerMessage(*chatReactionFriendly);
    else
      from->playerMessage(getName().the() + " " + *chatReactionFriendly);
  }
}

void Creature::learnLocation(const Location* loc) {
  controller->learnLocation(loc);
}

CreatureAction Creature::stealFrom(Vec2 direction, const vector<Item*>& items) const {
  for (const Square* s : getSquare(direction))
    if (s->getCreature())
      return CreatureAction(this, [=](Creature *c) {
        Creature* other = NOTNULL(c->getSafeSquare(direction)->getCreature());
        c->equipment.addItems(other->steal(items));
      });
  return CreatureAction();
}

bool Creature::isHidden() const {
  return hidden;
}

bool Creature::knowsHiding(const Creature* c) const {
  return knownHiding.count(c) == 1;
}

bool Creature::affects(LastingEffect effect) const {
  switch (effect) {
    case LastingEffect::RAGE:
    case LastingEffect::PANIC: return !isAffected(LastingEffect::SLEEP);
    case LastingEffect::POISON: return !isAffected(LastingEffect::POISON_RESISTANT) && !isNotLiving();
    case LastingEffect::ENTANGLED: return !uncorporal;
    default: return true;
  }
}

void Creature::onAffected(LastingEffect effect, bool msg) {
  switch (effect) {
    case LastingEffect::FLYING:
      if (msg) you(MsgType::ARE, "flying!");
      break;
    case LastingEffect::STUNNED:
      if (msg) you(MsgType::ARE, "stunned");
      break;
    case LastingEffect::PANIC:
      removeEffect(LastingEffect::RAGE, false);
      if (msg) you(MsgType::PANIC, "");
      break;
    case LastingEffect::RAGE:
      removeEffect(LastingEffect::PANIC, false);
      if (msg) you(MsgType::RAGE, "");
      break;
    case LastingEffect::HALLU: 
      if (!isBlind() && msg)
        playerMessage("The world explodes into colors!");
      break;
    case LastingEffect::BLIND:
      if (msg) you(MsgType::ARE, "blind!");
      modViewObject().setModifier(ViewObject::Modifier::BLIND);
      break;
    case LastingEffect::INVISIBLE:
      if (!isBlind() && msg)
        you(MsgType::TURN_INVISIBLE, "");
      modViewObject().setModifier(ViewObject::Modifier::INVISIBLE);
      break;
    case LastingEffect::POISON:
      if (msg) you(MsgType::ARE, "poisoned");
      modViewObject().setModifier(ViewObject::Modifier::POISONED);
      break;
    case LastingEffect::STR_BONUS: if (msg) you(MsgType::FEEL, "stronger"); break;
    case LastingEffect::DEX_BONUS: if (msg) you(MsgType::FEEL, "more agile"); break;
    case LastingEffect::SPEED: 
      if (msg) you(MsgType::ARE, "moving faster");
      removeEffect(LastingEffect::SLOWED, false);
      break;
    case LastingEffect::SLOWED: 
      if (msg) you(MsgType::ARE, "moving more slowly");
      removeEffect(LastingEffect::SPEED, false);
      break;
    case LastingEffect::ENTANGLED: if (msg) you(MsgType::ARE, "entangled in a web"); break;
    case LastingEffect::SLEEP: if (msg) you(MsgType::FALL_ASLEEP, ""); break;
    case LastingEffect::POISON_RESISTANT:
      if (msg) you(MsgType::ARE, "now poison resistant");
      removeEffect(LastingEffect::POISON, true);
      break;
    case LastingEffect::FIRE_RESISTANT: if (msg) you(MsgType::ARE, "now fire resistant"); break;
    case LastingEffect::INSANITY: if (msg) you(MsgType::BECOME, "insane"); break;
    case LastingEffect::MAGIC_SHIELD: if (msg) you(MsgType::FEEL, "protected"); break;
  }
}

void Creature::onRemoved(LastingEffect effect, bool msg) {
  switch (effect) {
    case LastingEffect::POISON:
      if (msg)
        you(MsgType::ARE, "cured from poisoning");
      modViewObject().removeModifier(ViewObject::Modifier::POISONED);
      break;
    default: onTimedOut(effect, msg); break;
  }
}

void Creature::onTimedOut(LastingEffect effect, bool msg) {
  switch (effect) {
    case LastingEffect::STUNNED: break;
    case LastingEffect::SLOWED: if (msg) you(MsgType::ARE, "moving faster again"); break;
    case LastingEffect::SLEEP: if (msg) you(MsgType::WAKE_UP, ""); break;
    case LastingEffect::SPEED: if (msg) you(MsgType::ARE, "moving more slowly again"); break;
    case LastingEffect::STR_BONUS: if (msg) you(MsgType::ARE, "weaker again"); break;
    case LastingEffect::DEX_BONUS: if (msg) you(MsgType::ARE, "less agile again"); break;
    case LastingEffect::PANIC:
    case LastingEffect::RAGE:
    case LastingEffect::HALLU: if (msg) playerMessage("Your mind is clear again"); break;
    case LastingEffect::ENTANGLED: if (msg) you(MsgType::BREAK_FREE, "the web"); break;
    case LastingEffect::BLIND:
      if (msg) 
        you("can see again");
      modViewObject().removeModifier(ViewObject::Modifier::BLIND);
      break;
    case LastingEffect::INVISIBLE:
      if (msg)
        you(MsgType::TURN_VISIBLE, "");
      modViewObject().removeModifier(ViewObject::Modifier::INVISIBLE);
      break;
    case LastingEffect::POISON:
      if (msg)
        you(MsgType::ARE, "no longer poisoned");
      modViewObject().removeModifier(ViewObject::Modifier::POISONED);
      break;
    case LastingEffect::POISON_RESISTANT: if (msg) you(MsgType::ARE, "no longer poison resistant"); break;
    case LastingEffect::FIRE_RESISTANT: if (msg) you(MsgType::ARE, "no longer fire resistant"); break;
    case LastingEffect::FLYING:
      if (msg) you(MsgType::FALL, getSquare()->getName());
      bleed(0.1);
      break;
    case LastingEffect::INSANITY: if (msg) you(MsgType::BECOME, "sane again"); break;
    case LastingEffect::MAGIC_SHIELD: if (msg) you(MsgType::FEEL, "less protected"); break;
  } 
}

void Creature::addEffect(LastingEffect effect, double time, bool msg) {
  if (lastingEffects[effect] < getTime() + time && affects(effect)) {
    if (!isAffected(effect))
      onAffected(effect, msg);
    lastingEffects[effect] = getTime() + time;
  }
}

void Creature::removeEffect(LastingEffect effect, bool msg) {
  if (!isAffected(effect))
    return;
  lastingEffects[effect] = 0;
  if (!isAffected(effect))
    onRemoved(effect, msg);
}

void Creature::addPermanentEffect(LastingEffect effect, bool msg) {
  if (!isAffected(effect))
    onAffected(effect, msg);
  ++permanentEffects[effect];
}

void Creature::removePermanentEffect(LastingEffect effect, bool msg) {
  --permanentEffects[effect];
  CHECK(permanentEffects[effect] >= 0);
  if (!isAffected(effect))
    onRemoved(effect, msg);
}

double Creature::getTimeRemaining(LastingEffect effect) const {
  CHECK(isAffected(effect));
  return lastingEffects[effect] - getTime();
}

bool Creature::isAffected(LastingEffect effect) const {
  return lastingEffects[effect] >= getTime() || permanentEffects[effect] > 0;
}

bool Creature::isAffectedPermanently(LastingEffect effect) const {
  return permanentEffects[effect] > 0;
}

bool Creature::isBlind() const {
  return isAffected(LastingEffect::BLIND) || (numLost(BodyPart::HEAD) > 0 && numBodyParts(BodyPart::HEAD) == 0);
}

double Creature::getRawAttr(AttrType type) const {
  return attr[type] + attrIncrease[type];
}

int attrBonus = 3;

map<BodyPart, int> dexPenalty {
  {BodyPart::ARM, 2},
  {BodyPart::LEG, 10},
  {BodyPart::WING, 3},
  {BodyPart::HEAD, 3}};

map<BodyPart, int> strPenalty {
  {BodyPart::ARM, 2},
  {BodyPart::LEG, 5},
  {BodyPart::WING, 2},
  {BodyPart::HEAD, 3}};

// penalty to strength and dexterity per extra attacker in a single turn
int simulAttackPen(int attackers) {
  return max(0, (attackers - 1) * 2);
}

int Creature::getAttr(AttrType type) const {
  int def = getRawAttr(type);
  for (Item* item : equipment.getItems())
    if (equipment.isEquiped(item))
      def += item->getAttr(type);
  switch (type) {
    case AttrType::STRENGTH:
        if (health < 1)
          def *= 0.666 + health / 3;
        if (isAffected(LastingEffect::STR_BONUS))
          def += attrBonus;
        for (auto elem : strPenalty)
          def -= elem.second * (numInjured(elem.first) + numLost(elem.first));
        def -= simulAttackPen(numAttacksThisTurn);
        break;
    case AttrType::DEXTERITY:
        if (health < 1)
          def *= 0.666 + health / 3;
        if (isAffected(LastingEffect::DEX_BONUS))
          def += attrBonus;
        for (auto elem : dexPenalty)
          def -= elem.second * (numInjured(elem.first) + numLost(elem.first));
        def -= simulAttackPen(numAttacksThisTurn);
        break;
    case AttrType::SPEED: {
        double totWeight = getInventoryWeight();
        if (!carryAnything && totWeight > getAttr(AttrType::STRENGTH))
          def -= 20.0 * totWeight / def;
        if (isAffected(LastingEffect::SLOWED))
          def /= 1.5;
        if (isAffected(LastingEffect::SPEED))
          def *= 1.5;
        break;}
  }
  return def;
}

int Creature::accuracyBonus() const {
  if (Item* weapon = getWeapon())
    return -max(0, weapon->getMinStrength() - getAttr(AttrType::STRENGTH));
  else
    return 0;
}

int Creature::getModifier(ModifierType type) const {
  int def = 0;
  for (Item* item : equipment.getItems())
    if (equipment.isEquiped(item))
      def += item->getModifier(type);
  for (SkillId skill : ENUM_ALL(SkillId))
    def += Skill::get(skill)->getModifier(this, type);
  switch (type) {
    case ModifierType::FIRED_DAMAGE: 
    case ModifierType::THROWN_DAMAGE: 
        def += getAttr(AttrType::DEXTERITY);
        if (isAffected(LastingEffect::PANIC))
          def -= attrBonus;
        if (isAffected(LastingEffect::RAGE))
          def += attrBonus;
        break;
    case ModifierType::DAMAGE: 
        def += getAttr(AttrType::STRENGTH);
        if (!getWeapon())
          def += barehandedDamage;
        if (isAffected(LastingEffect::PANIC))
          def -= attrBonus;
        if (isAffected(LastingEffect::RAGE))
          def += attrBonus;
        break;
    case ModifierType::DEFENSE: 
        def += getAttr(AttrType::STRENGTH);
        if (isAffected(LastingEffect::PANIC))
          def += attrBonus;
        if (isAffected(LastingEffect::RAGE))
          def -= attrBonus;
        if (isAffected(LastingEffect::SLEEP))
          def *= 0.66;
        if (isAffected(LastingEffect::MAGIC_SHIELD))
          def += 20;
        break;
    case ModifierType::FIRED_ACCURACY: 
    case ModifierType::THROWN_ACCURACY: 
        def += getAttr(AttrType::DEXTERITY);
        break;
    case ModifierType::ACCURACY: 
        def += accuracyBonus();
        def += getAttr(AttrType::DEXTERITY);
        if (isAffected(LastingEffect::SLEEP))
          def = 0;
        break;
    case ModifierType::INV_LIMIT:
        if (carryAnything)
          return 1000000;
        return getAttr(AttrType::STRENGTH) * 2;
  }
  return max(0, def);
}

int Creature::getPoints() const {
  return points;
}

double maxLevelGain = 2;
double minLevelGain = 0.1;
double maxLevelDiff = 7;

void Creature::onKillEvent(const Creature* victim, const Creature* killer) {
  if (killer == this) {
    points += victim->getDifficultyPoints();
    double levelDiff = victim->getExpLevelDouble() - getExpLevelDouble();
    increaseExpLevel(max(0.05, min(maxLevelGain, 
            (maxLevelGain - minLevelGain) * (levelDiff + maxLevelDiff) / (2.0 * maxLevelDiff) + minLevelGain)));
  }
}

double Creature::getInventoryWeight() const {
  double ret = 0;
  for (Item* item : getEquipment().getItems())
    ret += item->getWeight();
  return ret;
}

Tribe* Creature::getTribe() {
  return tribe;
}

const Tribe* Creature::getTribe() const {
  return tribe;
}

bool Creature::isFriend(const Creature* c) const {
  return !isEnemy(c);
}

pair<double, double> Creature::getStanding(const Creature* c) const {
  double bestWeight = 0;
  double standing = getTribe()->getStanding(c);
  if (contains(privateEnemies, c)) {
    standing = -1;
    bestWeight = 1;
  }
  for (EnemyCheck* enemyCheck : enemyChecks)
    if (enemyCheck->hasStanding(c) && enemyCheck->getWeight() > bestWeight) {
      standing = enemyCheck->getStanding(c);
      bestWeight = enemyCheck->getWeight();
    }
  return make_pair(standing, bestWeight);
}

void Creature::addEnemyCheck(EnemyCheck* c) {
  enemyChecks.push_back(c);
}

void Creature::removeEnemyCheck(EnemyCheck* c) {
  removeElement(enemyChecks, c);
}

bool Creature::isEnemy(const Creature* c) const {
  if (isAffected(LastingEffect::INSANITY))
    return c != this;
  pair<double, double> myStanding = getStanding(c);
  pair<double, double> hisStanding = c->getStanding(this);
  double standing = 0;
  if (myStanding.second > hisStanding.second)
    standing = myStanding.first;
  if (myStanding.second < hisStanding.second)
    standing = hisStanding.first;
  if (myStanding.second == hisStanding.second)
    standing = min(myStanding.first, hisStanding.first);
  return c != this && standing < 0;
}

vector<Item*> Creature::getGold(int num) const {
  vector<Item*> ret;
  for (Item* item : equipment.getItems([](Item* it) { return it->getClass() == ItemClass::GOLD; })) {
    ret.push_back(item);
    if (ret.size() == num)
      return ret;
  }
  return ret;
}

void Creature::setPosition(Vec2 pos) {
  position = pos;
}

void Creature::setLevel(Level* l) {
  level = l;
}

double Creature::getTime() const {
  return time;
}

void Creature::setTime(double t) {
  time = t;
}

void Creature::tick(double realTime) {
  updateVision();
  if (Random.roll(5))
    getDifficultyPoints();
  for (Item* item : equipment.getItems()) {
    item->tick(time, level, position);
    if (item->isDiscarded())
      equipment.removeItem(item);
  }
  for (LastingEffect effect : ENUM_ALL(LastingEffect))
    if (lastingEffects[effect] > 0 && lastingEffects[effect] < realTime) {
      lastingEffects[effect] = 0;
      if (!isAffected(effect))
        onTimedOut(effect, true);
    }
  if (isAffected(LastingEffect::POISON)) {
    bleed(1.0 / 60);
    playerMessage("You feel poison flowing in your veins.");
  }
  double delta = realTime - lastTick;
  lastTick = realTime;
  updateViewObject();
  if (isNotLiving() && lostOrInjuredBodyParts() >= 4) {
    you(MsgType::FALL_APART, "");
    die(lastAttacker);
    return;
  }
  if (health < 0.5) {
    health -= delta / 40;
    playerMessage("You are bleeding.");
  }
  if (health <= 0) {
    you(MsgType::DIE_OF, isAffected(LastingEffect::POISON) ? "poisoning" : "bleeding");
    die(lastAttacker);
  }
  if (getSquare()->sunlightBurns())
    shineLight();
}

BodyPart Creature::armOrWing() const {
  if (numGood(BodyPart::ARM) == 0)
    return BodyPart::WING;
  if (numGood(BodyPart::WING) == 0)
    return BodyPart::ARM;
  return chooseRandom({ BodyPart::WING, BodyPart::ARM }, {1, 1});
}

BodyPart Creature::getBodyPart(AttackLevel attack) const {
  if (isAffected(LastingEffect::FLYING))
    return chooseRandom({BodyPart::TORSO, BodyPart::HEAD, BodyPart::LEG, BodyPart::WING, BodyPart::ARM}, {1, 1, 1, 2, 1});
  switch (attack) {
    case AttackLevel::HIGH: 
       return BodyPart::HEAD;
    case AttackLevel::MIDDLE:
       if (size == CreatureSize::SMALL || size == CreatureSize::MEDIUM || collapsed)
         return BodyPart::HEAD;
       else
         return chooseRandom({BodyPart::TORSO, armOrWing()}, {1, 1});
    case AttackLevel::LOW:
       if (size == CreatureSize::SMALL || collapsed)
         return chooseRandom({BodyPart::TORSO, armOrWing(), BodyPart::HEAD, BodyPart::LEG}, {1, 1, 1, 1});
       if (size == CreatureSize::MEDIUM)
         return chooseRandom({BodyPart::TORSO, armOrWing(), BodyPart::LEG}, {1, 1, 3});
       else
         return BodyPart::LEG;
  }
  return BodyPart::ARM;
}

bool Creature::hasSuicidalAttack() const {
  return barehandedAttack == AttackType::POSSESS;
}

static string getAttackParam(AttackType type) {
  switch (type) {
    case AttackType::CUT: return "cut";
    case AttackType::STAB: return "stab";
    case AttackType::CRUSH: return "crush";
    case AttackType::PUNCH: return "punch";
    case AttackType::EAT:
    case AttackType::BITE: return "bite";
    case AttackType::HIT: return "hit";
    case AttackType::SHOOT: return "shot";
    case AttackType::SPELL: return "spell";
    case AttackType::POSSESS: return "touch";
  }
}

string Creature::getBodyPartName(BodyPart part) {
  switch (part) {
    case BodyPart::LEG: return "leg";
    case BodyPart::ARM: return "arm";
    case BodyPart::WING: return "wing";
    case BodyPart::HEAD: return "head";
    case BodyPart::TORSO: return "torso";
    case BodyPart::BACK: return "back";
  }
}

string Creature::getAttrName(AttrType attr) {
  switch (attr) {
    case AttrType::STRENGTH: return "strength";
    case AttrType::DEXTERITY: return "dexterity";
    case AttrType::SPEED: return "speed";
  }
}

static string getAttrNameMore(AttrType attr) {
  switch (attr) {
    case AttrType::STRENGTH: return "stronger";
    case AttrType::DEXTERITY: return "more agile";
    case AttrType::SPEED: return "faster";
  }
}

string Creature::getModifierName(ModifierType attr) {
  switch (attr) {
    case ModifierType::DAMAGE: return "damage";
    case ModifierType::ACCURACY: return "accuracy";
    case ModifierType::THROWN_DAMAGE: return "thrown damage";
    case ModifierType::THROWN_ACCURACY: return "thrown accuracy";
    case ModifierType::FIRED_DAMAGE: return "projectile damage";
    case ModifierType::FIRED_ACCURACY: return "projectile accuracy";
    case ModifierType::DEFENSE: return "defense";
    case ModifierType::INV_LIMIT: return "carry capacity";
  }
}

static string getBodyPartBone(BodyPart part) {
  switch (part) {
    case BodyPart::HEAD: return "skull";
    default: return "bone";
  }
}

void Creature::injureBodyPart(BodyPart part, bool drop) {
  if (bodyParts[part] == 0)
    return;
  if (drop) {
    if (contains({BodyPart::LEG, BodyPart::ARM, BodyPart::WING}, part))
      level->getModel()->getStatistics().add(StatId::CHOPPED_LIMB);
    else if (part == BodyPart::HEAD)
      level->getModel()->getStatistics().add(StatId::CHOPPED_HEAD);
    --bodyParts[part];
    ++lostBodyParts[part];
    if (injuredBodyParts[part] > bodyParts[part])
      --injuredBodyParts[part];
  }
  else if (injuredBodyParts[part] < bodyParts[part])
    ++injuredBodyParts[part];
  switch (part) {
    case BodyPart::LEG:
      if (!collapsed && !isAffected(LastingEffect::FLYING)) {
        you(MsgType::COLLAPSE, "");
        collapsed = true;
      }
      break;
    case BodyPart::ARM:
      if (getWeapon()) {
        you(MsgType::DROP_WEAPON, getWeapon()->getName());
        getSquare()->dropItem(equipment.removeItem(getWeapon()));
      }
      break;
    case BodyPart::WING:
      if (isAffectedPermanently(LastingEffect::FLYING)) {
        removePermanentEffect(LastingEffect::FLYING);
      }
      if ((numBodyParts(BodyPart::LEG) < 2 || numInjured(BodyPart::LEG) > 0) && !collapsed) {
        collapsed = true;
      }
      break;
    default: break;
  }
  if (drop)
    getSquare()->dropItem(ItemFactory::corpse(getName().bare() + " " + getBodyPartName(part),
        getName().bare() + " " + getBodyPartBone(part),
        getWeight() / 8, isFood ? ItemClass::FOOD : ItemClass::CORPSE));
}

static MsgType getAttackMsg(AttackType type, bool weapon, AttackLevel level) {
  if (weapon)
    return type == AttackType::STAB ? MsgType::THRUST_WEAPON : MsgType::SWING_WEAPON;
  switch (type) {
    case AttackType::EAT:
    case AttackType::BITE: return MsgType::BITE;
    case AttackType::PUNCH: return level == AttackLevel::LOW ? MsgType::KICK : MsgType::PUNCH;
    case AttackType::HIT: return MsgType::HIT;
    case AttackType::POSSESS: return MsgType::TOUCH;
    default: FAIL << "Unhandled barehanded attack: " << int(type);
  }
  return MsgType(0);
}

CreatureAction Creature::attack(const Creature* other, optional<AttackLevel> attackLevel1, bool spend) const {
  CHECK(!other->isDead());
  Vec2 dir = other->getPosition() - getPosition();
  if (dir.length8() != 1)
    return CreatureAction();
  if (attackLevel1 && !contains(getAttackLevels(), *attackLevel1))
    return CreatureAction("Invalid attack level.");
  return CreatureAction(this, [=] (Creature* c) {
  Debug() << getName().the() << " attacking " << other->getName().the();
  int accuracy =  getModifier(ModifierType::ACCURACY);
  int damage = getModifier(ModifierType::DAMAGE);
  int accuracyVariance = 1 + accuracy / 3;
  int damageVariance = 1 + damage / 3;
  auto rAccuracy = [=] () { return Random.get(-accuracyVariance, accuracyVariance); };
  auto rDamage = [=] () { return Random.get(-damageVariance, damageVariance); };
  accuracy += rAccuracy() + rAccuracy();
  damage += rDamage() + rDamage();
  bool backstab = false;
  string enemyName = getLevel()->playerCanSee(other) ? other->getName().the() : "something";
  if (other->isPlayer())
    enemyName = "";
  if (!other->canSee(this) && canSee(other)) {
 //   if (getWeapon() && getWeapon()->getAttackType() == AttackType::STAB) {
      damage += 10;
      backstab = true;
 //   }
    you(MsgType::ATTACK_SURPRISE, enemyName);
  }
  AttackLevel attackLevel = attackLevel1 ? (*attackLevel1) : getRandomAttackLevel();
  Attack attack(this, attackLevel, getAttackType(), accuracy, damage, backstab,
      getWeapon() ? getWeapon()->getAttackEffect() : attackEffect);
  Creature* other = c->getSafeSquare(dir)->getCreature();
  if (!other->dodgeAttack(attack)) {
    if (getWeapon()) {
      you(getAttackMsg(attack.getType(), true, attack.getLevel()), getWeapon()->getName());
      if (!canSee(other))
        playerMessage("You hit something.");
    }
    else {
      you(getAttackMsg(attack.getType(), false, attack.getLevel()), enemyName);
    }
    other->takeDamage(attack);
  }
  else
    you(MsgType::MISS_ATTACK, enemyName);
  GlobalEvents.addAttackEvent(other, c);
  double oldTime = getTime();
  if (spend)
    c->spendTime(1);
  c->modViewObject().addMovementInfo({dir, oldTime, getTime(), ViewObject::MovementInfo::ATTACK});
  });
}

bool Creature::dodgeAttack(const Attack& attack) {
  ++numAttacksThisTurn;
  Debug() << getName().the() << " dodging " << attack.getAttacker()->getName().bare()
    << " accuracy " << attack.getAccuracy() << " dodge " << getModifier(ModifierType::ACCURACY);
  if (const Creature* c = attack.getAttacker()) {
    if (!canSee(c))
      unknownAttacker.push_back(c);
    if (!contains(privateEnemies, c) && c->getTribe() != tribe)
      privateEnemies.push_back(c);
  }
  return canSee(attack.getAttacker()) && attack.getAccuracy() <= getModifier(ModifierType::ACCURACY);
}

double Creature::getMinDamage(BodyPart part) const {
  map<BodyPart, double> damage {
    {BodyPart::WING, 0.3},
    {BodyPart::ARM, 0.6},
    {BodyPart::LEG, 0.8},
    {BodyPart::HEAD, 0.8},
    {BodyPart::TORSO, 1.5},
    {BodyPart::BACK, 1.5}};
  if (isUndead())
    return damage.at(part) / 2;
  else
    return damage.at(part);
}

bool Creature::isCritical(BodyPart part) const {
  return contains({BodyPart::TORSO, BodyPart::BACK}, part)
    || (part == BodyPart::HEAD && numGood(part) == 0 && !isUndead());
}

bool Creature::takeDamage(const Attack& attack) {
  AttackType attackType = attack.getType();
  Creature* other = const_cast<Creature*>(attack.getAttacker());
  if (other)
    if (!contains(privateEnemies, other) && other->getTribe() != tribe)
      privateEnemies.push_back(other);
  if (attackType == AttackType::POSSESS) {
    you(MsgType::ARE, "possessed by " + other->getName().the());
    other->die(nullptr, false, false);
    addEffect(LastingEffect::INSANITY, 10);
    return false;
  }
  int defense = getModifier(ModifierType::DEFENSE);
  Debug() << getName().the() << " attacked by " << other->getName().the()
      << " damage " << attack.getStrength() << " defense " << defense;
  if (passiveAttack && other && other->getPosition().dist8(position) == 1) {
    Effect::applyToCreature(other, *passiveAttack, EffectStrength::NORMAL);
    other->lastAttacker = this;
  }
  if (isAffected(LastingEffect::MAGIC_SHIELD)) {
    lastingEffects[LastingEffect::MAGIC_SHIELD] -= 5;
    globalMessage("The magic shield absorbs the attack", "");
  }
  if (attack.getStrength() > defense) {
    if (attackType == AttackType::EAT) {
      if (isLarger(*other->size, *size) && Random.roll(3)) {
        you(MsgType::ARE, "devoured by " + other->getName().the());
        die(other, false, false);
        return true;
      } else
        attackType = AttackType::BITE;
    }
    lastAttacker = attack.getAttacker();
    double dam = (defense == 0) ? 1 : double(attack.getStrength() - defense) / defense;
    dam *= damageMultiplier;
    if (!isNotLiving())
      bleed(dam);
    if (!uncorporal) {
      if (attackType != AttackType::SPELL) {
        BodyPart part = attack.inTheBack() && Random.roll(3) ? BodyPart::BACK : getBodyPart(attack.getLevel());
        if (dam >= getMinDamage(part) && numGood(part) > 0) {
          youHit(part, attackType); 
          injureBodyPart(part, contains({AttackType::CUT, AttackType::BITE}, attackType));
          if (isCritical(part)) {
            you(MsgType::DIE, "");
            die(attack.getAttacker());
            return true;
          }
          if (health <= 0)
            health = 0.1;
          return false;
        }
      }
    } else {
      you(MsgType::TURN, " into a wisp of smoke");
      die(attack.getAttacker());
      return true;
    }
    if (health <= 0) {
      you(MsgType::ARE, "critically wounded");
      you(MsgType::DIE, "");
      die(attack.getAttacker());
      return true;
    } else
    if (health < 0.5)
      you(MsgType::ARE, "critically wounded");
    else {
      if (!isNotLiving())
        you(MsgType::ARE, "wounded");
      else if (!attack.getEffect())
        you(MsgType::ARE, "not hurt");
    }
    if (auto effect = attack.getEffect())
      Effect::applyToCreature(this, *effect, EffectStrength::WEAK);
  } else {
    you(MsgType::GET_HIT_NODAMAGE, getAttackParam(attackType));
    if (attack.getEffect() && attack.getAttacker()->harmlessApply)
      Effect::applyToCreature(this, *attack.getEffect(), EffectStrength::NORMAL);
  }
  if (isAffected(LastingEffect::SLEEP))
    removeEffect(LastingEffect::SLEEP);
  return false;
}

void Creature::updateViewObject() {
  modViewObject().setAttribute(ViewObject::Attribute::DEFENSE, getModifier(ModifierType::DEFENSE));
  modViewObject().setAttribute(ViewObject::Attribute::ATTACK, getModifier(ModifierType::DAMAGE));
  modViewObject().setAttribute(ViewObject::Attribute::LEVEL, getExpLevel());
  modViewObject().setAttribute(ViewObject::Attribute::MORALE, getMorale());
  modViewObject().setModifier(ViewObject::Modifier::DRAW_MORALE);
  modViewObject().setAdjectives(getAdjectives());
  if (isAffected(LastingEffect::SLEEP))
    modViewObject().setModifier(ViewObject::Modifier::SLEEPING);
  else
    modViewObject().removeModifier(ViewObject::Modifier::SLEEPING);
  modViewObject().setAttribute(ViewObject::Attribute::BLEEDING, 1 - health);
}

double Creature::getHealth() const {
  return health;
}

double Creature::getMorale() const {
  for (auto& elem : moraleOverrides)
    if (auto ret = elem->getMorale())
      return *ret;
  return morale;
}

void Creature::addMorale(double val) {
  morale = min(1.0, max(-1.0, morale + val));
}

void Creature::addMoraleOverride(PMoraleOverride mod) {
  moraleOverrides.push_back(std::move(mod));
}

double Creature::getWeight() const {
  return *weight;
}

string sizeStr(CreatureSize s) {
  switch (s) {
    case CreatureSize::SMALL: return "small";
    case CreatureSize::MEDIUM: return "medium";
    case CreatureSize::LARGE: return "large";
    case CreatureSize::HUGE: return "huge";
  }
  return 0;
}

static string adjectives(CreatureSize s, bool undead, bool notLiving) {
  vector<string> ret {sizeStr(s)};
  if (notLiving)
    ret.push_back("non-living");
  if (undead)
    ret.push_back("undead");
  return combine(ret);
}

string Creature::bodyDescription() const {
  vector<string> ret;
  for (BodyPart part : {BodyPart::ARM, BodyPart::LEG, BodyPart::WING})
    if (int num = numBodyParts(part))
      ret.push_back(getPlural(getBodyPartName(part), num));
  if (isHumanoid() && numBodyParts(BodyPart::HEAD) == 0)
    ret.push_back("no head");
  if (ret.size() > 0)
    return " with " + combine(ret);
  else
    return "";
}

string attrStr(bool strong, bool agile, bool fast) {
  vector<string> good;
  vector<string> bad;
  if (strong)
    good.push_back("strong");
  else
    bad.push_back("weak");
  if (agile)
    good.push_back("agile");
  else
    bad.push_back("clumsy");
  if (fast)
    good.push_back("fast");
  else
    bad.push_back("slow");
  string p1 = combine(good);
  string p2 = combine(bad);
  if (p1.size() > 0 && p2.size() > 0)
    p1.append(", but ");
  p1.append(p2);
  return p1;
}

bool Creature::isSpecialMonster() const {
  return specialMonster;
}

string Creature::getDescription() const {
  string weapon;
  string attack;
  if (attackEffect)
    attack = " It has a " + Effect::getName(*attackEffect) + " attack.";
  return getName().the() + " is a " + adjectives(*size, undead, notLiving) +
      (isHumanoid() ? " humanoid" : " beast") + (uncorporal ? " spirit" : "") + bodyDescription() + ". " +
     "It is " + attrStr(getRawAttr(AttrType::STRENGTH) > 16, getRawAttr(AttrType::DEXTERITY) > 16,
         getRawAttr(AttrType::SPEED) > 100) + "." + weapon + attack;
}

void Creature::setBoulderSpeed(double value) {
  attr[AttrType::SPEED] = value;
}
  
CreatureSize Creature::getSize() const {
  return *size;
}

void Creature::heal(double amount, bool replaceLimbs) {
  Debug() << getName().the() << " heal";
  if (health < 1) {
    health = min(1., health + amount);
    if (health >= 0.5) {
      for (BodyPart part : ENUM_ALL(BodyPart))
        if (int numInjured = injuredBodyParts[part]) {
          you(MsgType::YOUR, getBodyPartName(part) + (numInjured > 1 ? "s are" : " is") + " in better shape");
          if (part == BodyPart::LEG && !lostBodyParts[BodyPart::LEG] && collapsed) {
            collapsed = false;
            you(MsgType::STAND_UP, "");
          }
          injuredBodyParts[part] = 0;
        }
      if (replaceLimbs)
      for (BodyPart part : ENUM_ALL(BodyPart))
        if (int numInjured = lostBodyParts[part]) {
            you(MsgType::YOUR, getBodyPartName(part) + (numInjured > 1 ? "s grow back!" : " grows back!"));
            if (part == BodyPart::LEG && collapsed) {
              collapsed = false;
              you(MsgType::STAND_UP, "");
            }
            if (part == BodyPart::WING)
              addPermanentEffect(LastingEffect::FLYING);
            lostBodyParts[part] = 0;
          }
    }
    if (health == 1) {
      you(MsgType::BLEEDING_STOPS, "");
      health = 1;
      lastAttacker = nullptr;
    }
    updateViewObject();
  }
}

void Creature::bleed(double severity) {
  updateViewObject();
  health -= severity;
  updateViewObject();
  Debug() << getName().the() << " health " << health;
}

void Creature::setOnFire(double amount) {
  if (!isFireResistant()) {
    you(MsgType::ARE, "burnt by the fire");
    bleed(6. * amount / double(getAttr(AttrType::STRENGTH)));
  }
}

void Creature::poisonWithGas(double amount) {
  if (!isAffected(LastingEffect::POISON_RESISTANT) && breathing && !isNotLiving()) {
    you(MsgType::ARE, "poisoned by the gas");
    bleed(amount / double(getAttr(AttrType::STRENGTH)));
  }
}

void Creature::shineLight() {
  if (undead) {
    you(MsgType::ARE, "burnt by the sun");
    if (Random.roll(10)) {
      you(MsgType::YOUR, "body crumbles to dust");
      die(nullptr);
    }
  }
}

void Creature::setHeld(const Creature* c) {
  holding = c;
}

bool Creature::isHeld() const {
  return holding != nullptr;
}

bool Creature::canSleep() const {
  return !noSleep;
}

void Creature::take(vector<PItem> items) {
  for (PItem& elem : items)
    take(std::move(elem));
}

void Creature::take(PItem item) {
 /* item->identify();
  Debug() << (specialMonster ? "special monster " : "") + getName().the() << " takes " << item->getNameAndModifiers();*/
  Item* ref = item.get();
  equipment.addItem(std::move(item));
  if (auto action = equip(ref))
    action.perform(this);
}

void Creature::dropCorpse() {
  getSquare()->dropItems(getCorpse());
}

vector<PItem> Creature::getCorpse() {
  return makeVec<PItem>(
      ItemFactory::corpse(getName().bare() + " corpse", getName().bare() + " skeleton", getWeight(),
        isFood ? ItemClass::FOOD : ItemClass::CORPSE,
        {getUniqueId(), true, numBodyParts(BodyPart::HEAD) > 0, false}));
}

void Creature::die(const Creature* attacker, bool dropInventory, bool dCorpse) {
  CHECK(!dead);
  lastAttacker = attacker;
  Debug() << getName().the() << " dies. Killed by " << (attacker ? attacker->getName().bare() : "");
  controller->onKilled(attacker);
  if (attacker)
    attacker->kills.push_back(this);
  if (dropInventory)
    for (PItem& item : equipment.removeAllItems()) {
      getSquare()->dropItem(std::move(item));
    }
  dead = true;
  if (dropInventory && dCorpse && !uncorporal)
    dropCorpse();
  level->killCreature(this);
  GlobalEvents.addKillEvent(this, attacker);
  if (innocent)
    level->getModel()->getStatistics().add(StatId::INNOCENT_KILLED);
  level->getModel()->getStatistics().add(StatId::DEATH);
}

bool Creature::isInnocent() const {
  return innocent;
}

CreatureAction Creature::flyAway() const {
  if (!isAffected(LastingEffect::FLYING) || level->getCoverInfo(position).covered())
    return CreatureAction();
  return CreatureAction(this, [=](Creature* c) {
    Debug() << getName().the() << " fly away";
    monsterMessage(getName().the() + " flies away.");
    c->dead = true;
    level->killCreature(c);
  });
}

CreatureAction Creature::disappear() const {
  return CreatureAction(this, [=](Creature* c) {
    Debug() << getName().the() << " disappears";
    monsterMessage(getName().the() + " disappears.");
    c->dead = true;
    level->killCreature(c);
  });
}

CreatureAction Creature::torture(Creature* other) const {
  if (other->getSquare()->getApplyType(this) != SquareApplyType::TORTURE
      || other->getPosition().dist8(getPosition()) != 1)
    return CreatureAction();
  return CreatureAction(this, [=](Creature *c) {
    monsterMessage(getName().the() + " tortures " + other->getName().the());
    playerMessage("You torture " + other->getName().the());
    if (Random.roll(4))
      other->monsterMessage(other->getName().the() + " screams!", "You hear a horrible scream");
    other->addEffect(LastingEffect::STUNNED, 3, false);
    other->bleed(0.1);
    if (other->health < 0.3) {
      if (!Random.roll(8))
        other->heal();
      else
        other->bleed(1);
    }
    GlobalEvents.addTortureEvent(other, this);
    c->spendTime(1);
  });
}

void Creature::give(const Creature* whom, vector<Item*> items) {
  getLevel()->getSafeSquare(whom->getPosition())->getCreature()->takeItems(equipment.removeItems(items), this);
}

CreatureAction Creature::fire(Vec2 direction) const {
  CHECK(direction.length8() == 1);
  if (getEquipment().getItem(EquipmentSlot::RANGED_WEAPON).empty())
    return CreatureAction("You need a ranged weapon.");
  if (numGood(BodyPart::ARM) < 2)
    return CreatureAction("You need two hands to shoot a bow.");
  if (!getAmmo())
    return CreatureAction("Out of ammunition");
  return CreatureAction(this, [=](Creature* c) {
    PItem ammo = c->equipment.removeItem(NOTNULL(getAmmo()));
    RangedWeapon* weapon = NOTNULL(dynamic_cast<RangedWeapon*>(
        getOnlyElement(c->getEquipment().getItem(EquipmentSlot::RANGED_WEAPON))));
    weapon->fire(c, level, std::move(ammo), direction);
    c->spendTime(1);
  });
}

CreatureAction Creature::construct(Vec2 direction, SquareType type) const {
  for (const Square* s : getSquare(direction))
    if (s->canConstruct(type) && canConstruct(type))
      return CreatureAction(this, [=](Creature* c) {
        if (c->getSafeSquare(direction)->construct(type)) {
          if (type.getId() == SquareId::TREE_TRUNK) {
            monsterMessage(getName().the() + " cuts a tree");
            playerMessage("You cut a tree");
          } else
          if (type.getId() == SquareId::FLOOR) {
            monsterMessage(getName().the() + " digs a tunnel");
            playerMessage("You dig a tunnel");
          } else {
            monsterMessage(getName().the() + " builds " + getSafeSquare(direction)->getName());
            playerMessage("You build " + getSafeSquare(direction)->getName());
          }
        }
        c->spendTime(1);
      });
  return CreatureAction();
}

bool Creature::canConstruct(SquareType type) const {
  return hasSkill(Skill::get(SkillId::CONSTRUCTION));
}

CreatureAction Creature::eat(Item* item) const {
  return CreatureAction(this, [=](Creature* c) {
    c->getSquare()->removeItem(item);
    c->spendTime(3);
  });
}

CreatureAction Creature::destroy(Vec2 direction, DestroyAction dAction) const {
  for (const Square* s : getSquare(direction))
    if (direction.length8() == 1 && s->canDestroy(this))
      return CreatureAction(this, [=](Creature* c) {
        switch (dAction) {
          case BASH: 
            playerMessage("You bash the " + s->getName());
            monsterMessage(getName().the() + " bashes the " + s->getName(), "BANG!");
            break;
          case EAT: 
            playerMessage("You eat the " + s->getName());
            monsterMessage(getName().the() + " eats the " + s->getName(), "You hear chewing");
            break;
          case DESTROY: 
            playerMessage("You destroy the " + s->getName());
            monsterMessage(getName().the() + " destroys the " + s->getName(), "CRASH!");
            break;
      }
      c->getSafeSquare(direction)->destroyBy(c);
      c->spendTime(1);
    });
  return CreatureAction();
}

bool Creature::canCopulateWith(const Creature* c) const {
  return c->isCorporal() && c->getGender() != getGender() && c->isAffected(LastingEffect::SLEEP) && c->isHumanoid();
}

bool Creature::canConsume(const Creature* c) const {
  return c->isCorporal();
}

CreatureAction Creature::copulate(Vec2 direction) const {
  for (const Square* s : getSquare(direction)) {
    const Creature* other = s->getCreature();
    if (!other || !canCopulateWith(other))
      return CreatureAction();
    return CreatureAction(this, [=](Creature* c) {
      Debug() << getName().bare() << " copulate with " << other->getName().bare();
      you(MsgType::COPULATE, "with " + other->getName().the());
      c->spendTime(2);
    });
  }
  return CreatureAction();
}

static bool consumeProb() {
  return true;
}

template <typename T>
void consumeAttr(T& mine, const T& his, vector<string>& adjectives, const string& adj) {
  if (consumeProb() && mine < his) {
    mine = his;
    if (!adj.empty())
      adjectives.push_back(adj);
  }
}

void consumeAttr(Gender& mine, const Gender& his, vector<string>& adjectives) {
  if (consumeProb() && mine != his) {
    mine = his;
    adjectives.emplace_back(mine == Gender::male ? "more masculine" : "more feminine");
  }
}


template <typename T>
void consumeAttr(optional<T>& mine, const optional<T>& his, vector<string>& adjectives, const string& adj) {
  if (consumeProb() && !mine && his) {
    mine = *his;
    if (!adj.empty())
      adjectives.push_back(adj);
  }
}

void consumeAttr(Skillset& mine, const Skillset& his, vector<string>& adjectives) {
  bool was = false;
  for (SkillId id : his.getAllDiscrete())
    if (!mine.hasDiscrete(id) && Skill::get(id)->transferOnConsumption() && consumeProb()) {
      mine.insert(id);
      was = true;
    }
  for (SkillId id : ENUM_ALL(SkillId)) {
    if (!Skill::get(id)->isDiscrete() && mine.getValue(id) < his.getValue(id)) {
      mine.setValue(id, his.getValue(id));
      was = true;
    }
  }
  if (was)
    adjectives.push_back("more skillfull");
}

void Creature::consumeEffects(const EnumMap<LastingEffect, int>& effects) {
  for (LastingEffect effect : ENUM_ALL(LastingEffect))
    if (effects[effect] > 0 && !isAffected(effect) && consumeProb()) {
      addPermanentEffect(effect);
    }
}

void Creature::consumeBodyParts(const EnumMap<BodyPart, int>& parts) {
  for (BodyPart part : ENUM_ALL(BodyPart))
    if (parts[part] > bodyParts[part] && consumeProb()) {
      if (bodyParts[part] + 1 == parts[part])
        you(MsgType::GROW, "a " + getBodyPartName(part));
      else
        you(MsgType::GROW, toString(parts[part] - bodyParts[part]) + " " + getBodyPartName(part) + "s");
      bodyParts[part] = parts[part];
    }
}

vector<string> Creature::popPersonalEvents() {
  vector<string> ret = personalEvents;
  personalEvents.clear();
  return ret;
}

CreatureAction Creature::consume(Vec2 direction) const {
  if (!getLevel()->inBounds(position + direction))
    return CreatureAction();
  const Creature* other = getSafeSquare(direction)->getCreature();
  if (!hasSkill(Skill::get(SkillId::CONSUMPTION)) || !other || !other->isCorporal() || !isFriend(other))
    return CreatureAction();
  return CreatureAction(this, [=] (Creature* c) {
    Debug() << getName().bare() << " consume " << other->getName().bare();
    you(MsgType::CONSUME, other->getName().the());
    c->consumeBodyParts(other->bodyParts);
    if (*other->humanoid && !*humanoid 
        && bodyParts[BodyPart::ARM] >= 2 && bodyParts[BodyPart::LEG] >= 2 && bodyParts[BodyPart::HEAD] >= 1) {
      you(MsgType::BECOME, "a humanoid");
      c->personalEvents.push_back(getName().the() + " turns into a humanoid");
      *c->humanoid = true;
    }
    vector<string> adjectives;
    for (auto t : ENUM_ALL(AttrType))
      consumeAttr(c->attr[t], other->attr[t], adjectives, getAttrNameMore(t));
    consumeAttr(*c->size, *other->size, adjectives, "larger");
    consumeAttr(*c->weight, *other->weight, adjectives, "");
    consumeAttr(c->barehandedDamage, other->barehandedDamage, adjectives, "more dangerous");
    consumeAttr(c->barehandedAttack, other->barehandedAttack, adjectives, "");
    consumeAttr(c->attackEffect, other->attackEffect, adjectives, "");
    consumeAttr(c->passiveAttack, other->passiveAttack, adjectives, "");
    consumeAttr(c->gender, other->gender, adjectives);
    consumeAttr(c->skills, other->skills, adjectives);
    if (!adjectives.empty())
      you(MsgType::BECOME, combine(adjectives));
    c->personalEvents.push_back(getName().the() + " becomes " + combine(adjectives));
    c->consumeBodyParts(other->bodyParts);
    c->consumeEffects(other->permanentEffects);
    c->getSafeSquare(direction)->getCreature()->die(this, true, false);
    c->spendTime(2);
  });
}

vector<AttackLevel> Creature::getAttackLevels() const {
  if (isHumanoid() && !numGood(BodyPart::ARM))
    return {AttackLevel::LOW};
  switch (*size) {
    case CreatureSize::SMALL: return {AttackLevel::LOW};
    case CreatureSize::MEDIUM: return {AttackLevel::LOW, AttackLevel::MIDDLE};
    case CreatureSize::LARGE: return {AttackLevel::LOW, AttackLevel::MIDDLE, AttackLevel::HIGH};
    case CreatureSize::HUGE: return {AttackLevel::MIDDLE, AttackLevel::HIGH};
  }
  FAIL << "ewf";
  return {};
}

AttackLevel Creature::getRandomAttackLevel() const {
  return chooseRandom(getAttackLevels());
}

Item* Creature::getWeapon() const {
  vector<Item*> it = equipment.getItem(EquipmentSlot::WEAPON);
  if (it.empty())
    return nullptr;
  else
    return getOnlyElement(it);
}

AttackType Creature::getAttackType() const {
  if (getWeapon())
    return getWeapon()->getAttackType();
  else if (barehandedAttack)
    return *barehandedAttack;
  else
    return isHumanoid() ? AttackType::PUNCH : AttackType::BITE;
}

CreatureAction Creature::applyItem(Item* item) const {
  if (!contains({ItemClass::TOOL, ItemClass::POTION, ItemClass::FOOD, ItemClass::BOOK, ItemClass::SCROLL},
      item->getClass()) || !isHumanoid())
    return CreatureAction("You can't apply this item");
  if (numGood(BodyPart::ARM) == 0)
    return CreatureAction("You have no healthy arms!");
  return CreatureAction(this, [=] (Creature* c) {
      double time = item->getApplyTime();
      playerMessage("You " + item->getApplyMsgFirstPerson(isBlind()));
      monsterMessage(getName().the() + " " + item->getApplyMsgThirdPerson(isBlind()), item->getNoSeeApplyMsg());
      item->apply(c, level);
      if (item->isDiscarded()) {
        c->equipment.removeItem(item);
      }
      c->spendTime(time);
  });
}

CreatureAction Creature::throwItem(Item* item, Vec2 direction) const {
  if (!numGood(BodyPart::ARM) || !isHumanoid())
    return CreatureAction("You can't throw anything!");
  else if (item->getWeight() > 20)
    return CreatureAction(item->getTheName() + " is too heavy!");
  int dist = 0;
  int accuracyVariance = 10;
  int attackVariance = 10;
  int str = getAttr(AttrType::STRENGTH);
  if (item->getWeight() <= 0.5)
    dist = 10 * str / 15;
  else if (item->getWeight() <= 5)
    dist = 5 * str / 15;
  else if (item->getWeight() <= 20)
    dist = 2 * str / 15;
  else 
    FAIL << "Item too heavy.";
  int accuracy = Random.get(-accuracyVariance, accuracyVariance) +
      getModifier(ModifierType::THROWN_ACCURACY) + item->getModifier(ModifierType::THROWN_ACCURACY);
  int damage = Random.get(-attackVariance, attackVariance) +
      getModifier(ModifierType::THROWN_DAMAGE) + item->getModifier(ModifierType::THROWN_DAMAGE);
  if (item->getAttackType() == AttackType::STAB) {
    damage += Skill::get(SkillId::KNIFE_THROWING)->getModifier(this, ModifierType::THROWN_DAMAGE);
    accuracy += Skill::get(SkillId::KNIFE_THROWING)->getModifier(this, ModifierType::THROWN_ACCURACY);
  }
  Attack attack(this, getRandomAttackLevel(), item->getAttackType(), accuracy, damage, false, none);
  return CreatureAction(this, [=](Creature* c) {
    playerMessage("You throw " + item->getAName(false, isBlind()));
    monsterMessage(getName().the() + " throws " + item->getAName());
    level->throwItem(c->equipment.removeItem(item), attack, dist, getPosition(), direction, getVision());
    c->spendTime(1);
  });
}

bool Creature::canSee(const Creature* c) const {
  if (c->getLevel() != level)
    return false;
  for (CreatureVision* v : creatureVisions)
    if (v->canSee(this, c))
      return true;
  return !isBlind() && !c->isAffected(LastingEffect::INVISIBLE) &&
         (!c->isHidden() || c->knowsHiding(this)) && 
         getLevel()->canSee(this, c->getPosition());
}

bool Creature::canSee(Vec2 pos) const {
  return !isBlind() && 
      getLevel()->canSee(this, pos);
}
 
bool Creature::isPlayer() const {
  return controller->isPlayer();
}

optional<string> Creature::getFirstName() const {
  return firstName;
}

void Creature::setFirstName(const string& name) {
  firstName = name;
}

string Creature::getGroupName(int count) const {
  return groupName + " of " + getName().multiple(count);
}

const EntityName& Creature::getName() const {
  return *name;
}

string Creature::getSpeciesName() const {
  if (speciesName)
    return *speciesName;
  else
    return getName().bare();
}

bool Creature::isHumanoid() const {
  return *humanoid;
}

bool Creature::isAnimal() const {
  return animal;
}

bool Creature::isStationary() const {
  return stationary;
}

void Creature::setStationary() {
  stationary = true;
}

bool Creature::isInvincible() const {
  return invincible;
}

bool Creature::isUndead() const {
  return undead;
}

bool Creature::isNotLiving() const {
  return undead || notLiving || uncorporal;
}

bool Creature::isCorporal() const {
  return !uncorporal;
}

bool Creature::isWorshipped() const {
  return worshipped;
}

bool Creature::hasBrain() const {
  return brain;
}

bool Creature::isHatcheryAnimal() const {
  return hatcheryAnimal;
}

bool Creature::dontChase() const {
  return CreatureAttributes::dontChase;
}
optional<SpawnType> Creature::getSpawnType() const {
  return spawnType;
}

MovementType Creature::getMovementType() const {
  return MovementType(getTribe(), {
      true,
      isAffected(LastingEffect::FLYING),
      hasSkill(Skill::get(SkillId::SWIMMING)),
      contains({CreatureSize::HUGE, CreatureSize::LARGE}, *size),
      isBlind() || isHeld() || forceMovement,
      isFireResistant(),
      undead});
}

int Creature::numBodyParts(BodyPart part) const {
  return bodyParts[part];
}

int Creature::numLost(BodyPart part) const {
  return lostBodyParts[part];
}

int Creature::lostOrInjuredBodyParts() const {
  int ret = 0;
  for (BodyPart part : ENUM_ALL(BodyPart))
    ret += injuredBodyParts[part];
  for (BodyPart part : ENUM_ALL(BodyPart))
    ret += lostBodyParts[part];
  return ret;
}

int Creature::numInjured(BodyPart part) const {
  return injuredBodyParts[part];
}

int Creature::numGood(BodyPart part) const {
  return numBodyParts(part) - numInjured(part);
}

double Creature::getCourage() const {
  if (!hasBrain())
    return 1000;
  return courage;
}

void Creature::setCourage(double c) {
  courage = c;
}

Gender Creature::getGender() const {
  return gender;
}

double Creature::getExpLevelDouble() const {
  vector<pair<AttrType, int>> countAttr {
    {AttrType::STRENGTH, 12},
    {AttrType::DEXTERITY, 12}};
  double sum = 0;
  for (auto elem : countAttr)
    sum += 10.0 * (getRawAttr(elem.first) / elem.second - 1);
  return max(1.0, sum);
}

int Creature::getExpLevel() const {
  return getExpLevelDouble();
}

double exerciseMax = 2.0;
double increaseMult = 0.001; // This translates to about 690 stat exercises to reach 50% of the max increase,
                             // and 2300 to reach 90%

void Creature::exerciseAttr(AttrType t, double value) {
  attrIncrease[t] += ((exerciseMax - 1) * attr[t] - attrIncrease[t]) * increaseMult * value;
}

void Creature::increaseExpLevel(double increase) {
  double l = getExpLevelDouble();
  for (int i : Range(100000)) {
    exerciseAttr(chooseRandom<AttrType>(), 0.05);
    if (getExpLevelDouble() >= l + increase)
      break;
  }
}

int Creature::getDifficultyPoints() const {
  difficultyPoints = max<double>(difficultyPoints,
      getModifier(ModifierType::DEFENSE) + getModifier(ModifierType::ACCURACY) + getModifier(ModifierType::DAMAGE)
      + getAttr(AttrType::SPEED) / 10);
  return difficultyPoints;
}

CreatureAction Creature::continueMoving() {
  if (shortestPath && shortestPath->isReachable(getPosition())) {
    Vec2 pos2 = shortestPath->getNextMove(getPosition());
    return move(pos2 - getPosition());
  }
  return CreatureAction();
}

CreatureAction Creature::stayIn(const Location* location) {
  Rectangle area = location->getBounds();
  if (getLevel() != location->getLevel())
    return CreatureAction();
  if (!getPosition().inRectangle(area)) {
    for (Vec2 v : Vec2::directions8())
      if ((getPosition() + v).inRectangle(area))
        if (auto action = move(v))
          return action;
    return moveTowards(Vec2((area.getPX() + area.getKX()) / 2, (area.getPY() + area.getKY()) / 2));
  }
  return CreatureAction();
}

CreatureAction Creature::moveTowards(Vec2 pos, bool stepOnTile) {
  return moveTowards(pos, false, stepOnTile);
}

CreatureAction Creature::moveTowards(Vec2 pos, bool away, bool stepOnTile) {
  if (stepOnTile && !level->getSafeSquare(pos)->canEnterEmpty(this))
    return CreatureAction();
  if (!away) {
    bool sectorOk = false;
    MovementType movement = getMovementType();
    for (Vec2 v : pos.neighbors8())
      if (v.inRectangle(level->getBounds()) && level->areConnected(position, v, movement)) {
        sectorOk = true;
        break;
      }
    if (!sectorOk)
      return CreatureAction();
  }
  Debug() << "" << getPosition() << (away ? "Moving away from" : " Moving toward ") << pos;
  bool newPath = false;
  bool targetChanged = shortestPath && shortestPath->getTarget().dist8(pos) > getPosition().dist8(pos) / 10;
  if (!shortestPath || targetChanged || shortestPath->isReversed() != away) {
    newPath = true;
    if (!away)
      shortestPath = ShortestPath(getLevel(), this, pos, getPosition());
    else
      shortestPath = ShortestPath(getLevel(), this, pos, getPosition(), -1.5);
  }
  CHECK(shortestPath);
  if (shortestPath->isReachable(getPosition())) {
    Vec2 pos2 = shortestPath->getNextMove(getPosition());
    if (auto action = move(pos2 - getPosition()))
      return action;
  }
  if (newPath)
    return CreatureAction();
  Debug() << "Reconstructing shortest path.";
  if (!away)
    shortestPath = ShortestPath(getLevel(), this, pos, getPosition());
  else
    shortestPath = ShortestPath(getLevel(), this, pos, getPosition(), -1.5);
  if (shortestPath->isReachable(getPosition())) {
    Vec2 pos2 = shortestPath->getNextMove(getPosition());
    if (auto action = move(pos2 - getPosition()))
      return action;
    else {
      if (!getLevel()->getSafeSquare(pos2)->canEnterEmpty(this))
        if (auto action = destroy(pos2 - getPosition(), Creature::BASH))
          return action;
      return CreatureAction();
    }
  } else {
    Debug() << "Cannot move toward " << pos;
    return CreatureAction();
  }
}

CreatureAction Creature::moveAway(Vec2 pos, bool pathfinding) {
  if ((pos - getPosition()).length8() <= 5 && pathfinding)
    if (auto action = moveTowards(pos, true, false))
      return action;
  pair<Vec2, Vec2> dirs = (getPosition() - pos).approxL1();
  vector<CreatureAction> moves;
  if (auto action = move(dirs.first))
    moves.push_back(action);
  if (auto action = move(dirs.second))
    moves.push_back(action);
  if (moves.size() > 0)
    return moves[Random.get(moves.size())];
  return CreatureAction();
}

bool Creature::atTarget() const {
  return shortestPath && getPosition() == shortestPath->getTarget();
}

void Creature::youHit(BodyPart part, AttackType type) const {
  switch (part) {
    case BodyPart::BACK:
        switch (type) {
          case AttackType::SHOOT: you(MsgType::ARE, "shot in the back!"); break;
          case AttackType::BITE: you(MsgType::ARE, "bitten in the neck!"); break;
          case AttackType::CUT: you(MsgType::YOUR, "throat is cut!"); break;
          case AttackType::CRUSH: you(MsgType::YOUR, "spine is crushed!"); break;
          case AttackType::PUNCH: you(MsgType::YOUR, "neck is broken!"); break;
          case AttackType::HIT: you(MsgType::ARE, "hit in the back of the head!"); break;
          case AttackType::STAB: you(MsgType::ARE, "stabbed in the " + 
                                     chooseRandom<string>({"back", "neck"})); break;
          default: FAIL << "Unhandled attack type " << int(type);
        }
        break;
    case BodyPart::HEAD: 
        switch (type) {
          case AttackType::SHOOT: you(MsgType::ARE, "shot in the " +
                                      chooseRandom<string>({"eye", "neck", "forehead"}) + "!"); break;
          case AttackType::BITE: you(MsgType::YOUR, "head is bitten off!"); break;
          case AttackType::CUT: you(MsgType::YOUR, "head is chopped off!"); break;
          case AttackType::CRUSH: you(MsgType::YOUR, "skull is shattered!"); break;
          case AttackType::PUNCH: you(MsgType::YOUR, "neck is broken!"); break;
          case AttackType::HIT: you(MsgType::ARE, "hit in the head!"); break;
          case AttackType::STAB: you(MsgType::ARE, "stabbed in the eye!"); break;
          default: FAIL << "Unhandled attack type " << int(type);
        }
        break;
    case BodyPart::TORSO:
        switch (type) {
          case AttackType::SHOOT: you(MsgType::YOUR, "shot in the heart!"); break;
          case AttackType::BITE: you(MsgType::YOUR, "internal organs are ripped out!"); break;
          case AttackType::CUT: you(MsgType::ARE, "cut in half!"); break;
          case AttackType::STAB: you(MsgType::ARE, "stabbed in the " +
                                     chooseRandom<string>({"stomach", "heart"}, {1, 1}) + "!"); break;
          case AttackType::CRUSH: you(MsgType::YOUR, "ribs and internal organs are crushed!"); break;
          case AttackType::HIT: you(MsgType::ARE, "hit in the chest!"); break;
          case AttackType::PUNCH: you(MsgType::YOUR, "stomach receives a deadly blow!"); break;
          default: FAIL << "Unhandled attack type " << int(type);
        }
        break;
    case BodyPart::ARM:
        switch (type) {
          case AttackType::SHOOT: you(MsgType::YOUR, "shot in the arm!"); break;
          case AttackType::BITE: you(MsgType::YOUR, "arm is bitten off!"); break;
          case AttackType::CUT: you(MsgType::YOUR, "arm is chopped off!"); break;
          case AttackType::STAB: you(MsgType::ARE, "stabbed in the arm!"); break;
          case AttackType::CRUSH: you(MsgType::YOUR, "arm is smashed!"); break;
          case AttackType::HIT: you(MsgType::ARE, "hit in the arm!"); break;
          case AttackType::PUNCH: you(MsgType::YOUR, "arm is broken!"); break;
          default: FAIL << "Unhandled attack type " << int(type);
        }
        break;
    case BodyPart::WING:
        switch (type) {
          case AttackType::SHOOT: you(MsgType::YOUR, "shot in the wing!"); break;
          case AttackType::BITE: you(MsgType::YOUR, "wing is bitten off!"); break;
          case AttackType::CUT: you(MsgType::YOUR, "wing is chopped off!"); break;
          case AttackType::STAB: you(MsgType::ARE, "stabbed in the wing!"); break;
          case AttackType::CRUSH: you(MsgType::YOUR, "wing is smashed!"); break;
          case AttackType::HIT: you(MsgType::ARE, "hit in the wing!"); break;
          case AttackType::PUNCH: you(MsgType::YOUR, "wing is broken!"); break;
          default: FAIL << "Unhandled attack type " << int(type);
        }
        break;
    case BodyPart::LEG:
        switch (type) {
          case AttackType::SHOOT: you(MsgType::YOUR, "shot in the leg!"); break;
          case AttackType::BITE: you(MsgType::YOUR, "leg is bitten off!"); break;
          case AttackType::CUT: you(MsgType::YOUR, "leg is cut off!"); break;
          case AttackType::STAB: you(MsgType::YOUR, "stabbed in the leg!"); break;
          case AttackType::CRUSH: you(MsgType::YOUR, "knee is crushed!"); break;
          case AttackType::HIT: you(MsgType::ARE, "hit in the leg!"); break;
          case AttackType::PUNCH: you(MsgType::YOUR, "leg is broken!"); break;
          default: FAIL << "Unhandled attack type " << int(type);
        }
        break;
  }
}

vector<const Creature*> Creature::getUnknownAttacker() const {
  return filter(unknownAttacker, [](const Creature* c) { return !c->isDead();});
}

string Creature::getNameAndTitle() const {
  if (firstName)
    return *firstName + " the " + getName().bare();
  else if (speciesName)
    return getName().bare() + " the " + *speciesName;
  else
    return getName().the();
}

void Creature::updateVision() {
  if (hasSkill(Skill::get(SkillId::NIGHT_VISION)))
    vision = VisionId::NIGHT;
  else if (hasSkill(Skill::get(SkillId::ELF_VISION)) || isAffected(LastingEffect::FLYING))
    vision = VisionId::ELF;
  else
    vision = VisionId::NORMAL; 
}

VisionId Creature::getVision() const {
  return vision;
}

vector<Creature::SkillInfo> Creature::getSkillNames() const {
  vector<SkillInfo> ret;
  for (auto skill : getDiscreteSkills())
    ret.push_back({Skill::get(skill)->getName(), Skill::get(skill)->getHelpText()});
  for (SkillId id : ENUM_ALL(SkillId))
    if (!Skill::get(id)->isDiscrete() && getSkillValue(Skill::get(id)) > 0)
      ret.push_back({Skill::get(id)->getNameForCreature(this), Skill::get(id)->getHelpText()});
  return ret;
}

const MinionTaskMap& Creature::getMinionTasks() const {
  return minionTasks;
}

void Creature::updateVisibleCreatures(Rectangle range) {
  visibleEnemies.clear();
  for (const Creature* c : getLevel()->getAllCreatures(range)) 
    if (canSee(c) &&  isEnemy(c))
        visibleEnemies.push_back(c);
  for (const Creature* c : getUnknownAttacker())
    if (!contains(visibleEnemies, c))
      visibleEnemies.push_back(c);
}

vector<const Creature*> Creature::getVisibleEnemies() const {
  return visibleEnemies;
}

string Creature::getRemainingString(LastingEffect effect) const {
  return "[" + toString<int>(getTimeRemaining(effect)) + "]";
}

vector<string> Creature::getMainAdjectives() const {
  vector<string> ret;
  if (isBlind())
    ret.push_back("blind");
  if (isAffected(LastingEffect::INVISIBLE))
    ret.push_back("invisible");
  if (numBodyParts(BodyPart::ARM) == 1)
    ret.push_back("one-armed");
  if (numBodyParts(BodyPart::ARM) == 0)
    ret.push_back("armless");
  if (numBodyParts(BodyPart::LEG) == 1)
    ret.push_back("one-legged");
  if (numBodyParts(BodyPart::LEG) == 0)
    ret.push_back("legless");
  if (isAffected(LastingEffect::HALLU))
    ret.push_back("tripped");
  return ret;
}

vector<string> Creature::getAdjectives() const {
  vector<string> ret;
  if (const Item* weapon = getWeapon())
    ret.push_back("Wielding " + weapon->getAName());
  else
    ret.push_back("No weapon");
  append(ret, getMainAdjectives());
  for (BodyPart part : ENUM_ALL(BodyPart))
    if (int num = injuredBodyParts[part])
      ret.push_back(getPlural("injured " + getBodyPartName(part), num));
  for (BodyPart part : ENUM_ALL(BodyPart))
    if (int num = lostBodyParts[part])
      ret.push_back(getPlural("lost " + getBodyPartName(part), num));
  for (LastingEffect effect : ENUM_ALL(LastingEffect))
    if (isAffected(effect)) {
      bool addCount = true;
      switch (effect) {
        case LastingEffect::POISON: ret.push_back("poisoned"); break;
        case LastingEffect::SLEEP: ret.push_back("sleeping"); break;
        case LastingEffect::ENTANGLED: ret.push_back("entangled"); break;
        case LastingEffect::INVISIBLE: ret.push_back("invisible"); break;
        case LastingEffect::PANIC: ret.push_back("panic"); break;
        case LastingEffect::RAGE: ret.push_back("enraged"); break;
        case LastingEffect::HALLU: ret.push_back("hallucinating"); break;
        case LastingEffect::STR_BONUS: ret.push_back("strength bonus"); break;
        case LastingEffect::DEX_BONUS: ret.push_back("dexterity bonus"); break;
        case LastingEffect::SPEED: ret.push_back("speed bonus"); break;
        case LastingEffect::SLOWED: ret.push_back("slowed"); break;
        case LastingEffect::POISON_RESISTANT: ret.push_back("poison resistant"); break;
        case LastingEffect::FIRE_RESISTANT: ret.push_back("fire resistant"); break;
        case LastingEffect::FLYING: ret.push_back("flying"); break;
        case LastingEffect::INSANITY: ret.push_back("insane"); break;
        case LastingEffect::MAGIC_SHIELD: ret.push_back("magic shield"); break;
        default: addCount = false; break;
      }
      if (addCount && !isAffectedPermanently(effect))
        ret.back() += "  " + getRemainingString(effect);
    }
  if (isBlind())
    ret.push_back("blind"
        + (isAffected(LastingEffect::BLIND) ? (" " + getRemainingString(LastingEffect::BLIND)) : ""));
  return ret;
}

bool Creature::isSameSector(Vec2 pos) const {
  return level->areConnected(position, pos, getMovementType());
}

