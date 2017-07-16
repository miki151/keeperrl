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

#pragma once

#include <string>
#include <functional>

#include "util.h"
#include "skill.h"
#include "gender.h"
#include "creature_name.h"
#include "minion_task_map.h"
#include "skill.h"
#include "attr_type.h"
#include "lasting_effect.h"
#include "experience_type.h"

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
struct AdjectiveInfo;

class CreatureAttributes {
  public:
  CreatureAttributes(function<void(CreatureAttributes&)>);
  CreatureAttributes(const CreatureAttributes& other) = default;
  ~CreatureAttributes();
  SERIALIZATION_DECL(CreatureAttributes);

  CreatureAttributes& setCreatureId(CreatureId);
  const optional<CreatureId>& getCreatureId() const;
  Body& getBody();
  const Body& getBody() const;
  const CreatureName& getName() const;
  CreatureName& getName();
  double getRawAttr(WConstCreature c, AttrType) const;
  void setBaseAttr(AttrType, int);
  double getCourage() const;
  void setCourage(double);
  const Gender& getGender() const;
  double getExpLevel(ExperienceType type) const;
  const EnumMap<ExperienceType, double>& getExpLevel() const;
  const EnumMap<ExperienceType, int>& getMaxExpLevel() const;
  void increaseExpLevel(ExperienceType, double increase);
  bool isTrainingMaxedOut(ExperienceType) const;
  void increaseBaseExpLevel(ExperienceType type, double increase);
  string bodyDescription() const;
  SpellMap& getSpellMap();
  const SpellMap& getSpellMap() const;
  optional<SoundId> getAttackSound(AttackType, bool damage) const;
  bool isBoulder() const;
  Skillset& getSkills();
  const Skillset& getSkills() const;
  ViewObject createViewObject() const;
  const optional<ViewObject>& getIllusionViewObject() const;
  bool canEquip() const;
  void chatReaction(WCreature me, WCreature other);
  string getDescription() const;
  bool isAffected(LastingEffect, double globalTime) const;
  bool isAffectedPermanently(LastingEffect) const;
  double getTimeOut(LastingEffect) const;
  string getRemainingString(LastingEffect, double time) const;
  void shortenEffect(LastingEffect, double time);
  void clearLastingEffect(LastingEffect, double globalTime);
  void addPermanentEffect(LastingEffect, int count);
  void removePermanentEffect(LastingEffect, int count);
  bool considerTimeout(LastingEffect, double globalTime);
  void addLastingEffect(LastingEffect, double endtime);
  optional<double> getLastAffected(LastingEffect, double currentGlobalTime) const;
  AttackType getAttackType(WConstItem weapon) const;
  optional<EffectType> getAttackEffect() const;
  bool canSleep() const;
  bool isInnocent() const;
  void consume(WCreature self, const CreatureAttributes& other);
  optional<SpawnType> getSpawnType() const; 
  const MinionTaskMap& getMinionTasks() const;
  MinionTaskMap& getMinionTasks();
  bool dontChase() const;
  optional<ViewId> getRetiredViewId();
  void increaseExpFromCombat(double attackDiff);
  void getGoodAdjectives(vector<AdjectiveInfo>&) const;

  friend class CreatureFactory;

  private:
  void consumeEffects(const EnumMap<LastingEffect, int>&);
  MustInitialize<ViewId> SERIAL(viewId);
  optional<ViewId> SERIAL(retiredViewId);
  HeapAllocated<optional<ViewObject>> SERIAL(illusionViewObject);
  MustInitialize<CreatureName> SERIAL(name);
  EnumMap<AttrType, int> SERIAL(attr);
  HeapAllocated<Body> SERIAL(body);
  optional<string> SERIAL(chatReactionFriendly);
  optional<string> SERIAL(chatReactionHostile);
  optional<AttackType> SERIAL(barehandedAttack);
  HeapAllocated<optional<EffectType>> SERIAL(attackEffect);
  HeapAllocated<optional<EffectType>> SERIAL(passiveAttack);
  Gender SERIAL(gender) = Gender::male;
  optional<SpawnType> SERIAL(spawnType);
  bool SERIAL(innocent) = false;
  bool SERIAL(animal) = false;
  bool SERIAL(cantEquip) = false;
  double SERIAL(courage) = 1;
  bool SERIAL(boulder) = false;
  bool SERIAL(noChase) = false;
  bool SERIAL(isSpecial) = false;
  Skillset SERIAL(skills);
  HeapAllocated<SpellMap> SERIAL(spells);
  EnumMap<LastingEffect, int> SERIAL(permanentEffects);
  EnumMap<LastingEffect, double> SERIAL(lastingEffects);
  EnumMap<LastingEffect, optional<double>> SERIAL(lastAffected);
  MinionTaskMap SERIAL(minionTasks);
  EnumMap<ExperienceType, double> SERIAL(expLevel);
  EnumMap<ExperienceType, int> SERIAL(maxLevelIncrease);
  bool SERIAL(noAttackSound) = false;
  optional<CreatureId> SERIAL(creatureId);
  optional<double> SERIAL(moraleSpeedIncrease);
};
