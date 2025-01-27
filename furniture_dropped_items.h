#pragma once

#include "util.h"
#include "t_string.h"

class Position;
class Furniture;

class FurnitureDroppedItems {
  public:
  struct Water {
    TString SERIAL(verbSingle);
    TString SERIAL(verbPlural);
    optional<string> SERIAL(unseenMessage);
    SERIALIZE_ALL(NAMED(verbSingle), NAMED(verbPlural), NAMED(unseenMessage))
  };
  using DropData = Water;
  template <typename T>
  FurnitureDroppedItems(const T& t) : FurnitureDroppedItems(DropData(t)) {}
  FurnitureDroppedItems(DropData);
  vector<PItem> handle(Position, const Furniture*, vector<PItem>) const;

  SERIALIZATION_DECL(FurnitureDroppedItems)

  private:
  DropData SERIAL(dropData);
};
