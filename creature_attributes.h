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

#ifndef _CREATURE_ATTRIBUTES_H
#define _CREATURE_ATTRIBUTES_H

#include <string>
#include <functional>

#include "util.h"
#include "skill.h"
#include "gender.h"

// WTF is this defined
#undef HUGE

enum class CreatureSize {
  SMALL,
  MEDIUM,
  LARGE,
  HUGE
};

inline bool isLarger(CreatureSize s1, CreatureSize s2) {
  return int(s1) > int(s2);
}

RICH_ENUM(BodyPart,
  HEAD,
  TORSO,
  ARM,
  WING,
  LEG,
  BACK,
);

RICH_ENUM(LastingEffect,
    SLEEP,
    PANIC,
    RAGE,
    SLOWED,
    SPEED,
    STR_BONUS,
    DEX_BONUS,
    HALLU,
    BLIND,
    INVISIBLE,
    POISON,
    ENTANGLED,
    STUNNED,
    POISON_RESISTANT,
    FIRE_RESISTANT,
    FLYING
);

enum class MinionTask;

#define CATTR(X) CreatureAttributes([&](CreatureAttributes& c) { X })

struct SpellInfo;

class CreatureAttributes {
  public:
  CreatureAttributes(function<void(CreatureAttributes&)> fun) {
    fun(*this);
  }

  CreatureAttributes(const CreatureAttributes& other) = default;
  SERIALIZATION_DECL(CreatureAttributes);

  MustInitialize<ViewId> SERIAL(viewId);
  Optional<ViewId> SERIAL(undeadViewId);
  MustInitialize<string> SERIAL(name);
  Optional<string> SERIAL(undeadName);
  MustInitialize<int> SERIAL(speed);
  MustInitialize<CreatureSize> SERIAL(size);
  MustInitialize<int> SERIAL(strength);
  MustInitialize<int> SERIAL(dexterity);
  int SERIAL2(willpower, 12);
  MustInitialize<double> SERIAL(weight);
  Optional<string> SERIAL(chatReactionFriendly);
  Optional<string> SERIAL(chatReactionHostile);
  Optional<string> SERIAL(firstName);
  Optional<string> SERIAL(speciesName);
  bool SERIAL2(specialMonster, false);
  int SERIAL2(barehandedDamage, 0);
  Optional<AttackType> SERIAL(barehandedAttack);
  Optional<EffectType> SERIAL(attackEffect);
  bool SERIAL2(harmlessApply, false); // apply the attack effect even if attack was harmless
  Optional<EffectType> SERIAL(passiveAttack);
  Gender SERIAL2(gender, Gender::male);
  EnumMap<BodyPart, int> bodyParts { 
    { BodyPart::ARM, 2},
    { BodyPart::LEG, 2},
    { BodyPart::HEAD, 1}};
  SERIAL3(bodyParts);
  bool SERIAL2(canBeMinion, true);
  bool SERIAL2(innocent, false);
  bool SERIAL2(uncorporal, false);
  bool SERIAL2(fireCreature, false);
  bool SERIAL2(breathing, true);
  MustInitialize<bool> SERIAL(humanoid);
  bool SERIAL2(animal, false);
  bool SERIAL2(healer, false);
  bool SERIAL2(undead, false);
  bool SERIAL2(notLiving, false);
  bool SERIAL2(brain, true);
  bool SERIAL2(hatcheryAnimal, false);
  bool SERIAL2(isFood, false);
  bool SERIAL2(stationary, false);
  bool SERIAL2(noSleep, false);
  double SERIAL2(courage, 1);
  int SERIAL2(maxLevel, 10);
  bool SERIAL2(carryAnything, false);
  bool SERIAL2(invincible, false);
  bool SERIAL2(worshipped, false);
  double SERIAL2(damageMultiplier, 1);
  double SERIAL2(attributeGain, 0.5);
  EnumSet<SkillId> SERIAL(skills);
  map<int, Skill*> skillGain {
    {6, Skill::get(SkillId::KNIFE_THROWING)}};
  SERIAL3(skillGain);
  vector<SpellInfo> SERIAL(spells);
  EnumMap<LastingEffect, int> SERIAL(permanentEffects);
  EnumMap<MinionTask, double> SERIAL(minionTasks);
};

#endif
