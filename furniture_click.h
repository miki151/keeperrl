#pragma once

class Furniture;
class Position;


enum class FurnitureClickType { LOCK, UNLOCK };

class FurnitureClick {
  public:
  static void handle(FurnitureClickType, Position, const Furniture*);
};
