#pragma once

#include "position_map.h"
#include "furniture_type.h"
#include "furniture_layer.h"

class Position;
class SquareType;

class TileEfficiency {
  public:
  void setType(Position, FurnitureType);
  void removeType(Position, FurnitureType);
  void removeType(Position, FurnitureLayer);
  double getEfficiency(Position) const;

  SERIALIZATION_DECL(TileEfficiency)

  private:
  void update(Position);
  PositionMap<double> SERIAL(efficiency);
  PositionMap<EnumMap<FurnitureLayer, optional<FurnitureType>>> SERIAL(types);
};
