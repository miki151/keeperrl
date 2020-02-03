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
  static void handle(FurnitureClickType, Position, const Furniture*);
  static ViewObjectAction getClickAction(FurnitureClickType, Position, const Furniture*);
};
