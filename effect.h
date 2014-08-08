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

#ifndef _EFFECT_H
#define _EFFECT_H

#include "util.h"
#include "enums.h"

class Level;
class Creature;
class Item;
class Tribe;

class Effect {
  public:
  static void applyToCreature(Creature*, EffectType, EffectStrength);
  static void applyToPosition(Level*, Vec2, EffectType, EffectStrength);

  static void summon(Creature*, CreatureId, int num, int ttl);
  static void summon(Level*, CreatureId, Vec2 pos, Tribe*, int num, int ttl);

  template <class Archive>
  static void registerTypes(Archive& ar);
};

enum class EffectStrength { WEAK, NORMAL, STRONG };

enum class EffectType { 
    TELEPORT,
    HEAL,
    SLEEP,
    IDENTIFY,
    PANIC,
    RAGE,
    ROLLING_BOULDER,
    FIRE,
    SLOW,
    SPEED,
    HALLU,
    STR_BONUS,
    DEX_BONUS,
    BLINDNESS,
    INVISIBLE,
    PORTAL,
    DESTROY_EQUIPMENT,
    ENHANCE_ARMOR,
    ENHANCE_WEAPON,
    FIRE_SPHERE_PET,
    GUARDING_BOULDER,
    EMIT_POISON_GAS,
    POISON,
    WORD_OF_POWER,
    DECEPTION,
    SUMMON_INSECTS,
    ACID,
    ALARM,
    TELE_ENEMIES,
    WEB,
    TERROR,
    SUMMON_SPIRIT,
    LEAVE_BODY,
    STUN,
    POISON_RESISTANCE,
    FIRE_RESISTANCE,
    LEVITATION,
    SILVER_DAMAGE,
};

#endif
