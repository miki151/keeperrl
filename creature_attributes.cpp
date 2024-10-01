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

#include "creature_attributes.h"
#include "creature.h"
#include "sound.h"
#include "player_message.h"
#include "item.h"
#include "item_factory.h"
#include "body.h"
#include "attack_level.h"
#include "attack_type.h"
#include "view_object.h"
#include "spell_map.h"
#include "effect.h"
#include "minion_trait.h"
#include "view_id.h"
#include "companion_info.h"
#include "game.h"
#include "content_factory.h"

void CreatureAttributes::initializeLastingEffects() {
  for (LastingEffect effect : ENUM_ALL(LastingEffect))
    lastingEffects[effect] = GlobalTime(-500);
}

void CreatureAttributes::randomize() {
  int chosen = Random.get(genderAlternatives.size() + 1);
  if (chosen > 0) {
    gender = genderAlternatives[chosen - 1].first;
    viewId = genderAlternatives[chosen - 1].second;
  }
}

bool CreatureAttributes::isInstantPrisoner() const {
  return instantPrisoner;
}

CreatureAttributes::CreatureAttributes(function<void(CreatureAttributes&)> fun) {
  fun(*this);
  initializeLastingEffects();
}

CreatureAttributes::~CreatureAttributes() {}

CreatureAttributes::CreatureAttributes(const CreatureAttributes&) = default;
CreatureAttributes::CreatureAttributes(CreatureAttributes&&) noexcept = default;
CreatureAttributes& CreatureAttributes::operator =(const CreatureAttributes&) = default;
CreatureAttributes& CreatureAttributes::operator =(CreatureAttributes&&) = default;

template <class Archive>
void CreatureAttributes::serializeImpl(Archive& ar, const unsigned int version) {
  ar(NAMED(viewId), NAMED(illusionViewObject), NAMED(name), NAMED(attr), NAMED(chatReactionFriendly));
  ar(NAMED(chatReactionHostile), NAMED(passiveAttack), OPTION(gender), OPTION(viewIdUpgrades), OPTION(promotionCost));
  ar(NAMED(body), OPTION(deathDescription), NAMED(hatedByEffect), OPTION(instantPrisoner));
  ar(OPTION(cantEquip), OPTION(aiType), OPTION(canJoinCollective), OPTION(genderAlternatives), NAMED(promotionGroup));
  ar(OPTION(boulder), OPTION(noChase), OPTION(isSpecial), OPTION(spellSchools), OPTION(spells));
  ar(SKIP(permanentEffects), OPTION(lastingEffects), OPTION(minionActivities), OPTION(expLevel), OPTION(inventory));
  ar(OPTION(noAttackSound), OPTION(maxLevelIncrease), NAMED(creatureId), NAMED(petReaction));
  ar(OPTION(automatonParts), OPTION(specialAttr), NAMED(deathEffect), NAMED(chatEffect), OPTION(companions));
  ar(OPTION(maxPromotions), OPTION(afterKilledSomeone), SKIP(permanentBuffs), OPTION(killedAchievement));
  ar(OPTION(killedByAchievement), OPTION(steedAchievement), OPTION(fixedAttr), OPTION(grantsExperience));
  if (version >= 1)
    ar(OPTION(noCopulation));
  if (version >= 2) {
    ar(OPTION(copulationEffect));
    ar(OPTION(copulationClientEffect));
  }
  for (auto& a : attr)
    a.second = max(0, a.second);
}

template <class Archive>
void CreatureAttributes::serialize(Archive& ar, const unsigned int version) {
  serializeImpl(ar, version);
}

SERIALIZABLE(CreatureAttributes);

SERIALIZATION_CONSTRUCTOR_IMPL(CreatureAttributes);

CreatureAttributes& CreatureAttributes::setCreatureId(CreatureId id) {
  creatureId = id;
  return *this;
}

const optional<CreatureId>& CreatureAttributes::getCreatureId() const {
  return creatureId;
}

CreatureName& CreatureAttributes::getName() {
  return name;
}

const CreatureName& CreatureAttributes::getName() const {
  return name;
}

void CreatureAttributes::increaseBaseAttr(AttrType type, int v) {
  attr[type] += v;
  attr[type] = max(0, attr[type]);
}

HashMap<AttrType, int>& CreatureAttributes::getAllAttr() {
  return attr;
}

void CreatureAttributes::setBaseAttr(AttrType type, int v) {
  attr[type] = max(0, v);
}

void CreatureAttributes::setAIType(AIType type) {
  aiType = type;
}

AIType CreatureAttributes::getAIType() const {
  return aiType;
}

string CreatureAttributes::getDeathDescription(const ContentFactory* factory) const {
  return deathDescription.value_or(body->getDeathDescription(factory));
}

void CreatureAttributes::setDeathDescription(string c) {
  deathDescription = c;
}

