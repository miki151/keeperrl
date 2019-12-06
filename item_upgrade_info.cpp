#include "stdafx.h"
#include "item_upgrade_info.h"
#include "item_prefix.h"
#include "lasting_effect.h"
#include "attr_type.h"

static const char* getName(ItemUpgradeType type) {
  switch (type) {
    case ItemUpgradeType::ARMOR:
      return "armor";
    case ItemUpgradeType::WEAPON:
      return "weapons";
  }
}

vector<string> ItemUpgradeInfo::getDescription(const ContentFactory* factory) const {
  vector<string> ret { "Crafting upgrade for "_s + getName(type) + ":" };
  ret.append(getEffectDescription(factory, *prefix));
  return ret;
}

bool ItemUpgradeInfo::operator ==(const ItemUpgradeInfo& o) const {
  return type == o.type && prefix == o.prefix;
}

bool ItemUpgradeInfo::operator !=(const ItemUpgradeInfo& o) const {
  return !(*this == o);
}

SERIALIZE_DEF(ItemUpgradeInfo, type, prefix)

#include "pretty_archive.h"
template
void ItemUpgradeInfo::serialize(PrettyInputArchive& ar1, unsigned);
