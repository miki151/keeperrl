#ifndef _SQUARE_TYPE_H
#define _SQUARE_TYPE_H

#include "enums.h"
#include "creature_factory.h"

struct SquareType {
  enum Id {
    FLOOR,
    BRIDGE,
    PATH,
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
    SECRET_PASS,
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
  } id;

  SquareType(Id);
  SquareType();

  bool operator==(const SquareType&) const;
  bool operator!=(const SquareType&) const;
  bool operator==(SquareType::Id) const;
  bool operator!=(SquareType::Id) const;

  bool isWall() const;

  typedef DeityHabitat AltarInfo;
  SquareType(AltarInfo);

  typedef const Creature* CreatureAltarInfo;
  SquareType(CreatureAltarInfo);

  SquareType(Id, CreatureFactory);

  typedef const Tribe* TribeInfo;
  SquareType(Id, TribeInfo);

  variant<AltarInfo, CreatureAltarInfo, CreatureFactory, TribeInfo> values;
  AltarInfo& getAltarInfo() { return boost::get<AltarInfo>(values); }
  CreatureAltarInfo& getCreatureAltarInfo() { return boost::get<CreatureAltarInfo>(values); }
  CreatureFactory& getCreatureFactory() { return boost::get<CreatureFactory>(values); }
  TribeInfo& getTribeInfo() { return boost::get<TribeInfo>(values); }

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);
};

bool operator==(SquareType::Id, const SquareType&);

namespace std {
  template <> struct hash<SquareType> {
    size_t operator()(const SquareType& t) const {
      return (size_t)t.id;
    }
  };
}

#endif
