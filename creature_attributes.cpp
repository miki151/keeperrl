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

CreatureAttributes::CreatureAttributes(function<void(CreatureAttributes&)> fun) {
  fun(*this);
}

CreatureAttributes::~CreatureAttributes() {}

template <class Archive> 
void CreatureAttributes::serialize(Archive& ar, const unsigned int version) {
  serializeAll(ar, viewId, illusionViewObject, spawnType, name, size, attr, weight, chatReactionFriendly);
  serializeAll(ar, chatReactionHostile, barehandedDamage, barehandedAttack, attackEffect, passiveAttack, gender);
  serializeAll(ar, bodyParts, injuredBodyParts, lostBodyParts, innocent, uncorporal, fireCreature, breathing);
  serializeAll(ar, humanoid, animal, undead, notLiving, brain, isFood, stationary, noSleep, cantEquip, courage);
  serializeAll(ar, carryAnything, invincible, noChase, isSpecial, attributeGain, skills, spells);
  serializeAll(ar, permanentEffects, lastingEffects, minionTasks, attrIncrease, recruitmentCost, dyingSound);
  serializeAll(ar, noDyingSound, noAttackSound);
}

SERIALIZABLE(CreatureAttributes);

SERIALIZATION_CONSTRUCTOR_IMPL(CreatureAttributes);

BodyPart CreatureAttributes::getBodyPart(AttackLevel attack, bool flying, bool collapsed) const {
  if (flying)
    return Random.choose({BodyPart::TORSO, BodyPart::HEAD, BodyPart::LEG, BodyPart::WING, BodyPart::ARM},
        {1, 1, 1, 2, 1});
  switch (attack) {
    case AttackLevel::HIGH: 
       return BodyPart::HEAD;
    case AttackLevel::MIDDLE:
       if (getSize() == CreatureSize::SMALL || getSize() == CreatureSize::MEDIUM || collapsed)
         return BodyPart::HEAD;
       else
         return Random.choose({BodyPart::TORSO, armOrWing()}, {1, 1});
    case AttackLevel::LOW:
       if (getSize() == CreatureSize::SMALL || collapsed)
         return Random.choose({BodyPart::TORSO, armOrWing(), BodyPart::HEAD, BodyPart::LEG}, {1, 1, 1, 1});
       if (getSize() == CreatureSize::MEDIUM)
         return Random.choose({BodyPart::TORSO, armOrWing(), BodyPart::LEG}, {1, 1, 3});
       else
         return BodyPart::LEG;
  }
  return BodyPart::ARM;
}

CreatureName& CreatureAttributes::getName() {
  return *name;
}

const CreatureName& CreatureAttributes::getName() const {
  return *name;
}

CreatureSize CreatureAttributes::getSize() const {
  return *size;
}

BodyPart CreatureAttributes::armOrWing() const {
  if (numGood(BodyPart::ARM) == 0)
    return BodyPart::WING;
  if (numGood(BodyPart::WING) == 0)
    return BodyPart::ARM;
  return Random.choose({ BodyPart::WING, BodyPart::ARM }, {1, 1});
}

double CreatureAttributes::getRawAttr(AttrType type) const {
  return attr[type] + attrIncrease[type];
}

void CreatureAttributes::setBaseAttr(AttrType type, int v) {
  attr[type] = v;
}

int CreatureAttributes::numBodyParts(BodyPart part) const {
  return bodyParts[part];
}

int CreatureAttributes::numLost(BodyPart part) const {
  return CHECK_RANGE(lostBodyParts[part], -10000000, 10000000, name->bare());
}

int CreatureAttributes::lostOrInjuredBodyParts() const {
  int ret = 0;
  for (BodyPart part : ENUM_ALL(BodyPart))
    ret += injuredBodyParts[part];
  for (BodyPart part : ENUM_ALL(BodyPart))
    ret += lostBodyParts[part];
  return ret;
}

int CreatureAttributes::numInjured(BodyPart part) const {
  return CHECK_RANGE(injuredBodyParts[part], -10000000, 10000000, name->bare());
}

int CreatureAttributes::numGood(BodyPart part) const {
  return numBodyParts(part) - numInjured(part);
}

void CreatureAttributes::clearInjured(BodyPart part) {
  injuredBodyParts[part] = 0;
}

void CreatureAttributes::clearLost(BodyPart part) {
  lostBodyParts[part] = 0;
}

double CreatureAttributes::getCourage() const {
  if (!hasBrain())
    return 1000;
  return courage;
}

bool CreatureAttributes::hasBrain() const {
  return brain;
}

void CreatureAttributes::setCourage(double c) {
  courage = c;
}

