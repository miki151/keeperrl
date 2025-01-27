#pragma once

#include "util.h"

RICH_ENUM(
    ItemUpgradeType,
    WEAPON,
    ARMOR,
    BALSAM,
    BODY_PART,
    RANGED_WEAPON,
    AUTOMATONS_UPPER,
    AUTOMATONS_LOWER
);

const char* getItemTypeName(ItemUpgradeType);

class ItemPrefix;
class ContentFactory;
class TString;

struct ItemUpgradeInfo {
  vector<TString> getDescription(const ContentFactory*) const;
  ItemUpgradeType SERIAL(type);
  HeapAllocated<ItemPrefix> SERIAL(prefix);
  optional<pair<string, double>> SERIAL(diminishModifier);
  template <class Archive>
  void serialize(Archive& ar1, const unsigned int);
};
