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

function<bool(WConstItem)> getIndexPredicate(ItemIndex index) {
  switch (index) {
    case ItemIndex::GOLD: return Item::classPredicate(ItemClass::GOLD);
    case ItemIndex::WOOD: return [](WConstItem it) {
        return it->getResourceId() == CollectiveResourceId::WOOD; };
    case ItemIndex::IRON: return [](WConstItem it) {
        return it->getResourceId() == CollectiveResourceId::IRON; };
    case ItemIndex::STEEL: return [](WConstItem it) {
        return it->getResourceId() == CollectiveResourceId::STEEL; };
    case ItemIndex::STONE: return [](WConstItem it) {
        return it->getResourceId() == CollectiveResourceId::STONE; };
    case ItemIndex::REVIVABLE_CORPSE: return [](WConstItem it) {
        return it->getClass() == ItemClass::CORPSE && it->getCorpseInfo()->canBeRevived; };
    case ItemIndex::WEAPON: return [](WConstItem it) {
        return it->getClass() == ItemClass::WEAPON; };
    case ItemIndex::TRAP: return [](WConstItem it) { return !!it->getTrapType(); };
    case ItemIndex::CORPSE: return [](WConstItem it) {
        return it->getClass() == ItemClass::CORPSE; };
    case ItemIndex::MINION_EQUIPMENT: return [](WConstItem it) {
        return MinionEquipment::isItemUseful(it);};
    case ItemIndex::RANGED_WEAPON: return [](WConstItem it) {
        return it->getClass() == ItemClass::RANGED_WEAPON;};
    case ItemIndex::CAN_EQUIP: return [](WConstItem it) {return it->canEquip();};
    case ItemIndex::FOR_SALE: return [](WConstItem it) {return it->isOrWasForSale();};
    case ItemIndex::HEALING_ITEM: return [](WConstItem it) {return it->getEffectType() == EffectType(EffectTypes::Heal{});};
  }
}
