#pragma once

#include "util.h"

class Position;
class Furniture;
class Creature;

enum class FurnitureUsageType {
  CHEST,
  COFFIN,
  VAMPIRE_COFFIN,
  SLEEP,
  FOUNTAIN,
  KEEPER_BOARD,
  CROPS,
  STAIRS,
  TIE_UP,
  TRAIN,
  STUDY,
  PORTAL
};

class FurnitureUsage {
  public:
  static void handle(FurnitureUsageType, Position, WConstFurniture, WCreature);
  static bool canHandle(FurnitureUsageType, WConstCreature);
  static string getUsageQuestion(FurnitureUsageType, string furnitureName);
};
