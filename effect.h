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

#include "util.h"
#include "enums.h"
#include "position.h"
#include "enum_variant.h"
#include "game_time.h"
#include "fx_name.h"
#include "furniture_type.h"
#include "effect_ai_intent.h"
#include "health_type.h"
#include "creature_id.h"

class Level;
class Creature;
class Item;
class Tribe;
class CreatureGroup;


RICH_ENUM(FilterType, ALLY, ENEMY);

#define EFFECT_TYPE_INTERFACE \
  void applyToCreature(Creature*, Creature* attacker = nullptr) const;\
  string getName() const;\
  string getDescription() const


#define SIMPLE_EFFECT(Name) \
  struct Name { \
    EFFECT_TYPE_INTERFACE;\
    COMPARE_ALL()\
  }

class Effect {
  public:
  SIMPLE_EFFECT(Escape);
  SIMPLE_EFFECT(Teleport);
  struct Heal {
    EFFECT_TYPE_INTERFACE;
    HealthType healthType;
    COMPARE_ALL(healthType)
  };
  SIMPLE_EFFECT(Fire);
  SIMPLE_EFFECT(Ice);
  SIMPLE_EFFECT(DestroyEquipment);
  SIMPLE_EFFECT(DestroyWalls);
  SIMPLE_EFFECT(EnhanceArmor);
  SIMPLE_EFFECT(EnhanceWeapon);
  struct EmitPoisonGas {
    EFFECT_TYPE_INTERFACE;
    double amount = 0.8;
    COMPARE_ALL(amount)
  };
  SIMPLE_EFFECT(CircularBlast);
  SIMPLE_EFFECT(Deception);
  struct Summon {
    EFFECT_TYPE_INTERFACE;
    Summon(CreatureId id, Range c) : creature(id), count(c) {}
    Summon() {}
    CreatureId creature;
    Range count;
    COMPARE_ALL(creature, count)
  };
  SIMPLE_EFFECT(SummonElement);
  SIMPLE_EFFECT(Acid);
  struct Alarm {
    EFFECT_TYPE_INTERFACE;
    bool silent = false;
    COMPARE_ALL(silent)
  };
  SIMPLE_EFFECT(TeleEnemies);
  SIMPLE_EFFECT(SilverDamage);

  struct Lasting {
    EFFECT_TYPE_INTERFACE;
    LastingEffect lastingEffect;
    COMPARE_ALL(lastingEffect)
  };

  struct RemoveLasting {
    EFFECT_TYPE_INTERFACE;
    LastingEffect lastingEffect;
    COMPARE_ALL(lastingEffect)
  };

