#pragma once

#include "util.h"

class Position;
class Furniture;
class Creature;

enum class FurnitureUsageType { CHEST, COFFIN, VAMPIRE_COFFIN, SLEEP, FOUNTAIN, KEEPER_BOARD, CROPS, TORTURE, STAIRS };
class FurnitureUsage {
  public:
  static void handle(FurnitureUsageType, Position, Furniture*, Creature*);
  static string getUsageQuestion(FurnitureUsageType, string furnitureName);


};
