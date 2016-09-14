#pragma once

class Position;
class Furniture;

enum class FurnitureTickType { BED, PIGSTY, BOULDER_TRAP };

class FurnitureTick {
  public:
  static void handle(FurnitureTickType, Position, Furniture*);
};
