#include "stdafx.h"
#include "item_upgrade_info.h"

static const char* getName(ItemUpgradeType type) {
  switch (type) {
    case ItemUpgradeType::ARMOR:
      return "armor";
    case ItemUpgradeType::WEAPON:
      return "weapons";
  }
}

vector<string> ItemUpgradeInfo::getDescription() const {
  vector<string> ret { "Crafting upgrade for "_s + getName(type) + ":" };
  ret.append(getEffectDescription(prefix));
  return ret;
}
