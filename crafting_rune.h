#pragma once

#include "item_prefix.h"

RICH_ENUM(
    CraftingRuneType,
    WEAPON,
    ARMOR
);

struct CraftingRune {
  CraftingRuneType SERIAL(type);
  ItemPrefix SERIAL(prefix);
  SERIALIZE_ALL(type, prefix)
};
