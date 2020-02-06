#pragma once

#include "util.h"
#include "effect.h"

class Position;
class Furniture;

RICH_ENUM(BuiltinTickType,
  BED,
  PIGSTY,
  BOULDER_TRAP,
  PORTAL,
  METEOR_SHOWER,
  PIT,
  EXTINGUISH_FIRE,
  SET_FURNITURE_ON_FIRE
);

namespace TickDetail {

using Builtin = BuiltinTickType;

MAKE_VARIANT2(FurnitureTickType, Effect, Builtin);
}

using TickDetail::FurnitureTickType;


class FurnitureTick {
  public:
  static void handle(FurnitureTickType, Position, Furniture*);
};
