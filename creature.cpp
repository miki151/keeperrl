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

template <class Archive> 
void SpellInfo::serialize(Archive& ar, const unsigned int version) {
  ar& BOOST_SERIALIZATION_NVP(id)
    & BOOST_SERIALIZATION_NVP(name)
    & BOOST_SERIALIZATION_NVP(type)
    & BOOST_SERIALIZATION_NVP(ready)
    & BOOST_SERIALIZATION_NVP(difficulty);
}

SERIALIZABLE(SpellInfo);


template <class Archive> 
void Creature::serialize(Archive& ar, const unsigned int version) { 
  ar
    & SUBCLASS(CreatureAttributes)
    & SUBCLASS(CreatureView)
    & SUBCLASS(UniqueEntity)
    & SUBCLASS(EventListener)
    & SVAR(viewObject)
    & SVAR(level)
    & SVAR(position)
    & SVAR(time)
    & SVAR(equipment)
    & SVAR(shortestPath)
    & SVAR(knownHiding)
    & SVAR(tribe)
    & SVAR(enemyChecks)
    & SVAR(health)
    & SVAR(dead)
    & SVAR(lastTick)
    & SVAR(collapsed)
    & SVAR(injuredBodyParts)
    & SVAR(lostBodyParts)
    & SVAR(hidden)
    & SVAR(lastAttacker)
    & SVAR(swapPositionCooldown)
    & SVAR(lastingEffects)
    & SVAR(expLevel)
    & SVAR(unknownAttacker)
    & SVAR(privateEnemies)
    & SVAR(holding)
    & SVAR(controller)
    & SVAR(controllerStack)
    & SVAR(creatureVisions)
    & SVAR(kills)
    & SVAR(difficultyPoints)
    & SVAR(points)
    & SVAR(sectors)
    & SVAR(numAttacksThisTurn);
  CHECK_SERIAL;
}

SERIALIZABLE(Creature);

PCreature Creature::defaultCreature;
PCreature Creature::defaultFlyer;
PCreature Creature::defaultMinion;

void Creature::initialize() {
  defaultCreature.reset();
  defaultFlyer.reset();
  defaultMinion.reset();
}

Creature* Creature::getDefault() {
  if (!defaultCreature)
    defaultCreature = CreatureFactory::fromId(CreatureId::GNOME, Tribe::get(TribeId::MONSTER),
        MonsterAIFactory::idle());
  return defaultCreature.get();
}

Creature* Creature::getDefaultMinion() {
  if (!defaultMinion)
    defaultMinion = CreatureFactory::fromId(CreatureId::GNOME, Tribe::get(TribeId::KEEPER),
        MonsterAIFactory::idle());
  return defaultMinion.get();
}

Creature* Creature::getDefaultMinionFlyer() {
  if (!defaultFlyer)
    defaultFlyer = CreatureFactory::fromId(CreatureId::RAVEN, Tribe::get(TribeId::KEEPER),
        MonsterAIFactory::idle());
  return defaultFlyer.get();
}

Creature::Creature(ViewObject object, Tribe* t, const CreatureAttributes& attr, ControllerFactory f)
    : CreatureAttributes(attr), viewObject(object), tribe(t), controller(f.get(this)) {
  tribe->addMember(this);
  for (SkillId skill : skills)
    Skill::get(skill)->onTeach(this);
}

Creature::Creature(Tribe* t, const CreatureAttributes& attr, ControllerFactory f)
    : Creature(ViewObject(*attr.viewId, ViewLayer::CREATURE, *attr.name), t, attr, f) {}

Creature::~Creature() {
  tribe->removeMember(this);
}

ViewIndex Creature::getViewIndex(Vec2 pos) const {
  return level->getSquare(pos)->getViewIndex(this);
}

Creature::Action::Action(function<void()> f) : action(f) {
}

Creature::Action::Action(const string& msg)
    : action(nullptr), failedMessage(msg) {
}

#ifndef RELEASE
static bool usageCheck = false;

Creature::Action::~Action() {
  CHECK(wasUsed || !usageCheck);
}

void Creature::Action::checkUsage(bool b) {
  usageCheck = b;
}

Creature::Action::Action(const Action& a) : action(a.action), before(a.before), after(a.after),
  failedMessage(a.failedMessage), wasUsed(true) {
  a.wasUsed = true;
}
#endif

void Creature::Action::perform() {
  if (before)
    before();
  CHECK(action);
  action();
  if (after)
    after();
#ifndef RELEASE
  wasUsed = true;
#endif
}

Creature::Action Creature::Action::prepend(function<void()> f) {
  if (!before)
    before = f;
  else
    before = [=] { f(); before(); };
  return *this;
}

Creature::Action Creature::Action::append(function<void()> f) {
  if (!after)
    after = f;
  else
    after = [=] { after(); f(); };
  return *this;
}

string Creature::Action::getFailedReason() const {
  return failedMessage;
}

Creature::Action::operator bool() const {
#ifndef RELEASE
  wasUsed = true;
#endif
  return action != nullptr;
}

SpellInfo Creature::getSpell(SpellId id) {
  switch (id) {
    case SpellId::HEALING: return {id, "healing", EffectType::HEAL, 0, 30};
    case SpellId::SUMMON_INSECTS: return {id, "summon insects", EffectType::SUMMON_INSECTS, 0, 30};
    case SpellId::DECEPTION: return {id, "deception", EffectType::DECEPTION, 0, 60};
    case SpellId::SPEED_SELF: return {id, "haste self", EffectType::SPEED, 0, 60};
    case SpellId::STR_BONUS: return {id, "strength", EffectType::STR_BONUS, 0, 90};
    case SpellId::DEX_BONUS: return {id, "dexterity", EffectType::DEX_BONUS, 0, 90};
    case SpellId::FIRE_SPHERE_PET: return {id, "fire sphere", EffectType::FIRE_SPHERE_PET, 0, 20};
    case SpellId::TELEPORT: return {id, "escape", EffectType::TELEPORT, 0, 120};
    case SpellId::INVISIBILITY: return {id, "invisibility", EffectType::INVISIBLE, 0, 300};
    case SpellId::WORD_OF_POWER: return {id, "word of power", EffectType::WORD_OF_POWER, 0, 300};
    case SpellId::SUMMON_SPIRIT: return {id, "summon spirits", EffectType::SUMMON_SPIRIT, 0, 300};
    case SpellId::PORTAL: return {id, "portal", EffectType::PORTAL, 0, 200};
  }
  FAIL << "wpeofk";
  return getSpell(SpellId::HEALING);
}

bool Creature::isFireResistant() const {
  return fireCreature || isAffected(LastingEffect::FIRE_RESISTANT);
}

void Creature::addSpell(SpellId id) {
  for (SpellInfo& info : spells)
    if (info.id == id)
      return;
  spells.push_back(getSpell(id));
}

const vector<SpellInfo>& Creature::getSpells() const {
  return spells;
}

