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

class Level;
class Creature;
class Item;
class Tribe;
class CreatureFactory;
class DirEffectType;


#define EFFECT_TYPE_INTERFACE \
  void applyToCreature(WCreature, WCreature attacker = nullptr) const;\
  string getName() const;\
  string getDescription() const


#define SIMPLE_EFFECT(Name) \
  struct Name { \
    EFFECT_TYPE_INTERFACE;\
    COMPARE_ALL()\
  }

class Effect {
  public:
  SIMPLE_EFFECT(Teleport);
  SIMPLE_EFFECT(Heal);
  SIMPLE_EFFECT(Fire);
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
    CreatureId creature;
    COMPARE_ALL(creature)
  };
  SIMPLE_EFFECT(SummonElement);
  SIMPLE_EFFECT(Acid);
  SIMPLE_EFFECT(Alarm);
  SIMPLE_EFFECT(TeleEnemies);
  SIMPLE_EFFECT(SilverDamage);
  SIMPLE_EFFECT(CurePoison);
  struct Lasting {
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
  SIMPLE_EFFECT(RegrowBodyPart);

  MAKE_VARIANT(EffectType, Teleport, Heal, Fire, DestroyEquipment, EnhanceArmor, EnhanceWeapon,
      EmitPoisonGas, CircularBlast, Deception, Summon, SummonElement, Acid, Alarm, TeleEnemies, SilverDamage,
      CurePoison, Lasting, PlaceFurniture, Damage, InjureBodyPart, LooseBodyPart, RegrowBodyPart, DestroyWalls);

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

  void applyToCreature(WCreature, WCreature attacker = nullptr) const;
  string getName() const;
  string getDescription() const;

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

  static vector<WCreature> summon(WCreature, CreatureId, int num, TimeInterval ttl, TimeInterval delay = 0_visible);
  static vector<WCreature> summon(Position, CreatureFactory&, int num, TimeInterval ttl, TimeInterval delay = 0_visible);
  static vector<WCreature> summonCreatures(Position, int radius, vector<PCreature>, TimeInterval delay = 0_visible);
  static vector<WCreature> summonCreatures(WCreature, int radius, vector<PCreature>, TimeInterval delay = 0_visible);
  static void emitPoisonGas(Position, double amount, bool msg);

  private:
  EffectType SERIAL(effect);
};


enum class DirEffectId {
  BLAST,
  CREATURE_EFFECT,
};

class DirEffectType : public EnumVariant<DirEffectId, TYPES(Effect),
        ASSIGN(Effect, DirEffectId::CREATURE_EFFECT)> {
  public:
  template <typename ...Args>
  DirEffectType(int r, Args&&...args) : EnumVariant(std::forward<Args>(args)...), range(r) {}

  int getRange() const {
    return range;
  }

  private:
  int range;
};

extern string getDescription(const DirEffectType&);
extern void applyDirected(WCreature, Vec2 direction, const DirEffectType&);
