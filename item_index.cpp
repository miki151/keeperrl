#include "stdafx.h"
#include "item_index.h"
#include "item_class.h"
#include "item.h"
#include "minion_equipment.h"
#include "resource_id.h"
#include "corpse_info.h"
#include "effect_type.h"

const char* getName(ItemIndex index, int count) {
  switch (index) {
    case ItemIndex::RANGED_WEAPON: return count == 1 ? "ranged weapon" : "ranged weapons";
    default: FATAL << "Unimplemented";
  }
  return "";
}

function<bool (const WItem)> getIndexPredicate(ItemIndex index) {
  switch (index) {
    case ItemIndex::GOLD: return Item::classPredicate(ItemClass::GOLD);
    case ItemIndex::WOOD: return [](const WItem it) {
        return it->getResourceId() == CollectiveResourceId::WOOD; };
    case ItemIndex::IRON: return [](const WItem it) {
        return it->getResourceId() == CollectiveResourceId::IRON; };
    case ItemIndex::STEEL: return [](const WItem it) {
        return it->getResourceId() == CollectiveResourceId::STEEL; };
    case ItemIndex::STONE: return [](const WItem it) {
        return it->getResourceId() == CollectiveResourceId::STONE; };
    case ItemIndex::REVIVABLE_CORPSE: return [](const WItem it) {
        return it->getClass() == ItemClass::CORPSE && it->getCorpseInfo()->canBeRevived; };
    case ItemIndex::WEAPON: return [](const WItem it) {
        return it->getClass() == ItemClass::WEAPON; };
    case ItemIndex::TRAP: return [](const WItem it) { return !!it->getTrapType(); };
    case ItemIndex::CORPSE: return [](const WItem it) {
        return it->getClass() == ItemClass::CORPSE; };
    case ItemIndex::MINION_EQUIPMENT: return [](const WItem it) {
        return MinionEquipment::isItemUseful(it);};
    case ItemIndex::RANGED_WEAPON: return [](const WItem it) {
        return it->getClass() == ItemClass::RANGED_WEAPON;};
    case ItemIndex::CAN_EQUIP: return [](const WItem it) {return it->canEquip();};
    case ItemIndex::FOR_SALE: return [](const WItem it) {return it->isOrWasForSale();};
    case ItemIndex::HEALING_ITEM: return [](const WItem it) {return it->getEffectType() == EffectType(EffectId::HEAL);};
  }
}
