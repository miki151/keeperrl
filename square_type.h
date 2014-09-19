#ifndef _SQUARE_TYPE_H
#define _SQUARE_TYPE_H

#include "enums.h"
#include "creature_factory.h"
#include "enum_variant.h"

enum class SquareId {
  FLOOR,
  BRIDGE,
  ROAD,
  GRASS,
  CROPS,
  MUD,
  HELL_WALL,
  ROCK_WALL,
  LOW_ROCK_WALL,
  WOOD_WALL,
  BLACK_WALL,
  YELLOW_WALL,
  CASTLE_WALL,
  WELL,
  STATUE,
  MUD_WALL,
  MOUNTAIN,
  MOUNTAIN2,
  GLACIER,
  HILL,
  WATER,
  MAGMA,
  GOLD_ORE,
  IRON_ORE,
  STONE,
  ABYSS,
  SAND,
  DECID_TREE,
  CANIF_TREE,
  TREE_TRUNK,
  BUSH,
  TORCH,
  DORM,
  BED,
  STOCKPILE,
  STOCKPILE_EQUIP,
  STOCKPILE_RES,
  TORTURE_TABLE,
  IMPALED_HEAD,
  TRAINING_ROOM,
  LIBRARY,
  LABORATORY,
  RITUAL_ROOM,
  CAULDRON,
  BEAST_LAIR,
  BEAST_CAGE,
  WORKSHOP,
  HATCHERY,
  PRISON,
  CEMETERY,
  GRAVE,
  ROLLING_BOULDER,
  POISON_GAS,
  FOUNTAIN,
  CHEST,
  TREASURE_CHEST,
  COFFIN,
  IRON_BARS,
  DOOR,
  TRIBE_DOOR,
  BARRICADE,
  DOWN_STAIRS,
  UP_STAIRS,
  BORDER_GUARD,
  ALTAR,
  CREATURE_ALTAR,
  EYEBALL,
};

typedef EnumVariant<SquareId, TYPES(DeityHabitat, const Creature*, CreatureFactory, const Tribe*),
    ASSIGN(DeityHabitat, SquareId::ALTAR),
    ASSIGN(const Creature*, SquareId::CREATURE_ALTAR),
    ASSIGN(CreatureFactory,
        SquareId::CHEST,
        SquareId::COFFIN,
        SquareId::HATCHERY,
        SquareId::DECID_TREE,
        SquareId::CANIF_TREE,
        SquareId::BUSH),
    ASSIGN(const Tribe*,
        SquareId::TRIBE_DOOR,
        SquareId::BARRICADE)> SquareType;


bool isWall(SquareType);

namespace std {
  template <> struct hash<SquareType> {
    size_t operator()(const SquareType& t) const {
      return (size_t)t.getId();
    }
  };
}

#endif
