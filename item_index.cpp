#include "stdafx.h"
#include "item_index.h"
#include "item_class.h"
#include "item.h"
#include "minion_equipment.h"
#include "resource_id.h"
#include "corpse_info.h"
#include "effect.h"
#include "item_upgrade_info.h"
#include "item_type.h"
#include "effect_type.h"
#include "t_string.h"

optional<TStringId> getName(ItemIndex index, int count) {
  switch (index) {
    case ItemIndex::RANGED_WEAPON: return count == 1 ? TStringId("RANGED_WEAPON") : TStringId("RANGED_WEAPONS");
    default: FATAL << "Unimplemented";
  }
  return none;
}

bool hasIndex(ItemIndex index, const Item* item) {
  switch (index) {
    case ItemIndex::WEAPON:
      return item->getClass() == ItemClass::WEAPON;
    case ItemIndex::MINION_EQUIPMENT:
      return MinionEquipment::isItemUseful(item);
    case ItemIndex::RANGED_WEAPON:
      return item->getClass() == ItemClass::RANGED_WEAPON;
    case ItemIndex::FOR_SALE:
      return item->isOrWasForSale();
    case ItemIndex::RUNE:
      return !!item->getUpgradeInfo() || !!item->getIngredientType();
  }
}
