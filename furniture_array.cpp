#include "stdafx.h"
#include "furniture_array.h"

SERIALIZE_DEF(FurnitureArray, built, construction)
SERIALIZATION_CONSTRUCTOR_IMPL(FurnitureArray)

FurnitureArray::FurnitureArray(Rectangle bounds) :
  built([&](FurnitureLayer) { return Array(bounds); }),
construction([&](FurnitureLayer) { return Table<optional<Construction>>(bounds); }) {
}

const FurnitureArray::Array& FurnitureArray::getBuilt(FurnitureLayer layer) const {
  return built[layer];
}

FurnitureArray::Array& FurnitureArray::getBuilt(FurnitureLayer layer) {
  return built[layer];
}

const optional<FurnitureArray::Construction>& FurnitureArray::getConstruction(Vec2 pos, FurnitureLayer layer) const {
  return construction[layer][pos];
}

optional<FurnitureArray::Construction>& FurnitureArray::getConstruction(Vec2 pos, FurnitureLayer layer) {
  return construction[layer][pos];
}
