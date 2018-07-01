#pragma once

class Furniture;
class Position;

#include "util.h"

enum class FurnitureClickType { LOCK, KEEPER_BOARD };

class FurnitureClick {
  public:
  static void handle(FurnitureClickType, Position, WConstFurniture);
};
