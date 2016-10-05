#pragma once

#include "position_map.h"
#include "furniture_type.h"
#include "furniture_layer.h"

class Position;
class SquareType;

class TileEfficiency {
  public:
  void update(Position);
  double getEfficiency(Position) const;

  SERIALIZATION_DECL(TileEfficiency)

  private:
  PositionMap<double> SERIAL(efficiency);
};
