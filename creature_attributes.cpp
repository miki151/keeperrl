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

CreatureAttributes::CreatureAttributes(function<void(CreatureAttributes&)> fun) {
  fun(*this);
  for (LastingEffect effect : ENUM_ALL(LastingEffect))
    lastingEffects[effect] = GlobalTime(-500);
  for (auto effect : ENUM_ALL(LastingEffect))
    if (body->isIntrinsicallyAffected(effect))
      ++permanentEffects[effect];
}

CreatureAttributes::~CreatureAttributes() {}

template <class Archive> 
void CreatureAttributes::serialize(Archive& ar, const unsigned int version) {
  ar(viewId, retiredViewId, illusionViewObject, name, attr, chatReactionFriendly);
  ar(chatReactionHostile,passiveAttack, gender);
  ar(body, innocent, moraleSpeedIncrease, deathDescription);
  ar(animal, cantEquip, courage);
  ar(boulder, noChase, isSpecial, skills, spells);
  ar(permanentEffects, lastingEffects, minionTasks, expLevel);
  ar(noAttackSound, maxLevelIncrease, creatureId);
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
  return *name;
}

const CreatureName& CreatureAttributes::getName() const {
  return *name;
}

void CreatureAttributes::setBaseAttr(AttrType type, int v) {
  attr[type] = v;
}

double CreatureAttributes::getCourage() const {
  if (!body->hasBrain())
    return 1;
  return courage;
}

void CreatureAttributes::setCourage(double c) {
  courage = c;
}

string CreatureAttributes::getDeathDescription() const {
  return deathDescription;
}

void CreatureAttributes::setDeathDescription(string c) {
  deathDescription = c;
}

const Gender& CreatureAttributes::getGender() const {
  return gender;
}

double CreatureAttributes::getRawAttr(AttrType type) const {
  double ret = attr[type];
  if (auto expType = getExperienceType(type))
    ret += expLevel[*expType];
  return ret;
}

double CreatureAttributes::getExpLevel(ExperienceType type) const {
  return expLevel[type];
}

const EnumMap<ExperienceType, double>& CreatureAttributes::getExpLevel() const {
  return expLevel;
}

const EnumMap<ExperienceType, int>& CreatureAttributes::getMaxExpLevel() const {
  return maxLevelIncrease;
}

void CreatureAttributes::increaseExpLevel(ExperienceType type, double increase) {
  increase = max(0.0, min(increase, (double) maxLevelIncrease[type] - expLevel[type]));
  expLevel[type] += increase;
}

bool CreatureAttributes::isTrainingMaxedOut(ExperienceType type) const {
  return getExpLevel(type) >= maxLevelIncrease[type];
}

void CreatureAttributes::increaseBaseExpLevel(ExperienceType type, double increase) {
  for (auto attrType : getAttrIncreases()[type])
    attr[attrType] += increase;
}

SpellMap& CreatureAttributes::getSpellMap() {
  return *spells;
}

Body& CreatureAttributes::getBody() {
  return *body;
}

const Body& CreatureAttributes::getBody() const {
  return *body;
}

const SpellMap& CreatureAttributes::getSpellMap() const {
  return *spells;
}

optional<SoundId> CreatureAttributes::getAttackSound(AttackType type, bool damage) const {
  if (!noAttackSound)
    switch (type) {
      case AttackType::HIT:
      case AttackType::CRUSH: return damage ? SoundId::BLUNT_DAMAGE : SoundId::BLUNT_NO_DAMAGE;
      case AttackType::CUT:
      case AttackType::STAB: return damage ? SoundId::BLADE_DAMAGE : SoundId::BLADE_NO_DAMAGE;
      default: return none;
    }
  else
    return none;
}

string CreatureAttributes::getDescription() const {
  if (!isSpecial)
    return "";
  string attack;
  return body->getDescription() + ". " + attack;
}

void CreatureAttributes::chatReaction(WCreature me, WCreature other) {
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
}

