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

CreatureAttributes::CreatureAttributes(function<void(CreatureAttributes&)> fun) {
  fun(*this);
}

CreatureAttributes::~CreatureAttributes() {}

template <class Archive> 
void CreatureAttributes::serialize(Archive& ar, const unsigned int version) {
  ar& SVAR(viewId)
    & SVAR(illusionViewObject)
    & SVAR(spawnType)
    & SVAR(name)
    & SVAR(size)
    & SVAR(attr)
    & SVAR(weight)
    & SVAR(chatReactionFriendly)
    & SVAR(chatReactionHostile)
    & SVAR(firstName)
    & SVAR(speciesName)
    & SVAR(barehandedDamage)
    & SVAR(barehandedAttack)
    & SVAR(attackEffect)
    & SVAR(harmlessApply)
    & SVAR(passiveAttack)
    & SVAR(gender)
    & SVAR(bodyParts)
    & SVAR(injuredBodyParts)
    & SVAR(lostBodyParts)
    & SVAR(innocent)
    & SVAR(uncorporal)
    & SVAR(fireCreature)
    & SVAR(breathing)
    & SVAR(humanoid)
    & SVAR(animal)
    & SVAR(undead)
    & SVAR(notLiving)
    & SVAR(brain)
    & SVAR(isFood)
    & SVAR(stationary)
    & SVAR(noSleep)
    & SVAR(cantEquip)
    & SVAR(courage)
    & SVAR(carryAnything)
    & SVAR(invincible)
    & SVAR(worshipped)
    & SVAR(dontChase)
    & SVAR(isSpecial)
    & SVAR(attributeGain)
    & SVAR(skills)
    & SVAR(spells)
    & SVAR(permanentEffects)
    & SVAR(lastingEffects)
    & SVAR(minionTasks)
    & SVAR(groupName)
    & SVAR(attrIncrease)
    & SVAR(recruitmentCost)
    & SVAR(dyingSound);
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

string CreatureAttributes::getNameAndTitle() const {
  if (firstName)
    return *firstName + " the " + name->bare();
  else if (speciesName)
    return name->bare() + " the " + *speciesName;
  else
    return name->the();
}

string CreatureAttributes::getSpeciesName() const {
  if (speciesName)
    return *speciesName;
  else
    return name->bare();
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
  auto listParts = {BodyPart::ARM, BodyPart::LEG, BodyPart::WING};
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
  if (!dyingSound)
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
  return Sound(dyingSound ? *dyingSound : isHumanoid() ? SoundId::HUMANOID_DEATH : SoundId::BEAST_DEATH)
      .setPitch(getDeathSoundPitch(getSize()));
}

