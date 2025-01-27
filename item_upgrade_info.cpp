#include "stdafx.h"
#include "item_upgrade_info.h"
#include "item_prefix.h"
#include "lasting_effect.h"
#include "attr_type.h"
#include "t_string.h"

static TString getTargetName(ItemUpgradeType type) {
  switch (type) {
    case ItemUpgradeType::ARMOR:
      return TStringId("ARMOR_UPGRADE_TYPE");
    case ItemUpgradeType::WEAPON:
      return TStringId("WEAPONS_UPGRADE_TYPE");
    case ItemUpgradeType::RANGED_WEAPON:
      return TStringId("RANGED_WEAPONS_UPGRADE_TYPE");
    case ItemUpgradeType::BALSAM:
      return TStringId("BODIES_UPGRADE_TYPE");
    case ItemUpgradeType::BODY_PART:
      return TStringId("UNDEAD_KINGS_UPGRADE_TYPE");
    case ItemUpgradeType::AUTOMATONS_LOWER:
    case ItemUpgradeType::AUTOMATONS_UPPER:
      return TStringId("ABOMINATIONS_UPGRADE_TYPE");
  }
}

vector<TString> ItemUpgradeInfo::getDescription(const ContentFactory* factory) const {
  PROFILE;
  vector<TString> ret { TSentence("CRAFTING_UPDATE_FOR", getTargetName(type)) };
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
    case ItemUpgradeType::AUTOMATONS_LOWER:
    case ItemUpgradeType::AUTOMATONS_UPPER:
      return "automaton part";
  }
}

SERIALIZE_DEF(ItemUpgradeInfo, NAMED(type), NAMED(prefix), OPTION(diminishModifier))

#include "pretty_archive.h"
template
void ItemUpgradeInfo::serialize(PrettyInputArchive& ar1, unsigned);
