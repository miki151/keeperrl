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

#ifndef _ENUMS_H
#define _ENUMS_H

#include "serialization.h"

#define ENUM_HASH(name) \
namespace std { \
  template <> struct hash<name> { \
    size_t operator()(const name& l) const { \
      return (size_t)l; \
    } \
  }; \
}

#define END_CASE(X) case X::ENUM_END: FAIL << "enum end"; break;

typedef int UniqueId;

enum class MsgType {
    FEEL, // better
    BLEEDING_STOPS,
    COLLAPSE,
    FALL,
    PANIC,
    RAGE,
    DIE_OF,
    ARE, // bleeding
    YOUR, // your head is cut off
    FALL_ASLEEP, //
    WAKE_UP,
    DIE, //
    FALL_APART,
    TELE_APPEAR,
    TELE_DISAPPEAR,
    ATTACK_SURPRISE,
    CAN_SEE_HIDING,
    SWING_WEAPON,
    THRUST_WEAPON,
    KICK,
    PUNCH,
    BITE,
    HIT,
    CRAWL,
    TRIGGER_TRAP,
    DROP_WEAPON,
    GET_HIT_NODAMAGE, // body part
    HIT_THROWN_ITEM,
    HIT_THROWN_ITEM_PLURAL,
    MISS_THROWN_ITEM,
    MISS_THROWN_ITEM_PLURAL,
    ITEM_CRASHES,
    ITEM_CRASHES_PLURAL,
    STAND_UP,
    TURN_INVISIBLE,
    TURN_VISIBLE,
    ENTER_PORTAL,
    HAPPENS_TO,
    BURN,
    DROWN,
    SET_UP_TRAP,
    DECAPITATE,
    TURN,
    KILLED_BY,
    BREAK_FREE,
    MISS_ATTACK}; //
enum class BodyPart { HEAD, TORSO, ARM, WING, LEG, BACK,
  ENUM_END};
enum class AttackType { CUT, STAB, CRUSH, PUNCH, BITE, HIT, SHOOT, SPELL};
enum class AttackLevel { LOW, MIDDLE, HIGH };
enum class AttrType { STRENGTH,
  DAMAGE,
  TO_HIT,
  THROWN_DAMAGE,
  THROWN_TO_HIT,
  DEXTERITY,
  DEFENSE,
  SPEED,
  INV_LIMIT,
  WILLPOWER,

  ENUM_END};
enum class ItemType { WEAPON, RANGED_WEAPON, AMMO, ARMOR, SCROLL, POTION, BOOK, AMULET, RING, TOOL, OTHER, GOLD, FOOD,
    CORPSE };
enum class EquipmentSlot {
  WEAPON,
  RANGED_WEAPON,
  HELMET,
  GLOVES,
  BODY_ARMOR,
  BOOTS,
  AMULET,
  RINGS,
};

enum class SquareApplyType { DRINK, USE_CHEST, ASCEND, DESCEND, PRAY, SLEEP, TRAIN, WORKSHOP, TORTURE };

enum class MinionTask { SLEEP, GRAVE, TRAIN, WORKSHOP, STUDY, LABORATORY, PRISON, TORTURE};
enum class TrapType { BOULDER, POISON_GAS, ALARM, WEB, SURPRISE, TERROR };

enum class SquareAttrib;

ENUM_HASH(SquareAttrib);

enum class Dir { N, S, E, W, NE, NW, SE, SW };
ENUM_HASH(Dir);

enum class StairKey { DWARF, CRYPT, GOBLIN, PLAYER_SPAWN, HERO_SPAWN, PYRAMID, TOWER, CASTLE_CELLAR, DRAGON };
enum class StairDirection { UP, DOWN };

enum class CreatureId;


enum class ItemId;

enum class ViewLayer {
  CREATURE,
  LARGE_ITEM,
  ITEM,
  FLOOR,
  FLOOR_BACKGROUND,
};

enum class HighlightType {
  BUILD,
  RECT_SELECTION,
  POISON_GAS,
  FOG,
  MEMORY,
  NIGHT,
  EFFICIENCY,
};

enum class StairLook {
  NORMAL,
  HELL,
  CELLAR,
  PYRAMID,
  DUNGEON_ENTRANCE,
  DUNGEON_ENTRANCE_MUD,
};

enum class SettlementType {
  VILLAGE,
  VILLAGE2,
  CASTLE,
  CASTLE2,
  COTTAGE,
  WITCH_HOUSE,
  MINETOWN,
  VAULT,
};

const static vector<ViewLayer> allLayers =
    {ViewLayer::FLOOR_BACKGROUND, ViewLayer::FLOOR, ViewLayer::ITEM, ViewLayer::LARGE_ITEM, ViewLayer::CREATURE};

ENUM_HASH(ViewLayer);

enum class ViewId;

enum class EffectStrength;
enum class EffectType;

enum class AnimationId {
  EXPLOSION,
};

enum class SpellId {
  HEALING,
  SUMMON_INSECTS,
  DECEPTION,
  SPEED_SELF,
  STR_BONUS,
  DEX_BONUS,
  FIRE_SPHERE_PET,
  TELEPORT,
  INVISIBILITY,
  WORD_OF_POWER,
  SUMMON_SPIRIT,
  PORTAL,
};

enum class QuestId { DRAGON, CASTLE_CELLAR, BANDITS, GOBLINS, DWARVES,
  ENUM_END};

enum class TribeId {
  MONSTER,
  PEST,
  WILDLIFE,
  HUMAN,
  ELVEN,
  DWARVEN,
  GOBLIN,
  PLAYER,
  DRAGON,
  CASTLE_CELLAR,
  BANDIT,
  KILL_EVERYONE,
  PEACEFUL,
  KEEPER,
  LIZARD,

  ENUM_END
};

enum class TechId {
  ALCHEMY,
  ALCHEMY_ADV,
  GOLEM,
  GOLEM_ADV,
  GOLEM_MAS,
  GOBLIN,
  OGRE,
  HUMANOID_MUT,
  BEAST,
  BEAST_MUT,
  NECRO,
  VAMPIRE,
  VAMPIRE_ADV,
  CRAFTING,
  IRON_WORKING,
  TWO_H_WEAP,
  TRAPS,
  ARCHERY,
  SPELLS,
  SPELLS_ADV,
  SPELLS_MAS,

  ENUM_END
};

enum class SkillId {
  AMBUSH,
  KNIFE_THROWING,
  STEALING,
  SWIMMING,
  ARCHERY,
  CONSTRUCTION,
  ELF_VISION,
  NIGHT_VISION,

  ENUM_END
};

enum class VisionId {
  ELF, NIGHT, NORMAL,
  ENUM_END
};

enum class LastingEffect {
    SLEEP,
    PANIC,
    RAGE,
    SLOWED,
    SPEED,
    STR_BONUS,
    DEX_BONUS,
    HALLU,
    BLIND,
    INVISIBLE,
    POISON,
    ENTANGLED,
    STUNNED,
    POISON_RESISTANT,
    FIRE_RESISTANT,
    FLYING,

    ENUM_END
};

enum class DeityHabitat;

#endif