const Gender& CreatureAttributes::getGender() const {
  return gender;
}

int CreatureAttributes::getRawAttr(AttrType type) const {
  int ret = getValueMaybe(attr, type).value_or(0) + getValueMaybe(expLevel, type).value_or(0);
  if (type == AttrType("DEFENSE")) {
    int defense = 0;
    for (auto& elem : expLevel)
      defense = max(defense, (int) elem.second);
    ret += defense;
  }
  return ret;
}

double CreatureAttributes::getExpLevel(AttrType type) const {
  return getValueMaybe(expLevel, type).value_or(0);
}

const HashMap<AttrType, double>& CreatureAttributes::getExpLevel() const {
  return expLevel;
}

const HashMap<AttrType, int>& CreatureAttributes::getMaxExpLevel() const {
  return maxLevelIncrease;
}

void CreatureAttributes::increaseMaxExpLevel(AttrType type, int increase) {
  maxLevelIncrease[type] = max(0, maxLevelIncrease[type] + increase);
  expLevel[type] = min<double>(expLevel[type], maxLevelIncrease[type]);
}

void CreatureAttributes::increaseExpLevel(AttrType type, double increase) {
  increase = max(0.0, min(increase, (double) maxLevelIncrease[type] - expLevel[type]));
  expLevel[type] += increase;
}

bool CreatureAttributes::isTrainingMaxedOut(AttrType type) const {
  return getExpLevel(type) >= getValueMaybe(maxLevelIncrease, type).value_or(0);
}

vector<SpellSchoolId> CreatureAttributes::getSpellSchools() const {
  return spellSchools;
}

void CreatureAttributes::addSpellSchool(SpellSchoolId id) {
  spellSchools.push_back(id);
}

Body& CreatureAttributes::getBody() {
  return *body;
}

const Body& CreatureAttributes::getBody() const {
  return *body;
}

optional<SoundId> CreatureAttributes::getAttackSound(AttackType type, bool damage) const {
  if (!noAttackSound)
    switch (type) {
      case AttackType::HIT:
      case AttackType::CRUSH: return damage ? SoundId("BLUNT_DAMAGE") : SoundId("BLUNT_NO_DAMAGE");
      case AttackType::CUT:
      case AttackType::STAB: return damage ? SoundId("BLADE_DAMAGE") : SoundId("BLADE_NO_DAMAGE");
      default: return none;
    }
  else
    return none;
}

string CreatureAttributes::getDescription(const ContentFactory* factory) const {
  return body->getDescription(factory);
}

void CreatureAttributes::add(BodyPart p, int count, const ContentFactory* factory) {
  for (auto effect : ENUM_ALL(LastingEffect))
    if (body->isIntrinsicallyAffected(effect, factory))
      --permanentEffects[effect];
  body->addWithoutUpdatingPermanentEffects(p, count);
  for (auto effect : ENUM_ALL(LastingEffect))
    if (body->isIntrinsicallyAffected(effect, factory))
      ++permanentEffects[effect];
}

optional<string> CreatureAttributes::getPetReaction(const Creature* me) const {
  if (!petReaction)
    return none;
  if (petReaction->front() == '\"')
    return *petReaction;
  else
    return me->getName().the() + " " + *petReaction;
}

void CreatureAttributes::chatReaction(Creature* me, Creature* other) {
  if (me->isEnemy(other) && chatReactionHostile) {
    if (chatReactionHostile->front() == '\"')
      other->privateMessage(*chatReactionHostile);
    else
      other->privateMessage(me->getName().the() + " " + *chatReactionHostile);
  }
  if (!me->isEnemy(other) && chatReactionFriendly) {
    if (chatReactionFriendly->front() == '\"')
      other->privateMessage(*chatReactionFriendly);
    else
      other->privateMessage(me->getName().the() + " " + *chatReactionFriendly);
  }
  if (chatEffect)
    chatEffect->apply(other->getPosition(), me);
}

bool CreatureAttributes::isAffected(LastingEffect effect, GlobalTime time) const {
  //PROFILE;
  if (auto suppressor = LastingEffects::getSuppressor(effect))
    if (isAffected(*suppressor, time))
      return false;
  return lastingEffects[effect] > time || isAffectedPermanently(effect);
}

GlobalTime CreatureAttributes::getTimeOut(LastingEffect effect) const {
  return lastingEffects[effect];
}

void CreatureAttributes::copyLastingEffects(const CreatureAttributes& attr) {
  lastingEffects = attr.lastingEffects;
  permanentEffects[LastingEffect::STEED] = attr.permanentEffects[LastingEffect::STEED];
}

bool CreatureAttributes::considerTimeout(LastingEffect effect, GlobalTime current) {
  if (lastingEffects[effect] > GlobalTime(0) && lastingEffects[effect] <= current) {
    clearLastingEffect(effect);
    if (!isAffected(effect, current))
      return true;
  }
  return false;
}

