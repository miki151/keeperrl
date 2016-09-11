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
#include "creature_name.h"
#include "minion_task_map.h"
#include "skill.h"
#include "modifier_type.h"
#include "lasting_effect.h"

inline bool isLarger(CreatureSize s1, CreatureSize s2) {
  return int(s1) > int(s2);
}

enum class SpawnType;

#define CATTR(X) CreatureAttributes([&](CreatureAttributes& c) { X })

struct SpellInfo;
class MinionTaskMap;
class SpellMap;
class Body;
class SpellMap;
class EffectType;

class CreatureAttributes {
  public:
  CreatureAttributes(function<void(CreatureAttributes&)>);
  CreatureAttributes(const CreatureAttributes& other) = default;
  ~CreatureAttributes();
  SERIALIZATION_DECL(CreatureAttributes);

  Body& getBody();
  const Body& getBody() const;
  const CreatureName& getName() const;
  CreatureName& getName();
  double getRawAttr(AttrType) const;
  void setBaseAttr(AttrType, int);
  double getCourage() const;
  void setCourage(double);
  const Gender& getGender() const;
  double getExpLevel() const;
  double getBaseExpLevel() const;
  double getVisibleExpLevel() const;
  void increaseExpLevel(double increase);
  string bodyDescription() const;
  SpellMap& getSpellMap();
  const SpellMap& getSpellMap() const;
  optional<SoundId> getAttackSound(AttackType, bool damage) const;
  bool isStationary() const;
  void setStationary(bool);
  bool isInvincible() const;
  int getRecruitmentCost() const;
  Skillset& getSkills();
  const Skillset& getSkills() const;
  ViewObject createViewObject() const;
  const optional<ViewObject>& getIllusionViewObject() const;
  bool canEquip() const;
  void chatReaction(Creature* me, Creature* other);
  string getDescription() const;
  bool isAffected(LastingEffect, double globalTime) const;
  bool isAffectedPermanently(LastingEffect) const;
  string getRemainingString(LastingEffect, double time) const;
  void shortenEffect(LastingEffect, double time);
  void clearLastingEffect(LastingEffect);
  void addPermanentEffect(LastingEffect);
  void removePermanentEffect(LastingEffect);
  bool considerTimeout(LastingEffect, double globalTime);
  bool considerAffecting(LastingEffect, double globalTime, double timeout);
  bool canCarryAnything() const;
  int getBarehandedDamage() const;
  AttackType getAttackType(const Item* weapon) const;
  optional<EffectType> getAttackEffect() const;
  bool canSleep() const;
  bool isInnocent() const;
  void consume(Creature* self, const CreatureAttributes& other);
  optional<SpawnType> getSpawnType() const; 
  const MinionTaskMap& getMinionTasks() const;
  MinionTaskMap& getMinionTasks();
  bool dontChase() const;

  friend class CreatureFactory;

  private:
  void increaseAttr(AttrType, double value);
  void consumeEffects(const EnumMap<LastingEffect, int>&);
  MustInitialize<ViewId> SERIAL(viewId);
  HeapAllocated<optional<ViewObject>> SERIAL(illusionViewObject);
  MustInitialize<CreatureName> SERIAL(name);
  EnumMap<AttrType, int> SERIAL(attr);
  HeapAllocated<Body> SERIAL(body);
  optional<string> SERIAL(chatReactionFriendly);
  optional<string> SERIAL(chatReactionHostile);
  int SERIAL(barehandedDamage) = 0;
  optional<AttackType> SERIAL(barehandedAttack);
  HeapAllocated<optional<EffectType>> SERIAL(attackEffect);
  HeapAllocated<optional<EffectType>> SERIAL(passiveAttack);
  Gender SERIAL(gender) = Gender::male;
  optional<SpawnType> SERIAL(spawnType);
  bool SERIAL(innocent) = false;
  bool SERIAL(animal) = false;
  bool SERIAL(stationary) = false;
  bool SERIAL(cantEquip) = false;
  double SERIAL(courage) = 1;
  bool SERIAL(carryAnything) = false;
  bool SERIAL(invincible) = false;
  bool SERIAL(noChase) = false;
  bool SERIAL(isSpecial) = false;
  int SERIAL(recruitmentCost) = 0;
  Skillset SERIAL(skills);
  HeapAllocated<SpellMap> SERIAL(spells);
  EnumMap<LastingEffect, int> SERIAL(permanentEffects);
  EnumMap<LastingEffect, double> SERIAL(lastingEffects);
  MinionTaskMap SERIAL(minionTasks);
  EnumMap<AttrType, double> SERIAL(attrIncrease);
  bool SERIAL(noAttackSound) = false;
};

#endif
