#pragma once

#include "read_write_array.h"
#include "furniture_factory.h"
#include "furniture.h"

struct GenerateFurniture {
  PFurniture operator()(FurnitureParams p) {
    return FurnitureFactory::get(p.type, p.tribe);
  }
};

class FurnitureArray: public ReadWriteArray<Furniture, FurnitureParams, GenerateFurniture> {
  public:
  FurnitureArray(Rectangle bounds);

  struct Construction {
    FurnitureType SERIAL(type);
    int SERIAL(time);
    SERIALIZE_ALL(type, time)
  };
  Table<optional<Construction>> SERIAL(construction);

  SERIALIZATION_DECL(FurnitureArray)

};
