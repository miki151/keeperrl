#include "stdafx.h"
#include "item_upgrade_info.h"
#include "item_prefix.h"
#include "lasting_effect.h"
#include "attr_type.h"

static const char* getTargetName(ItemUpgradeType type) {
  switch (type) {
    case ItemUpgradeType::ARMOR:
      return "armor";
    case ItemUpgradeType::WEAPON:
      return "weapons";
    case ItemUpgradeType::RANGED_WEAPON:
      return "ranged weapons";
    case ItemUpgradeType::BALSAM:
      return "bodies";
    case ItemUpgradeType::BODY_PART:
      return "abominations";
  }
}

vector<string> ItemUpgradeInfo::getDescription(const ContentFactory* factory) const {
  vector<string> ret { "Crafting upgrade for "_s + getTargetName(type) + ":" };
  ret.append(getEffectDescription(factory, *prefix));
  return ret;
}

const char* getItemTypeName(ItemUpgradeType type) {
  switch (type) {
    case ItemUpgradeType::ARMOR:
    case ItemUpgradeType::WEAPON:
    case ItemUpgradeType::RANGED_WEAPON:
      return "glyph";
    case ItemUpgradeType::BALSAM:
      return "balsam";
    case ItemUpgradeType::BODY_PART:
      return "body part";
  }
}

SERIALIZE_DEF(ItemUpgradeInfo, type, prefix)

#include "pretty_archive.h"
template
void ItemUpgradeInfo::serialize(PrettyInputArchive& ar1, unsigned);