void CreatureAttributes::addLastingEffect(LastingEffect effect, GlobalTime endTime) {
  if (lastingEffects[effect] < endTime)
    lastingEffects[effect] = endTime;
}

static bool consumeProb() {
  return true;
}

template <typename T>
void consumeAttr(T& mine, const T& his, vector<string>& adjectives, const string& adj, const int& cap) {
  int hisCapped = (his > cap) ? cap : his;
  if (consumeProb() && mine < hisCapped) {
    mine = hisCapped;
    if (!adj.empty())
      adjectives.push_back(adj);
  }
}

void consumeAttr(Gender& mine, const Gender& his, vector<string>& adjectives) {
  if (consumeProb() && mine != his) {
    mine = his;
    adjectives.emplace_back(get(mine, "more masculine", "more feminine", "more neuter"));
  }
}


template <typename T>
void consumeAttr(heap_optional<T>& mine, const heap_optional<T>& his, vector<string>& adjectives, const string& adj) {
  if (consumeProb() && !mine && his) {
    mine = *his;
    if (!adj.empty())
      adjectives.push_back(adj);
  }
}

void CreatureAttributes::consumeEffects(Creature* self, const EnumMap<LastingEffect, int>& permanentEffects) {
  for (LastingEffect effect : ENUM_ALL(LastingEffect))
    if (permanentEffects[effect] > 0 && !isAffectedPermanently(effect) && consumeProb() &&
        LastingEffects::canConsume(effect)) {
      self->addPermanentEffect(effect);
    }
}

void CreatureAttributes::consume(Creature* self, CreatureAttributes& other) {
  INFO << name.bare() << " consume " << other.name.bare();
  self->you(MsgType::CONSUME, other.name.the());
  self->addPersonalEvent(self->getName().a() + " absorbs " + other.name.a());
  vector<string> adjectives;
  body->consumeBodyParts(self, other.getBody(), adjectives);
  auto factory = self->getGame()->getContentFactory();
  for (auto& t: factory->attrInfo)
    consumeAttr(attr[t.first], other.attr[t.first], adjectives,
      "more " + t.second.adjective, t.second.absorptionCap);
  consumeAttr(passiveAttack, other.passiveAttack, adjectives, "");
  consumeAttr(gender, other.gender, adjectives);
  if (!adjectives.empty()) {
    self->you(MsgType::BECOME, combine(adjectives));
    self->addPersonalEvent(getName().the() + " becomes " + combine(adjectives));
  }
  consumeEffects(self, other.permanentEffects);
}

string CreatureAttributes::getRemainingString(LastingEffect effect, GlobalTime time) const {
  return "[" + toString(lastingEffects[effect] - time) + "]";
}

bool CreatureAttributes::isBoulder() const {
  return boulder;
}

ViewObject CreatureAttributes::createViewObject() const {
  return ViewObject(viewId, ViewLayer::CREATURE, name.bare());
}

const heap_optional<ViewObject>& CreatureAttributes::getIllusionViewObject() const {
  return illusionViewObject;
}

heap_optional<ViewObject>& CreatureAttributes::getIllusionViewObject() {
  return illusionViewObject;
}

bool CreatureAttributes::canEquip() const {
  return !cantEquip;
}

bool CreatureAttributes::isAffectedPermanently(LastingEffect effect) const {
  return permanentEffects[effect] > 0;
}

void CreatureAttributes::clearLastingEffect(LastingEffect effect) {
  lastingEffects[effect] = GlobalTime(0);
}

void CreatureAttributes::addPermanentEffect(LastingEffect effect, int count) {
  permanentEffects[effect] += count;
}

void CreatureAttributes::removePermanentEffect(LastingEffect effect, int count) {
  permanentEffects[effect] -= count;
}

const MinionActivityMap& CreatureAttributes::getMinionActivities() const {
  return minionActivities;
}

MinionActivityMap& CreatureAttributes::getMinionActivities() {
  return minionActivities;
}

bool CreatureAttributes::getCanJoinCollective() const {
  return canJoinCollective;
}

void CreatureAttributes::setCanJoinCollective(bool b) {
  canJoinCollective = b;
}

optional<BuffId> CreatureAttributes::getHatedByEffect() const {
  return hatedByEffect;
}

#include "pretty_archive.h"
template<> void CreatureAttributes::serialize(PrettyInputArchive& ar1, unsigned version) {
  map<LastingOrBuff, int> permanentEffects;
  serializeImpl(ar1, version);
  ar1(OPTION(permanentEffects));
  ar1(endInput());
  for (auto& elem : permanentEffects)
    elem.first.visit(
        [&](LastingEffect e) { this->permanentEffects[e] = elem.second  ; },
        [&](BuffId e) { this->permanentBuffs.push_back(e); }
    );
  initializeLastingEffects();
}