  struct Permanent {
    EFFECT_TYPE_INTERFACE;
    LastingEffect lastingEffect;
    COMPARE_ALL(lastingEffect)
  };
  struct PlaceFurniture {
    EFFECT_TYPE_INTERFACE;
    FurnitureType furniture;
    COMPARE_ALL(furniture)
  };
  struct Damage {
    EFFECT_TYPE_INTERFACE;
    AttrType attr;
    AttackType attackType;
    COMPARE_ALL(attr, attackType)
  };
  struct IncreaseAttr {
    EFFECT_TYPE_INTERFACE;
    AttrType attr;
    int amount;
    const char* get(const char* ifIncrease, const char* ifDecrease) const;
    COMPARE_ALL(attr, amount)
  };
  struct InjureBodyPart {
    EFFECT_TYPE_INTERFACE;
    BodyPart part;
    COMPARE_ALL(part)
  };
  struct LooseBodyPart {
    EFFECT_TYPE_INTERFACE;
    BodyPart part;
    COMPARE_ALL(part)
  };
  struct Area {
    EFFECT_TYPE_INTERFACE;
    int radius;
    HeapAllocated<Effect> effect;
    COMPARE_ALL(radius, effect)
  };
  struct CustomArea {
    EFFECT_TYPE_INTERFACE;
    HeapAllocated<Effect> effect;
    vector<Vec2> positions;
    vector<Position> getTargetPos(const Creature* attacker, Position targetPos) const;
    COMPARE_ALL(effect, positions)
  };
  SIMPLE_EFFECT(RegrowBodyPart);
  SIMPLE_EFFECT(Suicide);
  SIMPLE_EFFECT(DoubleTrouble);
  SIMPLE_EFFECT(Blast);
  SIMPLE_EFFECT(Pull);
  SIMPLE_EFFECT(Shove);
  SIMPLE_EFFECT(SwapPosition);
  struct ReviveCorpse {
    EFFECT_TYPE_INTERFACE;
    vector<CreatureId> summoned;
    int ttl;
    COMPARE_ALL(summoned, ttl)
  };
  struct SummonGhost {
    EFFECT_TYPE_INTERFACE;
    optional<TimeInterval> ttl;
    Range count;
    int ghostPower;
    PCreature getBestSpirit(const Model*, TribeId tribe) const;
    COMPARE_ALL(ttl, count, ghostPower)
  };
  struct Filter {
    EFFECT_TYPE_INTERFACE;
    bool applies(const Creature* victim, const Creature* attacker) const;
    FilterType filter;
    HeapAllocated<Effect> effect;
    COMPARE_ALL(filter, effect)
  };

/*  struct Chain {
    EFFECT_TYPE_INTERFACE;
    vector<Effect> effects;
    COMPARE_ALL(effects)
  };*/
  MAKE_VARIANT(EffectType, Escape, Teleport, Heal, Fire, Ice, DestroyEquipment, EnhanceArmor, EnhanceWeapon, Suicide, IncreaseAttr,
      EmitPoisonGas, CircularBlast, Deception, Summon, SummonElement, Acid, Alarm, TeleEnemies, SilverDamage, DoubleTrouble,
      Lasting, RemoveLasting, Permanent, PlaceFurniture, Damage, InjureBodyPart, LooseBodyPart, RegrowBodyPart, DestroyWalls,
      Area, CustomArea, ReviveCorpse, Blast, Pull, Shove, SwapPosition, SummonGhost, Filter);

  template <typename T>
  Effect(T&& t) : effect(std::forward<T>(t)) {}
  Effect(const Effect&) = default;
  Effect(Effect&) = default;
  Effect(Effect&&) = default;
  Effect();
  Effect& operator = (const Effect&) = default;
  Effect& operator = (Effect&&) = default;

  bool operator == (const Effect&) const;
  bool operator != (const Effect&) const;
  template <class Archive>
  void serialize(Archive&, const unsigned int);

  void apply(Position, Creature* attacker = nullptr) const;
  string getName() const;
  string getDescription() const;

  EffectAIIntent shouldAIApply(const Creature* caster, Position) const;

  template <typename... Args>
  auto visit(Args&&...args) {
    return effect.visit(std::forward<Args>(args)...);
  }

  template <typename... Args>
  auto visit(Args&&...args) const {
    return effect.visit(std::forward<Args>(args)...);
  }

  template <typename T>
  bool isType() const {
    return effect.contains<T>();
  }

  template <typename T>
  auto getValueMaybe() const {
    return effect.getValueMaybe<T>();
  }

  static vector<Creature*> summon(Creature*, CreatureId, int num, optional<TimeInterval> ttl, TimeInterval delay = 0_visible);
  static vector<Creature*> summon(Position, CreatureGroup&, int num, optional<TimeInterval> ttl, TimeInterval delay = 0_visible);
  static vector<Creature*> summonCreatures(Position, int radius, vector<PCreature>, TimeInterval delay = 0_visible);
  static void emitPoisonGas(Position, double amount, bool msg);

  private:
  EffectAIIntent shouldAIApply(const Creature* victim, bool isEnemy) const;
  EffectType SERIAL(effect);
};