const Gender& CreatureAttributes::getGender() const {
  return gender;
}

double CreatureAttributes::getExpLevel() const {
  vector<pair<AttrType, int>> countAttr {
    {AttrType::STRENGTH, 12},
    {AttrType::DEXTERITY, 12}};
  double sum = 0;
  for (auto elem : countAttr)
    sum += 10.0 * (getRawAttr(elem.first) / elem.second - 1);
  return max(1.0, sum);
}

void CreatureAttributes::increaseExpLevel(double increase) {
  double l = getExpLevel();
  for (int i : Range(100000)) {
    exerciseAttr(Random.choose<AttrType>(), 0.05);
    if (getExpLevel() >= l + increase)
      break;
  }
}

double exerciseMax = 2.0;
double increaseMult = 0.001; // This translates to about 690 stat exercises to reach 50% of the max increase,
                             // and 2300 to reach 90%

void CreatureAttributes::exerciseAttr(AttrType t, double value) {
  attrIncrease[t] += ((exerciseMax - 1) * attr[t] - attrIncrease[t]) * increaseMult * value;
}

vector<AttackLevel> CreatureAttributes::getAttackLevels() const {
  if (isHumanoid() && !numGood(BodyPart::ARM))
    return {AttackLevel::LOW};
  switch (getSize()) {
    case CreatureSize::SMALL: return {AttackLevel::LOW};
    case CreatureSize::MEDIUM: return {AttackLevel::LOW, AttackLevel::MIDDLE};
    case CreatureSize::LARGE: return {AttackLevel::LOW, AttackLevel::MIDDLE, AttackLevel::HIGH};
    case CreatureSize::HUGE: return {AttackLevel::MIDDLE, AttackLevel::HIGH};
  }
}

AttackLevel CreatureAttributes::getRandomAttackLevel() const {
  return Random.choose(getAttackLevels());
}

bool CreatureAttributes::isHumanoid() const {
  return *humanoid;
}

string CreatureAttributes::bodyDescription() const {
  vector<string> ret;
  bool anyLimbs = false;
  vector<BodyPart> listParts = {BodyPart::ARM, BodyPart::LEG, BodyPart::WING};
  if (*humanoid)
    listParts = {BodyPart::WING};
  for (BodyPart part : listParts)
    if (int num = numBodyParts(part)) {
      ret.push_back(getPluralText(getBodyPartName(part), num));
      anyLimbs = true;
    }
  if (*humanoid) {
    bool noArms = numBodyParts(BodyPart::ARM) == 0;
    bool noLegs = numBodyParts(BodyPart::LEG) == 0;
    if (noArms && noLegs)
      ret.push_back("no limbs");
    else if (noArms)
      ret.push_back("no arms");
    else if (noLegs)
      ret.push_back("no legs");
  } else
  if (!anyLimbs)
    ret.push_back("no limbs");
  if (numBodyParts(BodyPart::HEAD) == 0)
    ret.push_back("no head");
  if (ret.size() > 0)
    return " with " + combine(ret);
  else
    return "";
}

string CreatureAttributes::getBodyPartName(BodyPart part) const {
  switch (part) {
    case BodyPart::LEG: return "leg";
    case BodyPart::ARM: return "arm";
    case BodyPart::WING: return "wing";
    case BodyPart::HEAD: return "head";
    case BodyPart::TORSO: return "torso";
    case BodyPart::BACK: return "back";
  }
}

SpellMap& CreatureAttributes::getSpellMap() {
  return spells;
}

const SpellMap& CreatureAttributes::getSpellMap() const {
  return spells;
}

optional<SoundId> CreatureAttributes::getAttackSound(AttackType type, bool damage) const {
  if (!noAttackSound)
    switch (type) {
      case AttackType::HIT:
      case AttackType::PUNCH:
      case AttackType::CRUSH: return damage ? SoundId::BLUNT_DAMAGE : SoundId::BLUNT_NO_DAMAGE;
      case AttackType::CUT:
      case AttackType::STAB: return damage ? SoundId::BLADE_DAMAGE : SoundId::BLADE_NO_DAMAGE;
      default: return none;
    }
  else
    return none;
}

static double getDeathSoundPitch(CreatureSize size) {
  switch (size) {
    case CreatureSize::HUGE: return 0.6;
    case CreatureSize::LARGE: return 0.9;
    case CreatureSize::MEDIUM: return 1.5;
    case CreatureSize::SMALL: return 3.3;
  }
}