bool CreatureAttributes::isAffected(LastingEffect effect, GlobalTime time) const {
  if (auto suppressor = LastingEffects::getSuppressor(effect))
    if (isAffected(*suppressor, time))
      return false;
  return lastingEffects[effect] > time || isAffectedPermanently(effect);
}

GlobalTime CreatureAttributes::getTimeOut(LastingEffect effect) const {
  return lastingEffects[effect];
}

bool CreatureAttributes::considerTimeout(LastingEffect effect, GlobalTime current) {
  if (lastingEffects[effect] > GlobalTime(0) && lastingEffects[effect] <= current) {
    clearLastingEffect(effect, current);
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

static string getAttrNameMore(AttrType attr) {
  switch (attr) {
    case AttrType::DAMAGE: return "more dangerous";
    case AttrType::DEFENSE: return "more protected";
    case AttrType::SPELL_DAMAGE: return "more powerful";
    case AttrType::RANGED_DAMAGE: return "more accurate";
  }
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

void CreatureAttributes::consumeEffects(const EnumMap<LastingEffect, int>& permanentEffects) {
  for (LastingEffect effect : ENUM_ALL(LastingEffect))
    if (permanentEffects[effect] > 0 && !isAffectedPermanently(effect) && consumeProb()) {
      addPermanentEffect(effect, 1);
    }
}

void CreatureAttributes::consume(WCreature self, CreatureAttributes& other) {
  INFO << name->bare() << " consume " << other.name->bare();
  self->you(MsgType::CONSUME, other.name->the());
  self->addPersonalEvent(self->getName().a() + " absorbs " + other.name->a());
  vector<string> adjectives;
  body->consumeBodyParts(self, other.getBody(), adjectives);
  for (auto t : ENUM_ALL(AttrType))
    consumeAttr(attr[t], other.attr[t], adjectives, getAttrNameMore(t));
  consumeAttr(*passiveAttack, *other.passiveAttack, adjectives, "");
  consumeAttr(gender, other.gender, adjectives);
  consumeAttr(skills, other.skills, adjectives);
  if (!adjectives.empty()) {
    self->you(MsgType::BECOME, combine(adjectives));
    self->addPersonalEvent(getName().the() + " becomes " + combine(adjectives));
  }
  consumeEffects(other.permanentEffects);
}

string CreatureAttributes::getRemainingString(LastingEffect effect, GlobalTime time) const {
  return "[" + toString(lastingEffects[effect] - time) + "]";
}

bool CreatureAttributes::isBoulder() const {
  return boulder;
}

Skillset& CreatureAttributes::getSkills() {
  return skills;
}

const Skillset& CreatureAttributes::getSkills() const {
  return skills;
}

ViewObject CreatureAttributes::createViewObject() const {
  return ViewObject(*viewId, ViewLayer::CREATURE, name->bare());
}

const optional<ViewObject>& CreatureAttributes::getIllusionViewObject() const {
  return *illusionViewObject;
}

bool CreatureAttributes::canEquip() const {
  return !cantEquip;
}

bool CreatureAttributes::isAffectedPermanently(LastingEffect effect) const {
  return permanentEffects[effect] > 0;
}

void CreatureAttributes::clearLastingEffect(LastingEffect effect, GlobalTime t) {
  lastingEffects[effect] = GlobalTime(0);
}

void CreatureAttributes::addPermanentEffect(LastingEffect effect, int count) {
  permanentEffects[effect] += count;
}

void CreatureAttributes::removePermanentEffect(LastingEffect effect, int count) {
  permanentEffects[effect] -= count;
}

bool CreatureAttributes::isInnocent() const {
  return innocent;
}
 
const MinionTaskMap& CreatureAttributes::getMinionTasks() const {
  return minionTasks;
}

MinionTaskMap& CreatureAttributes::getMinionTasks() {
  return minionTasks;
}

bool CreatureAttributes::dontChase() const {
  return noChase;
}

optional<ViewId> CreatureAttributes::getRetiredViewId() {
  return retiredViewId;
}

optional<double> CreatureAttributes::getMoraleSpeedIncrease() const {
  return moraleSpeedIncrease;
}
