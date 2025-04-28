#pragma once

#include "read_write_array.h"
#include "furniture_factory.h"
#include "creature_factory.h"
#include "game.h"
#include "furniture.h"
#include "furniture_layer.h"


class FurnitureArray {
  public:
  FurnitureArray(Rectangle bounds);

  typedef ReadWriteArray<Furniture, FurnitureParams> Array;

  const Array& getBuilt(FurnitureLayer) const;
  Array& getBuilt(FurnitureLayer);

  struct Construction {
    FurnitureType SERIAL(type);
    int SERIAL(time);
    SERIALIZE_ALL(type, time)
  };

  optional<const Construction&> getConstruction(Vec2, FurnitureLayer) const;
  optional<Construction&> getConstruction(Vec2, FurnitureLayer);
  void eraseConstruction(Vec2, FurnitureLayer);
  void setConstruction(Vec2, FurnitureLayer, Construction);

  SERIALIZATION_DECL(FurnitureArray)

  private:
  EnumMap<FurnitureLayer, Array> SERIAL(built);
  EnumMap<FurnitureLayer, HashMap<Vec2, Construction>> SERIAL(construction);
};
