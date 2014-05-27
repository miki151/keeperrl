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
#include "effect.h"
#include "skill.h"
#include "gender.h"

// WTF is this defined
#undef HUGE

enum class CreatureSize { SMALL, MEDIUM, LARGE, HUGE };

#define CATTR(X) CreatureAttributes([&](CreatureAttributes& c) { X })

class SpellInfo;

class CreatureAttributes {
  public:
  CreatureAttributes(function<void(CreatureAttributes&)> fun) {
    fun(*this);
  }

  CreatureAttributes(const CreatureAttributes& other) = default;
  SERIALIZATION_DECL(CreatureAttributes);

  MustInitialize<ViewId> SERIAL(viewId);
  MustInitialize<string> SERIAL(name);
  MustInitialize<int> SERIAL(speed);
  MustInitialize<CreatureSize> SERIAL(size);
  MustInitialize<int> SERIAL(strength);
  MustInitialize<int> SERIAL(dexterity);
  MustInitialize<int> SERIAL(weight);
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
  int SERIAL2(legs, 2);
  int SERIAL2(arms, 2);
  int SERIAL2(wings, 0);
  int SERIAL2(heads, 1);
  bool SERIAL2(innocent, false);
  bool SERIAL2(uncorporal, false);
  bool SERIAL2(fireResistant, false);
  bool SERIAL2(poisonResistant, false);
  bool SERIAL2(fireCreature, false);
  bool SERIAL2(breathing, true);
  MustInitialize<bool> SERIAL(humanoid);
  bool SERIAL2(animal, false);
  bool SERIAL2(healer, false);
  bool SERIAL2(flyer, false);
  bool SERIAL2(undead, false);
  bool SERIAL2(notLiving, false);
  bool SERIAL2(brain, true);
  bool SERIAL2(walker, true);
  bool SERIAL2(isFood, false);
  bool SERIAL2(stationary, false);
  bool SERIAL2(noSleep, false);
  double SERIAL2(courage, 1);
  int SERIAL2(maxLevel, 10);
  bool SERIAL2(carryAnything, false);
  bool SERIAL2(permanentlyBlind, false);
  bool SERIAL2(invincible, false);
  double SERIAL2(damageMultiplier, 1);
  double SERIAL2(attributeGain, 0.5);
  unordered_set<Skill*> SERIAL(skills);
  map<int, Skill*> skillGain {
    {6, Skill::get(SkillId::KNIFE_THROWING)}};
  SERIAL3(skillGain);
  vector<SpellInfo> SERIAL(spells);
};

#endif
