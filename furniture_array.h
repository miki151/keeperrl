#pragma once

#include "read_write_array.h"
#include "furniture_factory.h"
#include "furniture.h"
#include "furniture_layer.h"

struct GenerateFurniture {
  PFurniture operator()(FurnitureParams p) {
    return FurnitureFactory::get(p.type, p.tribe);
  }
};

class FurnitureArray {
  public:
  FurnitureArray(Rectangle bounds);

  typedef ReadWriteArray<Furniture, FurnitureParams, GenerateFurniture> Array;

  const Array& getBuilt(FurnitureLayer) const;
  Array& getBuilt(FurnitureLayer);

  struct Construction {
    FurnitureType SERIAL(type);
    int SERIAL(time);
    SERIALIZE_ALL(type, time)
  };

  const optional<Construction>& getConstruction(Vec2, FurnitureLayer) const;
  optional<Construction>& getConstruction(Vec2, FurnitureLayer);

  SERIALIZATION_DECL(FurnitureArray)

  private:
  EnumMap<FurnitureLayer, Array> SERIAL(built);
  EnumMap<FurnitureLayer, Table<optional<Construction>>> SERIAL(construction);
};
