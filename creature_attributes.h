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
#include "creature_name.h"
#include "view_object.h"
#include "spell_map.h"
#include "minion_task_map.h"
#include "skill.h"
#include "modifier_type.h"
#include "sound.h"

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
class MinionTaskMap;
class SpellMap;

class CreatureAttributes {
  public:
  CreatureAttributes(function<void(CreatureAttributes&)>);
  CreatureAttributes(const CreatureAttributes& other) = default;
  ~CreatureAttributes();
  SERIALIZATION_DECL(CreatureAttributes);

  BodyPart getBodyPart(AttackLevel attack, bool flying, bool collapsed) const;
  CreatureSize getSize() const;
  BodyPart armOrWing() const;
  const CreatureName& getName() const;
  CreatureName& getName();
  double getRawAttr(AttrType) const;
  void setBaseAttr(AttrType, int);
  int numBodyParts(BodyPart) const;
  int numLost(BodyPart) const;
  int lostOrInjuredBodyParts() const;
  int numInjured(BodyPart) const;
  void clearInjured(BodyPart);
  void clearLost(BodyPart);
  int numGood(BodyPart) const;
  double getCourage() const;
  void setCourage(double);
  const Gender& getGender() const;
  bool hasBrain() const;
  double getExpLevel() const;
  void increaseExpLevel(double increase);
  void exerciseAttr(AttrType, double value);
  bool isHumanoid() const;
  vector<AttackLevel> getAttackLevels() const;
  AttackLevel getRandomAttackLevel() const;
  string bodyDescription() const;
  string getBodyPartName(BodyPart) const;
  SpellMap& getSpellMap();
  const SpellMap& getSpellMap() const;
  optional<Sound> getDeathSound() const;
  optional<SoundId> getAttackSound(AttackType, bool damage) const;
  bool isStationary() const;
  void setStationary(bool);
  bool isUndead() const;
  bool isInvincible() const;
  int getRecruitmentCost() const;
  Skillset& getSkills();
  const Skillset& getSkills() const;
  bool isMinionFood() const;
  ViewObject createViewObject() const;
  const optional<ViewObject>& getIllusionViewObject() const;
  bool isFireCreature() const;
  bool canEquip() const;
  void chatReaction(Creature* me, Creature* other);
  string getDescription() const;
  bool isCorporal() const;
  bool isNotLiving() const;
  bool isBreathing() const;
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
  void looseBodyPart(BodyPart);
  void injureBodyPart(BodyPart);
  optional<EffectType> getAttackEffect() const;
  bool canSleep() const;
  bool isInnocent() const;
  void consumeBodyParts(const Creature*, const CreatureAttributes& other);
  void consume(Creature* self, const CreatureAttributes& other);
  optional<SpawnType> getSpawnType() const; 
  const MinionTaskMap& getMinionTasks() const;
  MinionTaskMap& getMinionTasks();
  PItem getBodyPartItem(BodyPart);
  vector<PItem> getCorpseItem(UniqueEntity<Creature>::Id);
  bool dontChase() const;
  double getMinDamage(BodyPart) const;

  friend class CreatureFactory;

  private:
  void consumeEffects(const EnumMap<LastingEffect, int>&);
  MustInitialize<ViewId> SERIAL(viewId);
  optional<ViewObject> SERIAL(illusionViewObject);
  MustInitialize<CreatureName> SERIAL(name);
  EnumMap<AttrType, int> SERIAL(attr);
  MustInitialize<CreatureSize> SERIAL(size);
  MustInitialize<double> SERIAL(weight);
  optional<string> SERIAL(chatReactionFriendly);
  optional<string> SERIAL(chatReactionHostile);
  int SERIAL(barehandedDamage) = 0;
  optional<AttackType> SERIAL(barehandedAttack);
  optional<EffectType> SERIAL(attackEffect);
  optional<EffectType> SERIAL(passiveAttack);
  Gender SERIAL(gender) = Gender::male;
  EnumMap<BodyPart, int> SERIAL(bodyParts) { 
    { BodyPart::ARM, 2},
    { BodyPart::LEG, 2},
    { BodyPart::HEAD, 1}};
  EnumMap<BodyPart, int> SERIAL(injuredBodyParts);
  EnumMap<BodyPart, int> SERIAL(lostBodyParts);
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
  bool SERIAL(cantEquip) = false;
  double SERIAL(courage) = 1;
  bool SERIAL(carryAnything) = false;
  bool SERIAL(invincible) = false;
  bool SERIAL(worshipped) = false;
  bool SERIAL(noChase) = false;
  bool SERIAL(isSpecial) = false;
  double SERIAL(attributeGain) = 0.5;
  int SERIAL(recruitmentCost) = 0;
  Skillset SERIAL(skills);
  SpellMap SERIAL(spells);
  EnumMap<LastingEffect, int> SERIAL(permanentEffects);
  EnumMap<LastingEffect, double> SERIAL(lastingEffects);
  MinionTaskMap SERIAL(minionTasks);
  EnumMap<AttrType, double> SERIAL(attrIncrease);
  optional<SoundId> SERIAL(dyingSound);
  bool SERIAL(noDyingSound) = false;
  bool SERIAL(noAttackSound) = false;
};

#endif
