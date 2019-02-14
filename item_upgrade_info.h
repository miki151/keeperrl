#pragma once

#include "item_prefix.h"

RICH_ENUM(
    ItemUpgradeType,
    WEAPON,
    ARMOR
);

struct ItemUpgradeInfo {
  vector<string> getDescription() const;
  ItemUpgradeType type;
  ItemPrefix prefix;
  COMPARE_ALL(type, prefix)
};

