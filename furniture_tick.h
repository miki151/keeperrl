#pragma once

#include "util.h"

class Position;
class Furniture;

RICH_ENUM(FurnitureTickType,
  BED,
  PIGSTY,
  BOULDER_TRAP,
  PORTAL,
  METEOR_SHOWER,
  PIT,
  EXTINGUISH_FIRE
);

class FurnitureTick {
  public:
  static void handle(FurnitureTickType, Position, WFurniture);
};
