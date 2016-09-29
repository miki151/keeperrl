#pragma once

#include "enums.h"
#include "creature_factory.h"
#include "enum_variant.h"
#include "util.h"
#include "stair_key.h"

RICH_ENUM(SquareId,
  FLOOR,
  BLACK_FLOOR,
  CUSTOM_FLOOR,
  GRASS,
  MUD,
  BLACK_WALL,
  HILL,
  WATER,
  WATER_WITH_DEPTH,
  MAGMA,
  ABYSS,
  SAND,
  BORDER_GUARD,
  SOKOBAN_HOLE
);

struct CustomFloorInfo {
  ViewId SERIAL(viewId);
  string SERIAL(name);
  SERIALIZE_ALL(viewId, name)
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
  public:
  using EnumVariant::EnumVariant;
  size_t getHash() const {
    return (size_t)getId();
  }
};
