#include "stdafx.h"
#include "furniture_array.h"

SERIALIZE_DEF(FurnitureArray, built, construction)
SERIALIZATION_CONSTRUCTOR_IMPL(FurnitureArray)

FurnitureArray::FurnitureArray(Rectangle bounds) : built([&](FurnitureLayer) { return Array(bounds); }) {
}

const FurnitureArray::Array& FurnitureArray::getBuilt(FurnitureLayer layer) const {
  return built[layer];
}

FurnitureArray::Array& FurnitureArray::getBuilt(FurnitureLayer layer) {
  return built[layer];
}

optional<const FurnitureArray::Construction&> FurnitureArray::getConstruction(Vec2 pos, FurnitureLayer layer) const {
  return getReferenceMaybe(construction[layer], pos);
}

optional<FurnitureArray::Construction&> FurnitureArray::getConstruction(Vec2 pos, FurnitureLayer layer) {
  return getReferenceMaybe(construction[layer], pos);
}

void FurnitureArray::eraseConstruction(Vec2 pos, FurnitureLayer layer) {
  construction[layer].erase(pos);
}

void FurnitureArray::setConstruction(Vec2 pos, FurnitureLayer layer, FurnitureArray::Construction c) {
  construction[layer][pos] = c;
}
