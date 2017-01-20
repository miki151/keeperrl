#pragma once

#include "enums.h"
#include "creature_factory.h"
#include "enum_variant.h"
#include "util.h"
#include "stair_key.h"

RICH_ENUM(SquareId,
  FLOOR,
  BLACK_FLOOR,
  GRASS,
  MUD,
  BLACK_WALL,
  HILL,
  WATER,
  WATER_WITH_DEPTH,
  MAGMA,
  SAND,
  BORDER_GUARD
);

class SquareType : public EnumVariant<SquareId, TYPES(double),
    ASSIGN(double,
        SquareId::WATER_WITH_DEPTH)> {
  public:
  using EnumVariant::EnumVariant;
  size_t getHash() const {
    return (size_t)getId();
  }
};
