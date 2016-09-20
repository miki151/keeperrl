#include "stdafx.h"
#include "tile_efficiency.h"
#include "collective_config.h"
#include "square_type.h"


SERIALIZE_DEF(TileEfficiency, types, efficiency)
SERIALIZATION_CONSTRUCTOR_IMPL(TileEfficiency)

void TileEfficiency::setType(Position pos, SquareType type) {
  types.set(pos, type);
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
    if (auto& type = types.get(p))
      value += CollectiveConfig::getEfficiencyBonus(*type);
  efficiency.set(pos, value);
  pos.setNeedsRenderUpdate(true);
}
