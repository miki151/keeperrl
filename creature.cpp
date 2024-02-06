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
#include "item.h"
#include "statistics.h"
#include "options.h"
#include "game.h"
#include "effect.h"
#include "item_factory.h"
#include "controller.h"
#include "player_message.h"
#include "attack.h"
#include "vision.h"
#include "equipment.h"
#include "shortest_path.h"
#include "spell_map.h"
#include "minion_activity_map.h"
#include "tribe.h"
#include "creature_attributes.h"
#include "position.h"
#include "view.h"
#include "sound.h"
#include "lasting_effect.h"
#include "attack_type.h"
#include "attack_level.h"
#include "model.h"
#include "view_object.h"
#include "spell.h"
#include "body.h"
#include "field_of_view.h"
#include "furniture.h"
#include "creature_debt.h"
#include "message_generator.h"
#include "weapon_info.h"
#include "time_queue.h"
#include "profiler.h"
#include "furniture_type.h"
#include "furniture_usage.h"
#include "fx_name.h"
#include "navigation_flags.h"
#include "game_event.h"
#include "spell_school.h"
#include "content_factory.h"
#include "health_type.h"
#include "effect_type.h"
#include "automaton_part.h"
#include "ai_type.h"
#include "companion_info.h"
#include "promotion_info.h"
#include "buff_info.h"

template <class Archive>
void Creature::serialize(Archive& ar, const unsigned int version) {
  ar(SUBCLASS(OwnedObject<Creature>), SUBCLASS(Renderable), SUBCLASS(UniqueEntity), SUBCLASS(EventListener));
  ar(attributes, position, equipment, shortestPath, knownHiding, tribe);
  ar(deathTime, hidden, lastMoveCounter, effectFlags, duelInfo, leadershipExp);
  ar(deathReason, nextPosIntent, globalTime, drops, promotions, maxPromotion);
  ar(unknownAttackers, privateEnemies, holding, attributesStack, butcherInfo);
  ar(controllerStack, kills, statuses, automatonParts, phylactery, highestAttackValueEver);
  ar(difficultyPoints, points, capture, spellMap, killTitles, companions, combatExperience, teamExperience);
  ar(vision, debt, lastCombatIntent, primaryViewId, steed, buffs, buffCount, buffPermanentCount);
}

SERIALIZABLE(Creature)
REGISTER_TYPE(ListenerTemplate<Creature>)

SERIALIZATION_CONSTRUCTOR_IMPL(Creature)

Creature::Creature(const ViewObject& object, TribeId t, CreatureAttributes attr, SpellMap spellMap)
    : Renderable(object), attributes(std::move(attr)), tribe(t), spellMap(std::move(spellMap)) {
  modViewObject().setGenericId(getUniqueId().getGenericId());
  modViewObject().setModifier(ViewObject::Modifier::CREATURE);
  if (auto& obj = attributes->getIllusionViewObject()) {
    obj->setGenericId(getUniqueId().getGenericId());
    obj->setModifier(ViewObject::Modifier::CREATURE);
  }
  capture = attributes->isInstantPrisoner();
}

Creature::Creature(TribeId t, CreatureAttributes attr, SpellMap spellMap)
    : Creature(attr.createViewObject(), t, std::move(attr), std::move(spellMap)) {
}

Creature::~Creature() {
}

vector<vector<Creature*>> Creature::stack(const vector<Creature*>& creatures, function<string(Creature*)> addSuffix) {
  map<string, vector<Creature*>> stacks;
  for (Creature* c : creatures)
    stacks[c->getName().stack() + addSuffix(c)].push_back(c);
  return getValues(stacks);
}

const ViewObject& Creature::getViewObjectFor(const Tribe* observer) const {
  if (attributes->getIllusionViewObject() && observer->isEnemy(this))
    return *attributes->getIllusionViewObject();
  else
    return getViewObject();
}

void Creature::setAlternativeViewId(optional<ViewId> id) {
  if (id) {
    primaryViewId = getViewObject().id();
    modViewObject().setId(*id);
  } else if (!!primaryViewId) {
    modViewObject().setId(*primaryViewId);
    primaryViewId = none;
  }
}

bool Creature::hasAlternativeViewId() const {
  return !!primaryViewId;
}

const Body& Creature::getBody() const {
  return attributes->getBody();
}

Body& Creature::getBody() {
  return attributes->getBody();
}

TimeInterval Creature::getSpellDelay(const Spell* spell) const {
  CHECK(!isReady(spell));
  return spellMap->getReadyTime(this, spell) - *getGlobalTime();
}

bool Creature::isReady(const Spell* spell) const {
  if (auto time = getGlobalTime())
    return spellMap->getReadyTime(this, spell) <= *time;
  else
    return true;
}

const SpellMap& Creature::getSpellMap() const {
  return *spellMap;
}

SpellMap& Creature::getSpellMap() {
  return *spellMap;
}

bool Creature::isAutomaton() const {
  return !automatonParts.empty();
}

const vector<AutomatonPart>& Creature::getAutomatonParts() const {
  return automatonParts;
}

void Creature::addAutomatonPart(AutomatonPart p) {
  automatonParts.push_back(std::move(p));
  sort(automatonParts.begin(), automatonParts.end(),
      [](const auto& p1, const auto& p2) { return p1.layer < p2.layer; });
}

const CreatureAttributes& Creature::getAttributes() const {
  return *attributes;
}

CreatureAttributes& Creature::getAttributes() {
  return *attributes;
}

void Creature::popAttributes() {
  if (!attributesStack.empty()) {
    setAttributes(std::move(attributesStack.back().first), std::move(attributesStack.back().second));
    attributesStack.pop_back();
  }
}

void Creature::pushAttributes(CreatureAttributes attr, SpellMap spells) {
  attributesStack.push_back(setAttributes(std::move(attr), std::move(spells)));
}

pair<CreatureAttributes, SpellMap> Creature::setAttributes(CreatureAttributes attr, SpellMap spells) {
  auto firstName = getName().first();
  for (auto item : equipment->getAllEquipped())
    item->onUnequip(this, false);
  for (auto item : equipment->getItems())
    item->onDropped(this, false);
  attr.copyLastingEffects(*attributes);
  for (auto effect : ENUM_ALL(LastingEffect))
    if (attributes->isAffectedPermanently(effect))
      LastingEffects::onRemoved(this, effect, false);
  auto permanentBuffs = getKeys(buffPermanentCount);
  for (auto b : permanentBuffs)
    removePermanentEffect(b, 1, false);
  auto ret = make_pair(std::move(*attributes), std::move(*spellMap));
  ret.first.permanentBuffs = std::move(permanentBuffs);
  attributes = std::move(attr);
  spellMap = std::move(spells);
  modViewObject() = attributes->createViewObject();
  modViewObject().setGenericId(getUniqueId().getGenericId());
  modViewObject().setModifier(ViewObject::Modifier::CREATURE);
  for (auto effect : ENUM_ALL(LastingEffect))
    if (attr.isAffectedPermanently(effect))
      LastingEffects::onAffected(this, effect, false);
  for (auto item : equipment->getAllEquipped())
    item->onEquip(this, false);
  for (auto item : equipment->getItems())
    item->onOwned(this, false);
  auto factory = getGame()->getContentFactory();
  for (auto effect : ENUM_ALL(LastingEffect))
    if (!LastingEffects::affects(this, effect, factory))
      removeEffect(effect, true);
  getName().setFirst(firstName);
  for (auto& b : attributes->permanentBuffs)
    addPermanentEffect(b, 1, false);
  attributes->permanentBuffs.clear();
  return ret;
}

void Creature::setViewId(ViewId id) {
  modViewObject().setId(id);
  attributes->viewId = id;
}

CreatureAction Creature::castSpell(const Spell* spell) const {
  return castSpell(spell, position);
}

TimeInterval Creature::calculateSpellCooldown(Range cooldown) const {
  PROFILE;
  return TimeInterval(cooldown.getStart() +
      (cooldown.getLength() - 1) * (100 - min(100, getAttr(AttrType("SPELL_SPEED")))) / 100);
}

CreatureAction Creature::castSpell(const Spell* spell, Position target) const {
  if (spell->getType() == SpellType::SPELL && isAffected(LastingEffect::MAGIC_CANCELLATION))
    return CreatureAction("You can't cast spells while under the effect of "
        + LastingEffects::getName(LastingEffect::MAGIC_CANCELLATION) + ".");
  auto verb = spell->getType() == SpellType::SPELL ? "cast this spell" : "use this ability";
  if (!isReady(spell))
    return CreatureAction("You can't "_s + verb + " yet.");
  if (target == position && !spell->canTargetSelf())
    return CreatureAction("You can't "_s + verb + " at yourself.");
  return CreatureAction(this, [=] (Creature* c) {
    if (auto sound = spell->getSound())
      c->position.addSound(*sound);
    spell->addMessage(c);
    if (spell->getType() == SpellType::SPELL)
      getGame()->getStatistics().add(StatId::SPELL_CAST);
    c->spellMap->setReadyTime(c, spell, *getGlobalTime() + calculateSpellCooldown(spell->getCooldown()));
    c->spendTime();
    spell->apply(c, target);
  });
}

void Creature::updateLastingFX(ViewObject& object, const ContentFactory* factory) {
  object.particleEffects.clear();
  if (phylactery)
    object.particleEffects.insert(FXVariantName::LICH);
  if (auto time = getGlobalTime())
    for (auto effect : ENUM_ALL(LastingEffect))
      if (isAffected(effect, *time))
        if (auto fx = LastingEffects::getFX(effect))
          object.particleEffects.insert(*fx);
  for (auto& buff : buffs)
    object.particleEffects.insert(factory->buffs.at(buff.first).fx);
  for (auto& buff : buffPermanentCount)
    object.particleEffects.insert(factory->buffs.at(buff.first).fx);
}


void Creature::pushController(PController ctrl) {
  if (auto controller = getController())
    if (controller->isPlayer())
      getGame()->removePlayer(this);
  if (ctrl->isPlayer())
    getGame()->addPlayer(this);
  controllerStack.push_back(std::move(ctrl));
  if (!isDead() && !getRider())
    if (auto m = position.getModel())
      // This actually moves the creature to the appropriate player/monster time queue,
      // which is the same logic as postponing.
      m->getTimeQueue().postponeMove(this);
}

void Creature::setController(PController ctrl) {
  while (!controllerStack.empty())
    popController();
  pushController(std::move(ctrl));
}

void Creature::popController() {
  if (!controllerStack.empty()) {
    if (getController()->isPlayer())
      getGame()->removePlayer(this);
    controllerStack.pop_back();
    if (auto controller = getController()) {
      if (controller->isPlayer())
        getGame()->addPlayer(this);
      if (!isDead())
        if (auto m = position.getModel())
          m->getTimeQueue().postponeMove(this);
    }
  }
}

bool Creature::isDead() const {
  return !!deathTime;
}

GlobalTime Creature::getDeathTime() const {
  return *deathTime;
}

void Creature::clearInfoForRetiring() {
  unsubscribe();
  lastAttacker = nullptr;
  lastCombatIntent = none;
  if (steed)
    steed->clearInfoForRetiring();
}

optional<string> Creature::getDeathReason() const {
  if (deathReason)
    return deathReason;
  if (lastAttacker)
    return "killed by " + lastAttacker->getName().a();
  return none;
}

const vector<Creature::KillInfo>& Creature::getKills() const {
  return kills;
}

const vector<string>& Creature::getKillTitles() const {
  return killTitles;
}

int Creature::getLastMoveCounter() const {
  return lastMoveCounter;
}

const EnumSet<CreatureStatus>& Creature::getStatus() const {
  return statuses;
}

bool Creature::canBeCaptured() const {
  PROFILE;
  return getBody().canBeCaptured(getGame()->getContentFactory()) && !isAffected(LastingEffect::STUNNED);
}

void Creature::setCaptureOrder(bool state) {
  if (!state || canBeCaptured()) {
    capture = state;
    updateViewObject(getGame()->getContentFactory());
    position.setNeedsRenderUpdate(true);
  }
}

void Creature::toggleCaptureOrder() {
  setCaptureOrder(!capture);
}

bool Creature::isCaptureOrdered() const {
  return capture;
}

EnumSet<CreatureStatus>& Creature::getStatus() {
  return statuses;
}

optional<MovementInfo> Creature::spendTime(TimeInterval t, SpeedModifier speedModifier) {
  PROFILE;
  getBody().affectPosition(position);
  if (!!getRider())
    return none;
  if (Model* m = position.getModel()) {
    MovementInfo ret(Vec2(0, 0), *getLocalTime(), *getLocalTime() + t, 0, MovementInfo::MOVE);
    lastMoveCounter = ret.moveCounter = position.getModel()->getMoveCounter();
    if (!isDead()) {
      if (!isAffected(LastingEffect::SPYING) && steed && !m->getTimeQueue().hasExtraMove(this))
        steed->makeMove();
      if (speedModifier == SpeedModifier::FAST && t == 1_visible) {
        if (m->getTimeQueue().hasExtraMove(this))
          ret.tBegin += 0.5;
        else
          ret.tEnd -= 0.5;
        m->getTimeQueue().makeExtraMove(this);
      } else {
        if (speedModifier == SpeedModifier::FAST)
          t = t - 1_visible;
        if (speedModifier == SpeedModifier::SLOW)
          t *= 2;
        m->getTimeQueue().increaseTime(this, t);
      }
    }
    m->increaseMoveCounter();
    hidden = false;
    return ret;
  }
  return none;
}

CreatureAction Creature::forceMove(Vec2 dir) const {
  return forceMove(getPosition().plus(dir));
}

