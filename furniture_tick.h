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
  EXTINGUISH_FIRE,
  SET_FURNITURE_ON_FIRE
);

class FurnitureTick {
  public:
  static void handle(FurnitureTickType, Position, Furniture*);
};
