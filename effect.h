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

class Level;
class Creature;
class Item;
class Tribe;
class CreatureFactory;
class EffectType;
class DirEffectType;


#define EFFECT_TYPE_INTERFACE\
  void applyToCreature(WCreature, WCreature attacker) const;\
  string getName() const;\
  string getDescription() const


#define SIMPLE_EFFECT(Name) \
  struct Name { \
    EFFECT_TYPE_INTERFACE;\
    COMPARE_ALL()\
  }

namespace EffectTypes {
  SIMPLE_EFFECT(Teleport);
  SIMPLE_EFFECT(Heal);
  SIMPLE_EFFECT(Fire);
  SIMPLE_EFFECT(DestroyEquipment);
  SIMPLE_EFFECT(EnhanceArmor);
  SIMPLE_EFFECT(EnhanceWeapon);
  struct EmitPoisonGas {
    EFFECT_TYPE_INTERFACE;
    double amount;
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
      CurePoison, Lasting, PlaceFurniture, Damage, InjureBodyPart, LooseBodyPart, RegrowBodyPart);
}

class EffectType : public EffectTypes::EffectType {
  public:
  using EffectTypes::EffectType::EffectType;
};

enum class DirEffectId {
  BLAST,
  CREATURE_EFFECT,
};

class DirEffectType : public EnumVariant<DirEffectId, TYPES(EffectType),
        ASSIGN(EffectType, DirEffectId::CREATURE_EFFECT)> {
  public:
  template <typename ...Args>
  DirEffectType(int r, Args&&...args) : EnumVariant(std::forward<Args>(args)...), range(r) {}

  int getRange() const {
    return range;
  }

  private:
  int range;
};



class Effect {
  public:
  static void applyToCreature(WCreature, const EffectType&, WCreature attacker = nullptr);
  static void applyDirected(WCreature, Vec2 direction, const DirEffectType&);

  static vector<WCreature> summon(WCreature, CreatureId, int num, int ttl, double delay = 0);
  static vector<WCreature> summon(Position, CreatureFactory&, int num, int ttl, double delay = 0);
  static vector<WCreature> summonCreatures(Position, int radius, vector<PCreature>, double delay = 0);
  static vector<WCreature> summonCreatures(WCreature, int radius, vector<PCreature>, double delay = 0);
  static void emitPoisonGas(Position, double amount, bool msg);
  static string getName(const EffectType&);
  static const char* getName(LastingEffect);
  static string getDescription(const EffectType&);
  static string getDescription(const DirEffectType&);
  static const char* getDescription(LastingEffect);

  template <class Archive>
  static void registerTypes(Archive& ar, int version);
};


