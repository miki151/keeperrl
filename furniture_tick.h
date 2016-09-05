#pragma once

class Position;
class Furniture;

enum class FurnitureTickType { BED, PIGSTY };

class FurnitureTick {
  public:
  static void handle(FurnitureTickType, Position, Furniture*);
};
