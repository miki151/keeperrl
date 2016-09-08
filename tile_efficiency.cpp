#include "stdafx.h"
#include "tile_efficiency.h"
#include "collective_config.h"
#include "square_type.h"


SERIALIZE_DEF(TileEfficiency, floors, efficiency)
SERIALIZATION_CONSTRUCTOR_IMPL(TileEfficiency)

static double getBonus(SquareType floor) {
  for (auto& info : CollectiveConfig::getFloors())
    if (info.type == floor)
      return info.efficiencyBonus;
  return 0;
}

void TileEfficiency::setFloor(Position pos, SquareType floor) {
  floors.set(pos, floor);
  for (Position p : concat({pos}, pos.neighbors8()))
    update(p);
}
static const double lightBase = 0.5;
static const double flattenVal = 0.9;

double TileEfficiency::getEfficiency(Position pos) const {
  return (1 + efficiency.get(pos)) * min(1.0, (lightBase + pos.getLight() * (1 - lightBase)) / flattenVal);
}

void TileEfficiency::update(Position pos) {
  double value = 0;
  for (Position p : concat({pos}, pos.neighbors8()))
    if (auto& type = floors.get(p))
      value += getBonus(*type);
  efficiency.set(pos, value);
  pos.setNeedsRenderUpdate(true);
}
