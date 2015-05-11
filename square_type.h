#ifndef _SQUARE_TYPE_H
#define _SQUARE_TYPE_H

#include "enums.h"
#include "creature_factory.h"
#include "enum_variant.h"
#include "util.h"

RICH_ENUM(SquareId,
  FLOOR,
  BLACK_FLOOR,
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
  BURNT_TREE,
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
  WORKSHOP,
  FORGE,
  LABORATORY,
  JEWELER,
  RITUAL_ROOM,
  CAULDRON,
  BEAST_LAIR,
  BEAST_CAGE,
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
  DOOR,
  TRIBE_DOOR,
  BARRICADE,
  DOWN_STAIRS,
  UP_STAIRS,
  BORDER_GUARD,
  ALTAR,
  CREATURE_ALTAR,
  EYEBALL,
  MINION_STATUE,
  THRONE
);

typedef EnumVariant<SquareId, TYPES(DeityHabitat, const Creature*, CreatureId, CreatureFactory::SingleCreature,
      const Tribe*),
    ASSIGN(DeityHabitat, SquareId::ALTAR),
    ASSIGN(const Creature*, SquareId::CREATURE_ALTAR),
    ASSIGN(CreatureId,
        SquareId::CHEST,
        SquareId::COFFIN,
        SquareId::DECID_TREE,
        SquareId::CANIF_TREE,
        SquareId::BUSH),
    ASSIGN(CreatureFactory::SingleCreature,
        SquareId::HATCHERY),
    ASSIGN(const Tribe*,
        SquareId::TRIBE_DOOR,
        SquareId::BARRICADE)
> SquareType;


bool isWall(SquareType);

namespace std {
  template <> struct hash<SquareType> {
    size_t operator()(const SquareType& t) const {
      return (size_t)t.getId();
    }
  };
}

#endif
