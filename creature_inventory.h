#pragma once

#include "util.h"
#include "item_type.h"
#include "item_types.h"

struct CreatureInventory {
  struct Elem {
    ItemTypeVariant SERIAL(type);
    int SERIAL(countMin) = 1;
    int SERIAL(countMax) = 1;
    double SERIAL(chance) = 1;
    double SERIAL(prefixChance) = 0;
    optional<ItemTypeVariant> SERIAL(alternative);
    template <class Archive>
    void serialize(Archive& ar1, const unsigned int);
  };
  vector<Elem> SERIAL(elems);
  SERIALIZE_ALL(elems)
};
