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
#include "minion_activity_map.h"
#include "attr_type.h"
#include "lasting_effect.h"
#include "experience_type.h"
#include "game_time.h"

inline bool isLarger(CreatureSize s1, CreatureSize s2) {
  return int(s1) > int(s2);
}

enum class SpawnType;

#define CATTR(X) CreatureAttributes([&](CreatureAttributes& c) { X })

struct SpellInfo;
class MinionActivityMap;
class SpellMap;
class Body;
class SpellMap;
class Effect;
struct AdjectiveInfo;

class CreatureAttributes {
  public:
  CreatureAttributes(function<void(CreatureAttributes&)>);
  CreatureAttributes(CreatureAttributes&& other) = default;
  ~CreatureAttributes();
  SERIALIZATION_DECL(CreatureAttributes)

  CreatureAttributes& setCreatureId(CreatureId);
  const optional<CreatureId>& getCreatureId() const;
  Body& getBody();
  const Body& getBody() const;
  const CreatureName& getName() const;
  CreatureName& getName();
  int getRawAttr(AttrType) const;
  void setBaseAttr(AttrType, int);
  void sufferScar(AttrType type);
  double getCourage() const;
  void setCourage(double);
  string getDeathDescription() const;
  void setDeathDescription(string);
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
  optional<ViewObject>& getIllusionViewObject();
  bool canEquip() const;
  void chatReaction(WCreature me, WCreature other);
  string getDescription() const;
  bool isAffected(LastingEffect, GlobalTime) const;
  bool isAffectedPermanently(LastingEffect) const;
  GlobalTime getTimeOut(LastingEffect) const;
  string getRemainingString(LastingEffect, GlobalTime) const;
  void clearLastingEffect(LastingEffect, GlobalTime);
  void addPermanentEffect(LastingEffect, int count);
  void removePermanentEffect(LastingEffect, int count);
  bool considerTimeout(LastingEffect, GlobalTime current);
  void addLastingEffect(LastingEffect, GlobalTime endtime);
  optional<GlobalTime> getLastAffected(LastingEffect, GlobalTime currentGlobalTime) const;
  bool canSleep() const;
  bool isInnocent() const;
  void consume(WCreature self, CreatureAttributes& other);
  const MinionActivityMap& getMinionActivities() const;
  MinionActivityMap& getMinionActivities();
  bool dontChase() const;
  bool getCanJoinCollective() const;
  optional<ViewId> getRetiredViewId();
  void increaseExpFromCombat(double attackDiff);

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
  HeapAllocated<optional<Effect>> SERIAL(passiveAttack);
  Gender SERIAL(gender) = Gender::male;
  bool SERIAL(animal) = false;
  bool SERIAL(cantEquip) = false;
  double SERIAL(courage) = 1;
  bool SERIAL(boulder) = false;
  bool SERIAL(noChase) = false;
  bool SERIAL(isSpecial) = false;
  Skillset SERIAL(skills);
  HeapAllocated<SpellMap> SERIAL(spells);
  EnumMap<LastingEffect, int> SERIAL(permanentEffects);
  EnumMap<LastingEffect, GlobalTime> SERIAL(lastingEffects);
  MinionActivityMap SERIAL(minionActivities);
  EnumMap<ExperienceType, double> SERIAL(expLevel);
  EnumMap<ExperienceType, int> SERIAL(maxLevelIncrease);
  bool SERIAL(noAttackSound) = false;
  optional<CreatureId> SERIAL(creatureId);
  string SERIAL(deathDescription) = "killed"_s;
  bool SERIAL(canJoinCollective) = true;
};
