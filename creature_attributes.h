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
#include "gender.h"
#include "creature_name.h"
#include "minion_activity_map.h"
#include "attr_type.h"
#include "lasting_effect.h"
#include "experience_type.h"
#include "game_time.h"
#include "view_id.h"
#include "spell_id.h"
#include "creature_id.h"
#include "spell_school_id.h"
#include "creature_inventory.h"
#include "creature_predicate.h"
#include "ai_type.h"

inline bool isLarger(CreatureSize s1, CreatureSize s2) {
  return int(s1) > int(s2);
}

enum class SpawnType;

#define CATTR(X) CreatureAttributes([&](CreatureAttributes& c) { X })

struct SpellInfo;
class MinionActivityMap;
class Body;
class Effect;
struct AdjectiveInfo;
class ItemType;
struct CompanionInfo;

class CreatureAttributes {
  public:
  CreatureAttributes(function<void(CreatureAttributes&)>);
  ~CreatureAttributes();
  CreatureAttributes(const CreatureAttributes&);
  CreatureAttributes& operator =(const CreatureAttributes&);
  CreatureAttributes(CreatureAttributes&&) noexcept;
  CreatureAttributes& operator =(CreatureAttributes&&);
  SERIALIZATION_DECL(CreatureAttributes)
  template <class Archive>
  void serializeImpl(Archive& ar, const unsigned int);

  CreatureAttributes& setCreatureId(CreatureId);
  const optional<CreatureId>& getCreatureId() const;
  Body& getBody();
  const Body& getBody() const;
  const CreatureName& getName() const;
  CreatureName& getName();
  int getRawAttr(AttrType) const;
  void increaseBaseAttr(AttrType, int);
  void setBaseAttr(AttrType, int);
  void setAIType(AIType);
  AIType getAIType() const;
  string getDeathDescription(const ContentFactory*) const;
  void setDeathDescription(string);
  const Gender& getGender() const;
  double getExpLevel(ExperienceType type) const;
  const EnumMap<ExperienceType, double>& getExpLevel() const;
  const EnumMap<ExperienceType, int>& getMaxExpLevel() const;
  void increaseMaxExpLevel(ExperienceType, int increase);
  void increaseExpLevel(ExperienceType, double increase);
  bool isTrainingMaxedOut(ExperienceType) const;
  void increaseBaseExpLevel(ExperienceType type, int increase);
  vector<SpellSchoolId> getSpellSchools() const;
  void addSpellSchool(SpellSchoolId);
  optional<SoundId> getAttackSound(AttackType, bool damage) const;
  bool isBoulder() const;
  ViewObject createViewObject() const;
  const heap_optional<ViewObject>& getIllusionViewObject() const;
  heap_optional<ViewObject>& getIllusionViewObject();
  bool canEquip() const;
  void chatReaction(Creature* me, Creature* other);
  optional<string> getPetReaction(const Creature* me) const;
  string getDescription(const ContentFactory*) const;
  void add(BodyPart, int count, const ContentFactory*);
  bool isAffected(LastingEffect, GlobalTime) const;
  bool isAffectedPermanently(LastingEffect) const;
  GlobalTime getTimeOut(LastingEffect) const;
  void copyLastingEffects(const CreatureAttributes&);
  string getRemainingString(LastingEffect, GlobalTime) const;
  void clearLastingEffect(LastingEffect);
  void addPermanentEffect(LastingEffect, int count);
  void removePermanentEffect(LastingEffect, int count);
  bool considerTimeout(LastingEffect, GlobalTime current);
  void addLastingEffect(LastingEffect, GlobalTime endtime);
  optional<GlobalTime> getLastAffected(LastingEffect, GlobalTime currentGlobalTime) const;
  bool canSleep() const;
  bool isInnocent() const;
  void consume(Creature* self, CreatureAttributes& other);
  const MinionActivityMap& getMinionActivities() const;
  MinionActivityMap& getMinionActivities();
  bool dontChase() const;
  bool getCanJoinCollective() const;
  void setCanJoinCollective(bool);
  void increaseExpFromCombat(double attackDiff);
  optional<BuffId> getHatedByEffect() const;
  void randomize();
  bool isInstantPrisoner() const;

  friend class ContentFactory;
  friend class CreatureFactory;

  ViewId SERIAL(viewId);
  vector<ViewId> SERIAL(viewIdUpgrades);
  vector<ItemType> SERIAL(automatonParts);
  map<AttrType, vector<pair<int, CreaturePredicate>>> SERIAL(specialAttr);
  heap_optional<Effect> SERIAL(deathEffect);
  heap_optional<Effect> SERIAL(afterKilledSomeone);

  vector<CompanionInfo> SERIAL(companions);
  optional<string> SERIAL(promotionGroup);
  double SERIAL(promotionCost) = 1.0;
  int SERIAL(maxPromotions) = 5;
  vector<BuffId> SERIAL(permanentBuffs);

  private:
  void consumeEffects(Creature* self, const EnumMap<LastingEffect, int>&);
  heap_optional<ViewObject> SERIAL(illusionViewObject);
  CreatureName SERIAL(name);
  map<AttrType, int> SERIAL(attr);
  HeapAllocated<Body> SERIAL(body);
  optional<string> SERIAL(chatReactionFriendly);
  optional<string> SERIAL(chatReactionHostile);
  heap_optional<Effect> SERIAL(chatEffect);
  heap_optional<Effect> SERIAL(passiveAttack);
  Gender SERIAL(gender) = Gender::MALE;
  vector<pair<Gender, ViewId>> SERIAL(genderAlternatives);
  bool SERIAL(cantEquip) = false;
  AIType SERIAL(aiType) = AIType::MELEE;
  bool SERIAL(boulder) = false;
  bool SERIAL(noChase) = false;
  bool SERIAL(isSpecial) = false;
  vector<SpellSchoolId> SERIAL(spellSchools);
  vector<SpellId> SERIAL(spells);
  EnumMap<LastingEffect, int> SERIAL(permanentEffects);
  EnumMap<LastingEffect, GlobalTime> SERIAL(lastingEffects);
  MinionActivityMap SERIAL(minionActivities);
  EnumMap<ExperienceType, double> SERIAL(expLevel);
  EnumMap<ExperienceType, int> SERIAL(maxLevelIncrease);
  bool SERIAL(noAttackSound) = false;
  optional<CreatureId> SERIAL(creatureId);
  optional<string> SERIAL(deathDescription);
  bool SERIAL(canJoinCollective) = true;
  optional<string> SERIAL(petReaction);
  optional<BuffId> SERIAL(hatedByEffect);
  bool SERIAL(instantPrisoner) = false;
  void initializeLastingEffects();
  CreatureInventory SERIAL(inventory);
};
