#pragma once

#include "util.h"
#include "effect.h"

struct ItemAttrBonus {
  AttrType attr;
  int value;
  COMPARE_ALL(attr, value)
};

using ItemPrefix = variant<LastingEffect, Effect, ItemAttrBonus>;

class ItemAttributes;
extern void applyPrefix(ItemPrefix, ItemAttributes&);