CreatureAction Creature::forceMove(Position pos) const {
  const_cast<Creature*>(this)->forceMovement = true;
  CreatureAction action = move(pos, none);
  const_cast<Creature*>(this)->forceMovement = false;
  if (action)
    return action.prepend([] (Creature* c) { c->forceMovement = true; })
      .append([] (Creature* c) { c->forceMovement = false; });
  else
    return action;
}

CreatureAction Creature::move(Vec2 dir) const {
  return move(getPosition().plus(dir), none);
}

static Creature::SpeedModifier getSpeedModifier(const Creature* c) {
  bool slow = c->isAffected(LastingEffect::SLOWED);
  bool fast = c->isAffected(LastingEffect::SPEED);
  if (slow)
    return fast ? Creature::SpeedModifier::NORMAL : Creature::SpeedModifier::SLOW;
  return fast ? Creature::SpeedModifier::FAST : Creature::SpeedModifier::NORMAL;
}

CreatureAction Creature::move(Position pos, optional<Position> nextPos) const {
  PROFILE;
  if (isAffected(LastingEffect::IMMOBILE) || !!getRider())
    return CreatureAction("");
  Vec2 direction = getPosition().getDir(pos);
  if (getHoldingCreature())
    return CreatureAction("You can't break free!");
  if (direction.length8() != 1)
    return CreatureAction();
  if (!position.canMoveCreature(direction)) {
    if (pos.getCreature()) {
      if (!canSwapPositionInMovement(pos.getCreature()))
        return CreatureAction(/*"You can't swap position with " + pos.getCreature()->getName().the()*/);
    } else
      return CreatureAction();
  }
  return CreatureAction(this, [=](Creature* self) {
    PROFILE;
    INFO << getName().the() << " moving " << direction;
    if (LastingEffects::restrictedMovement(this)) {
      secondPerson("You can't move!");
      thirdPerson(getName().the() + " can't move!");
      self->spendTime();
      return;
    }
    self->nextPosIntent = nextPos;
    if (position.canMoveCreature(direction))
      self->position.moveCreature(direction);
    else {
      self->swapPosition(direction);
      return;
    }
    auto timeSpent = 1_visible;
    if (isAffected(LastingEffect::COLLAPSED)) {
      you(MsgType::CRAWL, getPosition().getName());
      timeSpent = 3_visible;
    }
    self->addMovementInfo(self->spendTime(timeSpent, getSpeedModifier(self))->setDirection(direction));
  });
}

static bool posIntentsConflict(Position myPos, Position hisPos, optional<Position> hisIntent) {
  return hisIntent && *myPos.dist8(*hisIntent) > *hisPos.dist8(*hisIntent) && *hisPos.dist8(*hisIntent) <= 1;
}

bool Creature::canSwapPositionInMovement(Creature* other) const {
  PROFILE;
  if (!other->getPosition().isSameLevel(position))
    return !other->nextPosIntent;
  return canSwapPositionWithEnemy(other)
      && (!posIntentsConflict(position, other->position, other->nextPosIntent) ||
          isPlayer() || other->isAffected(LastingEffect::STUNNED) ||
          getGlobalTime()->getInternal() % 10 == (getUniqueId().getHash() % 10 + 10) % 10)
      && (!other->isPlayer() || isPlayer())
      && (!other->isEnemy(this) || other->isAffected(LastingEffect::STUNNED));
}

bool Creature::canSwapPositionWithEnemy(Creature* other) const {
  PROFILE;
  return !other->isAffected(LastingEffect::LOCKED_POSITION) &&
      (LastingEffects::canSwapPosition(other) || !LastingEffects::restrictedMovement(other)
          || other->isAffected(LastingEffect::STUNNED))
      && !other->getAttributes().isBoulder()
      && other->getPosition().canEnterEmpty(this)
      && getPosition().canEnterEmpty(other);
}

void Creature::displace(Vec2 dir) {
  position.getModel()->increaseMoveCounter();
  position.moveCreature(dir);
  auto time = *getLocalTime();
  addMovementInfo({dir, time, time + 1_visible, position.getModel()->getMoveCounter(), MovementInfo::MOVE});
  position.getModel()->increaseMoveCounter();
}

bool Creature::canTakeItems(const vector<Item*>& items) const {
  return getBody().isHumanoid() && canCarry(items) == items.size();
}

void Creature::takeItems(vector<PItem> items, Creature* from) {
  vector<Item*> ref = getWeakPointers(items);
  equipment->addItems(std::move(items), this);
  getController()->onItemsGiven(ref, from);
  if (auto game = getGame())
    game->addEvent(EventInfo::ItemsOwned{this, ref});
}

void Creature::you(MsgType type, const string& param) const {
  getController()->getMessageGenerator().add(this, type, param);
}

void Creature::you(const string& param) const {
  getController()->getMessageGenerator().add(this, param);
}

void Creature::verb(const string& second, const string& third, const string& param, MessagePriority priority) const {
  secondPerson(PlayerMessage("You "_s + second + (param.empty() ? "" : " " + param), priority));
  thirdPerson(PlayerMessage(getName().the() + " " + third + (param.empty() ? "" : " " + param), priority));
}

void Creature::secondPerson(const PlayerMessage& message) const {
  getController()->getMessageGenerator().addSecondPerson(this, message);
}

Controller* Creature::getController() const {
  if (!controllerStack.empty())
    return NOTNULL(controllerStack.back().get());
  else
    return nullptr;
}

void Creature::swapPosition(Vec2 direction, bool withExcuseMe) {
  CHECK(direction.length8() == 1);
  Creature* other = NOTNULL(getPosition().plus(direction).getCreature());
  if (withExcuseMe) {
    privateMessage("Excuse me!");
    other->privateMessage("Excuse me!");
  }
  position.swapCreatures(other);
  auto movementInfo = *spendTime();
  addMovementInfo(movementInfo.setDirection(direction));
  other->addMovementInfo(movementInfo.setDirection(-direction));
}

void Creature::makeMove() {
  PROFILE;
  auto time = *getGlobalTime();
  vision->update(this, time);
  CHECK(!isDead());
  if (LastingEffects::doesntMove(this)) {
    getController()->sleeping();
    spendTime();
    return;
  }
  {
    // Calls makeMove() while preventing Controller destruction by holding a shared_ptr on stack.
    // This is needed, otherwise Controller could be destroyed during makeMove() if creature committed suicide.
    shared_ptr<Controller> controllerTmp = controllerStack.back().giveMeSharedPointer();
    MEASURE(controllerTmp->makeMove(), "creature move time");
  }
  updateViewObject(getGame()->getContentFactory());
  unknownAttackers.clear();
  vision->update(this, time);
}

