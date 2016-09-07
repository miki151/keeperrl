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
  CUSTOM_FLOOR,
  BRIDGE,
  ROAD,
  GRASS,
  MUD,
  ROCK_WALL,
  WOOD_WALL,
  BLACK_WALL,
  CASTLE_WALL,
  MUD_WALL,
  MOUNTAIN,
  HILL,
  WATER,
  WATER_WITH_DEPTH,
  MAGMA,
  GOLD_ORE,
  IRON_ORE,
  STONE,
  ABYSS,
  SAND,
  BORDER_GUARD,
  SOKOBAN_HOLE
);

struct CustomFloorInfo {
  ViewId SERIAL(viewId);
  string SERIAL(name);
  SERIALIZE_ALL(viewId, name);
  bool operator == (const CustomFloorInfo& o) const {
    return viewId == o.viewId && name == o.name;
  }
};

class SquareType : public EnumVariant<SquareId, TYPES(CreatureId, StairKey, double, CustomFloorInfo),
    ASSIGN(double,
        SquareId::WATER_WITH_DEPTH),
    ASSIGN(StairKey,
        SquareId::SOKOBAN_HOLE),
    ASSIGN(CustomFloorInfo,
        SquareId::CUSTOM_FLOOR)> {
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
