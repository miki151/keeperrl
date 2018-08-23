#include "stdafx.h"
#include "item_index.h"
#include "item_class.h"
#include "item.h"
#include "minion_equipment.h"
#include "resource_id.h"
#include "corpse_info.h"
#include "effect.h"

const char* getName(ItemIndex index, int count) {
  switch (index) {
    case ItemIndex::RANGED_WEAPON: return count == 1 ? "ranged weapon" : "ranged weapons";
    default: FATAL << "Unimplemented";
  }
  return "";
}

bool hasIndex(ItemIndex index, WConstItem item) {
  switch (index) {
    case ItemIndex::GOLD:
      return item->getClass() == ItemClass::GOLD;
    case ItemIndex::WOOD:
      return item->getResourceId() == CollectiveResourceId::WOOD;
    case ItemIndex::IRON:
      return item->getResourceId() == CollectiveResourceId::IRON;
    case ItemIndex::ADA:
      return item->getResourceId() == CollectiveResourceId::ADA;
    case ItemIndex::STONE:
      return item->getResourceId() == CollectiveResourceId::STONE;
    case ItemIndex::REVIVABLE_CORPSE:
      return item->getClass() == ItemClass::CORPSE && item->getCorpseInfo()->canBeRevived;
    case ItemIndex::WEAPON:
      return item->getClass() == ItemClass::WEAPON;
    case ItemIndex::TRAP:
      return !!item->getTrapType();
    case ItemIndex::CORPSE:
      return item->getClass() == ItemClass::CORPSE;
    case ItemIndex::MINION_EQUIPMENT:
      return MinionEquipment::isItemUseful(item);
    case ItemIndex::RANGED_WEAPON:
      return item->getClass() == ItemClass::RANGED_WEAPON;
    case ItemIndex::CAN_EQUIP:
      return item->canEquip();
    case ItemIndex::FOR_SALE:
      return item->isOrWasForSale();
    case ItemIndex::HEALING_ITEM:
      return item->getEffect() == Effect(Effect::Heal{});
  }
}