optional<Sound> CreatureAttributes::getDeathSound() const {
  if (noDyingSound)
    return none;
  else
    return Sound(dyingSound ? *dyingSound : isHumanoid() ? SoundId::HUMANOID_DEATH : SoundId::BEAST_DEATH)
        .setPitch(getDeathSoundPitch(getSize()));
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

string CreatureAttributes::getDescription() const {
  if (!isSpecial)
    return "";
  string attack;
  if (attackEffect)
    attack = " It has a " + Effect::getName(*attackEffect) + " attack.";
  return adjectives(getSize(), isUndead(), notLiving) +
      (isHumanoid() ? " humanoid" : " beast") + (!isCorporal() ? " spirit" : "") +
      bodyDescription() + ". " + attack;
}

void CreatureAttributes::chatReaction(Creature* me, Creature* other) {
  if (me->isEnemy(other) && chatReactionHostile) {
    if (chatReactionHostile->front() == '\"')
      other->playerMessage(*chatReactionHostile);
    else
      other->playerMessage(me->getName().the() + " " + *chatReactionHostile);
  }
  if (!me->isEnemy(other) && chatReactionFriendly) {
    if (chatReactionFriendly->front() == '\"')
      other->playerMessage(*chatReactionFriendly);
    else
      other->playerMessage(me->getName().the() + " " + *chatReactionFriendly);
  }
}

bool CreatureAttributes::isAffected(LastingEffect effect, double time) const {
  return lastingEffects[effect] >= time || permanentEffects[effect] > 0;
}

bool CreatureAttributes::considerTimeout(LastingEffect effect, double globalTime) {
  if (lastingEffects[effect] > 0 && lastingEffects[effect] < globalTime) {
    clearLastingEffect(effect);
    if (!isAffected(effect, globalTime))
      return true;
  }
  return false;
}
  
bool CreatureAttributes::considerAffecting(LastingEffect effect, double globalTime, double timeout) {
  bool ret = false;
  if (lastingEffects[effect] < globalTime + timeout) {
    ret = !isAffected(effect, globalTime);
    lastingEffects[effect] = globalTime + timeout;
  }
  return ret;
}

void CreatureAttributes::looseBodyPart(BodyPart part) {
  --bodyParts[part];
  ++lostBodyParts[part];
  if (injuredBodyParts[part] > bodyParts[part])
    --injuredBodyParts[part];
}

void CreatureAttributes::injureBodyPart(BodyPart part) {
  if (injuredBodyParts[part] < bodyParts[part])
    ++injuredBodyParts[part];
}

bool CreatureAttributes::canSleep() const {
  return !noSleep;
}

static bool consumeProb() {
  return true;
}

void CreatureAttributes::consumeBodyParts(const Creature* c, const CreatureAttributes& other) {
  for (BodyPart part : ENUM_ALL(BodyPart))
    if (other.bodyParts[part] > bodyParts[part]) {
      if (bodyParts[part] + 1 == other.bodyParts[part])
        c->you(MsgType::GROW, "a " + getBodyPartName(part));
      else
        c->you(MsgType::GROW, toString(other.bodyParts[part] - bodyParts[part]) + " " +
            getBodyPartName(part) + "s");
      bodyParts[part] = other.bodyParts[part];
    }
}

static string getAttrNameMore(AttrType attr) {
  switch (attr) {
    case AttrType::STRENGTH: return "stronger";
    case AttrType::DEXTERITY: return "more agile";
    case AttrType::SPEED: return "faster";
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

void CreatureAttributes::consumeEffects(const EnumMap<LastingEffect, int>& effects) {
  for (LastingEffect effect : ENUM_ALL(LastingEffect))
    if (effects[effect] > 0 && !isAffectedPermanently(effect) && consumeProb()) {
      addPermanentEffect(effect);
    }
}

void CreatureAttributes::consume(Creature* self, const CreatureAttributes& other) {
  Debug() << name->bare() << " consume " << other.name->bare();
  self->you(MsgType::CONSUME, other.name->the());
  consumeBodyParts(self, other);
  if (other.isHumanoid() && !isHumanoid() && numBodyParts(BodyPart::ARM) >= 2 && numBodyParts(BodyPart::LEG) >= 2
      && numBodyParts(BodyPart::HEAD) >= 1) {
    self->you(MsgType::BECOME, "a humanoid");
    self->addPersonalEvent(getName().the() + " turns into a humanoid");
    humanoid = true;
  }
  vector<string> adjectives;
  for (auto t : ENUM_ALL(AttrType))
    consumeAttr(attr[t], other.attr[t], adjectives, getAttrNameMore(t));
  consumeAttr(*size, *other.size, adjectives, "larger");
  consumeAttr(*weight, *other.weight, adjectives, "");
  consumeAttr(barehandedDamage, other.barehandedDamage, adjectives, "more dangerous");
  consumeAttr(barehandedAttack, other.barehandedAttack, adjectives, "");
  consumeAttr(attackEffect, other.attackEffect, adjectives, "");
  consumeAttr(passiveAttack, other.passiveAttack, adjectives, "");
  consumeAttr(gender, other.gender, adjectives);
  consumeAttr(skills, other.skills, adjectives);
  if (!adjectives.empty()) {
    self->you(MsgType::BECOME, combine(adjectives));
    self->addPersonalEvent(getName().the() + " becomes " + combine(adjectives));
  }
  consumeBodyParts(self,other);
  consumeEffects(other.permanentEffects);
}

AttackType CreatureAttributes::getAttackType(const Item* weapon) const {
  if (weapon)
    return weapon->getAttackType();
  else if (barehandedAttack)
    return *barehandedAttack;
  else
    return isHumanoid() ? AttackType::PUNCH : AttackType::BITE;
}

string CreatureAttributes::getRemainingString(LastingEffect effect, double time) const {
  return "[" + toString<int>(lastingEffects[effect] - time) + "]";
}

bool CreatureAttributes::isStationary() const {
  return stationary;
}

void CreatureAttributes::setStationary(bool s) {
  stationary = s;
}

bool CreatureAttributes::isUndead() const {
  return undead;
}

bool CreatureAttributes::isInvincible() const {
  return invincible;
}

int CreatureAttributes::getRecruitmentCost() const {
  return recruitmentCost;
}

Skillset& CreatureAttributes::getSkills() {
  return skills;
}

const Skillset& CreatureAttributes::getSkills() const {
  return skills;
}

bool CreatureAttributes::isMinionFood() const {
  return isFood;
}

ViewObject CreatureAttributes::createViewObject() const {
  return ViewObject(*viewId, ViewLayer::CREATURE, name->bare());
}

const optional<ViewObject>& CreatureAttributes::getIllusionViewObject() const {
  return illusionViewObject;
}

bool CreatureAttributes::isFireCreature() const {
  return fireCreature;
}

bool CreatureAttributes::canEquip() const {
  return !cantEquip;
}

bool CreatureAttributes::isCorporal() const {
  return !uncorporal;
}

bool CreatureAttributes::isNotLiving() const {
  return undead || notLiving || uncorporal;
}

bool CreatureAttributes::isBreathing() const {
  return breathing;
}

bool CreatureAttributes::isAffectedPermanently(LastingEffect effect) const {
  return permanentEffects[effect] > 0;
}

void CreatureAttributes::shortenEffect(LastingEffect effect, double time) {
  CHECK(lastingEffects[effect] >= time);
  lastingEffects[effect] -= time;
}

void CreatureAttributes::clearLastingEffect(LastingEffect effect) {
  lastingEffects[effect] = 0;
}

void CreatureAttributes::addPermanentEffect(LastingEffect effect) {
  ++permanentEffects[effect];
}

void CreatureAttributes::removePermanentEffect(LastingEffect effect) {
  CHECK(--permanentEffects[effect] >= 0);
}

bool CreatureAttributes::canCarryAnything() const {
  return carryAnything;
}

int CreatureAttributes::getBarehandedDamage() const {
  return barehandedDamage;
}

optional<EffectType> CreatureAttributes::getAttackEffect() const {
  return attackEffect;
}

bool CreatureAttributes::isInnocent() const {
  return innocent;
}

optional<SpawnType> CreatureAttributes::getSpawnType() const {
  return spawnType;
}
 
const MinionTaskMap& CreatureAttributes::getMinionTasks() const {
  return minionTasks;
}

MinionTaskMap& CreatureAttributes::getMinionTasks() {
  return minionTasks;
}

static string getBodyPartBone(BodyPart part) {
  switch (part) {
    case BodyPart::HEAD: return "skull";
    default: return "bone";
  }
}

PItem CreatureAttributes::getBodyPartItem(BodyPart part) {
  return ItemFactory::corpse(getName().bare() + " " + getBodyPartName(part),
        getName().bare() + " " + getBodyPartBone(part),
        *weight / 8, isMinionFood() ? ItemClass::FOOD : ItemClass::CORPSE);
}

vector<PItem> CreatureAttributes::getCorpseItem(Creature::Id id) {
  return makeVec<PItem>(
      ItemFactory::corpse(getName().bare() + " corpse", getName().bare() + " skeleton", *weight,
        isMinionFood() ? ItemClass::FOOD : ItemClass::CORPSE,
        {id, true, numBodyParts(BodyPart::HEAD) > 0, false}));
}

bool CreatureAttributes::dontChase() const {
  return noChase;
}

double CreatureAttributes::getMinDamage(BodyPart part) const {
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


