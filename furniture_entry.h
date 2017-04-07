#pragma once

#include "util.h"

enum class FurnitureEntryType {
  SOKOBAN
};

class Position;
class Creature;
class Furniture;

class FurnitureEntry {
  public:
  static void handle(FurnitureEntryType, WConstFurniture, WCreature);
};