Creature::Action Creature::castSpell(int index) {
  CHECK(index >= 0 && index < spells.size());
  if (spells[index].ready > getTime())
    return Action("You can't cast this spell yet.");
  return Action([=] () {
    monsterMessage(getTheName() + " casts a spell");
    playerMessage("You cast " + spells[index].name);
    Effect::applyToCreature(this, spells[index].type, EffectStrength::NORMAL);
    Statistics::add(StatId::SPELL_CAST);
    spells[index].ready = getTime() + spells[index].difficulty;
    spendTime(1);
  });
}

void Creature::addCreatureVision(CreatureVision* creatureVision) {
  creatureVisions.push_back(creatureVision);
}

void Creature::removeCreatureVision(CreatureVision* vision) {
  removeElement(creatureVisions, vision);
}

void Creature::pushController(PController ctrl) {
  if (ctrl->isPlayer())
    viewObject.setModifier(ViewObject::PLAYER);
  controllerStack.push_back(std::move(controller));
  controller = std::move(ctrl);
  if (controller->isPlayer())
    level->setPlayer(this);
}

void Creature::popController() {
  if (controller->isPlayer())
    viewObject.removeModifier(ViewObject::PLAYER);
  CHECK(!controllerStack.empty());
  bool wasPlayer = controller->isPlayer();
  controller = std::move(controllerStack.back());
  controllerStack.pop_back();
  if (wasPlayer && !controller->isPlayer())
    level->setPlayer(nullptr);
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

Creature::Action Creature::move(Vec2 direction) {
  if (holding)
    return Action("You can't break free!");
  if ((direction.length8() != 1 || !level->canMoveCreature(this, direction)) && !swapPosition(direction))
    return Action("");
  return Action([=]() {
    stationary = false;
    Debug() << getTheName() << " moving " << direction;
    if (isAffected(ENTANGLED)) {
      playerMessage("You can't break free!");
      spendTime(1);
      return;
    }
    if (level->canMoveCreature(this, direction))
      level->moveCreature(this, direction);
    else
      swapPosition(direction).perform();
    if (collapsed) {
      you(MsgType::CRAWL, getConstSquare()->getName());
      spendTime(3);
    } else
      spendTime(1);
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

void Creature::playerMessage(const string& message) const {
  controller->privateMessage(message);
}

void Creature::grantIdentify(int numItems) {
  controller->grantIdentify(numItems);
}

Controller* Creature::getController() {
  return controller.get();
}

const MapMemory& Creature::getMemory() const {
  return controller->getMemory();
}

Creature::Action Creature::swapPosition(Vec2 direction, bool force) {
  Creature* c = getSquare(direction)->getCreature();
  if (!c)
    return Action("");
  if (c->isAffected(SLEEP) && !force)
    return Action(c->getTheName() + " is sleeping.");
  if ((swapPositionCooldown && !isPlayer()) || c->stationary || c->invincible ||
      direction.length8() != 1 || (c->isPlayer() && !force) || (c->isEnemy(this) && !force) ||
      !getConstSquare(direction)->canEnterEmpty(this) || !getConstSquare()->canEnterEmpty(c))
    return Action("");
  return Action([=]() {
    swapPositionCooldown = 4;
    if (!force)
      getConstSquare(direction)->getCreature()->playerMessage("Excuse me!");
    playerMessage("Excuse me!");
    level->swapCreatures(this, getSquare(direction)->getCreature());
  });
}

void Creature::makeMove() {
  numAttacksThisTurn = 0;
  CHECK(!isDead());
  if (holding && holding->isDead())
    holding = nullptr;
  if (isAffected(SLEEP)) {
    controller->sleeping();
    spendTime(1);
    return;
  }
  if (isAffected(STUNNED)) {
    spendTime(1);
    return;
  }
  updateVisibleCreatures();
  if (swapPositionCooldown)
    --swapPositionCooldown;
  MEASURE(controller->makeMove(), "creature move time");
  CHECK(!inEquipChain) << "Someone forgot to finishEquipChain()";
  if (!hidden)
    viewObject.removeModifier(ViewObject::HIDDEN);
  unknownAttacker.clear();
  if (fireCreature && Random.roll(5))
    getSquare()->setOnFire(1);
  if (level->getSunlight(position) > 0.99 )
    shineLight();
}

Square* Creature::getSquare() {
  return level->getSquare(position);
}

Square* Creature::getSquare(Vec2 direction) {
  return level->getSquare(position + direction);
}

const Square* Creature::getConstSquare() const {
  return getLevel()->getSquare(position);
}

const Square* Creature::getConstSquare(Vec2 direction) const {
  return getLevel()->getSquare(position + direction);
}

Creature::Action Creature::wait() {
  return Action([=]() {
    Debug() << getTheName() << " waiting";
    bool keepHiding = hidden;
    spendTime(1);
    hidden = keepHiding;
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
    if (item->getType() == ItemType::AMMO)
      return item;
  return nullptr;
}

const Level* Creature::getLevel() const {
  return level;
}

Level* Creature::getLevel() {
  return level;
}

Vec2 Creature::getPosition() const {
  return position;
}

void Creature::globalMessage(const string& playerCanSee, const string& cant) const {
  level->globalMessage(position, playerCanSee, cant);
}

void Creature::monsterMessage(const string& playerCanSee, const string& cant) const {
  if (!isPlayer())
    level->globalMessage(position, playerCanSee, cant);
}

void Creature::addSkill(Skill* skill) {
  if (!skills[skill->getId()]) {
    skills.insert(skill->getId());
    skill->onTeach(this);
    playerMessage(skill->getHelpText());
  }
}

bool Creature::hasSkill(Skill* skill) const {
  return skills[skill->getId()];
}

bool Creature::hasSkillToUseWeapon(const Item* it) const {
  return !it->isWieldedTwoHanded() || hasSkill(Skill::get(SkillId::TWO_HANDED_WEAPON));
}

vector<Skill*> Creature::getSkills() const {
  vector<Skill*> ret;
  for (SkillId id : skills)
    ret.push_back(Skill::get(id));
  return ret;
}

vector<Item*> Creature::getPickUpOptions() const {
  if (!isHumanoid())
    return vector<Item*>();
  else
    return level->getSquare(getPosition())->getItems();
}

string Creature::getPluralName(Item* item, int num) {
  if (num == 1)
    return item->getTheName(false, isBlind());
  else
    return convertToString(num) + " " + item->getTheName(true, isBlind());
}


Creature::Action Creature::pickUp(const vector<Item*>& items, bool spendT) {
  if (!isHumanoid())
    return Action("You can't pick up anything!");
  double weight = getInventoryWeight();
  for (Item* it : items)
    weight += it->getWeight();
  if (weight > 2 * getAttr(AttrType::INV_LIMIT))
    return Action("You are carrying too much to pick this up.");
  return Action([=]() {
    Debug() << getTheName() << " pickup ";
    if (spendT)
      for (auto elem : Item::stackItems(items)) {
        monsterMessage(getTheName() + " picks up " + elem.first);
        playerMessage("You pick up " + elem.first);
      }
    for (auto item : items) {
      equipment.addItem(level->getSquare(getPosition())->removeItem(item));
    }
    if (getInventoryWeight() > getAttr(AttrType::INV_LIMIT))
      playerMessage("You are overloaded.");
    EventListener::addPickupEvent(this, items);
    if (spendT)
      spendTime(1);
  });
}

Creature::Action Creature::drop(const vector<Item*>& items) {
  if (!isHumanoid())
    return Action("You can't drop this item!");
  return Action([=]() {
    Debug() << getTheName() << " drop";
    for (auto elem : Item::stackItems(items)) {
      monsterMessage(getTheName() + " drops " + elem.first);
      playerMessage("You drop " + elem.first);
    }
    for (auto item : items) {
      level->getSquare(getPosition())->dropItem(equipment.removeItem(item));
    }
    EventListener::addDropEvent(this, items);
    spendTime(1);
  });
}

void Creature::drop(vector<PItem> items) {
  getSquare()->dropItems(std::move(items));
}

void Creature::startEquipChain() {
  inEquipChain = true;
}

void Creature::finishEquipChain() {
  inEquipChain = false;
  if (numEquipActions > 0)
    spendTime(1);
  numEquipActions = 0;
}

bool Creature::canEquipIfEmptySlot(const Item* item, string* reason) const {
  if (!isHumanoid()) {
    if (reason)
      *reason = "You can't equip items!";
    return false;
  }
  if (numGood(BodyPart::ARM) == 0) {
    if (reason)
      *reason = "You have no healthy arms!";
    return false;
  }
  if (!hasSkill(Skill::get(SkillId::TWO_HANDED_WEAPON)) && item->isWieldedTwoHanded()) {
    if (reason)
      *reason = "You don't have the skill to use two-handed weapons.";
    return false;
  }
  if (!hasSkill(Skill::get(SkillId::ARCHERY)) && item->getType() == ItemType::RANGED_WEAPON) {
    if (reason)
      *reason = "You don't have the skill to shoot a bow.";
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
  return !equipment.getItem(item->getEquipmentSlot()) && canEquipIfEmptySlot(item, nullptr);
}

Creature::Action Creature::equip(Item* item) {
  string reason;
  if (!canEquipIfEmptySlot(item, &reason))
    return Action(reason);
  if (equipment.getItem(item->getEquipmentSlot()))
    return Action("This slot is already equiped.");
  return Action([=]() {
    Debug() << getTheName() << " equip " << item->getName();
    EquipmentSlot slot = item->getEquipmentSlot();
    equipment.equip(item, slot);
    item->onEquip(this);
    EventListener::addEquipEvent(this, item);
    if (!inEquipChain)
      spendTime(1);
    else
      ++numEquipActions;
  });
}

Creature::Action Creature::unequip(Item* item) {
  if (!equipment.isEquiped(item))
    return Action("This item is not equiped.");
  if (!isHumanoid())
    return Action("You can't remove this item!");
  if (numGood(BodyPart::ARM) == 0)
    return Action("You have no healthy arms!");
  return Action([=]() {
    Debug() << getTheName() << " unequip";
    EquipmentSlot slot = item->getEquipmentSlot();
    CHECK(equipment.getItem(slot) == item) << "Item not equiped.";
    equipment.unequip(slot);
    item->onUnequip(this);
    if (!inEquipChain)
      spendTime(1);
    else
      ++numEquipActions;
  });
}

Creature::Action Creature::heal(Vec2 direction) {
  Creature* other = level->getSquare(position + direction)->getCreature();
  if (!healer || !other || other->getHealth() >= 0.9999)
    return Action("");
  return Action([=]() {
    Creature* other = level->getSquare(position + direction)->getCreature();
    other->playerMessage("\"Let me help you my friend.\"");
    other->you(MsgType::ARE, "healed by " + getTheName());
    other->heal();
    spendTime(1);
  });
}

Creature::Action Creature::bumpInto(Vec2 direction) {
  if (Creature* other = level->getSquare(getPosition() + direction)->getCreature())
    return Action([=]() {
      other->controller->onBump(this);
      spendTime(1);
    });
  else
    return Action("");
}

Creature::Action Creature::applySquare() {
  if (getSquare()->getApplyType(this))
    return Action([=]() {
      Debug() << getTheName() << " applying " << getSquare()->getName();;
      getSquare()->onApply(this);
      spendTime(1);
    });
  else
    return Action("");
}

Creature::Action Creature::hide() {
  if (!skills[SkillId::AMBUSH])
    return Action("You don't have this skill.");
  if (!getConstSquare()->canHide())
    return Action("You can't hide here.");
  return Action([=]() {
    playerMessage("You hide behind the " + getConstSquare()->getName());
    knownHiding.clear();
    viewObject.setModifier(ViewObject::HIDDEN);
    for (const Creature* c : getLevel()->getAllCreatures())
      if (c->canSee(this) && c->isEnemy(this)) {
        knownHiding.insert(c);
        if (!isBlind())
          you(MsgType::CAN_SEE_HIDING, c->getTheName());
      }
    spendTime(1);
    hidden = true;
  });
}

Creature::Action Creature::chatTo(Vec2 direction) {
  if (Creature* c = getSquare(direction)->getCreature())
    return Action([=]() {
      playerMessage("You chat with " + c->getTheName());
      c->onChat(this);
      spendTime(1);
    });
  else
    return Action("");
}

void Creature::onChat(Creature* from) {
  if (isEnemy(from) && chatReactionHostile) {
    if (chatReactionHostile->front() == '\"')
      from->playerMessage(*chatReactionHostile);
    else
      from->playerMessage(getTheName() + " " + *chatReactionHostile);
  }
  if (!isEnemy(from) && chatReactionFriendly) {
    if (chatReactionFriendly->front() == '\"')
      from->playerMessage(*chatReactionFriendly);
    else
      from->playerMessage(getTheName() + " " + *chatReactionFriendly);
  }
}

void Creature::learnLocation(const Location* loc) {
  controller->learnLocation(loc);
}

Creature::Action Creature::stealFrom(Vec2 direction, const vector<Item*>& items) {
  return Action([=]() {
    Creature* c = NOTNULL(getSquare(direction)->getCreature());
    equipment.addItems(c->steal(items));
  });
}

bool Creature::isHidden() const {
  return hidden;
}

bool Creature::knowsHiding(const Creature* c) const {
  return knownHiding.count(c) == 1;
}

bool Creature::affects(LastingEffect effect) const {
  switch (effect) {
    case RAGE:
    case PANIC: return !isAffected(SLEEP);
    case POISON: return !isAffected(LastingEffect::POISON_RESISTANT) && !isNotLiving();
    case ENTANGLED: return !uncorporal;
    default: return true;
  }
}

void Creature::onAffected(LastingEffect effect, bool msg) {
  switch (effect) {
    case STUNNED:
      if (msg) you(MsgType::ARE, "stunned");
    case PANIC:
      removeEffect(RAGE, false);
      if (msg) you(MsgType::PANIC, "");
      break;
    case RAGE:
      removeEffect(PANIC, false);
      if (msg) you(MsgType::RAGE, "");
      break;
    case HALLU: 
      if (!isBlind() && msg)
        playerMessage("The world explodes into colors!");
      break;
    case BLIND:
      if (msg) you(MsgType::ARE, "blind!");
      viewObject.setModifier(ViewObject::BLIND);
      break;
    case INVISIBLE:
      if (!isBlind() && msg)
        you(MsgType::TURN_INVISIBLE, "");
      viewObject.setModifier(ViewObject::INVISIBLE);
      break;
    case POISON:
      if (msg) you(MsgType::ARE, "poisoned");
      viewObject.setModifier(ViewObject::POISONED);
      break;
    case STR_BONUS: if (msg) you(MsgType::FEEL, "stronger"); break;
    case DEX_BONUS: if (msg) you(MsgType::FEEL, "more agile"); break;
    case SPEED: 
      if (msg) you(MsgType::ARE, "moving faster");
      removeEffect(SLOWED, false);
      break;
    case SLOWED: 
      if (msg) you(MsgType::ARE, "moving more slowly");
      removeEffect(SPEED, false);
      break;
    case ENTANGLED: if (msg) you(MsgType::ARE, "entangled in a web"); break;
    case SLEEP: if (msg) you(MsgType::FALL_ASLEEP, ""); break;
    case POISON_RESISTANT: if (msg) you(MsgType::ARE, "now poison resistant"); break;
    case FIRE_RESISTANT: if (msg) you(MsgType::ARE, "now fire resistant"); break;
    END_CASE(LastingEffect);
  }
}

void Creature::onRemoved(LastingEffect effect, bool msg) {
  switch (effect) {
    case POISON:
      if (msg)
        you(MsgType::ARE, "cured from poisoning");
      viewObject.removeModifier(ViewObject::POISONED);
      break;
    default: onTimedOut(effect, msg); break;
  }
}

void Creature::onTimedOut(LastingEffect effect, bool msg) {
  switch (effect) {
    case STUNNED: break;
    case SLOWED: if (msg) you(MsgType::ARE, "moving faster again"); break;
    case SLEEP: if (msg) you(MsgType::WAKE_UP, ""); break;
    case SPEED: if (msg) you(MsgType::ARE, "moving more slowly again"); break;
    case STR_BONUS: if (msg) you(MsgType::ARE, "weaker again"); break;
    case DEX_BONUS: if (msg) you(MsgType::ARE, "less agile again"); break;
    case PANIC:
    case RAGE:
    case HALLU: if (msg) playerMessage("Your mind is clear again"); break;
    case ENTANGLED: if (msg) you(MsgType::BREAK_FREE, "the web"); break;
    case BLIND:
      if (msg) 
        you("can see again");
      viewObject.removeModifier(ViewObject::BLIND);
      break;
    case INVISIBLE:
      if (msg)
        you(MsgType::TURN_VISIBLE, "");
      viewObject.removeModifier(ViewObject::INVISIBLE);
      break;
    case POISON:
      if (msg)
        you(MsgType::ARE, "no longer poisoned");
      viewObject.removeModifier(ViewObject::POISONED);
      break;
    case POISON_RESISTANT: if (msg) you(MsgType::ARE, "no longer resistant"); break;
    case FIRE_RESISTANT: if (msg) you(MsgType::ARE, "no longer resistant"); break;
    END_CASE(LastingEffect);
  } 
}

void Creature::addEffect(LastingEffect effect, double time, bool msg) {
  if (lastingEffects[effect] < getTime() + time && affects(effect)) {
    lastingEffects[effect] = getTime() + time;
    onAffected(effect, msg);
  }
}

void Creature::removeEffect(LastingEffect effect, bool msg) {
  lastingEffects[effect] = 0;
  onRemoved(effect, msg);
}

bool Creature::isAffected(LastingEffect effect) const {
  return lastingEffects[effect] >= getTime();
}

bool Creature::isBlind() const {
  return isAffected(BLIND) || (numLost(BodyPart::HEAD) > 0 && numBodyParts(BodyPart::HEAD) == 0);
}

int Creature::getAttrVal(AttrType type) const {
  switch (type) {
    case AttrType::SPEED: return *speed + getExpLevel() * 3;
    case AttrType::DEXTERITY: return *dexterity + (double(getExpLevel()) * attributeGain);
    case AttrType::STRENGTH: return *strength + (double(getExpLevel() - 1) * attributeGain);
    default: return 0;
  }
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
  int def = getAttrVal(type);
  for (Item* item : equipment.getItems())
    if (equipment.isEquiped(item))
      def += item->getModifier(type);
  switch (type) {
    case AttrType::STRENGTH:
        def += tribe->getHandicap();
        if (health < 1)
          def *= 0.666 + health / 3;
        if (isAffected(SLEEP))
          def *= 0.66;
        if (isAffected(STR_BONUS))
          def += attrBonus;
        for (auto elem : strPenalty)
          def -= elem.second * (numInjured(elem.first) + numLost(elem.first));
        def -= simulAttackPen(numAttacksThisTurn);
        break;
    case AttrType::DEXTERITY:
        def += tribe->getHandicap();
        if (health < 1)
          def *= 0.666 + health / 3;
        if (isAffected(SLEEP))
          def = 0;
        if (isAffected(DEX_BONUS))
          def += attrBonus;
        for (auto elem : dexPenalty)
          def -= elem.second * (numInjured(elem.first) + numLost(elem.first));
        def -= simulAttackPen(numAttacksThisTurn);
        break;
    case AttrType::THROWN_DAMAGE: 
        def += getAttr(AttrType::DEXTERITY);
        if (isAffected(PANIC))
          def -= attrBonus;
        if (isAffected(RAGE))
          def += attrBonus;
        break;
    case AttrType::DAMAGE: 
        def += getAttr(AttrType::STRENGTH);
        if (!getWeapon())
          def += barehandedDamage;
        if (isAffected(PANIC))
          def -= attrBonus;
        if (isAffected(RAGE))
          def += attrBonus;
        break;
    case AttrType::DEFENSE: 
        def += getAttr(AttrType::STRENGTH);
        if (isAffected(PANIC))
          def += attrBonus;
        if (isAffected(RAGE))
          def -= attrBonus;
        break;
    case AttrType::THROWN_TO_HIT: 
    case AttrType::TO_HIT: 
        def += getAttr(AttrType::DEXTERITY);
        break;
    case AttrType::SPEED: {
        double totWeight = getInventoryWeight();
        if (!carryAnything && totWeight > getAttr(AttrType::STRENGTH))
          def -= 20.0 * totWeight / def;
        if (isAffected(SLOWED))
          def /= 2;
        if (isAffected(SPEED))
          def *= 2;
        break;}
    case AttrType::INV_LIMIT:
        if (carryAnything)
          return 1000000;
        return getAttr(AttrType::STRENGTH) * 2;
 //   default:
 //       break;
  }
  return max(0, def);
}

int Creature::getPoints() const {
  return points;
}

void Creature::onKillEvent(const Creature* victim, const Creature* killer) {
  if (killer == this)
    points += victim->getDifficultyPoints();
}

double Creature::getInventoryWeight() const {
  double ret = 0;
  for (Item* item : getEquipment().getItems())
    ret += item->getWeight();
  return ret;
}

Tribe* Creature::getTribe() const {
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
  for (Item* item : equipment.getItems([](Item* it) { return it->getType() == ItemType::GOLD; })) {
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
  getDifficultyPoints();
  for (Item* item : equipment.getItems()) {
    item->tick(time, level, position);
    if (item->isDiscarded())
      equipment.removeItem(item);
  }
  for (LastingEffect effect : ENUM_ALL(LastingEffect))
    if (lastingEffects[effect] > 0 && lastingEffects[effect] < realTime) {
      lastingEffects[effect] = 0;
      onTimedOut(effect, true);
    }
  else if (isAffected(POISON)) {
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
    you(MsgType::DIE_OF, isAffected(POISON) ? "poisoning" : "bleeding");
    die(lastAttacker);
  }

}

BodyPart Creature::armOrWing() const {
  if (numGood(BodyPart::ARM) == 0)
    return BodyPart::WING;
  if (numGood(BodyPart::WING) == 0)
    return BodyPart::ARM;
  return chooseRandom({ BodyPart::WING, BodyPart::ARM }, {1, 1});
}

BodyPart Creature::getBodyPart(AttackLevel attack) const {
  if (flyer)
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

static string getAttackParam(AttackType type) {
  switch (type) {
    case AttackType::CUT: return "cut";
    case AttackType::STAB: return "stab";
    case AttackType::CRUSH: return "crush";
    case AttackType::PUNCH: return "punch";
    case AttackType::BITE: return "bite";
    case AttackType::HIT: return "hit";
    case AttackType::SHOOT: return "shot";
    case AttackType::SPELL: return "spell";
  }
  return "";
}

static string getBodyPartName(BodyPart part) {
  switch (part) {
    case BodyPart::LEG: return "leg";
    case BodyPart::ARM: return "arm";
    case BodyPart::WING: return "wing";
    case BodyPart::HEAD: return "head";
    case BodyPart::TORSO: return "torso";
    case BodyPart::BACK: return "back";
    END_CASE(BodyPart);
  }
  FAIL <<"Wf";
  return "";
}

static string getBodyPartBone(BodyPart part) {
  switch (part) {
    case BodyPart::HEAD: return "skull";
    default: return "bone";
  }
  FAIL <<"Wf";
  return "";
}

void Creature::injureBodyPart(BodyPart part, bool drop) {
  if (bodyParts[part] == 0)
    return;
  if (drop) {
    if (contains({BodyPart::LEG, BodyPart::ARM, BodyPart::WING}, part))
      Statistics::add(StatId::CHOPPED_LIMB);
    else if (part == BodyPart::HEAD)
      Statistics::add(StatId::CHOPPED_HEAD);
    --bodyParts[part];
    ++lostBodyParts[part];
    if (injuredBodyParts[part] > bodyParts[part])
      --injuredBodyParts[part];
  }
  else if (injuredBodyParts[part] < bodyParts[part])
    ++injuredBodyParts[part];
  switch (part) {
    case BodyPart::LEG:
      if (!collapsed) {
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
      if (flyer) {
        you(MsgType::FALL, getSquare()->getName());
        flyer = false;
      }
      if ((numBodyParts(BodyPart::LEG) < 2 || numInjured(BodyPart::LEG) > 0) && !collapsed) {
        collapsed = true;
      }
      break;
    default: break;
  }
  if (drop)
    getSquare()->dropItem(ItemFactory::corpse(*name + " " + getBodyPartName(part),
        *name + " " + getBodyPartBone(part),
        *weight / 8, isFood ? ItemType::FOOD : ItemType::CORPSE));
}

static MsgType getAttackMsg(AttackType type, bool weapon, AttackLevel level) {
  if (weapon)
    return type == AttackType::STAB ? MsgType::THRUST_WEAPON : MsgType::SWING_WEAPON;
  switch (type) {
    case AttackType::BITE: return MsgType::BITE;
    case AttackType::PUNCH: return level == AttackLevel::LOW ? MsgType::KICK : MsgType::PUNCH;
    case AttackType::HIT: return MsgType::HIT;
    default: FAIL << "Unhandled barehanded attack: " << int(type);
  }
  return MsgType(0);
}

Creature::Action Creature::attack(const Creature* c1, Optional<AttackLevel> attackLevel1, bool spend) {
  Creature* c = const_cast<Creature*>(c1);
  if (c->getPosition().dist8(position) != 1)
    return Action("");
  if (attackLevel1 && !contains(getAttackLevels(), *attackLevel1))
    return Action("Invalid attack level.");
  return Action([=] () {
  Debug() << getTheName() << " attacking " << c->getName();
  int toHit =  getAttr(AttrType::TO_HIT);
  int damage = getAttr(AttrType::DAMAGE);
  int toHitVariance = 1 + toHit / 3;
  int damageVariance = 1 + damage / 3;
  auto rToHit = [=] () { return Random.getRandom(-toHitVariance, toHitVariance); };
  auto rDamage = [=] () { return Random.getRandom(-damageVariance, damageVariance); };
  toHit += rToHit() + rToHit();
  damage += rDamage() + rDamage();
  bool backstab = false;
  string enemyName = getLevel()->playerCanSee(c) ? c->getTheName() : "something";
  if (c->isPlayer())
    enemyName = "";
  if (!c->canSee(this) && canSee(c)) {
 //   if (getWeapon() && getWeapon()->getAttackType() == AttackType::STAB) {
      damage += 10;
      backstab = true;
 //   }
    you(MsgType::ATTACK_SURPRISE, enemyName);
  }
  AttackLevel attackLevel = attackLevel1 ? (*attackLevel1) : getRandomAttackLevel();
  Attack attack(this, attackLevel, getAttackType(), toHit, damage, backstab, attackEffect);
  if (!c->dodgeAttack(attack)) {
    if (getWeapon()) {
      you(getAttackMsg(attack.getType(), true, attack.getLevel()), getWeapon()->getName());
      if (!canSee(c))
        playerMessage("You hit something.");
    }
    else {
      you(getAttackMsg(attack.getType(), false, attack.getLevel()), enemyName);
    }
    c->takeDamage(attack);
    if (c->isDead())
      increaseExpLevel(c->getDifficultyPoints() / 200);
  }
  else
    you(MsgType::MISS_ATTACK, enemyName);
  if (spend)
    spendTime(1);
  });
}

bool Creature::dodgeAttack(const Attack& attack) {
  ++numAttacksThisTurn;
  Debug() << getTheName() << " dodging " << attack.getAttacker()->getName() << " to hit " << attack.getToHit() << " dodge " << getAttr(AttrType::TO_HIT);
  if (const Creature* c = attack.getAttacker()) {
    if (!canSee(c))
      unknownAttacker.push_back(c);
    EventListener::addAttackEvent(this, c);
    if (!contains(privateEnemies, c) && c->getTribe() != tribe)
      privateEnemies.push_back(c);
  }
  return canSee(attack.getAttacker()) && attack.getToHit() <= getAttr(AttrType::TO_HIT);
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
    || (part == BodyPart::HEAD && numGood(part) == 1 && !isUndead());
}

bool Creature::takeDamage(const Attack& attack) {
  if (isAffected(SLEEP))
    removeEffect(SLEEP);
  if (const Creature* c = attack.getAttacker())
    if (!contains(privateEnemies, c) && c->getTribe() != tribe)
      privateEnemies.push_back(c);
  int defense = getAttr(AttrType::DEFENSE);
  Debug() << getTheName() << " attacked by " << attack.getAttacker()->getName() << " damage " << attack.getStrength() << " defense " << defense;
  if (passiveAttack && attack.getAttacker() && attack.getAttacker()->getPosition().dist8(position) == 1) {
    Creature* other = const_cast<Creature*>(attack.getAttacker());
    Effect::applyToCreature(other, *passiveAttack, EffectStrength::NORMAL);
    other->lastAttacker = this;
  }
  if (attack.getStrength() > defense) {
    if (auto effect = attack.getEffect())
      Effect::applyToCreature(this, *effect, EffectStrength::NORMAL);
    lastAttacker = attack.getAttacker();
    double dam = (defense == 0) ? 1 : double(attack.getStrength() - defense) / defense;
    dam *= damageMultiplier;
    if (!isNotLiving())
      bleed(dam);
    if (!uncorporal) {
      if (attack.getType() != AttackType::SPELL) {
        BodyPart part = attack.inTheBack() ? BodyPart::BACK : getBodyPart(attack.getLevel());
        if (dam >= getMinDamage(part) && numGood(part) > 0) {
          youHit(part, attack.getType()); 
          injureBodyPart(part, attack.getType() == AttackType::CUT || attack.getType() == AttackType::BITE);
          if (isCritical(part)) {
            you(MsgType::DIE, "");
            die(attack.getAttacker());
            return true;
          }
          if (health <= 0)
            health = 0.01;
          return false;
        }
      }
    } else {
      you(MsgType::TURN, "wisp of smoke");
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
      else
        you(MsgType::ARE, "not hurt");
    }
  } else {
    you(MsgType::GET_HIT_NODAMAGE, getAttackParam(attack.getType()));
    if (attack.getEffect() && attack.getAttacker()->harmlessApply)
      Effect::applyToCreature(this, *attack.getEffect(), EffectStrength::NORMAL);
  }
  const Creature* c = attack.getAttacker();
  return false;
}

void Creature::updateViewObject() {
  viewObject.setDefense(getAttr(AttrType::DEFENSE));
  viewObject.setAttack(getAttr(AttrType::DAMAGE));
  viewObject.setLevel(getExpLevel());
  if (const Creature* c = getLevel()->getPlayer()) {
    if (isEnemy(c))
      viewObject.setEnemyStatus(ViewObject::HOSTILE);
    else
      viewObject.setEnemyStatus(ViewObject::FRIENDLY);
  }
  viewObject.setBleeding(1 - health);
}

double Creature::getHealth() const {
  return health;
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

static string adjectives(CreatureSize s, bool undead, bool notLiving, bool uncorporal) {
  vector<string> ret {sizeStr(s)};
  if (notLiving)
    ret.push_back("non-living");
  if (undead)
    ret.push_back("undead");
  if (uncorporal)
    ret.push_back("body-less");
  return combine(ret);
}

string Creature::bodyDescription() const {
  vector<string> ret;
  for (BodyPart part : {BodyPart::ARM, BodyPart::LEG, BodyPart::WING})
    ret.push_back(getPlural(getBodyPartName(part), numBodyParts(part)));
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
  /*if (Item* item = getEquipment().getItem(EquipmentSlot::WEAPON))
    weapon = " It's wielding " + item->getAName() + ".";
  else
  if (Item* item = getEquipment().getItem(EquipmentSlot::RANGED_WEAPON))
    weapon = " It's wielding " + item->getAName() + ".";*/
  string attack;
  if (attackEffect) {
    switch (*attackEffect) {
      case EffectType::POISON: attack = "poison"; break;
      case EffectType::FIRE: attack = "fire"; break;
      default: FAIL << "Unhandled monster attack " << int(*attackEffect);
    }
    attack = " It has a " + attack + " attack.";
  }
  return getTheName() + " is a " + adjectives(*size, undead, notLiving, uncorporal) +
      (isHumanoid() ? " humanoid creature" : " beast") + bodyDescription() + ". " +
     "It is " + attrStr(*strength > 16, *dexterity > 16, *speed > 100) + "." + weapon + attack;
}

void Creature::setSpeed(double value) {
  speed = value;
}

double Creature::getSpeed() const {
  return *speed;
}
  
CreatureSize Creature::getSize() const {
  return *size;
}

void Creature::heal(double amount, bool replaceLimbs) {
  Debug() << getTheName() << " heal";
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
              flyer = true;
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
  Debug() << getTheName() << " health " << health;
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
    if (Random.roll(10)) {
      you(MsgType::YOUR, "body crumbles to dust");
      die(nullptr);
    } else
      you(MsgType::ARE, "burnt by the sun");
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
  Debug() << (specialMonster ? "special monster " : "") + getTheName() << " takes " << item->getNameAndModifiers();*/
  if (item->isWieldedTwoHanded())
    addSkill(Skill::get(SkillId::TWO_HANDED_WEAPON));
  if (item->getType() == ItemType::RANGED_WEAPON)
    addSkill(Skill::get(SkillId::ARCHERY));
  Item* ref = item.get();
  equipment.addItem(std::move(item));
  if (auto action = equip(ref))
    action.perform();
}

void Creature::dropCorpse() {
  getSquare()->dropItem(ItemFactory::corpse(*name + " corpse", *name + " skeleton", *weight,
        isFood ? ItemType::FOOD : ItemType::CORPSE, {true, numBodyParts(BodyPart::HEAD) > 0, false}));
}

void Creature::die(const Creature* attacker, bool dropInventory, bool dCorpse) {
  Debug() << getTheName() << " dies. Killed by " << (attacker ? attacker->getName() : "");
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
  EventListener::addKillEvent(this, attacker);
  if (innocent)
    Statistics::add(StatId::INNOCENT_KILLED);
  Statistics::add(StatId::DEATH);
}

Creature::Action Creature::flyAway() {
  if (!canFly() || level->getCoverInfo(position).covered)
    return Action("");
  return Action([=]() {
    Debug() << getTheName() << " fly away";
    monsterMessage(getTheName() + " flies away.");
    dead = true;
    level->killCreature(this);
  });
}

Creature::Action Creature::torture(Creature* c) {
  if (c->getSquare()->getApplyType(this) != SquareApplyType::TORTURE
      || c->getPosition().dist8(getPosition()) != 1)
    return Action("");
  return Action([=]() {
    monsterMessage(getTheName() + " tortures " + c->getTheName());
    playerMessage("You torture " + c->getTheName());
    if (Random.roll(4))
      c->monsterMessage(c->getTheName() + " screams!", "You hear a terrifying scream");
    c->addEffect(STUNNED, 3, false);
    c->bleed(0.1);
    if (c->health < 0.3) {
      if (!Random.roll(15))
        c->heal();
      else
      c->bleed(1);
    }
    EventListener::addTortureEvent(c, this);
    spendTime(1);
  });
}

void Creature::give(const Creature* whom, vector<Item*> items) {
  getLevel()->getSquare(whom->getPosition())->getCreature()->takeItems(equipment.removeItems(items), this);
}

Creature::Action Creature::fire(Vec2 direction) {
  CHECK(direction.length8() == 1);
  if (!getEquipment().getItem(EquipmentSlot::RANGED_WEAPON))
    return Action("You need a ranged weapon.");
  if (!hasSkill(Skill::get(SkillId::ARCHERY)))
    return Action("You don't have this skill.");
  if (numGood(BodyPart::ARM) < 2)
    return Action("You need two hands to shoot a bow.");
  if (!getAmmo())
    return Action("Out of ammunition");
  return Action([=]() {
    PItem ammo = equipment.removeItem(NOTNULL(getAmmo()));
    RangedWeapon* weapon = NOTNULL(dynamic_cast<RangedWeapon*>(getEquipment().getItem(EquipmentSlot::RANGED_WEAPON)));
    weapon->fire(this, level, std::move(ammo), direction);
    spendTime(1);
  });
}

Creature::Action Creature::construct(Vec2 direction, SquareType type) {
  if (getConstSquare(direction)->canConstruct(type) && canConstruct(type))
    return Action([=]() {
      getSquare(direction)->construct(type);
      spendTime(1);
    });
  else
    return Action("");
}

bool Creature::canConstruct(SquareType type) const {
  return hasSkill(Skill::get(SkillId::CONSTRUCTION));
}

Creature::Action Creature::eat(Item* item) {
  return Action([=]() {
    getSquare()->removeItem(item);
    spendTime(3);
  });
}

Creature::Action Creature::destroy(Vec2 direction, DestroyAction dAction) {
  if (direction.length8() == 1 && getConstSquare(direction)->canDestroy(this))
    return Action([=]() {
      switch (dAction) {
        case BASH: 
          playerMessage("You bash the " + getSquare(direction)->getName());
          monsterMessage(getTheName() + " bashes the " + getSquare(direction)->getName(), "You hear a bang");
          break;
        case EAT: 
          playerMessage("You eat the " + getSquare(direction)->getName());
          monsterMessage(getTheName() + " eats the " + getSquare(direction)->getName(), "You hear chewing");
          break;
        case DESTROY: 
          playerMessage("You destroy the " + getSquare(direction)->getName());
          monsterMessage(getTheName() + " destroys the " + getSquare(direction)->getName(), "You hear a crash");
          break;
      }
      getSquare(direction)->destroy(this);
      spendTime(1);
    });
  else
    return Action("");
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
  return equipment.getItem(EquipmentSlot::WEAPON);
}

AttackType Creature::getAttackType() const {
  if (getWeapon())
    return getWeapon()->getAttackType();
  else if (barehandedAttack)
    return *barehandedAttack;
  else
    return isHumanoid() ? AttackType::PUNCH : AttackType::BITE;
}

Creature::Action Creature::applyItem(Item* item) {
  if (!contains({ItemType::TOOL, ItemType::POTION, ItemType::FOOD, ItemType::BOOK, ItemType::SCROLL},
      item->getType()) || !isHumanoid())
    return Action("You can't apply this item");
  if (numGood(BodyPart::ARM) == 0)
    return Action("You have no healthy arms!");
  return Action([=] () {
      double time = item->getApplyTime();
      playerMessage("You " + item->getApplyMsgFirstPerson());
      monsterMessage(getTheName() + " " + item->getApplyMsgThirdPerson(), item->getNoSeeApplyMsg());
      item->apply(this, level);
      if (item->isDiscarded()) {
        equipment.removeItem(item);
      }
      spendTime(time);
  });
}

Creature::Action Creature::throwItem(Item* item, Vec2 direction) {
  if (!numGood(BodyPart::ARM) || !isHumanoid())
    return Action("You can't throw anything!");
  else if (item->getWeight() > 20)
    return Action(item->getTheName() + " is too heavy!");
  int dist = 0;
  int toHitVariance = 10;
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
  int toHit = Random.getRandom(-toHitVariance, toHitVariance) +
      getAttr(AttrType::THROWN_TO_HIT) + item->getModifier(AttrType::THROWN_TO_HIT);
  int damage = Random.getRandom(-attackVariance, attackVariance) +
      getAttr(AttrType::THROWN_DAMAGE) + item->getModifier(AttrType::THROWN_DAMAGE);
  if (hasSkill(Skill::get(SkillId::KNIFE_THROWING)) && item->getAttackType() == AttackType::STAB) {
    damage += 7;
    toHit += 4;
  }
  Attack attack(this, getRandomAttackLevel(), item->getAttackType(), toHit, damage, false, Nothing());
  return Action([=]() {
    playerMessage("You throw " + item->getAName(false, isBlind()));
    monsterMessage(getTheName() + " throws " + item->getAName());
    level->throwItem(equipment.removeItem(item), attack, dist, getPosition(), direction, getVision());
    spendTime(1);
  });
}

const ViewObject& Creature::getViewObject() const {
  return viewObject;
}

bool Creature::canSee(const Creature* c) const {
  if (c->getLevel() != level)
    return false;
  for (CreatureVision* v : creatureVisions)
    if (v->canSee(this, c))
      return true;
  return !isBlind() && !c->isAffected(INVISIBLE) &&
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
  
string Creature::getTheName() const {
  if (islower((*name)[0]))
    return "the " + *name;
  return *name;
}
 
string Creature::getAName() const {
  if (islower((*name)[0]))
    return "a " + *name;
  return *name;
}

Optional<string> Creature::getFirstName() const {
  return firstName;
}

string Creature::getName() const {
  return *name;
}

string Creature::getSpeciesName() const {
  if (speciesName)
    return *speciesName;
  else
    return *name;
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

bool Creature::hasBrain() const {
  return brain;
}

bool Creature::canSwim() const {
  return skills[SkillId::SWIMMING];
}

bool Creature::canFly() const {
  return flyer;
}

bool Creature::canWalk() const {
  return walker;
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
  return courage;
}

Gender Creature::getGender() const {
  return gender;
}

void Creature::increaseExpLevel(double amount) {
  expLevel = min<double>(maxLevel, amount + expLevel);
  if (skillGain.count(getExpLevel()) && isHumanoid()) {
    you(MsgType::ARE, "more experienced");
    addSkill(skillGain.at(getExpLevel()));
    skillGain.erase(getExpLevel());
  }
}

int Creature::getExpLevel() const {
  return expLevel;
}

int Creature::getDifficultyPoints() const {
  difficultyPoints = max<double>(difficultyPoints,
      getAttr(AttrType::DEFENSE) + getAttr(AttrType::TO_HIT) + getAttr(AttrType::DAMAGE)
      + getAttr(AttrType::SPEED) / 10);
  return difficultyPoints;
}

void Creature::addSectors(Sectors* s) {
  CHECK(!sectors);
  sectors = s;
}

Creature::Action Creature::continueMoving() {
  if (shortestPath && shortestPath->isReachable(getPosition())) {
    Vec2 pos2 = shortestPath->getNextMove(getPosition());
    return move(pos2 - getPosition());
  }
  return Action("");
}

Creature::Action Creature::moveTowards(Vec2 pos, bool stepOnTile) {
  return moveTowards(pos, false, stepOnTile);
}

Creature::Action Creature::moveTowards(Vec2 pos, bool away, bool stepOnTile) {
  if (stepOnTile && !level->getSquare(pos)->canEnterEmpty(this))
    return Action("");
  if (!away && sectors) {
    bool sectorOk = false;
    for (Vec2 v : pos.neighbors8())
      if (v.inRectangle(level->getBounds()) && sectors->same(getPosition(), v)) {
        sectorOk = true;
        break;
      }
    if (!sectorOk)
      return Action("");
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
    return Action("");
  Debug() << "Reconstructing shortest path.";
  if (!away)
    shortestPath = ShortestPath(getLevel(), this, pos, getPosition());
  else
    shortestPath = ShortestPath(getLevel(), this, pos, getPosition(), -1.5);
  if (shortestPath->isReachable(getPosition())) {
    Vec2 pos2 = shortestPath->getNextMove(getPosition());
    return move(pos2 - getPosition());
  } else {
    Debug() << "Cannot move toward " << pos;
    return Action("");
  }
}

Creature::Action Creature::moveAway(Vec2 pos, bool pathfinding) {
  if ((pos - getPosition()).length8() <= 5 && pathfinding)
    if (auto action = moveTowards(pos, true, false))
      return action;
  pair<Vec2, Vec2> dirs = (getPosition() - pos).approxL1();
  vector<Action> moves;
  if (auto action = move(dirs.first))
    moves.push_back(action);
  if (auto action = move(dirs.second))
    moves.push_back(action);
  if (moves.size() > 0)
    return moves[Random.getRandom(moves.size())];
  return Action("");
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
    END_CASE(BodyPart);
  }
}

vector<const Creature*> Creature::getUnknownAttacker() const {
  return unknownAttacker;
}

string Creature::getNameAndTitle() const {
  if (firstName)
    return *firstName + " the " + getName();
  else
    return getTheName();
}

Vision* Creature::getVision() const {
  if (hasSkill(Skill::get(SkillId::ELF_VISION)))
    return Vision::get(VisionId::ELF);
  if (hasSkill(Skill::get(SkillId::NIGHT_VISION)))
    return Vision::get(VisionId::NIGHT);
  else
    return Vision::get(VisionId::NORMAL); 
}

string Creature::getRemainingString(LastingEffect effect) const {
  return "[" + convertToString<int>(lastingEffects[effect] - time) + "]";
}

vector<string> Creature::getMainAdjectives() const {
  vector<string> ret;
  if (isBlind())
    ret.push_back("blind");
  if (isAffected(INVISIBLE))
    ret.push_back("invisible");
  if (numBodyParts(BodyPart::ARM) == 1)
    ret.push_back("one-armed");
  if (numBodyParts(BodyPart::ARM) == 0)
    ret.push_back("armless");
  if (numBodyParts(BodyPart::LEG) == 1)
    ret.push_back("one-legged");
  if (numBodyParts(BodyPart::LEG) == 0)
    ret.push_back("legless");
  if (isAffected(HALLU))
    ret.push_back("tripped");
  return ret;
}

vector<string> Creature::getAdjectives() const {
  vector<string> ret;
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
        case POISON: ret.push_back("poisoned"); break;
        case SLEEP: ret.push_back("sleeping"); break;
        case ENTANGLED: ret.push_back("entangled"); break;
        case INVISIBLE: ret.push_back("invisible"); break;
        case PANIC: ret.push_back("panic"); break;
        case RAGE: ret.push_back("enraged"); break;
        case HALLU: ret.push_back("hallucinating"); break;
        case STR_BONUS: ret.push_back("strength bonus"); break;
        case DEX_BONUS: ret.push_back("dexterity bonus"); break;
        case SPEED: ret.push_back("speed bonus"); break;
        case SLOWED: ret.push_back("slowed"); break;
        default: addCount = false; break;
      }
      if (addCount)
        ret.back() += "  " + getRemainingString(effect);
    }
  if (isBlind())
    ret.push_back("blind" + isAffected(BLIND) ? (" " + getRemainingString(BLIND)) : "");
  return ret;
}

void Creature::refreshGameInfo(View::GameInfo& gameInfo) const {
  gameInfo.infoType = View::GameInfo::InfoType::PLAYER;
  Model::SunlightInfo sunlightInfo = level->getModel()->getSunlightInfo();
  gameInfo.sunlightInfo.description = sunlightInfo.getText();
  gameInfo.sunlightInfo.timeRemaining = sunlightInfo.timeRemaining;
  View::GameInfo::PlayerInfo& info = gameInfo.playerInfo;
  if (firstName) {
    info.playerName = *firstName;
    info.title = *name;
  } else {
    info.playerName = "";
    info.title = *name;
  }
  info.possessed = !controllerStack.empty();
  info.spellcaster = !spells.empty();
  info.adjectives = getMainAdjectives();
  Item* weapon = getEquipment().getItem(EquipmentSlot::WEAPON);
  info.weaponName = weapon ? weapon->getName() : "";
  const Location* location = getLevel()->getLocation(getPosition());
  info.levelName = location && location->hasName() 
    ? capitalFirst(location->getName()) : getLevel()->getName();
  info.speed = getAttr(AttrType::SPEED);
  info.speedBonus = isAffected(SPEED) ? 1 : isAffected(SLOWED) ? -1 : 0;
  info.defense = getAttr(AttrType::DEFENSE);
  info.defBonus = isAffected(RAGE) ? -1 : isAffected(PANIC) ? 1 : 0;
  info.attack = getAttr(AttrType::DAMAGE);
  info.attBonus = isAffected(RAGE) ? 1 : isAffected(PANIC) ? -1 : 0;
  info.strength = getAttr(AttrType::STRENGTH);
  info.strBonus = isAffected(STR_BONUS);
  info.dexterity = getAttr(AttrType::DEXTERITY);
  info.dexBonus = isAffected(DEX_BONUS);
  info.time = getTime();
  info.numGold = getGold(100000000).size();
  info.elfStanding = Tribe::get(TribeId::ELVEN)->getStanding(this);
  info.dwarfStanding = Tribe::get(TribeId::DWARVEN)->getStanding(this);
  info.goblinStanding = Tribe::get(TribeId::GOBLIN)->getStanding(this);
  info.effects.clear();
  for (string s : getAdjectives())
    info.effects.push_back({s, true});
}

