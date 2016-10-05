#include "stdafx.h"
#include "tile_efficiency.h"
#include "collective_config.h"
#include "furniture.h"

SERIALIZE_DEF(TileEfficiency, types, efficiency)
SERIALIZATION_CONSTRUCTOR_IMPL(TileEfficiency)

void TileEfficiency::setType(Position pos, FurnitureType type) {
  types.getOrInit(pos)[Furniture::getLayer(type)] = type;
  for (Position p : concat({pos}, pos.neighbors8()))
    update(p);
}

void TileEfficiency::removeType(Position pos, FurnitureType type) {
  removeType(pos, Furniture::getLayer(type));
}

void TileEfficiency::removeType(Position pos, FurnitureLayer layer) {
  types.getOrInit(pos)[layer] = none;
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
    for (auto layer : ENUM_ALL(FurnitureLayer))
      if (auto& type = types.get(p)[layer])
        value += CollectiveConfig::getEfficiencyBonus(*type);
  efficiency.set(pos, value);
  pos.setNeedsRenderUpdate(true);
}
