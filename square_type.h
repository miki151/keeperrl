#ifndef _SQUARE_TYPE_H
#define _SQUARE_TYPE_H

#include "enums.h"
#include "creature_factory.h"
#include "enum_variant.h"
#include "util.h"
#include "stair_key.h"

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
  WHIPPING_POST,
  NOTICE_BOARD,
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
  STAIRS,
  BORDER_GUARD,
  ALTAR,
  CREATURE_ALTAR,
  EYEBALL,
  MINION_STATUE,
  THRONE
);

enum class StairLook {
  NORMAL,
  HELL,
  CELLAR,
  PYRAMID,
  DUNGEON_ENTRANCE,
  DUNGEON_ENTRANCE_MUD,
};

struct StairInfo {
  StairKey SERIAL(key);
  StairLook SERIAL(look);
  enum Direction { UP, DOWN } SERIAL(direction);
  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar & SVAR(key) & SVAR(look) & SVAR(direction);
  }
  bool operator == (const StairInfo& o) const {
    return key == o.key && look == o.look && direction == o.direction;
  }
};

struct ChestInfo {
  ChestInfo(CreatureFactory::SingleCreature c, double p, int n) : creature(c), creatureChance(p), numCreatures(n) {}
  ChestInfo() : creatureChance(0), numCreatures(0) {}
  optional<CreatureFactory::SingleCreature> SERIAL(creature);
  double SERIAL(creatureChance);
  int SERIAL(numCreatures);
  template <class Archive>
  void serialize(Archive& ar, const unsigned int version) {
    ar & SVAR(creature) & SVAR(creatureChance) & SVAR(numCreatures);
  }
  bool operator == (const ChestInfo& o) const {
    return creature == o.creature && creatureChance == o.creatureChance && numCreatures == o.numCreatures;
  }
};

class SquareType : public EnumVariant<SquareId, TYPES(DeityHabitat, const Creature*, CreatureId,
      CreatureFactory::SingleCreature, ChestInfo, const Tribe*, StairInfo, string),
    ASSIGN(DeityHabitat, SquareId::ALTAR),
    ASSIGN(const Creature*, SquareId::CREATURE_ALTAR),
    ASSIGN(CreatureId,
        SquareId::DECID_TREE,
        SquareId::CANIF_TREE,
        SquareId::BUSH),
    ASSIGN(CreatureFactory::SingleCreature,
        SquareId::HATCHERY),
    ASSIGN(ChestInfo,
        SquareId::CHEST,
        SquareId::COFFIN),
    ASSIGN(string,
        SquareId::NOTICE_BOARD),
    ASSIGN(const Tribe*,
        SquareId::TRIBE_DOOR,
        SquareId::BARRICADE),
    ASSIGN(StairInfo,
        SquareId::STAIRS)> {
  using EnumVariant::EnumVariant;
};


bool isWall(SquareType);

namespace std {
  template <> struct hash<SquareType> {
    size_t operator()(const SquareType& t) const {
      return (size_t)t.getId();
    }
  };
}

#endif
