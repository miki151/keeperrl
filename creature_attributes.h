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
#include "effect.h"
#include "minion_task.h"
#include "item_attributes.h"
#include "minion_task_map.h"
#include "entity_name.h"
#include "spell_map.h"

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
  BACK
);

enum class SpawnType;

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
  optional<ViewId> SERIAL(undeadViewId);
  MustInitialize<EntityName> SERIAL(name);
  optional<string> SERIAL(undeadName);
  EnumMap<AttrType, int> SERIAL(attr);
  MustInitialize<CreatureSize> SERIAL(size);
  MustInitialize<double> SERIAL(weight);
  optional<string> SERIAL(chatReactionFriendly);
  optional<string> SERIAL(chatReactionHostile);
  optional<string> SERIAL(firstName);
  optional<string> SERIAL(speciesName);
  bool SERIAL(specialMonster) = false;
  int SERIAL(barehandedDamage) = 0;
  optional<AttackType> SERIAL(barehandedAttack);
  optional<EffectType> SERIAL(attackEffect);
  bool SERIAL(harmlessApply) = false; // apply the attack effect even if attack was harmless
  optional<EffectType> SERIAL(passiveAttack);
  Gender SERIAL(gender) = Gender::male;
  EnumMap<BodyPart, int> SERIAL(bodyParts) { 
    { BodyPart::ARM, 2},
    { BodyPart::LEG, 2},
    { BodyPart::HEAD, 1}};
  optional<SpawnType> SERIAL(spawnType);
  bool SERIAL(innocent) = false;
  bool SERIAL(uncorporal) = false;
  bool SERIAL(fireCreature) = false;
  bool SERIAL(breathing) = true;
  MustInitialize<bool> SERIAL(humanoid);
  bool SERIAL(animal) = false;
  bool SERIAL(undead) = false;
  bool SERIAL(notLiving) = false;
  bool SERIAL(brain) = true;
  bool SERIAL(isFood) = false;
  bool SERIAL(stationary) = false;
  bool SERIAL(noSleep) = false;
  double SERIAL(courage) = 1;
  bool SERIAL(carryAnything) = false;
  bool SERIAL(invincible) = false;
  bool SERIAL(worshipped) = false;
  bool SERIAL(dontChase) = false;
  double SERIAL(damageMultiplier) = 1;
  double SERIAL(attributeGain) = 0.5;
  Skillset SERIAL(skills);
  SpellMap SERIAL(spells);
  EnumMap<LastingEffect, int> SERIAL(permanentEffects);
  MinionTaskMap SERIAL(minionTasks);
  string SERIAL(groupName) = "group";
};

#endif
