#pragma once

#include "util.h"

class Position;
class Furniture;

class FurnitureDroppedItems {
  public:
  struct Water {
    string SERIAL(verbSingle);
    string SERIAL(verbPlural);
    optional<string> SERIAL(unseenMessage);
    SERIALIZE_ALL(verbSingle, verbPlural, unseenMessage)
  };
  using DropData = variant<Water>;
  template <typename T>
  FurnitureDroppedItems(const T& t) : FurnitureDroppedItems(DropData(t)) {}
  FurnitureDroppedItems(DropData);
  vector<PItem> handle(Position, WConstFurniture, vector<PItem>) const;

  SERIALIZATION_DECL(FurnitureDroppedItems)

  private:
  DropData SERIAL(dropData);
};
