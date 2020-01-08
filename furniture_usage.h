#pragma once

#include "util.h"
#include "effect.h"

class Position;
class Furniture;
class Creature;

RICH_ENUM(
  BuiltinUsageId,
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
  ARCHERY_RANGE,
  STUDY,
  PORTAL,
  SIT_ON_THRONE,
  DESECRATE,
  DEMON_RITUAL
);

MAKE_VARIANT2(FurnitureUsageType, BuiltinUsageId, Effect);

class FurnitureUsage {
  public:
  static void handle(FurnitureUsageType, Position, WConstFurniture, Creature*);
  static bool canHandle(FurnitureUsageType, const Creature*);
  static string getUsageQuestion(FurnitureUsageType, string furnitureName);
  static void beforeRemoved(FurnitureUsageType, Position);
};
