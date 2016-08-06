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
#include "ranged_weapon.h"
#include "statistics.h"
#include "options.h"
#include "game.h"
#include "effect.h"
#include "item_factory.h"
#include "location.h"
#include "controller.h"
#include "player_message.h"
#include "attack.h"
#include "vision.h"
#include "square_type.h"
#include "square_apply_type.h"
#include "equipment.h"
#include "shortest_path.h"
#include "spell_map.h"
#include "minion_task_map.h"
#include "tribe.h"
#include "creature_attributes.h"
#include "position.h"
#include "view.h"
#include "sound.h"
#include "trigger.h"
#include "lasting_effect.h"
#include "attack_type.h"
#include "attack_level.h"
#include "model.h"
#include "view_object.h"
#include "spell.h"
#include "body.h"
#include "field_of_view.h"

template <class Archive> 
void Creature::MoraleOverride::serialize(Archive& ar, const unsigned int version) {
}

SERIALIZABLE(Creature::MoraleOverride);

template <class Archive> 
void Creature::serialize(Archive& ar, const unsigned int version) { 
  ar & SUBCLASS(Renderable) & SUBCLASS(UniqueEntity);
  serializeAll(ar, attributes, position, equipment, shortestPath, knownHiding, tribe, morale);
  serializeAll(ar, deathTime, hidden, lastAttacker, deathReason, swapPositionCooldown);
  serializeAll(ar, unknownAttackers, privateEnemies, holding, controller, controllerStack, creatureVisions, kills);
  serializeAll(ar, difficultyPoints, points, numAttacksThisTurn, moraleOverride);
  serializeAll(ar, vision, personalEvents, lastCombatTime);
}

SERIALIZABLE(Creature);

SERIALIZATION_CONSTRUCTOR_IMPL(Creature);

Creature::Creature(const ViewObject& object, TribeId t, const CreatureAttributes& attr,
    const ControllerFactory& f)
    : Renderable(object), attributes(attr), tribe(t), controller(f.get(this)) {
  modViewObject().setCreatureId(getUniqueId());
  updateVision();    
}

Creature::Creature(TribeId t, const CreatureAttributes& attr, const ControllerFactory& f)
    : Creature(attr.createViewObject(), t, attr, f) {
}

Creature::~Creature() {
}

vector<vector<Creature*>> Creature::stack(const vector<Creature*>& creatures) {
  map<string, vector<Creature*>> stacks;
  for (Creature* c : creatures)
    stacks[c->getName().stack()].push_back(c);
  return getValues(stacks);
}

const ViewObject& Creature::getViewObjectFor(const Tribe* observer) const {
  if (attributes->getIllusionViewObject() && observer->isEnemy(this))
    return *attributes->getIllusionViewObject();
  else
    return getViewObject();
}

const Body& Creature::getBody() const {
  return attributes->getBody();
}

Body& Creature::getBody() {
  return attributes->getBody();
}

double Creature::getSpellDelay(Spell* spell) const {
  CHECK(!isReady(spell));
  return attributes->getSpellMap().getReadyTime(spell) - getGlobalTime();
}

bool Creature::isReady(Spell* spell) const {
  return attributes->getSpellMap().getReadyTime(spell) < getGlobalTime();
}

static double getWillpowerMult(double sorcerySkill) {
  return 2 * pow(0.25, sorcerySkill); 
}

const CreatureAttributes& Creature::getAttributes() const {
  return *attributes;
}

CreatureAttributes& Creature::getAttributes() {
  return *attributes;
}

CreatureAction Creature::castSpell(Spell* spell) const {
  if (!attributes->getSpellMap().contains(spell))
    return CreatureAction("You don't know this spell.");
  CHECK(!spell->isDirected());
  if (!isReady(spell))
    return CreatureAction("You can't cast this spell yet.");
  return CreatureAction(this, [=] (Creature *c) {
    c->addSound(spell->getSound());
    spell->addMessage(c);
    Effect::applyToCreature(c, spell->getEffectType(), EffectStrength::NORMAL);
    getGame()->getStatistics().add(StatId::SPELL_CAST);
    c->attributes->getSpellMap().setReadyTime(spell, getGlobalTime() + spell->getDifficulty()
        * getWillpowerMult(attributes->getSkills().getValue(SkillId::SORCERY)));
    c->spendTime(1);
  });
}

