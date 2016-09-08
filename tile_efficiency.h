#pragma once

#include "position_map.h"
#include "square_type.h"

class Position;
class SquareType;

class TileEfficiency {
  public:
  void setFloor(Position, SquareType);
  double getEfficiency(Position) const;

  SERIALIZATION_DECL(TileEfficiency)

  private:
  void update(Position);
  PositionMap<double> SERIAL(efficiency);
  PositionMap<optional<SquareType>> SERIAL(floors);
};
