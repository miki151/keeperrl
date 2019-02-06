#pragma once

#include "util.h"
#include "item_type.h"

struct CreatureInventory {
  struct Elem {
    ItemType::Type SERIAL(type);
    int SERIAL(countMin) = 1;
    int SERIAL(countMax) = 1;
    double SERIAL(chance) = 1;
    double SERIAL(prefixChance) = 0;
    optional<ItemType::Type> SERIAL(alternative);
    SERIALIZE_ALL(NAMED(type), OPTION(countMin), OPTION(countMax), OPTION(chance), OPTION(prefixChance), NAMED(alternative))
  };
  vector<Elem> SERIAL(elems);
  SERIALIZE_ALL(elems)
};