CreatureAction Creature::castSpell(Spell* spell, Vec2 dir) const {
  CHECK(attributes->getSpellMap().contains(spell));
  CHECK(spell->isDirected());
  CHECK(dir.length8() == 1);
  if (!isReady(spell))
    return CreatureAction("You can't cast this spell yet.");
  return CreatureAction(this, [=] (Creature *c) {
    c->addSound(spell->getSound());
    monsterMessage(getName().the() + " casts a spell");
    playerMessage("You cast " + spell->getName());
    Effect::applyDirected(c, dir, spell->getDirEffectType(), EffectStrength::NORMAL);
    getGame()->getStatistics().add(StatId::SPELL_CAST);
    c->attributes->getSpellMap().setReadyTime(spell, getGlobalTime() + spell->getDifficulty()
        * getWillpowerMult(attributes->getSkills().getValue(SkillId::SORCERY)));
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
  if (ctrl->isPlayer()) {
    modViewObject().setModifier(ViewObject::Modifier::PLAYER);
    if (Game* g = getGame())
      g->setPlayer(this);
  }
  controller = std::move(ctrl);
}

void Creature::popController() {
  if (controller->isPlayer()) {
    modViewObject().removeModifier(ViewObject::Modifier::PLAYER);
    getGame()->cancelPlayer(this);
  }
  CHECK(!controllerStack.empty());
  controller = std::move(controllerStack.back());
  controllerStack.pop_back();
}

bool Creature::isDead() const {
  return !!deathTime;
}

double Creature::getDeathTime() const {
  return *deathTime;
}

const Creature* Creature::getLastAttacker() const {
  return lastAttacker;
}

void Creature::clearLastAttacker() {
  lastAttacker = nullptr;
}

optional<string> Creature::getDeathReason() const {
  if (deathReason)
    return deathReason;
  if (lastAttacker)
    return "killed by " + lastAttacker->getName().a();
  return none;
}

const EntitySet<Creature>& Creature::getKills() const {
  return kills;
}

void Creature::spendTime(double t) {
  if (!isDead())
    if (Model* m = position.getModel())
      m->increaseLocalTime(this, 100.0 * t / (double) getAttr(AttrType::SPEED));
  hidden = false;
}

CreatureAction Creature::forceMove(Vec2 dir) const {
  return forceMove(getPosition().plus(dir));
}

CreatureAction Creature::forceMove(Position pos) const {
  const_cast<Creature*>(this)->forceMovement = true;
  CreatureAction action = move(pos);
  const_cast<Creature*>(this)->forceMovement = false;
  if (action)
    return action.prepend([this] (Creature* c) { c->forceMovement = true; })
      .append([this] (Creature* c) { c->forceMovement = false; });
  else
    return action;
}

CreatureAction Creature::move(Vec2 dir) const {
  return move(getPosition().plus(dir));
}

CreatureAction Creature::move(Position pos) const {
  Vec2 direction = getPosition().getDir(pos);
  if (holding)
    return CreatureAction("You can't break free!");
  if (direction.length8() != 1)
    return CreatureAction();
  if (!position.canMoveCreature(direction)) {
    auto action = swapPosition(direction);
    if (!action) // this is so the player gets relevant info why the move failed
      return action;
  }
  return CreatureAction(this, [=](Creature* self) {
    Debug() << getName().the() << " moving " << direction;
    if (isAffected(LastingEffect::ENTANGLED) || isAffected(LastingEffect::TIED_UP)) {
      playerMessage("You can't break free!");
      self->spendTime(1);
      return;
    }
    if (position.canMoveCreature(direction))
      self->position.moveCreature(direction);
    else
      swapPosition(direction).perform(self);
    self->attributes->setStationary(false);
    double oldTime = getLocalTime();
    if (getBody().isCollapsed(this)) {
      you(MsgType::CRAWL, getPosition().getName());
      self->spendTime(3);
    } else
      self->spendTime(1);
    self->addMovementInfo({direction, oldTime, getLocalTime(), MovementInfo::MOVE});
  });
}

void Creature::displace(double time, Vec2 dir) {
  position.moveCreature(dir);
  controller->onDisplaced();
  addMovementInfo({dir, time, time + 1, MovementInfo::MOVE});
}

int Creature::getDebt(const Creature* debtor) const {
  return controller->getDebt(debtor);
}

bool Creature::canTakeItems(const vector<Item*>& items) const {
  return getBody().isHumanoid() && canCarry(items);
}

void Creature::takeItems(vector<PItem> items, const Creature* from) {
  vector<Item*> ref = extractRefs(items);
  equipment->addItems(std::move(items));
  controller->onItemsGiven(ref, from);
}

void Creature::you(MsgType type, const vector<string>& param) const {
  controller->you(type, param);
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

bool Creature::hasFreeMovement() const {
  return !isAffected(LastingEffect::SLEEP) &&
    !isAffected(LastingEffect::STUNNED) &&
    !isAffected(LastingEffect::ENTANGLED) &&
    !isAffected(LastingEffect::TIED_UP);
}

CreatureAction Creature::swapPosition(Vec2 direction, bool force) const {
  if (Creature* other = getPosition().plus(direction).getCreature())
    return swapPosition(other, force);
  else
    return CreatureAction();
}

CreatureAction Creature::swapPosition(Creature* other, bool force) const {
  Vec2 direction = position.getDir(other->getPosition());
  CHECK(direction.length8() == 1);
  if (!other->hasFreeMovement() && !force)
    return CreatureAction(other->getName().the() + " cannot move.");
  if ((swapPositionCooldown && !isPlayer()) || other->attributes->isStationary() || 
      other->getAttributes().isInvincible() ||
      (other->isPlayer() && !force) || (other->isEnemy(this) && !force) ||
      !other->getPosition().canEnterEmpty(this) || !getPosition().canEnterEmpty(other))
    return CreatureAction();
  return CreatureAction(this, [=](Creature* self) {
    self->swapPositionCooldown = 4;
    if (!force)
      other->playerMessage("Excuse me!");
    playerMessage("Excuse me!");
    self->position.swapCreatures(other);
    other->addMovementInfo({-direction, getLocalTime(), other->getLocalTime(),
        MovementInfo::MOVE});
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
  updateVisibleCreatures();
  updateViewObject();
  if (swapPositionCooldown)
    --swapPositionCooldown;
  MEASURE(controller->makeMove(), "creature move time");
  Debug() << getName().bare() << " morale " << getMorale();
  if (!hidden)
    modViewObject().removeModifier(ViewObject::Modifier::HIDDEN);
  unknownAttackers.clear();
  getBody().affectPosition(position);
}

CreatureAction Creature::wait() const {
  return CreatureAction(this, [=](Creature* self) {
    Debug() << getName().the() << " waiting";
    bool keepHiding = hidden;
    self->spendTime(1);
    self->hidden = keepHiding;
  });
}

const Equipment& Creature::getEquipment() const {
  return *equipment;
}

Equipment& Creature::getEquipment() {
  return *equipment;
}

vector<PItem> Creature::steal(const vector<Item*> items) {
  return equipment->removeItems(items);
}

Item* Creature::getAmmo() const {
  for (Item* item : equipment->getItems())
    if (item->getClass() == ItemClass::AMMO)
      return item;
  return nullptr;
}

Level* Creature::getLevel() const {
  return getPosition().getLevel();
}

Game* Creature::getGame() const {
  return getPosition().getGame();
}

Position Creature::getPosition() const {
  return position;
}

void Creature::globalMessage(const PlayerMessage& playerCanSee) const {
  globalMessage(playerCanSee, "");
}

void Creature::globalMessage(const PlayerMessage& playerCanSee, const PlayerMessage& cant) const {
  position.globalMessage(this, playerCanSee, cant);
}

void Creature::monsterMessage(const PlayerMessage& playerCanSee, const PlayerMessage& cant) const {
  if (!isPlayer())
    position.globalMessage(this, playerCanSee, cant);
}

void Creature::monsterMessage(const PlayerMessage& playerCanSee) const {
  monsterMessage(playerCanSee, "");
}

void Creature::addSkill(Skill* skill) {
  if (!attributes->getSkills().hasDiscrete(skill->getId())) {
    attributes->getSkills().insert(skill->getId());
    playerMessage(skill->getHelpText());
  }
}

vector<Item*> Creature::getPickUpOptions() const {
  if (!getBody().isHumanoid())
    return vector<Item*>();
  else
    return getPosition().getItems();
}

string Creature::getPluralTheName(Item* item, int num) const {
  if (num == 1)
    return item->getTheName(false, this);
  else
    return toString(num) + " " + item->getTheName(true, this);
}

string Creature::getPluralAName(Item* item, int num) const {
  if (num == 1)
    return item->getAName(false, this);
  else
    return toString(num) + " " + item->getAName(true, this);
}

bool Creature::canCarry(const vector<Item*>& items) const {
  double weight = getInventoryWeight();
  for (Item* it : items)
    weight += it->getWeight();
  return weight <= 2 * getModifier(ModifierType::INV_LIMIT);
}

CreatureAction Creature::pickUp(const vector<Item*>& items) const {
  if (!getBody().isHumanoid())
    return CreatureAction("You can't pick up anything!");
  if (!canCarry(items))
    return CreatureAction("You are carrying too much to pick this up.");
  return CreatureAction(this, [=](Creature* self) {
    Debug() << getName().the() << " pickup ";
    for (auto stack : stackItems(items)) {
      monsterMessage(getName().the() + " picks up " + getPluralAName(stack[0], stack.size()));
      playerMessage("You pick up " + getPluralTheName(stack[0], stack.size()));
    }
    self->equipment->addItems(self->getPosition().removeItems(items));
    if (getInventoryWeight() > getModifier(ModifierType::INV_LIMIT))
      playerMessage("You are overloaded.");
    getGame()->addEvent({EventId::PICKED_UP, EventInfo::ItemsHandled{self, items}});
    self->spendTime(1);
  });
}

vector<vector<Item*>> Creature::stackItems(vector<Item*> items) const {
  map<string, vector<Item*> > stacks = groupBy<Item*, string>(items, 
      [this] (Item* const& item) { return item->getNameAndModifiers(false, this); });
  return getValues(stacks);
}

CreatureAction Creature::drop(const vector<Item*>& items) const {
  if (!getBody().isHumanoid())
    return CreatureAction("You can't drop this item!");
  return CreatureAction(this, [=](Creature* self) {
    Debug() << getName().the() << " drop";
    for (auto stack : stackItems(items)) {
      monsterMessage(getName().the() + " drops " + getPluralAName(stack[0], stack.size()));
      playerMessage("You drop " + getPluralTheName(stack[0], stack.size()));
    }
    for (auto item : items)
      self->getPosition().dropItem(self->equipment->removeItem(item));
    getGame()->addEvent({EventId::DROPPED, EventInfo::ItemsHandled{self, items}});
    self->spendTime(1);
  });
}

void Creature::drop(vector<PItem> items) {
  getPosition().dropItems(std::move(items));
}

bool Creature::canEquipIfEmptySlot(const Item* item, string* reason) const {
  if (!getBody().isHumanoid()) {
    if (reason)
      *reason = "Only humanoids can equip items!";
    return false;
  }
  if (!attributes->canEquip()) {
    if (reason)
      *reason = "You can't equip items!";
    return false;
  }
  if (getBody().numGood(BodyPart::ARM) == 0) {
    if (reason)
      *reason = "You have no healthy arms!";
    return false;
  }
  if (getBody().numGood(BodyPart::ARM) == 1 && item->isWieldedTwoHanded()) {
    if (reason)
      *reason = "You need two hands to wield " + item->getAName() + "!";
    return false;
  }
  return item->canEquip();
}

bool Creature::canEquip(const Item* item) const {
  return canEquipIfEmptySlot(item, nullptr) && equipment->canEquip(item);
}

bool Creature::isEquipmentAppropriate(const Item* item) const {
  return item->getClass() != ItemClass::WEAPON || item->getMinStrength() <= getAttr(AttrType::STRENGTH);
}

CreatureAction Creature::equip(Item* item) const {
  string reason;
  if (!canEquipIfEmptySlot(item, &reason))
    return CreatureAction(reason);
  if (contains(equipment->getItem(item->getEquipmentSlot()), item))
    return CreatureAction();
  return CreatureAction(this, [=](Creature *self) {
    Debug() << getName().the() << " equip " << item->getName();
    EquipmentSlot slot = item->getEquipmentSlot();
    if (self->equipment->getItem(slot).size() >= self->equipment->getMaxItems(slot)) {
      Item* previousItem = self->equipment->getItem(slot)[0];
      self->equipment->unequip(previousItem);
      previousItem->onUnequip(self);
    }
    self->equipment->equip(item, slot);
    playerMessage("You equip " + item->getTheName(false, this));
    monsterMessage(getName().the() + " equips " + item->getAName());
    item->onEquip(self);
    if (Game* game = getGame())
      game->addEvent({EventId::EQUIPED, EventInfo::ItemsHandled{self, {item}}});
    self->spendTime(1);
  });
}

CreatureAction Creature::unequip(Item* item) const {
  if (!equipment->isEquipped(item))
    return CreatureAction("This item is not equipped.");
  if (!getBody().isHumanoid())
    return CreatureAction("You can't remove this item!");
  if (getBody().numGood(BodyPart::ARM) == 0)
    return CreatureAction("You have no healthy arms!");
  return CreatureAction(this, [=](Creature* self) {
    Debug() << getName().the() << " unequip";
    CHECK(equipment->isEquipped(item)) << "Item not equipped.";
    EquipmentSlot slot = item->getEquipmentSlot();
    self->equipment->unequip(item);
    playerMessage("You " + string(slot == EquipmentSlot::WEAPON ? " sheathe " : " remove ") +
        item->getTheName(false, this));
    monsterMessage(getName().the() + (slot == EquipmentSlot::WEAPON ? " sheathes " : " removes ") +
        item->getAName());
    item->onUnequip(self);
    self->spendTime(1);
  });
}

CreatureAction Creature::heal(Vec2 direction) const {
  const Creature* other = getPosition().plus(direction).getCreature();
  if (!attributes->getSkills().hasDiscrete(SkillId::HEALING) || !other || !other->getBody().canHeal() ||
      other == this)
    return CreatureAction();
  return CreatureAction(this, [=](Creature* self) {
    Creature* other = getPosition().plus(direction).getCreature();
    other->playerMessage("\"Let me help you my friend.\"");
    other->you(MsgType::ARE, "healed by " + getName().the());
    other->heal();
    self->spendTime(1);
  });
}

CreatureAction Creature::bumpInto(Vec2 direction) const {
  if (const Creature* other = getPosition().plus(direction).getCreature())
    return CreatureAction(this, [=](Creature* self) {
      other->controller->onBump(self);
      self->spendTime(1);
    });
  else
    return CreatureAction();
}

CreatureAction Creature::applySquare() const {
  if (getPosition().getApplyType(this))
    return CreatureAction(this, [=](Creature* self) {
      Debug() << getName().the() << " applying " << getPosition().getName();
      self->getPosition().apply(self);
      self->spendTime(self->getPosition().getApplyTime());
    });
  else
    return CreatureAction();
}

CreatureAction Creature::hide() const {
  if (!attributes->getSkills().hasDiscrete(SkillId::AMBUSH))
    return CreatureAction("You don't have this skill.");
  if (!getPosition().canHide())
    return CreatureAction("You can't hide here.");
  return CreatureAction(this, [=](Creature* self) {
    playerMessage("You hide behind the " + getPosition().getName());
    self->knownHiding.clear();
    self->modViewObject().setModifier(ViewObject::Modifier::HIDDEN);
    for (Creature* other : getLevel()->getAllCreatures())
      if (other->canSee(this) && other->isEnemy(this)) {
        self->knownHiding.insert(other);
        if (!isBlind())
          you(MsgType::CAN_SEE_HIDING, other->getName().the());
      }
    self->spendTime(1);
    self->hidden = true;
  });
}

CreatureAction Creature::chatTo(Creature* other) const {
  CHECK(other);
  return CreatureAction(this, [=](Creature* self) {
      playerMessage("You chat with " + other->getName().the());
      other->onChat(self);
      self->spendTime(1);
  });
  return CreatureAction();
}

void Creature::onChat(Creature* from) {
  attributes->chatReaction(this, from);
}

CreatureAction Creature::stealFrom(Vec2 direction, const vector<Item*>& items) const {
  if (getPosition().plus(direction).getCreature())
    return CreatureAction(this, [=](Creature *self) {
        Creature* other = NOTNULL(getPosition().plus(direction).getCreature());
        self->equipment->addItems(other->steal(items));
      });
  return CreatureAction();
}

bool Creature::isHidden() const {
  return hidden;
}

bool Creature::knowsHiding(const Creature* c) const {
  return knownHiding.contains(c);
}

void Creature::addEffect(LastingEffect effect, double time, bool msg) {
  if (LastingEffects::affects(this, effect) && attributes->considerAffecting(effect, getGlobalTime(), time))
    LastingEffects::onAffected(this, effect, msg);
}

void Creature::removeEffect(LastingEffect effect, bool msg) {
  if (!isAffected(effect))
    return;
  attributes->clearLastingEffect(effect);
  if (!isAffected(effect))
    LastingEffects::onRemoved(this, effect, msg);
}

void Creature::addPermanentEffect(LastingEffect effect, bool msg) {
  if (!isAffected(effect))
    LastingEffects::onAffected(this, effect, msg);
  attributes->addPermanentEffect(effect);
}

void Creature::removePermanentEffect(LastingEffect effect, bool msg) {
  attributes->removePermanentEffect(effect);
  if (!isAffected(effect))
    LastingEffects::onRemoved(this, effect, msg);
}

bool Creature::isAffected(LastingEffect effect) const {
  return attributes->isAffected(effect, getGlobalTime());
}

bool Creature::isBlind() const {
  return isAffected(LastingEffect::BLIND) ||
    (getBody().numLost(BodyPart::HEAD) > 0 && getBody().numBodyParts(BodyPart::HEAD) == 0);
}

bool Creature::isDarknessSource() const {
  return isAffected(LastingEffect::DARKNESS_SOURCE);
}

// penalty to strength and dexterity per extra attacker in a single turn
int simulAttackPen(int attackers) {
  return max(0, (attackers - 1) * 2);
}

int Creature::getAttr(AttrType type) const {
  int def = getBody().modifyAttr(type, attributes->getRawAttr(type));
  for (Item* item : equipment->getAllEquipped())
    def += CHECK_RANGE(item->getAttr(type), -10000000, 10000000, getName().bare());
  switch (type) {
    case AttrType::DEXTERITY:
    case AttrType::STRENGTH:
        def -= simulAttackPen(numAttacksThisTurn);
        break;
    case AttrType::SPEED: {
        double totWeight = getInventoryWeight();
        if (!attributes->canCarryAnything() && totWeight > getAttr(AttrType::STRENGTH))
          def -= 20.0 * totWeight / def;
        CHECK(def > 0);
        break;}
  }
  LastingEffects::modifyAttr(this, type, def);
  return max(0, def);
}

int Creature::accuracyBonus() const {
  if (Item* weapon = getWeapon())
    return -max(0, weapon->getMinStrength() - getAttr(AttrType::STRENGTH));
  else
    return 0;
}

int Creature::getModifier(ModifierType type) const {
  int def = 0;
  for (Item* item : equipment->getAllEquipped())
      def += CHECK_RANGE(item->getModifier(type), -10000000, 10000000, getName().bare());
  for (SkillId skill : ENUM_ALL(SkillId))
    def += CHECK_RANGE(Skill::get(skill)->getModifier(this, type), -10000000, 10000000, getName().bare());
  switch (type) {
    case ModifierType::FIRED_DAMAGE: 
    case ModifierType::THROWN_DAMAGE: 
        def += getAttr(AttrType::DEXTERITY);
        break;
    case ModifierType::DAMAGE: 
        def += getAttr(AttrType::STRENGTH);
        if (!getWeapon())
          def += attributes->getBarehandedDamage();
        break;
    case ModifierType::DEFENSE: 
        def += getAttr(AttrType::STRENGTH);
        break;
    case ModifierType::FIRED_ACCURACY: 
    case ModifierType::THROWN_ACCURACY: 
        def += getAttr(AttrType::DEXTERITY);
        break;
    case ModifierType::ACCURACY: 
        def += accuracyBonus();
        def += getAttr(AttrType::DEXTERITY);
        break;
    case ModifierType::INV_LIMIT:
        if (attributes->canCarryAnything())
          return 1000000;
        return getAttr(AttrType::STRENGTH) * 2;
  }
  LastingEffects::modifyMod(this, type, def);
  return max(0, def);
}

int Creature::getPoints() const {
  return points;
}

double maxLevelGain = 5.0;
double minLevelGain = 0.02;
double equalLevelGain = 0.2;
double maxLevelDiff = 30;

void Creature::onKilled(Creature* victim) {
  int difficulty = victim->getDifficultyPoints();
  CHECK(difficulty >=0 && difficulty < 100000) << difficulty << " " << victim->getName().bare();
  points += difficulty;
  double levelDiff = victim->attributes->getExpLevel() - attributes->getExpLevel();
  attributes->increaseExpLevel(max(minLevelGain, min(maxLevelGain, 
      (maxLevelGain - equalLevelGain) * levelDiff / maxLevelDiff + equalLevelGain)));
  kills.insert(victim);
}

double Creature::getInventoryWeight() const {
  double ret = 0;
  for (Item* item : getEquipment().getItems())
    ret += item->getWeight();
  return ret;
}

Tribe* Creature::getTribe() {
  return getGame()->getTribe(tribe);
}

const Tribe* Creature::getTribe() const {
  return getGame()->getTribe(tribe);
}

TribeId Creature::getTribeId() const {
  return tribe;
}

void Creature::setTribe(TribeId t) {
  tribe = t;
}

bool Creature::isFriend(const Creature* c) const {
  return !isEnemy(c);
}

bool Creature::isEnemy(const Creature* c) const {
  if (c == this)
    return false;
  if (isAffected(LastingEffect::INSANITY))
    return c != this;
  return getTribe()->isEnemy(c) || c->getTribe()->isEnemy(this) ||
    privateEnemies.contains(c) || c->privateEnemies.contains(this);
}

vector<Item*> Creature::getGold(int num) const {
  vector<Item*> ret;
  for (Item* item : equipment->getItems([](Item* it) { return it->getClass() == ItemClass::GOLD; })) {
    ret.push_back(item);
    if (ret.size() == num)
      return ret;
  }
  return ret;
}

void Creature::setPosition(Position pos) {
  if (!pos.isSameLevel(position)) {
    modViewObject().clearMovementInfo();
  }
  if (shortestPath && shortestPath->getLevel() != pos.getLevel())
    shortestPath.reset();
  position = pos;
}

double Creature::getLocalTime() const {
  if (Model* m = position.getModel())
    return m->getLocalTime(this);
  else
    return 0;
}

double Creature::getGlobalTime() const {
  if (Game* g = getGame())
    return g->getGlobalTime();
  else
    return 1;
}

void Creature::tick() {
  updateVision();
  if (Random.roll(5))
    getDifficultyPoints();
  for (Item* item : equipment->getItems()) {
    item->tick(position);
    if (item->isDiscarded())
      equipment->removeItem(item);
  }
  double globalTime = getGlobalTime();
  for (LastingEffect effect : ENUM_ALL(LastingEffect))
    if (attributes->considerTimeout(effect, globalTime))
      LastingEffects::onTimedOut(this, effect, true);
  if (isAffected(LastingEffect::POISON)) {
    getBody().affectByPoison(this);
    playerMessage("You feel poison flowing in your veins.");
  }
  updateViewObject();
  if (getBody().tick(this)) {
    die(lastAttacker);
    return;
  }
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

string Creature::getAttrName(AttrType attr) {
  switch (attr) {
    case AttrType::STRENGTH: return "strength";
    case AttrType::DEXTERITY: return "dexterity";
    case AttrType::SPEED: return "speed";
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

void Creature::dropWeapon() {
  Item* weapon = NOTNULL(getWeapon());
  you(MsgType::DROP_WEAPON, weapon->getName());
  getPosition().dropItem(equipment->removeItem(weapon));
}

CreatureAction Creature::attack(Creature* other, optional<AttackParams> attackParams, bool spend) const {
  CHECK(!other->isDead());
  if (!position.isSameLevel(other->getPosition()))
    return CreatureAction();
  Vec2 dir = getPosition().getDir(other->getPosition());
  if (dir.length8() != 1)
    return CreatureAction();
  return CreatureAction(this, [=] (Creature* c) {
  Debug() << getName().the() << " attacking " << other->getName().the();
  int accuracy = getModifier(ModifierType::ACCURACY);
  int damage = getModifier(ModifierType::DAMAGE);
  int accuracyVariance = 1 + accuracy / 3;
  int damageVariance = 1 + damage / 3;
  auto rAccuracy = [=] () { return Random.get(-accuracyVariance, accuracyVariance); };
  auto rDamage = [=] () { return Random.get(-damageVariance, damageVariance); };
  double timeSpent = 1;
  accuracy += rAccuracy() + rAccuracy();
  damage += rDamage() + rDamage();
  vector<string> attackAdjective;
  if (attackParams && attackParams->mod)
    switch (*attackParams->mod) {
      case AttackParams::WILD: 
        damage *= 1.2;
        accuracy *= 0.8;
        timeSpent *= 1.5;
        attackAdjective.push_back("wildly");
        break;
      case AttackParams::SWIFT: 
        damage *= 0.8;
        accuracy *= 1.2;
        timeSpent *= 0.7;
        attackAdjective.push_back("swiftly");
        break;
    }
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
  AttackLevel attackLevel = Random.choose(getBody().getAttackLevels());
  if (attackParams && attackParams->level)
    attackLevel = *attackParams->level;
  Attack attack(c, attackLevel, attributes->getAttackType(getWeapon()), accuracy, damage, backstab,
      getWeapon() ? getWeapon()->getAttackEffect() : attributes->getAttackEffect());
  if (!other->dodgeAttack(attack)) {
    if (getWeapon()) {
      you(getAttackMsg(attack.getType(), true, attack.getLevel()),
          concat({getWeapon()->getName()}, attackAdjective));
      if (!canSee(other))
        playerMessage("You hit something.");
    } else
      you(getAttackMsg(attack.getType(), false, attack.getLevel()), concat({enemyName}, attackAdjective));
    other->takeDamage(attack);
  } else {
    you(MsgType::MISS_ATTACK, enemyName);
    addSound(SoundId::MISSED_ATTACK);
  }
  double oldTime = getLocalTime();
  if (spend)
    c->spendTime(timeSpent);
  c->addMovementInfo({dir, oldTime, getLocalTime(), MovementInfo::ATTACK});
  });
}

bool Creature::dodgeAttack(const Attack& attack) {
  ++numAttacksThisTurn;
  Creature* attacker = attack.getAttacker();
  if (attacker) {
    if (!canSee(attacker))
      unknownAttackers.insert(attacker);
  }
  return (!attacker || canSee(attacker)) && attack.getAccuracy() <= getModifier(ModifierType::ACCURACY);
}

bool Creature::takeDamage(const Attack& attack) {
  AttackType attackType = attack.getType();
  int defense = getModifier(ModifierType::DEFENSE);
  if (Creature* attacker = attack.getAttacker()) {
    if (attacker->tribe != tribe || Random.roll(3))
      privateEnemies.insert(attacker);
    if (!attacker->getAttributes().getSkills().hasDiscrete(SkillId::STEALTH))
      for (Position p : visibleCreatures)
        if (p.dist8(position) < 10 && p.getCreature() && !p.getCreature()->isDead())
          p.getCreature()->removeEffect(LastingEffect::SLEEP);
    if (attackType == AttackType::POSSESS) {
      you(MsgType::ARE, "possessed by " + attacker->getName().the());
      attacker->die(nullptr, false, false);
      addEffect(LastingEffect::INSANITY, 10);
      return false;
    }
    Debug() << getName().the() << " attacked by " << attacker->getName().the()
      << " damage " << attack.getStrength() << " defense " << defense;
    lastAttacker = attack.getAttacker();
  }
  if (auto sound = attributes->getAttackSound(attack.getType(), attack.getStrength() > defense))
    addSound(*sound);
  if (attack.getStrength() > defense) {
    double dam = (defense == 0) ? 1 : double(attack.getStrength() - defense) / defense;
    if (attributes->getBody().takeDamage(attack, this, dam))
      return true;
  } else {
    you(MsgType::GET_HIT_NODAMAGE, getAttackParam(attackType));
    if (attack.getEffect())
      Effect::applyToCreature(this, *attack.getEffect(), EffectStrength::NORMAL);
  }
  for (LastingEffect effect : ENUM_ALL(LastingEffect))
    if (isAffected(effect))
      LastingEffects::onCreatureDamage(this, effect);
  return false;
}

static vector<string> extractNames(const vector<AdjectiveInfo>& adjectives) {
  return transform2<string>(adjectives, [] (const AdjectiveInfo& e) { return e.name; });
}

void Creature::updateViewObject() {
  modViewObject().setAttribute(ViewObject::Attribute::DEFENSE, getModifier(ModifierType::DEFENSE));
  modViewObject().setAttribute(ViewObject::Attribute::ATTACK, getModifier(ModifierType::DAMAGE));
  modViewObject().setAttribute(ViewObject::Attribute::LEVEL, attributes->getExpLevel());
  modViewObject().setAttribute(ViewObject::Attribute::MORALE, getMorale());
  modViewObject().setModifier(ViewObject::Modifier::DRAW_MORALE);
  modViewObject().setAdjectives(extractNames(concat(
          getWeaponAdjective(), getBadAdjectives(), getGoodAdjectives())));
  if (isAffected(LastingEffect::SLEEP))
    modViewObject().setModifier(ViewObject::Modifier::SLEEPING);
  else
    modViewObject().removeModifier(ViewObject::Modifier::SLEEPING);
  getBody().updateViewObject(modViewObject());
  modViewObject().setDescription(getName().bare());
  getPosition().setNeedsRenderUpdate(true);
}

double Creature::getMorale() const {
  if (moraleOverride)
    if (auto ret = moraleOverride->getMorale(this))
      return *ret;
  return morale;
}

void Creature::addMorale(double val) {
  morale = min(1.0, max(-1.0, morale + val));
}

void Creature::setMoraleOverride(PMoraleOverride mod) {
  moraleOverride = std::move(mod);
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

void Creature::heal(double amount, bool replaceLimbs) {
  if (getBody().heal(this, amount, replaceLimbs))
    clearLastAttacker();
  updateViewObject();
}

void Creature::setOnFire(double amount) {
  if (!isAffected(LastingEffect::FIRE_RESISTANT))
    getBody().setOnFire(this, amount);
}

void Creature::affectBySilver() {
  if (getBody().affectBySilver(this))
    die();
}

void Creature::affectByAcid() {
  if (getBody().affectByAcid(this))
    die();
}

void Creature::poisonWithGas(double amount) {
  if (isAffected(LastingEffect::POISON)) {
    getBody().affectByPoison(this);
    you(MsgType::ARE, "poisoned by the gas");
  }
}

void Creature::setHeld(const Creature* c) {
  holding = c;
}

bool Creature::isHeld() const {
  return holding != nullptr;
}

void Creature::take(vector<PItem> items) {
  for (PItem& elem : items)
    take(std::move(elem));
}

void Creature::take(PItem item) {
  Item* ref = item.get();
  equipment->addItem(std::move(item));
  if (auto action = equip(ref))
    action.perform(this);
}

void Creature::die(const string& reason, bool dropInventory, bool dCorpse) {
  deathReason = reason;
  die(nullptr, dropInventory, dCorpse);
}

void Creature::die(Creature* attacker, bool dropInventory, bool dCorpse) {
  CHECK(!isDead());
  deathTime = getGlobalTime();
  if (dCorpse)
    if (auto sound = getBody().getDeathSound())
      addSound(*sound);
  lastAttacker = attacker;
  Debug() << getName().the() << " dies. Killed by " << (attacker ? attacker->getName().bare() : "");
  controller->onKilled(attacker);
  if (dropInventory)
    for (PItem& item : equipment->removeAllItems())
      getPosition().dropItem(std::move(item));
  if (dropInventory && dCorpse)
    getPosition().dropItems(getBody().getCorpseItem(getName().bare(), getUniqueId()));
  getGame()->addEvent({EventId::KILLED, EventInfo::Attacked{this, attacker}});
  if (attacker)
    attacker->onKilled(this);
  getTribe()->onMemberKilled(this, attacker);
  getLevel()->killCreature(this);
  if (attributes->isInnocent())
    getGame()->getStatistics().add(StatId::INNOCENT_KILLED);
  getGame()->getStatistics().add(StatId::DEATH);
}

CreatureAction Creature::flyAway() const {
  if (!isAffected(LastingEffect::FLYING) || getPosition().isCovered())
    return CreatureAction();
  return CreatureAction(this, [=](Creature* self) {
    Debug() << getName().the() << " fly away";
    monsterMessage(getName().the() + " flies away.");
    self->die(nullptr, false, false);
  });
}

CreatureAction Creature::disappear() const {
  return CreatureAction(this, [=](Creature* self) {
    Debug() << getName().the() << " disappears";
    monsterMessage(getName().the() + " disappears.");
    self->die(nullptr, false, false);
  });
}

CreatureAction Creature::torture(Creature* other) const {
  if (other->getPosition().getApplyType(this) != SquareApplyType::TORTURE
      || other->getPosition().dist8(getPosition()) != 1)
    return CreatureAction();
  return CreatureAction(this, [=](Creature* self) {
    monsterMessage(getName().the() + " tortures " + other->getName().the());
    playerMessage("You torture " + other->getName().the());
    if (Random.roll(4))
      other->monsterMessage(other->getName().the() + " screams!", "You hear a horrible scream");
    other->addEffect(LastingEffect::STUNNED, 3, false);
    other->getBody().affectByTorture(self);
    if (!Random.roll(8))
      other->heal();
    else
      other->die();
    getGame()->addEvent({EventId::TORTURED, EventInfo::Attacked{other, self}});
    self->spendTime(1);
  });
}

void Creature::surrender(Creature* to) {
  getGame()->addEvent({EventId::TORTURED, EventInfo::Attacked{this, to}});
}

CreatureAction Creature::give(Creature* whom, vector<Item*> items) {
  if (!getBody().isHumanoid() || !whom->canTakeItems(items))
    return CreatureAction(whom->getName().the() + (items.size() == 1 ? " can't take this item."
        : " can't take these items."));
  return CreatureAction(this, [=](Creature* self) {
    for (auto stack : stackItems(items)) {
      monsterMessage(getName().the() + " gives " + getPluralAName(stack[0], stack.size()) + " to " +
          whom->getName().the());
      playerMessage("You give " + getPluralTheName(stack[0], stack.size()) + " to " +
        whom->getName().the());
    }
    whom->takeItems(equipment->removeItems(items), this);
  });
}

CreatureAction Creature::fire(Vec2 direction) const {
  CHECK(direction.length8() == 1);
  if (getEquipment().getItem(EquipmentSlot::RANGED_WEAPON).empty())
    return CreatureAction("You need a ranged weapon.");
  if (getBody().numGood(BodyPart::ARM) < 2)
    return CreatureAction("You need two hands to shoot a bow.");
  if (!getAmmo())
    return CreatureAction("Out of ammunition");
  return CreatureAction(this, [=](Creature* self) {
    PItem ammo = self->equipment->removeItem(NOTNULL(getAmmo()));
    RangedWeapon* weapon = NOTNULL(dynamic_cast<RangedWeapon*>(
        getOnlyElement(self->getEquipment().getItem(EquipmentSlot::RANGED_WEAPON))));
    weapon->fire(self, std::move(ammo), direction);
    self->spendTime(1);
  });
}

CreatureAction Creature::placeTorch(Dir attachmentDir, function<void(Trigger*)> builtCallback) const {
  return CreatureAction(this, [=](Creature* self) {
      PTrigger torch = Trigger::getTorch(attachmentDir, position);
      Trigger* tRef = torch.get();
      getPosition().addTrigger(std::move(torch));
      addSound(Sound(SoundId::DIGGING).setPitch(0.5));
      builtCallback(tRef);
      self->spendTime(1);
  });
}

void Creature::addMovementInfo(const MovementInfo& info) {
  modViewObject().addMovementInfo(info);
  getPosition().setNeedsRenderUpdate(true);
}

CreatureAction Creature::whip(const Position& pos) const {
  Creature* whipped = pos.getCreature();
  if (pos.dist8(position) > 1 || !whipped)
    return CreatureAction();
  return CreatureAction(this, [=](Creature* self) {
    monsterMessage(PlayerMessage(getName().the() + " whips " + whipped->getName().the()));
    double oldTime = getLocalTime();
    self->spendTime(1);
    if (Random.roll(3)) {
      addSound(SoundId::WHIP);
      self->addMovementInfo({position.getDir(pos), oldTime, getLocalTime(), MovementInfo::ATTACK});
    }
    if (Random.roll(5))
      whipped->monsterMessage(whipped->getName().the() + " screams!", "You hear a horrible scream!");
    if (Random.roll(10)) {
      whipped->addMorale(0.05);
      whipped->you(MsgType::FEEL, "happier");
    }
  });
}

void Creature::addSound(const Sound& sound1) const {
  Sound sound(sound1);
  sound.setPosition(getPosition());
  getGame()->getView()->addSound(sound);
}

CreatureAction Creature::construct(Vec2 direction, const SquareType& type) const {
  if (getPosition().plus(direction).canConstruct(type) && canConstruct(type))
    return CreatureAction(this, [=](Creature* self) {
        if (type.getId() == SquareId::FLOOR)
          addSound(SoundId::DIGGING);
        else if (type.getId() == SquareId::TREE_TRUNK)
          addSound(SoundId::TREE_CUTTING);
        else
          addSound(Sound(SoundId::DIGGING).setPitch(0.5));
        if (getPosition().plus(direction).construct(type)) {
          if (type.getId() == SquareId::TREE_TRUNK) {
            monsterMessage(getName().the() + " cuts a tree");
            playerMessage("You cut a tree");
          } else
          if (type.getId() == SquareId::FLOOR) {
            monsterMessage(getName().the() + " digs a tunnel");
            playerMessage("You dig a tunnel");
          } else
          if (type.getId() == SquareId::MOUNTAIN) {
            monsterMessage(getName().the() + " fills up a tunnel");
            playerMessage("You fill up a tunnel");
          } else {
            monsterMessage(getName().the() + " builds " + getPosition().plus(direction).getName());
            playerMessage("You build " + getPosition().plus(direction).getName());
          }
        }
        self->spendTime(1);
      });
  return CreatureAction();
}

bool Creature::canConstruct(const SquareType& type) const {
  return attributes->getSkills().hasDiscrete(SkillId::CONSTRUCTION);
}

CreatureAction Creature::eat(Item* item) const {
  return CreatureAction(this, [=](Creature* self) {
    monsterMessage(getName().the() + " eats " + item->getAName());
    playerMessage("You eat " + item->getAName());
    self->getPosition().removeItem(item);
    self->spendTime(3);
  });
}

CreatureAction Creature::destroy(Vec2 direction, DestroyAction dAction) const {
  if (direction.length8() == 1 && getPosition().plus(direction).canDestroy(this))
      return CreatureAction(this, [=](Creature* self) {
        string name = getPosition().plus(direction).getName();
        switch (dAction) {
          case BASH: 
            playerMessage("You bash the " + name);
            monsterMessage(getName().the() + " bashes the " + name, "BANG!");
            break;
          case EAT: 
            playerMessage("You eat the " + name);
            monsterMessage(getName().the() + " eats the " + name, "You hear chewing");
            break;
          case DESTROY: 
            playerMessage("You destroy the " + name);
            monsterMessage(getName().the() + " destroys the " + name, "CRASH!");
            break;
      }
      getPosition().plus(direction).destroyBy(self);
      self->spendTime(1);
    });
  return CreatureAction();
}

bool Creature::canCopulateWith(const Creature* c) const {
  return c->getBody().canCopulateWith() && c->attributes->getGender() != attributes->getGender() &&
    c->isAffected(LastingEffect::SLEEP);
}

bool Creature::canConsume(const Creature* c) const {
  return c->getBody().canConsume() && attributes->getSkills().hasDiscrete(SkillId::CONSUMPTION) && isFriend(c);
}

CreatureAction Creature::copulate(Vec2 direction) const {
  const Creature* other = getPosition().plus(direction).getCreature();
  if (!other || !canCopulateWith(other))
    return CreatureAction();
  return CreatureAction(this, [=](Creature* self) {
      Debug() << getName().bare() << " copulate with " << other->getName().bare();
      you(MsgType::COPULATE, "with " + other->getName().the());
      self->spendTime(2);
    });
}

vector<string> Creature::popPersonalEvents() {
  vector<string> ret = personalEvents;
  personalEvents.clear();
  return ret;
}

void Creature::addPersonalEvent(const string& s) {
  personalEvents.push_back(s);
}

CreatureAction Creature::consume(Creature* other) const {
  if (!other || !canConsume(other))
    return CreatureAction();
  return CreatureAction(this, [=] (Creature* self) {
    self->attributes->consume(self, *other->attributes);
    other->die(self, true, false);
    self->spendTime(2);
  });
}

Item* Creature::getWeapon() const {
  vector<Item*> it = equipment->getItem(EquipmentSlot::WEAPON);
  if (it.empty())
    return nullptr;
  else
    return getOnlyElement(it);
}

CreatureAction Creature::applyItem(Item* item) const {
  if (!contains({ItemClass::TOOL, ItemClass::POTION, ItemClass::FOOD, ItemClass::BOOK, ItemClass::SCROLL},
      item->getClass()) || !getBody().isHumanoid())
    return CreatureAction("You can't apply this item");
  if (getBody().numGood(BodyPart::ARM) == 0)
    return CreatureAction("You have no healthy arms!");
  return CreatureAction(this, [=] (Creature* self) {
      double time = item->getApplyTime();
      playerMessage("You " + item->getApplyMsgFirstPerson(self));
      monsterMessage(getName().the() + " " + item->getApplyMsgThirdPerson(self), item->getNoSeeApplyMsg());
      item->apply(self);
      if (item->isDiscarded()) {
        self->equipment->removeItem(item);
      }
      self->spendTime(time);
  });
}

CreatureAction Creature::throwItem(Item* item, Vec2 direction) const {
  if (!getBody().numGood(BodyPart::ARM) || !getBody().isHumanoid())
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
  return CreatureAction(this, [=](Creature* self) {
    Attack attack(self, Random.choose(getBody().getAttackLevels()), item->getAttackType(), accuracy, damage, false,
        none);
    playerMessage("You throw " + item->getAName(false, this));
    monsterMessage(getName().the() + " throws " + item->getAName());
    self->getPosition().throwItem(self->equipment->removeItem(item), attack, dist, direction, getVision());
    self->spendTime(1);
  });
}

bool Creature::canSee(const Creature* c) const {
  if (!c->getPosition().isSameLevel(position))
    return false;
  for (CreatureVision* v : creatureVisions)
    if (v->canSee(this, c))
      return true;
  return !isBlind() && !c->isAffected(LastingEffect::INVISIBLE) &&
         (!c->isHidden() || c->knowsHiding(this)) && c->getPosition().isVisibleBy(this);
}

bool Creature::canSee(Position pos) const {
  return !isBlind() && pos.isVisibleBy(this);
}

bool Creature::canSee(Vec2 pos) const {
  return !isBlind() && position.withCoord(pos).isVisibleBy(this);
}
  
bool Creature::isPlayer() const {
  return controller->isPlayer();
}

const CreatureName& Creature::getName() const {
  return attributes->getName();
}

CreatureName& Creature::getName() {
  return attributes->getName();
}

TribeSet Creature::getFriendlyTribes() const {
  if (Game* game = getGame())
    return game->getTribe(tribe)->getFriendlyTribes();
  else
    return TribeSet().insert(tribe);
}

MovementType Creature::getMovementType() const {
  return MovementType(getFriendlyTribes(), {
      true,
      isAffected(LastingEffect::FLYING),
      attributes->getSkills().hasDiscrete(SkillId::SWIMMING),
      getBody().canWade()})
    .setForced(isBlind() || isHeld() || forceMovement)
    .setFireResistant(isAffected(LastingEffect::FIRE_RESISTANT))
    .setSunlightVulnerable(getBody().isSunlightVulnerable() && !isAffected(LastingEffect::DARKNESS_SOURCE)
        && (!getGame() || getGame()->getSunlightInfo().getState() == SunlightState::DAY));
}

int Creature::getDifficultyPoints() const {
  difficultyPoints = max<double>(difficultyPoints,
      getModifier(ModifierType::DEFENSE) + getModifier(ModifierType::ACCURACY) + getModifier(ModifierType::DAMAGE)
      + getAttr(AttrType::SPEED) / 10);
  CHECK(difficultyPoints >=0 && difficultyPoints < 100000) << getModifier(ModifierType::DEFENSE) << " "
     << getModifier(ModifierType::ACCURACY) << " " << getModifier(ModifierType::DAMAGE) << " "
     << getAttr(AttrType::SPEED) << " " << getName().bare();
  return difficultyPoints;
}

CreatureAction Creature::continueMoving() {
  if (shortestPath && shortestPath->isReachable(getPosition()))
    return move(shortestPath->getNextMove(getPosition()));
  else
    return CreatureAction();
}

CreatureAction Creature::stayIn(const Location* location) {
  if (!location->contains(getPosition())) {
    for (Position v : getPosition().neighbors8(Random))
      if (location->contains(v))
        if (auto action = move(v))
          return action;
    return moveTowards(location->getMiddle());
  }
  return CreatureAction();
}

CreatureAction Creature::moveTowards(Position pos, bool stepOnTile) {
  if (!pos.isValid())
    return CreatureAction();
  if (pos.isSameLevel(position))
    return moveTowards(pos, false, stepOnTile);
  else if (auto stairs = position.getStairsTo(pos)) {
    if (stairs == position)
      return applySquare();
    else
      return moveTowards(*stairs, false, true);
  } else
    return CreatureAction();
}

bool Creature::canNavigateTo(Position pos) const {
  MovementType movement = getMovementType();
  for (Position v : pos.neighbors8())
    if (v.isConnectedTo(position, movement))
      return true;
  return false;
}

CreatureAction Creature::moveTowards(Position pos, bool away, bool stepOnTile) {
  CHECK(pos.isSameLevel(position));
  if (stepOnTile && !pos.canEnterEmpty(this))
    return CreatureAction();
  MEASURE(
  if (!away && !canNavigateTo(pos))
    return CreatureAction();
  , "Creature Sector checking " + getName().bare() + " from " + toString(position) + " to " + toString(pos));
  //Debug() << "" << getPosition().getCoord() << (away ? "Moving away from" : " Moving toward ") << pos.getCoord();
  bool newPath = false;
  bool targetChanged = shortestPath && shortestPath->getTarget().dist8(pos) > getPosition().dist8(pos) / 10;
  if (!shortestPath || targetChanged || shortestPath->isReversed() != away) {
    newPath = true;
    if (!away)
      shortestPath.reset(new LevelShortestPath(this, pos, position));
    else
      shortestPath.reset(new LevelShortestPath(this, pos, position, -1.5));
  }
  CHECK(shortestPath);
  if (shortestPath->isReachable(position))
    if (auto action = move(shortestPath->getNextMove(position)))
      return action;
  if (newPath)
    return CreatureAction();
  Debug() << "Reconstructing shortest path.";
  if (!away)
    shortestPath.reset(new LevelShortestPath(this, pos, position));
  else
    shortestPath.reset(new LevelShortestPath(this, pos, position, -1.5));
  if (shortestPath->isReachable(position)) {
    Position pos2 = shortestPath->getNextMove(position);
    if (auto action = move(pos2))
      return action;
    else {
      if (!pos2.canEnterEmpty(this))
        if (auto action = destroy(getPosition().getDir(pos2), Creature::BASH))
          return action;
      return CreatureAction();
    }
  } else {
    //Debug() << "Cannot move toward " << pos.getCoord();
    return CreatureAction();
  }
}

CreatureAction Creature::moveAway(Position pos, bool pathfinding) {
  CHECK(pos.isSameLevel(position));
  if (pos.dist8(getPosition()) <= 5 && pathfinding)
    if (auto action = moveTowards(pos, true, false))
      return action;
  pair<Vec2, Vec2> dirs = pos.getDir(getPosition()).approxL1();
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
          case AttackType::STAB: you(MsgType::ARE, "stabbed in the "_s + 
                                     Random.choose("back"_s, "neck"_s)); break;
          case AttackType::SPELL: you(MsgType::ARE, "ripped to pieces!"); break;
          default: FAIL << "Unhandled attack type " << int(type);
        }
        break;
    case BodyPart::HEAD: 
        switch (type) {
          case AttackType::SHOOT: you(MsgType::ARE, "shot in the " +
                                      Random.choose("eye"_s, "neck"_s, "forehead"_s) + "!"); break;
          case AttackType::BITE: you(MsgType::YOUR, "head is bitten off!"); break;
          case AttackType::CUT: you(MsgType::YOUR, "head is chopped off!"); break;
          case AttackType::CRUSH: you(MsgType::YOUR, "skull is shattered!"); break;
          case AttackType::PUNCH: you(MsgType::YOUR, "neck is broken!"); break;
          case AttackType::HIT: you(MsgType::ARE, "hit in the head!"); break;
          case AttackType::STAB: you(MsgType::ARE, "stabbed in the eye!"); break;
          case AttackType::SPELL: you(MsgType::YOUR, "head is ripped to pieces!"); break;
          default: FAIL << "Unhandled attack type " << int(type);
        }
        break;
    case BodyPart::TORSO:
        switch (type) {
          case AttackType::SHOOT: you(MsgType::YOUR, "shot in the heart!"); break;
          case AttackType::BITE: you(MsgType::YOUR, "internal organs are ripped out!"); break;
          case AttackType::CUT: you(MsgType::ARE, "cut in half!"); break;
          case AttackType::STAB: you(MsgType::ARE, "stabbed in the " +
                                     Random.choose<string>({"stomach", "heart"}, {1, 1}) + "!"); break;
          case AttackType::CRUSH: you(MsgType::YOUR, "ribs and internal organs are crushed!"); break;
          case AttackType::HIT: you(MsgType::ARE, "hit in the chest!"); break;
          case AttackType::PUNCH: you(MsgType::YOUR, "stomach receives a deadly blow!"); break;
          case AttackType::SPELL: you(MsgType::ARE, "ripped to pieces!"); break;
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
          case AttackType::SPELL: you(MsgType::YOUR, "arm is ripped to pieces!"); break;
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
          case AttackType::SPELL: you(MsgType::YOUR, "wing is ripped to pieces!"); break;
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
          case AttackType::SPELL: you(MsgType::YOUR, "leg is ripped to pieces!"); break;
          default: FAIL << "Unhandled attack type " << int(type);
        }
        break;
  }
}

bool Creature::isUnknownAttacker(const Creature* c) const {
  return unknownAttackers.contains(c);
}

void Creature::updateVision() {
  if (attributes->getSkills().hasDiscrete(SkillId::NIGHT_VISION))
    vision = VisionId::NIGHT;
  else if (attributes->getSkills().hasDiscrete(SkillId::ELF_VISION) || isAffected(LastingEffect::FLYING))
    vision = VisionId::ELF;
  else
    vision = VisionId::NORMAL; 
}

VisionId Creature::getVision() const {
  return vision;
}

void Creature::updateVisibleCreatures() {
  int range = FieldOfView::sightRange;
  visibleEnemies.clear();
  visibleCreatures.clear();
  for (Creature* c : position.getAllCreatures(range)) 
    if (canSee(c) || isUnknownAttacker(c)) {
      visibleCreatures.push_back(c->getPosition());
      if (isEnemy(c))
        visibleEnemies.push_back(c->getPosition());
    }
}

vector<Creature*> Creature::getVisibleEnemies() const {
  vector<Creature*> ret;
  for (Position p : visibleEnemies)
    if (Creature* c = p.getCreature())
      if (!c->isDead())
        ret.push_back(c);
  return ret;
}

vector<Creature*> Creature::getVisibleCreatures() const {
  vector<Creature*> ret;
  for (Position p : visibleCreatures)
    if (Creature* c = p.getCreature())
      if (!c->isDead())
        ret.push_back(c);
  return ret;
}

vector<Position> Creature::getVisibleTiles() const {
  if (isBlind())
    return {};
  else
    return getPosition().getVisibleTiles(getVision());
}

const char* getMoraleText(double morale) {
  if (morale >= 0.7)
    return "ecstatic";
  if (morale >= 0.2)
    return "merry";
  if (morale < -0.7)
    return "depressed";
  if (morale < -0.2)
    return "unhappy";
  return nullptr;
}

vector<string> Creature::getMainAdjectives() const {
  vector<string> ret;
  if (isBlind())
    ret.push_back("blind");
  if (isAffected(LastingEffect::INVISIBLE))
    ret.push_back("invisible");
  if (getBody().numBodyParts(BodyPart::ARM) == 1)
    ret.push_back("one-armed");
  if (getBody().numBodyParts(BodyPart::ARM) == 0)
    ret.push_back("armless");
  if (getBody().numBodyParts(BodyPart::LEG) == 1)
    ret.push_back("one-legged");
  if (getBody().numBodyParts(BodyPart::LEG) == 0)
    ret.push_back("legless");
  if (isAffected(LastingEffect::HALLU))
    ret.push_back("tripped");
  if (auto text = getMoraleText(getMorale()))
    ret.push_back(text);
  return ret;
}

vector<AdjectiveInfo> Creature::getWeaponAdjective() const {
  if (const Item* weapon = getWeapon())
    return {{"Wielding " + weapon->getAName(), ""}};
  else
    return {};
}

vector<AdjectiveInfo> Creature::getGoodAdjectives() const {
  vector<AdjectiveInfo> ret;
  if (!getWeapon() && !getBody().isHumanoid()) {
    ret.push_back({"+" + toString(attributes->getBarehandedDamage()) + " unarmed attack", ""});
  }
  for (LastingEffect effect : ENUM_ALL(LastingEffect))
    if (isAffected(effect))
      if (const char* name = LastingEffects::getGoodAdjective(effect)) {
        ret.push_back({ name, Effect::getDescription(effect) });
        if (!attributes->isAffectedPermanently(effect))
          ret.back().name += "  " + attributes->getRemainingString(effect, getGlobalTime());
      }
  if (getBody().isUndead())
    ret.push_back({"Undead",
        "Undead creatures don't take regular damage and need to be killed by chopping up or using fire."});
  if (morale > 0)
    if (auto text = getMoraleText(getMorale()))
      ret.push_back({capitalFirst(text),
          "Morale affects minion's productivity and chances of fleeing from battle."});
  return ret;
}

vector<AdjectiveInfo> Creature::getBadAdjectives() const {
  vector<AdjectiveInfo> ret;
  if (!getWeapon() && getBody().isHumanoid()) {
    ret.push_back({"+" + toString(attributes->getBarehandedDamage()) + " unarmed attack", ""});
  }
  getBody().getBadAdjectives(ret);
  for (LastingEffect effect : ENUM_ALL(LastingEffect))
    if (isAffected(effect))
      if (const char* name = LastingEffects::getBadAdjective(effect)) {
        ret.push_back({ name, Effect::getDescription(effect) });
        if (!attributes->isAffectedPermanently(effect))
          ret.back().name += "  " + attributes->getRemainingString(effect, getGlobalTime());
      }
  if (isBlind())
    ret.push_back({"Blind" + (isAffected(LastingEffect::BLIND) ?
          (" " + attributes->getRemainingString(LastingEffect::BLIND, getGlobalTime())) : ""), ""});
  if (morale < 0)
    if (auto text = getMoraleText(getMorale()))
      ret.push_back({capitalFirst(text),
          "Morale affects minion's productivity and chances of fleeing from battle."});
  return ret;
}

bool Creature::isSameSector(Position pos) const {
  return pos.isConnectedTo(position, getMovementType());
}

void Creature::setInCombat() {
  lastCombatTime = getGame()->getGlobalTime();
}

bool Creature::wasInCombat(double numLastTurns) const {
  return lastCombatTime && *lastCombatTime >= getGame()->getGlobalTime() - numLastTurns;
}

