#pragma once

#include "util.h"

class Position;
class Furniture;

enum class FurnitureTickType { BED, PIGSTY, BOULDER_TRAP, PORTAL, METEOR_SHOWER };

class FurnitureTick {
  public:
  static void handle(FurnitureTickType, Position, WFurniture);
};