CreatureAction Creature::wait() {
  return CreatureAction([=](Creature* self) {
    self->nextPosIntent = none;
    INFO << self->getName().the() << " waiting";
    bool keepHiding = self->hidden;
    self->spendTime();
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
  return equipment->removeItems(items, this);
}

Level* Creature::getLevel() const {
  return getPosition().getLevel();
}

Game* Creature::getGame() const {
  if (!gameCache)
    gameCache = getPosition().getGame();
  return gameCache;
}

Position Creature::getPosition() const {
  return position;
}

void Creature::message(const PlayerMessage& msg) const {
  if (isPlayer())
    getController()->privateMessage(msg);
  else
    getPosition().globalMessage(msg);
}

void Creature::privateMessage(const PlayerMessage& msg) const {
  getController()->privateMessage(msg);
}

void Creature::addFX(const FXInfo& fx) const {
  if (auto game = getGame())
    game->addEvent(EventInfo::FX{position, fx});
}

void Creature::thirdPerson(const PlayerMessage& playerCanSee) const {
  getController()->getMessageGenerator().addThirdPerson(this, playerCanSee);
}

vector<Item*> Creature::getPickUpOptions() const {
  PROFILE;
  if (!getBody().canPickUpItems())
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

bool Creature::canCarryMoreWeight(double weight) const {
  return getBody().getCarryLimit() >= equipment->getTotalWeight() ||
      isAffected(LastingEffect::NO_CARRY_LIMIT);
}

int Creature::canCarry(const vector<Item*>& items) const {
  int ret = 0;
  if (!isAffected(LastingEffect::NO_CARRY_LIMIT)) {
    int limit = getBody().getCarryLimit();
    double weight = equipment->getTotalWeight();
    if (weight > limit)
      return 0;
    for (auto& it : items) {
      weight += it->getWeight();
      ++ret;
      if (weight > limit)
        break;
    }
    return ret;
  } else
    return items.size();
}

CreatureAction Creature::pickUp(const vector<Item*>& itemsAll) const {
  if (!getBody().canPickUpItems())
    return CreatureAction("You can't pick up anything!");
  auto items = itemsAll.getPrefix(canCarry(itemsAll));
  if (items.empty())
    return CreatureAction("You are carrying too much to pick this up.");
  return CreatureAction(this, [=](Creature* self) {
    INFO << getName().the() << " pickup ";
    for (auto stack : stackItems(items)) {
      thirdPerson(getName().the() + " picks up " + getPluralAName(stack[0], stack.size()));
      secondPerson("You pick up " + getPluralTheName(stack[0], stack.size()));
    }
    self->equipment->addItems(self->getPosition().removeItems(items), self);
    if (!isAffected(LastingEffect::NO_CARRY_LIMIT) &&
        equipment->getTotalWeight() > getBody().getCarryLimit())
      you(MsgType::ARE, "overloaded");
    getGame()->addEvent(EventInfo::ItemsPickedUp{self, items});
    //self->spendTime();
  });
}

vector<vector<Item*>> Creature::stackItems(vector<Item*> items) const {
  PROFILE;
  auto factory = getGame()->getContentFactory();
  map<string, vector<Item*> > stacks = groupBy<Item*, string>(items,
      [this, factory] (Item* const& item) { return item->getNameAndModifiers(factory, false, this); });
  return getValues(stacks);
}

CreatureAction Creature::drop(const vector<Item*>& items) const {
  return CreatureAction(this, [=](Creature* self) {
    INFO << getName().the() << " drop";
    for (auto stack : stackItems(items)) {
      thirdPerson(getName().the() + " drops " + getPluralAName(stack[0], stack.size()));
      secondPerson("You drop " + getPluralTheName(stack[0], stack.size()));
    }
    getGame()->addEvent(EventInfo::ItemsDropped{self, items});
    self->getPosition().dropItems(self->equipment->removeItems(items, self));
    //self->spendTime();
  });
}

void Creature::drop(vector<PItem> items) {
  getPosition().dropItems(std::move(items));
}

bool Creature::canEquipIfEmptySlot(const Item* item, string* reason) const {
  PROFILE;
  auto setReason = [reason] (string s) {
    if (reason)
      *reason = std::move(s);
  };
  if (!getBody().isHumanoid()) {
    setReason("Only humanoids can equip items!");
    return false;
  }
  if (!attributes->canEquip()) {
    setReason("You can't equip items!");
    return false;
  }
  if (getBody().numGood(BodyPart::ARM) == 0) {
    setReason("You have no healthy arms!");
    return false;
  }
  if (getBody().numGood(BodyPart::ARM) == 1 && item->getWeaponInfo().twoHanded) {
    setReason("You need two hands to wield " + item->getAName() + "!");
    return false;
  }
  if (!item->canEquip()) {
    setReason("This item can't be equipped");
    return false;
  }
  if (item->getModifier(AttrType("RANGED_DAMAGE")) > 0 && item->getEquipmentSlot() == EquipmentSlot::RANGED_WEAPON &&
      getAttr(AttrType("RANGED_DAMAGE"), false) == 0 && attributes->getMaxExpLevel().count(AttrType("RANGED_DAMAGE"))) {
    setReason("You are not skilled in archery");
    return false;
  }
  if (equipment->getMaxItems(item->getEquipmentSlot(), this) == 0) {
    setReason("You lack a required body part to equip this type of item");
    return false;
  }
  return true;
}

bool Creature::canEquip(const Item* item) const {
  return canEquipIfEmptySlot(item, nullptr) && equipment->canEquip(item, this);
}

CreatureAction Creature::equip(Item* item, const ContentFactory* factory) const {
  string reason;
  if (!canEquipIfEmptySlot(item, &reason))
    return CreatureAction(reason);
  if (equipment->getSlotItems(item->getEquipmentSlot()).contains(item))
    return CreatureAction();
  EquipmentSlot slot = item->getEquipmentSlot();
  auto ret = CreatureAction(this, [=](Creature* self) {
    INFO << getName().the() << " equip " << item->getName();
    secondPerson("You equip " + item->getTheName(false, self));
    thirdPerson(getName().the() + " equips " + item->getAName());
    self->equipment->equip(item, slot, self, factory ? factory : getGame()->getContentFactory());
    if (auto game = getGame())
      game->addEvent(EventInfo::ItemsOwned{self, {item}});
    //self->spendTime();
  });
  vector<Item*> toUnequip;
  if (equipment->getSlotItems(slot).size() >= equipment->getMaxItems(slot, this))
    toUnequip.push_back(equipment->getSlotItems(slot)[0]);
  for (auto other : equipment->getAllEquipped())
    if (item->isConflictingEquipment(other))
      toUnequip.push_back(other);
  for (auto item : toUnequip) {
    if (auto action = unequip(item, factory))
      ret = ret.prepend(std::move(action));
    else
      return action;
  }
  return ret;
}

CreatureAction Creature::unequip(Item* item, const ContentFactory* factory) const {
  if (!equipment->isEquipped(item))
    return CreatureAction("This item is not equipped.");
  if (!getBody().isHumanoid())
    return CreatureAction("You can't remove this item!");
  if (getBody().numGood(BodyPart::ARM) == 0)
    return CreatureAction("You have no healthy arms!");
  return CreatureAction(this, [=](Creature* self) {
    INFO << getName().the() << " unequip";
    CHECK(equipment->isEquipped(item)) << "Item not equipped.";
    EquipmentSlot slot = item->getEquipmentSlot();
    secondPerson("You " + string(slot == EquipmentSlot::WEAPON ? " sheathe " : " remove ") +
        item->getTheName(false, this));
    thirdPerson(getName().the() + (slot == EquipmentSlot::WEAPON ? " sheathes " : " removes ") +
        item->getAName());
    self->equipment->unequip(item, self, factory);
    //self->spendTime();
  });
}

CreatureAction Creature::push(Creature* other) {
  Vec2 goDir = position.getDir(other->position);
  if (!goDir.isCardinal8() || !other->position.plus(goDir).canEnter(
      other->getMovementType()))
    return CreatureAction("You can't push " + other->getName().the());
  if (!getBody().canPush(other->getBody()) &&
      !other->isAffected(LastingEffect::IMMOBILE) &&
      !other->isAffected(LastingEffect::TURNED_OFF)) // everyone can push immobile and turned off creatures
    return CreatureAction("You are too small to push " + other->getName().the());
  return CreatureAction(this, [=](Creature* self) {
    self->verb("push", "pushes", other->getName().the());
    other->displace(goDir);
    if (auto m = self->move(goDir))
      m.perform(self);
  });
}

CreatureAction Creature::applySquare(Position pos, FurnitureLayer layer) const {
  CHECK(*pos.dist8(getPosition()) <= 1);
  if (auto furniture = pos.getFurniture(layer))
    if (furniture->canUse(this))
      return CreatureAction(this, [=](Creature* self) {
        self->nextPosIntent = none;
        INFO << getName().the() << " applying " << getPosition().getName();
        auto originalPos = getPosition();
        auto usageTime = furniture->getUsageTime();
        furniture->use(pos, self);
        CHECK(!getRider()) << "Steed " << identify() << " applying " << furniture->getName();
        auto movementInfo = *self->spendTime(usageTime);
        if (pos != getPosition() && getPosition() == originalPos)
          self->addMovementInfo(movementInfo
              .setDirection(getPosition().getDir(pos))
              .setMaxLength(1_visible)
              .setType(MovementInfo::WORK)
              .setFX(furniture->getUsageFX()));
      });
  return CreatureAction();
}

CreatureAction Creature::hide() const {
  PROFILE;
  if (!isAffected(BuffId("AMBUSH_SKILL")))
    return CreatureAction("You don't have this skill.");
  if (auto furniture = getPosition().getFurniture(FurnitureLayer::MIDDLE))
    if (furniture->canHide())
      return CreatureAction(this, [=](Creature* self) {
        secondPerson("You hide behind the " + furniture->getName());
        thirdPerson(getName().the() + " hides behind the " + furniture->getName());
        self->knownHiding.clear();
        self->modViewObject().setModifier(ViewObject::Modifier::HIDDEN);
        for (Creature* other : getLevel()->getAllCreatures())
          if (other->canSee(this) && other->isEnemy(this)) {
            self->knownHiding.insert(other);
            if (!isAffected(LastingEffect::BLIND))
              you(MsgType::CAN_SEE_HIDING, other->getName().the());
          }
        self->spendTime();
        self->hidden = true;
      });
  return CreatureAction("You can't hide here.");
}

CreatureAction Creature::chatTo(Creature* other) const {
  CHECK(other);
  if (other->getPosition().dist8(getPosition()).value_or(2) <= 1 && getBody().isHumanoid())
    return CreatureAction(this, [=](Creature* self) {
        secondPerson("You chat with " + other->getName().the());
        thirdPerson(getName().the() + " chats with " + other->getName().the());
        other->getAttributes().chatReaction(other, self);
        self->spendTime();
    });
  else
    return CreatureAction();
}

CreatureAction Creature::pet(Creature* other) const {
  CHECK(other);
  if (other->getPosition().dist8(getPosition()).value_or(2) <= 1 && other->getAttributes().getPetReaction(other) &&
      isFriend(other) && getBody().isHumanoid())
    return CreatureAction(this, [=](Creature* self) {
        secondPerson("You pet " + other->getName().the());
        thirdPerson(getName().the() + " pets " + other->getName().the());
        self->message(*other->getAttributes().getPetReaction(other));
        self->spendTime();
    });
  else
    return CreatureAction();
}

CreatureAction Creature::stealFrom(Vec2 direction, const vector<Item*>& items) const {
  if (getPosition().plus(direction).getCreature())
    return CreatureAction(this, [=](Creature* self) {
        Creature* other = NOTNULL(getPosition().plus(direction).getCreature());
        self->equipment->addItems(other->steal(items), self);
      });
  return CreatureAction();
}

bool Creature::isHidden() const {
  return hidden;
}

bool Creature::knowsHiding(const Creature* c) const {
  return knownHiding.contains(c);
}

bool Creature::addEffect(LastingEffect effect, TimeInterval time, bool msg) {
  return addEffect(effect, time, *getGlobalTime(), msg);
}

bool Creature::addEffect(LastingEffect effect, TimeInterval time, GlobalTime globalTime, bool msg,
    const ContentFactory* factory) {
  PROFILE;
  if (!factory)
    factory = getGame()->getContentFactory();
  if (LastingEffects::affects(this, effect, factory)) {
    bool was = isAffected(effect, globalTime);
    if (!was || LastingEffects::canProlong(effect))
      attributes->addLastingEffect(effect, globalTime + time);
    if (!was && isAffected(effect, globalTime)) {
      LastingEffects::onAffected(this, effect, msg);
      if (auto g = getGame())
        updateViewObject(g->getContentFactory());
      if (auto fx = LastingEffects::getApplicationFX(effect))
        addFX(*fx);
      return true;
    }
  }
  return false;
}

bool Creature::removeEffect(LastingEffect effect, bool msg) {
  PROFILE;
  bool was = isAffected(effect);
  attributes->clearLastingEffect(effect);
  if (was && !isAffected(effect)) {
    LastingEffects::onRemoved(this, effect, msg);
    if (auto g = getGame())
      updateViewObject(g->getContentFactory());
    addFX(FXInfo(FXName::CIRCULAR_SPELL, Color::WHITE));
    return true;
  }
  return false;
}

bool Creature::addPermanentEffect(LastingEffect effect, int count, bool msg, const ContentFactory* factory) {
  PROFILE;
  if (!factory)
    factory = getGame()->getContentFactory();
  if (LastingEffects::affects(this, effect, factory)) {
    bool was = attributes->isAffectedPermanently(effect);
    attributes->addPermanentEffect(effect, count);
    if (!was && attributes->isAffectedPermanently(effect)) {
      LastingEffects::onAffected(this, effect, msg);
      if (msg)
        message(PlayerMessage("The effect is permanent", MessagePriority::HIGH));
      if (auto g = getGame())
        updateViewObject(g->getContentFactory());
      return true;
    }
  }
  return false;
}

bool Creature::removePermanentEffect(LastingEffect effect, int count, bool msg) {
  PROFILE;
  bool was = isAffected(effect);
  attributes->removePermanentEffect(effect, count);
  if (was && !isAffected(effect)) {
    LastingEffects::onRemoved(this, effect, msg);
    if (auto g = getGame())
      updateViewObject(g->getContentFactory());
    return true;
  }
  return false;
}

bool Creature::isAffected(LastingEffect effect) const {
  PROFILE;
  if (LastingEffects::inheritsFromSteed(effect) && steed)
    return steed->isAffected(effect);
  if (auto time = getGlobalTime())
    return attributes->isAffected(effect, *time);
  else
    return attributes->isAffectedPermanently(effect);
}

bool Creature::isAffected(LastingEffect effect, optional<GlobalTime> time) const {
  PROFILE;
  if (LastingEffects::inheritsFromSteed(effect) && steed)
    return steed->isAffected(effect, time);
  if (time)
    return attributes->isAffected(effect, *time);
  else
    return attributes->isAffectedPermanently(effect);
}

bool Creature::isAffected(LastingEffect effect, GlobalTime time) const {
  //PROFILE;
  if (LastingEffects::inheritsFromSteed(effect) && steed)
    return steed->attributes->isAffected(effect, time);
  return attributes->isAffected(effect, time);
}

optional<TimeInterval> Creature::getTimeRemaining(LastingEffect effect) const {
  auto t = attributes->getTimeOut(effect);
  if (auto global = getGlobalTime())
    if (t >= *global)
      return t - *global;
  return none;
}

bool Creature::addEffect(BuffId id, TimeInterval time, GlobalTime global, const ContentFactory* f, bool msg) {
  if (getBody().isImmuneTo(id, f))
    return false;
  auto& info = f->buffs.at(id);
  auto add = [&] {
    if (!info.stacks)
      for (auto& b : buffs)
        if (b.first == id) {
          if (b.second < global + time)
            b.second = global + time;
          return false;
        }
    return true;
  };
  if (add()) {
    buffs.push_back(make_pair(id, global + time));
    if (++buffCount[id] == 1 || info.stacks) {
      if (msg && info.addedMessage)
        applyMessage(*info.addedMessage, this);
      if (info.startEffect)
        info.startEffect->applyToCreature(this);
      return true;
    }
  }
  return false;
}

bool Creature::addEffect(BuffId id, TimeInterval time, bool msg) {
  if (auto global = getGlobalTime())
    return addEffect(id, time, *global, getGame()->getContentFactory(), msg);
  return false;
}

bool Creature::removeBuff(int index, bool msg) {
  auto id = buffs[index].first;
  buffs.removeIndex(index);
  auto& info = getGame()->getContentFactory()->buffs.at(id);
  if (--buffCount[id] == 0 || info.stacks) {
    if (buffCount[id] == 0)
      buffCount.erase(id);
    if (msg && info.removedMessage)
      applyMessage(*info.removedMessage, this);
    if (info.endEffect)
      info.endEffect->applyToCreature(this);
    return true;
  }
  return false;
}

bool Creature::removeEffect(BuffId id, bool msg) {
  for (int i : All(buffs))
    if (buffs[i].first == id)
      return removeBuff(i, msg);
  return false;
}

bool Creature::addPermanentEffect(BuffId id, int count, bool msg, const ContentFactory* factory) {
  if (!factory)
    factory = getGame()->getContentFactory();
  auto& info = factory->buffs.at(id);
  if (++buffPermanentCount[id] == 1) {
    if (msg && info.addedMessage)
      applyMessage(*info.addedMessage, this);
    if (info.startEffect)
      info.startEffect->applyToCreature(this);
    return true;
  }
  return false;
}

bool Creature::removePermanentEffect(BuffId id, int count, bool msg, const ContentFactory* factory) {
  if (!factory)
    factory = getGame()->getContentFactory();
  auto& info = factory->buffs.at(id);
  if (--buffPermanentCount[id] <= 0) {
    buffPermanentCount.erase(id);
    if (msg && info.removedMessage)
      applyMessage(*info.removedMessage, this);
    if (info.endEffect)
      info.endEffect->applyToCreature(this);
    return true;
  }
  return false;
}

bool Creature::isAffected(BuffId id) const {
  // Check attributes->permanentBuffs in case this is before they were copied over in tick()
  return buffCount.count(id) || buffPermanentCount.count(id) || attributes->permanentBuffs.contains(id);
}

bool Creature::isAffectedPermanently(BuffId id) const {
  return buffPermanentCount.count(id);
}

// penalty to strength and dexterity per extra attacker in a single turn
int simulAttackPen(int attackers) {
  return max(0, (attackers - 1) * 2);
}

int Creature::getAttrBonus(AttrType type, int rawAttr, bool includeWeapon) const {
  PROFILE
  int def = min(killTitles.size(), rawAttr);
  for (auto& item : equipment->getAllEquipped())
    if (item->getClass() != ItemClass::WEAPON || type != item->getWeaponInfo().meleeAttackAttr)
      def += item->getModifier(type);
  if (includeWeapon)
    if (auto item = getFirstWeapon())
      if (type == item->getWeaponInfo().meleeAttackAttr)
        def += item->getModifier(type);
  def += LastingEffects::getAttrBonus(this, type);
  return def;
}

int Creature::getAttr(AttrType type, bool includeWeapon, bool includeTeamExp) const {
  return getAttrWithExp(type, getCombatExperience(true, includeTeamExp), includeWeapon);
}

int Creature::getAttrWithExp(AttrType type, int combatExp, bool includeWeapon) const {
  PROFILE
  auto raw = getRawAttr(type, combatExp);
  return max(0, raw + getAttrBonus(type, raw, includeWeapon));
}

int Creature::getSpecialAttr(AttrType type, const Creature* against) const {
  int ret = 0;
  if (auto elems = getReferenceMaybe(attributes->specialAttr, type))
    for (auto& elem : *elems)
      if (elem.second.apply(against->getPosition(), this))
        ret += elem.first;
  for (auto& item : equipment->getAllEquipped())
    if (auto elem = getValueMaybe(item->getSpecialModifiers(), type))
      if (elem->second.apply(against->getPosition(), this))
        ret += elem->first;
  return ret;
}

int Creature::getPoints() const {
  return points;
}

int Creature::getRawAttr(AttrType type, int combatExp) const {
  PROFILE;
  int ret = attributes->getRawAttr(type);
  if (attributes->getMaxExpLevel().count(type) || type == AttrType("DEFENSE") || (
      ret > 0 && !attributes->fixedAttr.count(type))) {
    ret += combatExp;
  }
  return ret;
}

double Creature::getCombatExperience(bool respectMaxPromotion, bool includeTeamExp) const {
  auto ret = combatExperience;
  if (respectMaxPromotion)
    ret = min<double>(getCombatExperienceCap(), ret);
  if (includeTeamExp) {
    if (leadershipExp)
      ret = teamExperience;
    else if (teamExperience > 0)
      ret = (teamExperience + ret) / 2;
  }
  return ret;
}

double Creature::getTeamExperience() const {
  if (leadershipExp)
    return teamExperience - combatExperience;
  else if (teamExperience > 0)
    return (teamExperience - combatExperience) / 2;
  return 0.0;
}

void Creature::setCombatExperience(double value) {
  combatExperience = value;
}

void Creature::setMaxPromotion(int level) {
  maxPromotion = level;
}

int Creature::getCombatExperienceCap() const {
  return 5 * maxPromotion;
}

void Creature::updateCombatExperience(Creature* victim) {
  if (getBody().hasBrain(getGame()->getContentFactory()) && victim->attributes->grantsExperience) {
    double attackDiff = victim->highestAttackValueEver - highestAttackValueEver;
    constexpr double maxLevelGain = 3.0;
    constexpr double minLevelGain = 0.02;
    constexpr double equalLevelGain = 0.5;
    constexpr double maxLevelDiff = 10;
    double expIncrease = max(minLevelGain, min(maxLevelGain,
        (maxLevelGain - equalLevelGain) * attackDiff / maxLevelDiff + equalLevelGain));
    int curLevel = combatExperience;
    combatExperience += expIncrease;
    int newLevel = combatExperience;
    if (curLevel != newLevel) {
      you(MsgType::ARE, "more experienced");
      addPersonalEvent(getName().a() + " reaches experience level " + toString(newLevel));
    }
  }
}

Creature* Creature::getsCreditForKills() {
  for (auto pos : position.getRectangle(Rectangle::centered(10)))
    if (auto c = pos.getCreature())
      if (c->getCompanions(true).contains(this)) {
        return c;
      }
  return this;
}

void Creature::onKilledOrCaptured(Creature* victim) {
  if (!victim->getBody().isFarmAnimal() && !victim->getAttributes().getIllusionViewObject()) {
    updateCombatExperience(victim);
    int difficulty = victim->getDifficultyPoints();
    CHECK(difficulty >= 0 && difficulty < 100000) << difficulty << " " << victim->getName().bare();
    points += difficulty;
    kills.push_back(KillInfo{victim->getUniqueId(), victim->getViewObject().getViewIdList()});
    if (victim->getStatus().contains(CreatureStatus::LEADER) && getBody().hasBrain(getGame()->getContentFactory())) {
      auto victimName = victim->getName();
      victimName.setKillTitle(none);
      auto title = capitalFirst(victimName.bare()) + " Slayer";
      if (!killTitles.contains(title)) {
        attributes->getName().setKillTitle(title);
        killTitles.push_back(title);
      }
    }
    if (attributes->afterKilledSomeone)
      attributes->afterKilledSomeone->apply(position);
  }
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
  PROFILE;
  if (c == this || c->statuses.contains(CreatureStatus::PRISONER) || statuses.contains(CreatureStatus::PRISONER))
    return false;
  if (duelInfo && duelInfo->timeout > *globalTime && c->tribe == duelInfo->enemy &&
      c->getUniqueId() != duelInfo->opponent)
    return false;
  auto result = getTribe()->isEnemy(c) || c->getTribe()->isEnemy(this) ||
    privateEnemies.hasKey(c) || c->privateEnemies.hasKey(this);
  if (auto time = getGlobalTime())
    return LastingEffects::modifyIsEnemyResult(this, c, *time, result);
  else
    return result;
}

vector<Item*> Creature::getGold(int num) const {
  vector<Item*> ret;
  for (Item* item : equipment->getItems().filter([](const Item* it) { return it->getClass() == ItemClass::GOLD; })) {
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
    shortestPath = none;
  position = pos;
  if (nextPosIntent && !position.isSameLevel(*nextPosIntent))
    nextPosIntent = none;
  if (steed)
    steed->setPosition(pos);
}

optional<LocalTime> Creature::getLocalTime() const {
  if (Model* m = position.getModel())
    return m->getLocalTime();
  else
    return none;
}

optional<GlobalTime> Creature::getGlobalTime() const {
  PROFILE;
  return globalTime;
}

void Creature::considerMovingFromInaccessibleSquare() {
  PROFILE;
  auto movement = getMovementType();
  auto forced = movement;
  forced.setForced();
  if (!position.canEnterEmpty(forced))
    for (auto neighbor : position.neighbors8(Random))
      if (neighbor.canEnter(movement)) {
        displace(position.getDir(neighbor));
        CHECK(getPosition().getCreature() == this);
        break;
      }
}

void Creature::setTeamExperience(double value, bool leadership) {
  teamExperience = value;
  leadershipExp = leadership;
}

void Creature::tick() {
  PROFILE_BLOCK("Creature::tick");
  for (auto& b : attributes->permanentBuffs)
    addPermanentEffect(b, 1, false);
  attributes->permanentBuffs.clear();
  if (!!phylactery && !isSubscribed())
    subscribeTo(position.getModel());
  auto factory = getGame()->getContentFactory();
  highestAttackValueEver = max(highestAttackValueEver, getBestAttackWithExp(factory, getCombatExperience(false, false)).value);
  if (phylactery && phylactery->killedBy) {
    auto attacker = phylactery->killedBy;
    phylactery = none;
    dieWithAttacker(attacker);
    return;
  }
  if (!!steed && isEnemy(steed.get()))
    tryToDismount();
  const auto privateEnemyTimeout = 50_visible;
  for (auto c : privateEnemies.getKeys())
    if (privateEnemies.getOrFail(c) < *globalTime - privateEnemyTimeout)
      privateEnemies.erase(c);
  considerMovingFromInaccessibleSquare();
  auto time = *getGlobalTime();
  vision->update(this, time);
  if (Random.roll(30))
    getDifficultyPoints();
  equipment->tick(position, this);
  if (isDead())
    return;
  tickCompanions();
  for (LastingEffect effect : ENUM_ALL(LastingEffect)) {
    if (attributes->considerTimeout(effect, time))
      LastingEffects::onTimedOut(this, effect, true);
    if (isDead())
      return;
    if (isAffected(effect, time) && LastingEffects::tick(this, effect))
      return;
  }
  if (processBuffs())
    return;
  updateViewObject(factory);
  if (getBody().tick(this)) {
    dieWithAttacker(lastAttacker);
    return;
  }
  if (steed)
    steed->tick();
  if (auto sound = getBody().rollAmbientSound())
    position.addSound(*sound);
}

bool Creature::processBuffs() {
  PROFILE
  auto factory = getGame()->getContentFactory();
  {
    PROFILE_BLOCK("intrinsic effects")
    for (auto effect : ENUM_ALL(LastingEffect))
      if (!attributes->isAffectedPermanently(effect) && getBody().isIntrinsicallyAffected(effect, factory))
        addPermanentEffect(effect, 1, false);
    for (auto effect : factory->buffs)
      if (!isAffectedPermanently(effect.first) && getBody().isIntrinsicallyAffected(effect.first, factory))
        addPermanentEffect(effect.first, 1, false);
  }
  auto buffsCopy = buffs;
  auto time = *getGlobalTime();
  {
    PROFILE_BLOCK("ticks and timeouts")
    for (int index : All(buffsCopy).reverse()) {
      auto buff = buffsCopy[index];
      auto& info = factory->buffs.at(buff.first);
      if (info.tickEffect)
        info.tickEffect->applyToCreature(this);
      if (buff.second < time)
        removeBuff(index, true);
      if (isDead())
        return true;
    }
  }
  {
    PROFILE_BLOCK("permanent ticks")
    for (auto& buff : buffPermanentCount) {
      auto& info = factory->buffs.at(buff.first);
      if (info.tickEffect)
        info.tickEffect->applyToCreature(this);
      if (isDead())
        return true;
    }
  }
  return false;
}

static vector<Creature*> summonPersonal(Creature* c, CreatureId id, optional<int> strength,
    optional<Position> position) {
  auto spirits = Effect::summon(c, id, 1, none, 0_visible, position);
  for (auto spirit : spirits) {
    if (strength) {
      spirit->getAttributes().setBaseAttr(AttrType("DEFENSE"), *strength);
      for (auto& attr : c->getGame()->getContentFactory()->attrInfo)
        if (attr.second.isAttackAttr)
          spirit->getAttributes().setBaseAttr(attr.first, *strength);
    }
    spirit->getAttributes().setCanJoinCollective(false);
    spirit->getBody().setCanBeCaptured(false);
    if (!position)
      c->verb("have", "has", "summoned " + spirit->getName().a());
  }
  return spirits;
}

static optional<Position> getCompanionPosition(Creature* c) {
  auto level = c->getLevel();
  for (int i : Range(100)) {
    Position pos(level->getBounds().random(Random), level);
    if (pos.isCovered() || c->canSee(pos) || !pos.canEnter(MovementTrait::WALK))
      continue;
    return pos;
  }
  return none;
}

void Creature::removeCompanions(int index) {
  attributes->companions.removeIndexPreserveOrder(index);
  if (index < companions.size()) {
    for (auto c : companions[index].creatures)
      if (!c->isDead()) {
        c->addFX(FXName::SPAWN);
        c->dieNoReason(DropType::NOTHING);
      }
    companions.removeIndexPreserveOrder(index);
  }
}

void Creature::tickCompanions() {
  PROFILE;
  for (auto& summonsType : companions)
    for (auto elem : copyOf(summonsType.creatures)) {
      if (elem->isDead())
        summonsType.creatures.removeElement(elem);
      else if (summonsType.statsBase) { // update the spirit's attributes
        auto base = getAttr(*summonsType.statsBase);
        elem->getAttributes().setBaseAttr(AttrType("DEFENSE"), base);
        for (auto& attr : getGame()->getContentFactory()->attrInfo)
          if (attr.second.isAttackAttr)
            elem->getAttributes().setBaseAttr(attr.first, base);
      }
    }
  while (companions.size() < attributes->companions.size())
    companions.push_back(CompanionGroup{{}, attributes->companions[companions.size()].statsBase,
        attributes->companions[companions.size()].getsKillCredit});
  for (int i : All(attributes->companions)) {
    auto& summonsInfo = attributes->companions[i];
    if (companions[i].creatures.size() < summonsInfo.count && Random.chance(summonsInfo.summonFreq))
      append(companions[i].creatures, summonPersonal(this, Random.choose(summonsInfo.creatures),
          summonsInfo.statsBase ? optional<int>(getAttr(*summonsInfo.statsBase)) : optional<int>(),
          summonsInfo.spawnAway ? getCompanionPosition(this) : none));
    if (summonsInfo.hostile)
      for (auto c : companions[i].creatures) {
        c->setTribe(TribeId::getHostile());
        c->unknownAttackers.insert(this);
      }
  }
}

vector<Creature*> Creature::getCompanions(bool withNoKillCreditOnly) const {
  vector<Creature*> ret;
  for (auto& group : companions)
    if (!withNoKillCreditOnly || !group.getsKillCredit)
      for (auto c : group.creatures)
        ret.push_back(c.get());
  return ret;
}

Creature* Creature::getFirstCompanion() const {
  for (auto& group : companions)
    for (auto c : group.creatures)
      return c.get();
  return nullptr;
}

void Creature::upgradeViewId(int level) {
  if (!hasAlternativeViewId() && level > 0 && !attributes->viewIdUpgrades.empty()) {
    level = min(level, attributes->viewIdUpgrades.size());
    modViewObject().setId(attributes->viewIdUpgrades[level - 1]);
  }
}

ViewIdList Creature::getMaxViewIdUpgrade() const {
  ViewIdList ret = {!attributes->viewIdUpgrades.empty()
    ? attributes->viewIdUpgrades.back()
    : getViewObject().id()};
  ret.append(getViewObject().partIds);
  return ret;
}

ViewIdList Creature::getViewIdWithWeapon() const {
  auto ret = getViewObject().getViewIdList();
  vector<Item*> it = equipment->getSlotItems(EquipmentSlot::WEAPON);
  if (!it.empty())
    ret.push_front(it[0]->getEquipedViewId());
  return ret;
}

void Creature::dropUnsupportedEquipment() {
  for (auto slot : ENUM_ALL(EquipmentSlot)) {
    auto& items = equipment->getSlotItems(slot);
    for (int i : Range(equipment->getMaxItems(slot, this), items.size())) {
      verb("drop your", "drops "_s + his(attributes->getGender()), items.back()->getName());
      position.dropItem(equipment->removeItem(items.back(), this));
    }
  }
}

void Creature::dropWeapon() {
  for (auto weapon : equipment->getSlotItems(EquipmentSlot::WEAPON)) {
    verb("drop your", "drops "_s + his(attributes->getGender()), weapon->getName());
    position.dropItem(equipment->removeItem(weapon, this));
    break;
  }
}


CreatureAction Creature::execute(Creature* c, const char* verbSecond, const char* verbThird) const {
  if (c->getPosition().dist8(getPosition()).value_or(2) > 1)
    return CreatureAction();
  return CreatureAction(this, [=] (Creature* self) {
    self->verb(verbSecond, verbThird, c->getName().the());
    c->dieWithAttacker(self);
  });
}

int Creature::getDefaultWeaponDamage() const {
  PROFILE;
  if (auto weapon = getFirstWeapon())
    return getAttr(weapon->getWeaponInfo().meleeAttackAttr);
  else
    return 0;
}

AttrType Creature::modifyDamageAttr(AttrType type, const ContentFactory* factory) const {
  for (auto& buff : buffs)
    if (auto& mod = factory->buffs.at(buff.first).modifyDamageAttr)
      if (mod->first == type)
        return mod->second;
  for (auto& buff : buffPermanentCount)
    if (auto& mod = factory->buffs.at(buff.first).modifyDamageAttr)
      if (mod->first == type)
        return mod->second;
  return type;
}

CreatureAction Creature::attack(Creature* other) const {
  CHECK(!other->isDead());
  if (!position.isSameLevel(other->getPosition()))
    return CreatureAction();
  Vec2 dir = getPosition().getDir(other->getPosition());
  if (dir.length8() != 1)
    return CreatureAction();
  auto weapons = getRandomWeapons();
  if (weapons.empty())
    return CreatureAction("No available weapon or intrinsic attack");
  auto ret = CreatureAction(this, [=] (Creature* self) {
    other->addCombatIntent(self, CombatIntentInfo::Type::ATTACK);
    INFO << getName().the() << " attacking " << other->getName().the();
    bool wasDamaged = false;
    for (auto weapon : weapons) {
      auto weaponInfo = weapon.first->getWeaponInfo();
      auto damageAttr = weaponInfo.meleeAttackAttr;
      const int damage = max(1, int(weapon.second * (getAttr(damageAttr, false) +
          getSpecialAttr(damageAttr, other) + weapon.first->getModifier(damageAttr))));
      AttackLevel attackLevel = Random.choose(getBody().getAttackLevels());
      damageAttr = modifyDamageAttr(damageAttr, getGame()->getContentFactory());
      vector<Effect> victimEffects;
      for (auto& e : weaponInfo.victimEffect)
        if (Random.chance(e.chance))
          victimEffects.push_back(e.effect);
      Attack attack(self, attackLevel, weaponInfo.attackType, damage, damageAttr, std::move(victimEffects));
      string enemyName = other->getController()->getMessageGenerator().getEnemyName(other);
      if (!canSee(other))
        enemyName = "something";
      weapon.first->getAttackMsg(this, enemyName);
      getGame()->addEvent(EventInfo::CreatureAttacked{other, self, damageAttr});
      wasDamaged |= other->takeDamage(attack);
      for (auto& e : weaponInfo.attackerEffect) {
        e.apply(position);
        if (self->isDead())
          break;
      }
      if (self->isDead() || other->isDead() || other->isAffected(LastingEffect::STUNNED) ||
          other->position.dist8(position).value_or(2) > 1)
        break;
    }
    if (auto movementInfo = self->spendTime()) {
      movementInfo->setDirection(dir).setType(MovementInfo::ATTACK);
      if (wasDamaged)
        movementInfo->setVictim(other->getUniqueId());
      self->addMovementInfo(*movementInfo);
    }
  });
  return ret;
}

void Creature::onAttackedBy(Creature* attacker) {
  if (!canSee(attacker))
    unknownAttackers.insert(attacker);
  if (attacker->tribe != tribe && globalTime)
    // This attack may be accidental, so only do this for creatures from another tribe.
    // To handle intended attacks within one tribe, private enemy will be added in addCombatIntent
    privateEnemies.set(attacker, *globalTime);
  lastAttacker = attacker;
  addCombatIntent(attacker, CombatIntentInfo::Type::ATTACK);
  if (hasAlternativeViewId())
    attacker->tryToDestroyLastingEffect(LastingEffect::SPYING);
}

void Creature::removePrivateEnemy(const Creature* c) {
  privateEnemies.erase(c);
}

void Creature::setDuel(TribeId enemyTribe, Creature* opponent, GlobalTime timeout) {
  duelInfo = DuelInfo{enemyTribe, opponent ? opponent->getUniqueId() : Creature::Id{}, timeout};
  if (steed)
    steed->duelInfo = DuelInfo{enemyTribe, Creature::Id{}, timeout};
}

constexpr double getDamage(double damageRatio) {
  constexpr double minRatio = 0.3;  // the ratio at which the damage drops to 0
  constexpr double maxRatio = 2.2;     // the ratio at which the damage reaches 1
  constexpr double damageAtOne = 0.12;// damage dealt at a ratio of 1
  if (damageRatio <= minRatio)
    return 0;
  else if (damageRatio <= 1)
    return damageAtOne * (damageRatio - minRatio) / (1.0 - minRatio);
  else if (damageRatio <= maxRatio)
    return damageAtOne + (1.0 - damageAtOne) * (damageRatio - 1.0) / (maxRatio - 1.0);
  else
    return 1.0;
}

bool Creature::captureDamage(double damage, Creature* attacker) {
  getBody().bleed(this, damage);
  auto factory = getGame()->getContentFactory();
  updateViewObject(factory);
  if (getBody().getHealth() <= 0) {
    if (attributes->isInstantPrisoner()) {
      setTribe(attacker->getTribeId());
      you(MsgType::ARE, "captured");
    } else {
      addEffect(LastingEffect::STUNNED, 300_visible);
      if (steed)
        tryToDismount();
      if (auto rider = getRider())
        rider->tryToDismount();
      heal(1.0);
      updateViewObject(factory);
    }
    setCaptureOrder(false);
    return true;
  } else
    return false;
}

double Creature::getFlankedMod() const {
  PROFILE;
  if (!getGame())
    return 1.0;
  int cnt = 1 + getAttr(AttrType("PARRY"));
  for (auto pos : position.neighbors8())
    if (auto c = pos.getCreature()) {
      if (isEnemy(c))
        --cnt;
      else
        ++cnt;
    }
  return pow(1.07, min(0, cnt));
}

bool Creature::takeDamage(const Attack& attack) {
  if (steed && Random.roll(10))
    return steed->takeDamage(attack);
  PROFILE;
  const double hitPenalty = 0.95;
  double defense = getAttr(AttrType("DEFENSE"));
  if (Creature* attacker = attack.attacker) {
    onAttackedBy(attacker);
    for (auto c : getVisibleCreatures())
      if (*c->getPosition().dist8(position) < 10)
        c->removeEffect(LastingEffect::SLEEP);
    defense += getSpecialAttr(AttrType("DEFENSE"), attacker);
  }
  defense *= getFlankedMod();
  for (LastingEffect effect : ENUM_ALL(LastingEffect))
    if (isAffected(effect))
      defense = LastingEffects::modifyCreatureDefense(this, effect, defense, attack.damageType);
  auto factory = getGame()->getContentFactory();
  auto modifyDefense = [&](BuffId id) {
    auto& info = factory->buffs.at(id);
    if (!info.defenseMultiplierAttr || info.defenseMultiplierAttr == attack.damageType)
      defense *= info.defenseMultiplier;
  };
  for (auto& buff : buffs)
    modifyDefense(buff.first);
  for (auto& buff : buffPermanentCount)
    modifyDefense(buff.first);
  double damage = getDamage((double) attack.strength / defense);
  if (attack.withSound)
    if (auto sound = attributes->getAttackSound(attack.type, damage > 0))
      position.addSound(*sound);
  bool returnValue = damage > 0;
  if (damage > 0) {
    bool canCapture = capture && attack.attacker;
    if (canCapture && captureDamage(damage, attack.attacker)) {
      auto attacker = (attack.attacker ? attack.attacker : lastAttacker)->getsCreditForKills();
      if (attacker)
        attack.attacker->onKilledOrCaptured(this);
      getGame()->addEvent(EventInfo::CreatureStunned{this, attacker});
      return true;
    }
    if (!canCapture) {
      auto res = attributes->getBody().takeDamage(attack, this, damage);
      returnValue = (res != Body::NOT_HURT);
      if (res == Body::KILLED)
        return true;
    }
  } else
    message(attack.harmlessMessage);
  if (auto& effect = factory->attrInfo.at(attack.damageType).onAttackedEffect)
    effect->applyToCreature(this, attack.attacker);
  for (auto& e : attack.effect) {
    e.apply(position, attack.attacker);
    if (isDead())
      break;
  }
  for (LastingEffect effect : ENUM_ALL(LastingEffect))
    if (isAffected(effect))
      LastingEffects::afterCreatureDamage(this, effect);
  return returnValue;
}

static vector<string> extractNames(const vector<AdjectiveInfo>& adjectives) {
  return adjectives.transform([] (const AdjectiveInfo& e) -> string { return e.name; });
}

void Creature::updateViewObjectFlanking() {
  PROFILE;
  auto& object = modViewObject();
  auto flankedMod = getFlankedMod();
  if (flankedMod < 1.0)
    object.setAttribute(ViewObject::Attribute::FLANKED_MOD, flankedMod);
  else
    object.resetAttribute(ViewObject::Attribute::FLANKED_MOD);
}

void Creature::updateViewObject(const ContentFactory* factory) {
  PROFILE;
  auto& object = modViewObject();
  auto attrs = ViewObject::CreatureAttributes();
  attrs.reserve(factory->attrInfo.size());
  for (auto& attr : factory->attrInfo)
    attrs.push_back(make_pair(attr.second.viewId, getAttr(attr.first)));
  object.setCreatureAttributes(std::move(attrs));
  updateViewObjectFlanking();
  object.setModifier(ViewObject::Modifier::DRAW_MORALE);
  object.setModifier(ViewObject::Modifier::STUNNED, isAffected(LastingEffect::STUNNED) || isAffected(LastingEffect::COLLAPSED));
  object.setModifier(ViewObject::Modifier::FLYING, isAffected(LastingEffect::FLYING));
  auto rider = getRider();
  object.setModifier(ViewObject::Modifier::INVISIBLE, isAffected(LastingEffect::INVISIBLE) ||
      (rider && rider->isAffected(LastingEffect::INVISIBLE)));
  object.setModifier(ViewObject::Modifier::FROZEN, isAffected(LastingEffect::FROZEN));
  object.setModifier(ViewObject::Modifier::TURNED_OFF, isAffected(LastingEffect::TURNED_OFF));
  object.setModifier(ViewObject::Modifier::HIDDEN, hidden);
  object.getCreatureStatus() = getStatus();
  object.setGoodAdjectives(combine(extractNames(getGoodAdjectives(factory)), true));
  object.setBadAdjectives(combine(extractNames(getBadAdjectives(factory)), true));
  getBody().updateViewObject(object, factory);
  object.setModifier(ViewObject::Modifier::CAPTURE_BAR, capture);
  object.setDescription(getName().title());
  getPosition().setNeedsRenderUpdate(true);
  updateLastingFX(object, factory);
  object.partIds.clear();
  for (int i : All(automatonParts)) {
    if (i == 0 || automatonParts[i].layer != automatonParts[i - 1].layer)
      object.partIds.push_back(automatonParts[i].installedId);
    else
      object.partIds.back() = automatonParts[i].installedId;
    }
  object.setModifier(ViewObject::Modifier::IMMOBILE,
      (isAutomaton() && isAffected(LastingEffect::IMMOBILE))
      || (isAffected(LastingEffect::TURNED_OFF) && isAffected(LastingEffect::FLYING)));
  if (steed)
    steed->updateViewObject(factory);
  if (auto it = getFirstWeapon())
    object.weaponViewId = it->getEquipedViewId();
  else
    object.weaponViewId = none;
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

bool Creature::heal(double amount) {
  PROFILE;
  if (getBody().heal(this, amount)) {
    if (!capture)
      you(MsgType::ARE, getBody().hasHealth(HealthType::FLESH, getGame()->getContentFactory())
          ? "fully healed" : "fully materialized");
    lastAttacker = nullptr;
    return true;
  }
  return false;
}

void Creature::setHeld(Creature* c) {
  holding = c->getUniqueId();
}

Creature* Creature::getHoldingCreature() const {
  PROFILE;
  if (holding)
    for (auto pos : getPosition().neighbors8())
      if (auto c = pos.getCreature())
        if (c->getUniqueId() == *holding)
          return c;
  return nullptr;
}

void Creature::take(vector<PItem> items) {
  for (PItem& elem : items)
    take(std::move(elem));
}

void Creature::take(PItem item, const ContentFactory* factory) {
  Item* ref = item.get();
  equipment->addItem(std::move(item), this, factory);
  if (auto action = equip(ref, factory))
    action.perform(this);
}

void Creature::dieWithReason(const string& reason, DropType drops) {
  CHECK(!isDead()) << getName().bare() << " is already dead. " << getDeathReason().value_or("");
  deathReason = reason;
  dieNoReason(drops);
}

void Creature::dieWithLastAttacker(DropType drops) {
  dieWithAttacker(lastAttacker, drops);
}

vector<PItem> Creature::generateCorpse(const ContentFactory* factory, Game* game, bool instantlyRotten) {
  auto ret = getBody().getCorpseItems(getName().bare(), getUniqueId(), instantlyRotten, factory, game);
  append(ret, std::move(drops));
  return ret;
}

static optional<Position> findInaccessiblePos(Position startingPos) {
  PositionSet visited;
  queue<Position> q;
  q.push(startingPos);
  visited.insert(startingPos);
  while (!q.empty()) {
    auto pos = q.front();
    q.pop();
    for (auto v : pos.neighbors8())
      if (v.canEnterEmpty({MovementTrait::WALK}) && !visited.count(v)) {
        visited.insert(v);
        q.push(v);
      }
  }
  auto notVisited = startingPos.getLevel()->getAllPositions().filter(
      [&](auto& pos) { return pos.canEnter({MovementTrait::WALK}) && !visited.count(pos); });
  if (!notVisited.empty())
    return Random.choose(notVisited);
  return none;
}

void Creature::tryToDestroyLastingEffect(LastingEffect effect) {
  removeEffect(effect, false);
  for (auto item : copyOf(equipment->getAllEquipped()))
    if (item->getEquipedEffects().contains(effect)) {
      you(MsgType::YOUR, item->getName() + " crumbles to dust");
      equipment->removeItem(item, this);
      return;
    }
  if (attributes->isAffectedPermanently(effect))
    attributes->removePermanentEffect(effect, 1);
}

bool Creature::considerSavingLife(DropType drops, const Creature* attacker) {
  if (drops != DropType::NOTHING && isAffected(LastingEffect::LIFE_SAVED)) {
    message("But wait!");
    secondPerson(PlayerMessage("You have escaped death!", MessagePriority::HIGH));
    thirdPerson(PlayerMessage(getName().the() + " has escaped death!", MessagePriority::HIGH));
    if (attacker && attacker->getName().bare() == "Death") {
      if (auto target = findInaccessiblePos(position))
        position.moveCreature(*target, true);
      getGame()->achieve(AchievementId("tricked_death"));
    }
    tryToDestroyLastingEffect(LastingEffect::LIFE_SAVED);
    heal();
    removeEffect(BuffId("BLEEDING"), false);
    getBody().healBodyParts(this, 1000);
    forceMovement = false;
    if (!position.canEnterEmpty(this))
      if (auto neighbor = position.getLevel()->getClosestLanding({position}, this))
        displace(position.getDir(*neighbor));
    return true;
  }
  return false;
}

bool Creature::considerPhylactery(DropType drops, const Creature* attacker) {
  if (phylactery) {
    message("But wait!");
    secondPerson(PlayerMessage("You have escaped death!", MessagePriority::HIGH));
    thirdPerson(PlayerMessage(getName().the() + " has escaped death!", MessagePriority::HIGH));
    heal();
    removeEffect(BuffId("BLEEDING"), false);
    getBody().healBodyParts(this, 1000);
    forceMovement = false;
    auto pos = phylactery->pos;
    pos.removeFurniture(FurnitureLayer::MIDDLE);
    if (pos.canEnter(this))
      position.moveCreature(pos, true);
    phylactery = none;
    getGame()->achieve(AchievementId("saved_by_phylactery"));
    return true;
  }
  return false;
}

bool Creature::considerPhylacteryOrSavingLife(DropType drops, const Creature* attacker) {
  if (attacker && attacker->getName().bare() == "Death")
    return considerSavingLife(drops, attacker) || considerPhylactery(drops, attacker);
  else
    return considerPhylactery(drops, attacker) || considerSavingLife(drops, attacker);
}

Creature* Creature::getRider() const {
  if (position.getCreature() != this)
    return position.getCreature();
  return nullptr;
}

void Creature::dieWithAttacker(Creature* attacker, DropType drops) {
  auto oldPos = position;
  CHECK(!isDead()) << getName().bare() << " is already dead. " << getDeathReason().value_or("");
  if (considerPhylacteryOrSavingLife(drops, attacker))
    return;
  if (isAffected(LastingEffect::FROZEN) && drops == DropType::EVERYTHING)
    drops = DropType::ONLY_INVENTORY;
  getController()->onKilled(attacker);
  deathTime = *getGlobalTime();
  if (attacker)
    attacker = attacker->getsCreditForKills();
  lastAttacker = attacker;
  INFO << getName().the() << " dies. Killed by " << (attacker ? attacker->getName().bare() : "");
  if (drops == DropType::EVERYTHING || drops == DropType::ONLY_INVENTORY)
    for (PItem& item : equipment->removeAllItems(this))
      position.dropItem(std::move(item));
  auto game = getGame();
  auto factory = game->getContentFactory();
  if (drops == DropType::EVERYTHING) {
    position.dropItems(generateCorpse(factory, game));
    if (auto sound = getBody().getDeathSound())
      position.addSound(*sound);
    if (getBody().hasHealth(HealthType::FLESH, factory)) {
      auto addBlood = [](Position v) {
        for (auto f : v.modFurniture())
          if (f->onBloodNear(v)) {
            f->spreadBlood(v);
            return true;
          }
        return false;
      };
      if (!addBlood(position))
        for (auto v : Random.permutation(position.getRectangle(Rectangle::centered(4))))
          if (v != position && addBlood(v))
            break;
    }
  }
  if (statuses.contains(CreatureStatus::CIVILIAN))
    game->getStatistics().add(StatId::INNOCENT_KILLED);
  game->getStatistics().add(StatId::DEATH);
  if (attacker)
    attacker->onKilledOrCaptured(this);
  game->addEvent(EventInfo::CreatureKilled{this, attacker});
  getTribe()->onMemberKilled(this, attacker);
  if (auto rider = getRider()) {
    oldPos.getModel()->killCreature(std::move(rider->steed));
  } else
    getLevel()->killCreature(this);
  if (steed) {
    oldPos.landCreature(std::move(steed));
  }
  setController(makeOwner<DoNothingController>(this));
  if (attributes->deathEffect)
    attributes->deathEffect->apply(oldPos, this);
}

void Creature::dieNoReason(DropType drops) {
  dieWithAttacker(nullptr, drops);
}

CreatureAction Creature::flyAway() const {
  PROFILE;
  if (!isAffected(LastingEffect::FLYING) || getPosition().isCovered())
    return CreatureAction();
  return CreatureAction(this, [=](Creature* self) {
    INFO << getName().the() << " fly away";
    thirdPerson(getName().the() + " flies away.");
    self->dieNoReason(Creature::DropType::NOTHING);
  });
}

CreatureAction Creature::disappear() const {
  return CreatureAction(this, [=](Creature* self) {
    INFO << getName().the() << " disappears";
    thirdPerson(getName().the() + " disappears.");
    self->dieNoReason(Creature::DropType::NOTHING);
  });
}

static Sound getTortureSound(Creature* c) {
  if (!c->getBody().isHumanoid())
    return SoundId("TORTURE_BEAST");
  return SoundId(c->getAttributes().getGender() == Gender::FEMALE ? "TORTURE_FEMALE" : "TORTURE_MALE");
}

CreatureAction Creature::torture(Creature* other) const {
  if (!LastingEffects::restrictedMovement(other) || other->getPosition().dist8(getPosition()) != 1)
    return CreatureAction();
  return CreatureAction(this, [=](Creature* self) {
    thirdPerson(getName().the() + " tortures " + other->getName().the());
    secondPerson("You torture " + other->getName().the());
    if (Random.roll(4)) {
      other->thirdPerson(other->getName().the() + " screams!");
      other->getPosition().unseenMessage("You hear a horrible scream");
      position.addSound(getTortureSound(other).setVolume(0.3).setPitch(other->getBody().getDeathSoundPitch()));
    }
    auto dir = position.getDir(other->position);
    // Note: the event can kill the victim
    getGame()->addEvent(EventInfo::CreatureTortured{other, self});
    auto movementInfo = (*self->spendTime())
        .setDirection(dir)
        .setType(MovementInfo::WORK)
        .setVictim(other->getUniqueId());
    self->addMovementInfo(movementInfo);
  });
}

void Creature::retire() {
  unsubscribe();
  for (LastingEffect effect : ENUM_ALL(LastingEffect))
    if (attributes->considerTimeout(effect, GlobalTime(1000000)))
      LastingEffects::onTimedOut(this, effect, false);
  spellMap->setAllReady();
  attributes->promotionGroup = none;
}

void Creature::removeGameReferences() {
  unsubscribe();
  position = Position();
  shortestPath.clear();
  lastAttacker = nullptr;
  nextPosIntent.reset();
  while (!controllerStack.empty())
    popController();
  visibleEnemies.reset();
  visibleCreatures.reset();
  lastCombatIntent.reset();
  gameCache = nullptr;
  companions.clear();
  phylactery = none;
}

void Creature::increaseExpLevel(AttrType type, double increase) {
  int curLevel = (int)getAttributes().getExpLevel(type);
  getAttributes().increaseExpLevel(type, increase);
  int newLevel = (int)getAttributes().getExpLevel(type);
  if (curLevel != newLevel) {
    you(MsgType::ARE, "more skilled");
    if (auto game = getGame())
      addPersonalEvent(getName().a() + " reaches " + game->getContentFactory()->attrInfo.at(type).name +
          " training level " + toString(newLevel));
    spellMap->onExpLevelReached(this, type, newLevel);
  }
}

BestAttack Creature::getBestAttack(const ContentFactory* factory) const {
  return getBestAttackWithExp(factory, getCombatExperience(true, true));
}

BestAttack Creature::getBestAttackWithExp(const ContentFactory* factory, int combatExp) const {
  PROFILE;
  auto viewId = factory->attrInfo.at(AttrType("DAMAGE")).viewId;
  auto value = 0;
  for (auto& a : factory->attrInfo)
    if (a.second.isAttackAttr) {
      auto damage = getAttrWithExp(a.first, combatExp);
      if (damage > value) {
        value = damage;
        viewId = a.second.viewId;
      }
    }
  return {viewId, value};
}

CreatureAction Creature::give(Creature* whom, vector<Item*> items) const {
  if (!getBody().isHumanoid() || !whom->canTakeItems(items))
    return CreatureAction(whom->getName().the() + (items.size() == 1 ? " can't take this item."
        : " can't take these items."));
  return CreatureAction(this, [=](Creature* self) {
    for (auto stack : stackItems(items)) {
      thirdPerson(getName().the() + " gives " + whom->getController()->getMessageGenerator().getEnemyName(whom) + " "
          + getPluralAName(stack[0], (int) stack.size()));
      secondPerson("You give " + getPluralTheName(stack[0], (int) stack.size()) + " to " +
          whom->getName().title());
    }
    whom->takeItems(self->equipment->removeItems(items, self), self);
    self->spendTime();
  });
}

CreatureAction Creature::payFor(const vector<Item*>& items) const {
  int totalPrice = std::accumulate(items.begin(), items.end(), 0,
      [](int sum, const Item* it) { return sum + it->getPrice(); });
  return give(items[0]->getShopkeeper(this), getGold(totalPrice))
      .append([=](Creature*) { for (auto it : items) it->setShopkeeper(nullptr); });
}

void Creature::addMovementInfo(MovementInfo info) {
  auto updateObject = [&info](ViewObject& object, GenericId id) {
    if (info.dirX > 0)
      object.setModifier(ViewObject::Modifier::FLIPX);
    else if (info.dirX < 0)
      object.setModifier(ViewObject::Modifier::FLIPX, false);

    // add generic id since otherwise there is an unknown crash where
    // object has movement info and no id
    object.addMovementInfo(info, id);
  };
  updateObject(modViewObject(), getUniqueId().getGenericId());
  if (steed)
    updateObject(steed->modViewObject(), steed->getUniqueId().getGenericId());
  getPosition().setNeedsRenderUpdate(true);

  if (isAffected(LastingEffect::FLYING))
    return;

  // We're assuming here that position has already been updated
  Position oldPos = position.minus(info.getDir());
  if (auto ground = oldPos.getFurniture(FurnitureLayer::GROUND))
    if (!oldPos.getFurniture(FurnitureLayer::MIDDLE))
      ground->onCreatureWalkedOver(oldPos, info.getDir());
  if (auto ground = position.getFurniture(FurnitureLayer::GROUND))
    if (!position.getFurniture(FurnitureLayer::MIDDLE))
      ground->onCreatureWalkedInto(position, info.getDir());
}

CreatureAction Creature::whip(const Position& pos, double animChance) const {
  Creature* whipped = pos.getCreature();
  if (pos.dist8(position).value_or(2) > 1 || !whipped)
    return CreatureAction();
  return CreatureAction(this, [=](Creature* self) {
    thirdPerson(PlayerMessage(getName().the() + " whips " + whipped->getName().the()));
    auto moveInfo = *self->spendTime();
    if (Random.chance(animChance)) {
      position.addSound(SoundId("WHIP"));
      self->addMovementInfo(moveInfo
          .setDirection(position.getDir(pos))
          .setType(MovementInfo::ATTACK)
          .setVictim(whipped->getUniqueId()));
    }
    if (Random.roll(5)) {
      whipped->thirdPerson(whipped->getName().the() + " screams!");
      whipped->getPosition().unseenMessage("You hear a horrible scream!");
    }
    if (Random.roll(20))
      whipped->addEffect(BuffId("HIGH_MORALE"), 400_visible);
  });
}

CreatureAction Creature::construct(Vec2 direction, FurnitureType type) const {
  if (getPosition().plus(direction).canConstruct(type))
    return CreatureAction(this, [=](Creature* self) {
        position.addSound(Sound(SoundId("DIGGING")).setPitch(0.5));
        getPosition().plus(direction).construct(type, self);
        self->spendTime();
      });
  return CreatureAction();
}

CreatureAction Creature::eat(Item* item) const {
  return CreatureAction(this, [=](Creature* self) {
    thirdPerson(getName().the() + " eats " + item->getAName());
    secondPerson("You eat " + item->getAName());
    self->addEffect(LastingEffect::SATIATED, 500_visible);
    self->getPosition().removeItem(item);
    self->spendTime(3_visible);
  });
}

void Creature::destroyImpl(Vec2 direction, const DestroyAction& action) {
  auto pos = getPosition().plus(direction);
  if (auto furniture = pos.modFurniture(FurnitureLayer::MIDDLE)) {
    string name = furniture->getName();
    secondPerson("You "_s + action.getVerbSecondPerson() + " the " + name);
    thirdPerson(getName().the() + " " + action.getVerbThirdPerson() + " the " + name);
    pos.unseenMessage(action.getSoundText());
    furniture->tryToDestroyBy(pos, this, action);
  }
}

CreatureAction Creature::destroy(Vec2 direction, const DestroyAction& action) const {
  auto pos = getPosition().plus(direction);
  if (auto furniture = pos.getFurniture(FurnitureLayer::MIDDLE))
    if (direction.length8() <= 1 && furniture->canDestroy(pos, getMovementType(), action))
      return CreatureAction(this, [=](Creature* self) {
        self->destroyImpl(direction, action);
        if (auto movementInfo = self->spendTime())
          if (direction.length8() == 1 && action.destroyAnimation())
            self->addMovementInfo(movementInfo
                ->setDirection(getPosition().getDir(pos))
                .setMaxLength(1_visible)
                .setType(MovementInfo::WORK));
            });
  return CreatureAction();
}

void Creature::forceMount(Creature* whom) {
  CHECK(isAffected(LastingEffect::RIDER)) << identify() << " is not a rider";
  CHECK(whom->isAffected(LastingEffect::STEED)) << whom->identify() << " is not a steed";
  if (!steed && !whom->getRider()) {
    whom->removeEffect(LastingEffect::SLEEP);
    steed = whom->position.getModel()->extractCreature(whom);
    steed->position = position;
    steed->modViewObject().setModifier(ViewObjectModifier::FLIPX,
        getViewObject().hasModifier(ViewObjectModifier::FLIPX));
  }
}

CreatureAction Creature::mount(Creature* whom) const {
  if (!isSteedGoodSize(whom) || whom->getPosition().dist8(position) != 1 || !!steed || !!whom->steed || whom->getRider() ||
      whom->isPlayer() || isEnemy(whom) || !whom->isAffected(LastingEffect::STEED) || !isAffected(LastingEffect::RIDER))
    return CreatureAction();
  return CreatureAction(this, [=](Creature* self) {
    auto dir = position.getDir(whom->position);
    self->forceMount(whom);
    self->position.moveCreature(dir);
    auto timeSpent = 1_visible;
    self->addMovementInfo(self->spendTime(timeSpent, getSpeedModifier(self))->setDirection(dir));
    self->verb("mount", "mounts", whom->getName().the());
  });
}

vector<string> Creature::getSteedSizes() const {
  vector<string> ret;
  auto mySize = getBody().getSize();
  ret.push_back(::getName(mySize));
  if (mySize == BodySize::LARGE)
    ret.push_back(::getName(BodySize::HUGE));
  return ret;
}

bool Creature::isSteedGoodSize(Creature* whom) const {
  auto mySize = getBody().getSize();
  auto hisSize = whom->getBody().getSize();
  return mySize == hisSize || (mySize == BodySize::LARGE && hisSize == BodySize::HUGE);
}

Creature* Creature::getSteed() const {
  return steed.get();
}

void Creature::forceDismount(Vec2 v) {
  auto oldPos = position;
  CHECK(!!steed);
  auto steedTmp = std::move(steed);
  position.moveCreature(v);
  oldPos.landCreature(std::move(steedTmp));
}

void Creature::tryToDismount() {
  CHECK(!!steed);
  for (auto v : position.neighbors8(Random))
    if (v.canEnter(getMovementTypeNotSteed(getGame()))) {
      verb("fall off", "falls off", steed->getName().the());
      forceDismount(position.getDir(v));
      return;
    }
}

CreatureAction Creature::dismount() const {
  if (!steed)
    return CreatureAction();
  for (auto v : position.neighbors8(Random))
    if (v.canEnter(getMovementTypeNotSteed(getGame())))
      return CreatureAction(this, [=](Creature* self) {
        auto dir = position.getDir(v);
        self->verb("dismount", "dismounts", steed->getName().the());
        self->forceDismount(dir);
        auto timeSpent = 1_visible;
        self->addMovementInfo(self->spendTime(timeSpent, getSpeedModifier(self))->setDirection(dir));
      });
  return CreatureAction();
}

bool Creature::canCopulateWith(const Creature* c) const {
  PROFILE;
  return isAffected(BuffId("COPULATION_SKILL")) &&
      c->getBody().canCopulateWith(getGame()->getContentFactory()) &&
      c->attributes->getGender() != attributes->getGender() &&
      c->isAffected(LastingEffect::SLEEP);
}

bool Creature::canConsume(const Creature* c) const {
  return c != this &&
      c->getBody().canConsume() &&
      isAffected(BuffId("CONSUMPTION_SKILL")) &&
      c->getTribeId() == tribe;
}

CreatureAction Creature::copulate(Vec2 direction) const {
  Creature* other = getPosition().plus(direction).getCreature();
  if (!other || !canCopulateWith(other))
    return CreatureAction();
  return CreatureAction(this, [=](Creature* self) {
      INFO << getName().bare() << " copulate with " << other->getName().bare();
      you(MsgType::COPULATE, "with " + other->getName().the());
      getGame()->addEvent(EventInfo::FX{self->position, {FXName::LOVE}, self->position.getDir(other->position)});
      auto movementInfo = *self->spendTime(2_visible);
      self->addMovementInfo(movementInfo
          .setDirection(self->position.getDir(other->position))
          .setMaxLength(1_visible)
          .setType(MovementInfo::WORK));
    });
}

void Creature::addPersonalEvent(const string& s) {
  if (Model* m = position.getModel())
    m->addEvent(EventInfo::CreatureEvent{this, s});
}

CreatureAction Creature::consume(Creature* other) const {
  if (!other || !canConsume(other) || other->getPosition().dist8(getPosition()).value_or(2) > 1)
    return CreatureAction();
  return CreatureAction(this, [=] (Creature* self) {
    auto factory = getGame()->getContentFactory();
    vector<string> adjectives;
    for (auto& buff : other->buffPermanentCount) {
      auto& info = factory->buffs.at(buff.first);
      if (info.canAbsorb) {
        self->addPermanentEffect(buff.first);
        adjectives.push_back(info.adjective);
      }
    }
    other->dieWithAttacker(self, Creature::DropType::ONLY_INVENTORY);
    self->attributes->consume(self, *other->attributes);
    if (!adjectives.empty()) {
      self->you(MsgType::BECOME, combine(adjectives));
      self->addPersonalEvent(getName().the() + " becomes " + combine(adjectives));
    }
    self->spendTime(2_visible);
  });
}

template <typename Visitor>
static void visitMaxSimultaneousWeapons(const Creature* c, Visitor visitor) {
  PROFILE;
  constexpr double minMultiplier = 0.5;
  const double skillValue = min(1.0, double(c->getAttr(AttrType("MULTI_WEAPON"))) * 0.02);
  visitor(1);
  if (skillValue < 0.01)
    return;
  if (skillValue > 0.95) {
    for (int i : Range(10))
      visitor(1);
    return;
  }
  visitor(skillValue);
  double curMultiplier = skillValue * skillValue;
  while (curMultiplier >= minMultiplier) {
    visitor(curMultiplier);
    curMultiplier *= skillValue;
  }
}

int Creature::getMaxSimultaneousWeapons() const {
  int res = 0;
  visitMaxSimultaneousWeapons(this, [&](double){ ++res; });
  return res;
}

vector<pair<Item*, double>> Creature::getRandomWeapons() const {
  vector<double> multipliers;
  visitMaxSimultaneousWeapons(this, [&](double m) { multipliers.push_back(m); });
  return getBody().chooseRandomWeapon(equipment->getSlotItems(EquipmentSlot::WEAPON), multipliers);
}

Item* Creature::getFirstWeapon() const {
  vector<Item*> it = equipment->getSlotItems(EquipmentSlot::WEAPON);
  if (!it.empty())
    return it[0];
  return getBody().chooseFirstWeapon();
}

CreatureAction Creature::applyItem(Item* item) const {
  if (!item->canApply() || !getBody().canPickUpItems())
    return CreatureAction("You can't apply this item");
  if (getBody().numGood(BodyPart::ARM) == 0)
    return CreatureAction("You have no healthy arms!");
  if (auto& pred = item->getApplyPredicate())
    if (!pred->apply(position, nullptr))
      return CreatureAction("You can't use this item");
  return CreatureAction(this, [=] (Creature* self) {
      auto time = item->getApplyTime();
      secondPerson("You " + item->getApplyMsgFirstPerson(self));
      thirdPerson(getName().the() + " " + item->getApplyMsgThirdPerson(self));
      item->apply(self);
      if (!isDead()) {
        if (item->isDiscarded())
          self->equipment->removeItem(item, self);
        self->spendTime(time);
      }
  });
}

optional<int> Creature::getThrowDistance(const Item* item) const {
  if (item->getWeight() <= 0.5)
    return 15;
  else if (item->getWeight() <= 5)
    return 6;
  else if (item->getWeight() <= 20)
    return 2;
  else
    return none;
}

CreatureAction Creature::throwItem(Item* item, Position target, bool isFriendlyAI) const {
  PROFILE;
  if (target == position)
    return CreatureAction();
  if (!getBody().numGood(BodyPart::ARM) || !getBody().isHumanoid())
    return CreatureAction("You can't throw anything!");
  auto dist = getThrowDistance(item);
  if (!dist)
    return CreatureAction(item->getTheName() + " is too heavy!");
  int damage = getAttr(AttrType("RANGED_DAMAGE")) + item->getModifier(AttrType("RANGED_DAMAGE"));
  return CreatureAction(this, [=](Creature* self) {
    Attack attack(isFriendlyAI ? nullptr : self, Random.choose(getBody().getAttackLevels()),
        item->getWeaponInfo().attackType, damage, AttrType("DAMAGE"));
    secondPerson("You throw " + item->getAName(false, this));
    thirdPerson(getName().the() + " throws " + item->getAName());
    self->getPosition().throwItem(makeVec(self->equipment->removeItem(item, self)), attack, *dist, target, getVision().getId());
    self->spendTime();
  });
}

bool Creature::canSeeOutsidePosition(const Creature* c, GlobalTime time) const {
  return LastingEffects::canSee(this, c, time);
}

bool Creature::canSeeInPositionIfNotBlind(const Creature* c, GlobalTime time) const {
  PROFILE;
  return (!c->isAffected(LastingEffect::INVISIBLE, time) || isFriend(c)) &&
      (!c->isHidden() || c->knowsHiding(this));
}

bool Creature::canSeeInPosition(const Creature* c, GlobalTime time) const {
  return !isAffected(LastingEffect::BLIND) && canSeeInPositionIfNotBlind(c, time);
}

bool Creature::canSeeIfNotBlind(const Creature* c, GlobalTime time) const {
  PROFILE;
  return (canSeeInPositionIfNotBlind(c, time) &&
          getLevel()->canSee(position.getCoord(), c->position.getCoord(), getVision())) ||
      canSeeOutsidePosition(c, time);
}

bool Creature::canSee(const Creature* c) const {
  PROFILE;
  auto time = getGlobalTime();
  return time &&
      ((canSeeInPosition(c, *time) && c->getPosition().isVisibleBy(this)) || canSeeOutsidePosition(c, *time));
}

bool Creature::canSee(Position pos) const {
  PROFILE;
  return !isAffected(LastingEffect::BLIND) && pos.isVisibleBy(this);
}

bool Creature::canSee(Vec2 pos) const {
  PROFILE;
  return !isAffected(LastingEffect::BLIND) && position.withCoord(pos).isVisibleBy(this);
}

bool Creature::isPlayer() const {
  if (auto ctrl = getController())
    return ctrl->isPlayer();
  return false;
}

const CreatureName& Creature::getName() const {
  return attributes->getName();
}

CreatureName& Creature::getName() {
  return attributes->getName();
}

const char* Creature::identify() const {
  return getName().identify();
}

TribeSet Creature::getFriendlyTribes() const {
  if (auto game = getGame())
    return game->getTribe(tribe)->getFriendlyTribes();
  else
    return TribeSet().insert(tribe);
}

MovementType Creature::getMovementType() const {
  return getMovementType(getGame());
}

MovementType Creature::getMovementTypeNotSteed(Game* game) const {
  auto time = getGlobalTime();
  return MovementType(hasAlternativeViewId() ? TribeSet::getFull() : getFriendlyTribes(), {
      true,
      isAffected(LastingEffect::FLYING, time),
      isAffected(BuffId("SWIMMING_SKILL")),
      getBody().canWade()})
    .setDestroyActions(EnumSet<DestroyAction::Type>([this](auto t) { return DestroyAction(t).canNavigate(this); }))
    .setForced(isAffected(LastingEffect::BLIND, time) || getHoldingCreature() || forceMovement)
    .setFireResistant(isAffected(BuffId("FIRE_IMMUNITY")))
    .setSunlightVulnerable(isAffected(LastingEffect::SUNLIGHT_VULNERABLE, time)
        && !isAffected(LastingEffect::DARKNESS_SOURCE, time)
        && (!game || game->getSunlightInfo().getState() == SunlightState::DAY))
    .setCanBuildBridge(isAffected(BuffId("BRIDGE_BUILDING_SKILL")))
    .setFarmAnimal(getBody().isFarmAnimal());
}

MovementType Creature::getMovementType(Game* game) const {
  PROFILE;
  if (steed)
    return steed->getMovementTypeNotSteed(game);
  else
    return getMovementTypeNotSteed(game);
}

int Creature::getDifficultyPoints() const {
  PROFILE;
  int value = getAttr(AttrType("DEFENSE"));
  for (auto& attr : getGame()->getContentFactory()->attrInfo)
    if (attr.second.isAttackAttr)
      value += getAttr(attr.first);
  difficultyPoints = max(value, difficultyPoints);
  return difficultyPoints;
}

vector<Position> Creature::getCurrentPath() const {
  if (shortestPath)
    return shortestPath->getPath();
  return {};
}

CreatureAction Creature::moveTowards(Position pos, NavigationFlags flags) {
  PROFILE;
  if (!position.isSameModel(pos) || !pos.isValid() || !!getRider())
    return CreatureAction();
  auto movement = getMovementType();
  if (pos.isSameLevel(position)) {
    bool connected = pos.isConnectedTo(position, movement);
    if (!connected && !flags.stepOnTile)
      for (auto v : pos.neighbors8())
        if (v.isConnectedTo(position, movement)) {
          connected = true;
          break;
        }
    if (connected)
      return moveTowards(pos, false, flags);
  }
  if ((flags.stepOnTile && !pos.canNavigateTo(position, movement)) ||
      (!flags.stepOnTile && !pos.canNavigateToOrNeighbor(position, movement)))
    return CreatureAction();
  auto getStairs = [&pos, this, &flags, &movement] () -> optional<Position> {
    if (lastStairsNavigation && lastStairsNavigation->from == position.getLevel() &&
        lastStairsNavigation->target == pos)
      return lastStairsNavigation->stairs;
    if (auto res = position.getStairsTo(pos, movement, !flags.stepOnTile)) {
      lastStairsNavigation = LastStairsNavigation { position.getLevel(), pos, res->first };
      return res->first;
    }
    return none;
  };
  if (auto stairs = getStairs()) {
    if (stairs == position)
      return applySquare(position, FurnitureLayer::MIDDLE)
          .append([](Creature* self) { self->nextPosIntent = self->position;});
    else
      return moveTowards(*stairs, false, flags);
  } else
    return CreatureAction();
}

CreatureAction Creature::moveTowards(Position pos) {
  return moveTowards(pos, NavigationFlags{});
}

bool Creature::canNavigateToOrNeighbor(Position pos) const {
  return pos.canNavigateToOrNeighbor(position, getMovementType());
}

bool Creature::canNavigateTo(Position pos) const {
  return pos.canNavigateTo(position, getMovementType());
}

CreatureAction Creature::moveTowards(Position pos, bool away, NavigationFlags flags) {
  PROFILE;
  CHECK(pos.isSameLevel(position));
  if (flags.stepOnTile && !pos.canEnterEmpty(this))
    return CreatureAction();
  if (!away && !canNavigateToOrNeighbor(pos))
    return CreatureAction();
  auto currentPath = shortestPath;
  for (int i : Range(2)) {
    bool wasNew = false;
    if (!currentPath || Random.roll(10) || currentPath->isReversed() != away ||
        currentPath->getTarget().dist8(pos).value_or(10000000) > *position.dist8(pos) / 10) {
      INFO << "Calculating new path";
      currentPath = LevelShortestPath(this, pos, away ? -1.5 : 0);
      wasNew = true;
    }
    if (currentPath->isReachable(position)) {
      INFO << "Position reachable";
      Position pos2 = currentPath->getNextMove(position);
      if (pos2.dist8(position).value_or(2) > 1)
        if (auto f = position.getFurniture(FurnitureLayer::MIDDLE))
          if (f->hasUsageType(BuiltinUsageId::PORTAL))
            return applySquare(position, FurnitureLayer::MIDDLE);
      if (auto action = move(pos2, currentPath->getNextNextMove(position))) {
        if (flags.swapPosition || !pos2.getCreature())
          return action.append([path = *currentPath](Creature* c) { c->shortestPath = path; });
        else
          return CreatureAction();
      } else {
        INFO << "Trying to destroy";
        if (!pos2.canEnterEmpty(this) && flags.destroy) {
          if (auto destroyAction = pos2.getBestDestroyAction(getMovementType()))
            if (auto action = destroy(getPosition().getDir(pos2), *destroyAction)) {
              INFO << "Destroying";
              return action.append([path = *currentPath](Creature* c) { c->shortestPath = path; });
            }
          if (auto bridge = pos2.getFurniture(FurnitureLayer::GROUND)->getDefaultBridge())
            if (isAffected(BuffId("BRIDGE_BUILDING_SKILL")))
              if (auto bridgeAction = construct(getPosition().getDir(pos2), *bridge))
                return bridgeAction.append([path = *currentPath](Creature* c) { c->shortestPath = path; });
        }
      }
    } else
      INFO << "Position unreachable";
    shortestPath = none;
    currentPath = none;
    if (wasNew)
      break;
    else
      continue;
  }
  return CreatureAction();
}

CreatureAction Creature::moveAway(Position pos, bool pathfinding) {
  CHECK(pos.isSameLevel(position));
  if (pos.dist8(getPosition()).value_or(6) < 6 && pathfinding)
    if (auto action = moveTowards(pos, true, NavigationFlags().noDestroying()))
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

bool Creature::isUnknownAttacker(const Creature* c) const {
  return unknownAttackers.contains(c);
}

const Vision& Creature::getVision() const {
  return *vision;
}

const CreatureDebt& Creature::getDebt() const {
  return *debt;
}

CreatureDebt& Creature::getDebt() {
  return *debt;
}

Creature::MoveId Creature::getCurrentMoveId() const {
  if (auto model = position.getModel())
    return {model->getMoveCounter(), model->getUniqueId()};
  return {0, 0};
}

const vector<Creature*>& Creature::getVisibleEnemies() const {
  PROFILE;
  auto get = [&] {
    vector<Creature*> ret;
    for (Creature* c : getVisibleCreatures())
      if (isEnemy(c)) {
        ret.push_back(c);
      }
    return ret;
  };
  auto currentMoveId = getCurrentMoveId();
  if (!visibleEnemies || visibleEnemies->first != currentMoveId)
    visibleEnemies.emplace(make_pair(currentMoveId, get()));
  return visibleEnemies->second;
}

const vector<Creature*>& Creature::getVisibleCreatures() const {
  PROFILE;
  auto get = [&] {
    vector<Creature*> ret;
    if (!getGlobalTime())
      return ret;
    auto globalTime = *getGlobalTime();
    if (isAffected(LastingEffect::BLIND)) {
      for (Creature* c : position.getAllCreatures(FieldOfView::sightRange))
        if (canSeeOutsidePosition(c, globalTime) || isUnknownAttacker(c))
          ret.push_back(c);
    } else
      for (Creature* c : position.getAllCreatures(FieldOfView::sightRange))
        if (canSeeIfNotBlind(c, globalTime) || isUnknownAttacker(c)) {
          ret.push_back(c);
        }
    return ret;
  };
  auto currentMoveId = getCurrentMoveId();
  if (!visibleCreatures || visibleCreatures->first != currentMoveId)
    visibleCreatures.emplace(make_pair(currentMoveId, get()));
  return visibleCreatures->second;
}

bool Creature::shouldAIAttack(const Creature* other) const {
  if (isAffected(LastingEffect::PANIC) || getStatus().contains(CreatureStatus::CIVILIAN))
    return false;
  return LastingEffects::doesntMove(other)
      || attributes->getAIType() == AIType::MELEE
      || getDefaultWeaponDamage() > other->getDefaultWeaponDamage();
}

bool Creature::shouldAIChase(const Creature* enemy) const {
  auto factory = getGame()->getContentFactory();
  return !!getFirstWeapon() && getBestAttack(factory).value < 5 * enemy->getBestAttack(factory).value;
}

vector<Position> Creature::getVisibleTiles() const {
  PROFILE;
  if (isAffected(LastingEffect::BLIND))
    return {};
  else
  return getPosition().getVisibleTiles(getVision());
}

void Creature::setGlobalTime(GlobalTime t) {
  if (steed)
    steed->setGlobalTime(t);
  globalTime = t;
}

vector<AdjectiveInfo> Creature::getSpecialAttrAdjectives(const ContentFactory* factory, bool good) const {
  vector<AdjectiveInfo> ret;
  auto withTip = [](string s) {
    return AdjectiveInfo{s, s};
  };
  for (auto& elems : attributes->specialAttr)
    for (auto& elem : elems.second)
      if (elem.first > 0 == good)
        ret.push_back(withTip(
            toStringWithSign(elem.first) + " " + factory->attrInfo.at(elems.first).name + " " + 
                elem.second.getName(factory)));
  for (auto& item : equipment->getAllEquipped())
    for (auto& elem : item->getSpecialModifiers())
      if (elem.second.first > 0 == good)
        ret.push_back(withTip(
            toStringWithSign(elem.second.first) + " " + factory->attrInfo.at(elem.first).name + " " +
                elem.second.second.getName(factory)));
  return ret;
}

vector<AdjectiveInfo> Creature::getLastingEffectAdjectives(const ContentFactory* factory, bool bad) const {
  vector<AdjectiveInfo> ret;
  auto time = getGlobalTime();
  for (auto& buff : buffs) {
    auto& info = factory->buffs.at(buff.first);
    if (info.consideredBad == bad) {
      ret.push_back({ capitalFirst(info.adjective), info.description });
      if (time)
        ret.back().name += "[" + toString(buff.second - *time) + "]";
    }
  }
  for (auto& buff : buffPermanentCount) {
    auto& info = factory->buffs.at(buff.first);
    if (info.consideredBad == bad)
      ret.push_back({ capitalFirst(info.adjective), info.description });
  }
  if (time) {
    for (LastingEffect effect : ENUM_ALL(LastingEffect))
      if (attributes->isAffected(effect, *time))
        if (LastingEffects::isConsideredBad(effect) == bad) {
          auto name = LastingEffects::getAdjective(effect);
          ret.push_back({ name, LastingEffects::getDescription(effect) });
          if (!attributes->isAffectedPermanently(effect))
            ret.back().name += attributes->getRemainingString(effect, *getGlobalTime());
        }
  } else
    for (LastingEffect effect : ENUM_ALL(LastingEffect))
      if (attributes->isAffectedPermanently(effect))
        if (LastingEffects::isConsideredBad(effect) == bad) {
          auto name = LastingEffects::getAdjective(effect);
          ret.push_back({ name, LastingEffects::getDescription(effect) });
        }
  return ret;
}

vector<AdjectiveInfo> Creature::getGoodAdjectives(const ContentFactory* factory) const {
  PROFILE;
  vector<AdjectiveInfo> ret;
  if (getPhylactery())
    ret.push_back({"Bound to a phylactery",
        "The creature will respawn at its phylactery when it is killed"});
  ret.append(getLastingEffectAdjectives(factory, false));
  if (getBody().isUndead(factory))
    ret.push_back({"Undead",
        "Undead creatures don't take regular damage and need to be killed by chopping up or using fire."});
  append(ret, getSpecialAttrAdjectives(factory, true));
  return ret;
}

vector<AdjectiveInfo> Creature::getBadAdjectives(const ContentFactory* factory) const {
  PROFILE;
  vector<AdjectiveInfo> ret;
  getBody().getBadAdjectives(ret);
  ret.append(getLastingEffectAdjectives(factory, true));
  append(ret, getSpecialAttrAdjectives(factory, false));
  return ret;
}


bool Creature::CombatIntentInfo::isHostile() const {
  return type != Creature::CombatIntentInfo::Type::RETREAT;
}

void Creature::addCombatIntent(Creature* attacker, CombatIntentInfo::Type type) {
  if (attacker != this) {
    lastCombatIntent = CombatIntentInfo{type, attacker, *getGlobalTime()};
    if (globalTime && type == CombatIntentInfo::Type::ATTACK && (!attacker->isAffected(LastingEffect::INSANITY) ||
        attacker->getAttributes().isAffectedPermanently(LastingEffect::INSANITY)))
      privateEnemies.set(attacker, *globalTime);
  }
}

optional<Creature::CombatIntentInfo> Creature::getLastCombatIntent() const {
  if (lastCombatIntent && lastCombatIntent->attacker && !lastCombatIntent->attacker->isDead() &&
      !LastingEffects::restrictedMovement(lastCombatIntent->attacker))
    return lastCombatIntent;
  else
    return none;
}

Creature* Creature::getClosestEnemy(bool meleeOnly) const {
  PROFILE;
  int dist = 1000000000;
  Creature* result = nullptr;
  for (Creature* other : getVisibleEnemies()) {
    int curDist = *other->getPosition().dist8(position);
    if (curDist < dist &&
        (!other->getAttributes().dontChase() || curDist == 1) &&
        !other->isAffected(LastingEffect::STUNNED) &&
        (!meleeOnly || other->shouldAIAttack(this))) {
      result = other;
      dist = *position.dist8(other->getPosition());
    }
  }
  return result;
}

void Creature::setPhylactery(Position pos, FurnitureType type) {
  phylactery = PhylacteryInfo{pos, type};
  if (!isSubscribed())
    subscribeTo(pos.getModel());
}

const optional<Creature::PhylacteryInfo>& Creature::getPhylactery() const {
  return phylactery;
}

void Creature::unbindPhylactery() {
  phylactery = none;
}

void Creature::addPromotion(PromotionInfo info) {
  info.applied.apply(position);
  promotions.push_back(info);
}

const vector<PromotionInfo>& Creature::getPromotions() const {
  return promotions;
}

bool Creature::addButcheringEvent(const string& villageName) {
  if (!butcherInfo || butcherInfo->villageName != villageName) {
    butcherInfo = ButcherInfo {villageName, 1};
  }
  else if (++butcherInfo->kills >= 5) {
    attributes->getName().setKillTitle("butcher of " + villageName);
    return true;
  }
  return false;
}

#define CASE(VAR, ELEM, TYPE, ...) \
    case std::remove_reference<decltype(VAR)>::type::TYPE##Tag: {\
      auto ELEM = event.getReferenceMaybe<std::remove_reference<decltype(VAR)>::type::TYPE>();\
      __VA_ARGS__\
      break;\
    }

void Creature::onEvent(const GameEvent& event) {
  if (phylactery)
    switch (event.index) {
      CASE(event, elem, FurnitureRemoved,
        if (elem->position == phylactery->pos && elem->type == phylactery->type) {
          if (elem->destroyedBy)
            phylactery->killedBy = elem->destroyedBy;
          else {
            phylactery = none;
          }
        }
      )
    }
}
