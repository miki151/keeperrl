#pragma once

class Furniture;
class Position;

#include "util.h"

RICH_ENUM(
  FurnitureClickType,
  LOCK,
  KEEPER_BOARD
);

class FurnitureClick {
  public:
  static void handle(FurnitureClickType, Position, WConstFurniture);
  static ViewObjectAction getClickAction(FurnitureClickType, Position, WConstFurniture);
};
