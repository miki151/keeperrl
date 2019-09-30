#pragma once

#include "util.h"

RICH_ENUM(
    ItemUpgradeType,
    WEAPON,
    ARMOR
);

class ItemPrefix;

struct ItemUpgradeInfo {
  vector<string> getDescription() const;
  ItemUpgradeType SERIAL(type);
  HeapAllocatedSerializationWorkaround<ItemPrefix> SERIAL(prefix);
  bool operator == (const ItemUpgradeInfo& o) const;
  bool operator != (const ItemUpgradeInfo& o) const;
  template <class Archive>
  void serialize(Archive& ar1, const unsigned int);
};

