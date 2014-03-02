#include "stdafx.h"

#include "equipment.h"

template <class Archive> 
void Equipment::serialize(Archive& ar, const unsigned int version) {
  ar & SUBCLASS(Inventory) & BOOST_SERIALIZATION_NVP(items);
}

SERIALIZABLE(Equipment);

class Item;
Item* Equipment::getItem(EquipmentSlot slot) const {
  if (items.count(slot) > 0)
    return items.at(slot);
  else
    return nullptr;
}

bool Equipment::isEquiped(const Item* item) const {
  for (auto elem : items)
    if (elem.second == item) {
      return true;
    }
  return false;
}

EquipmentSlot Equipment::getSlot(const Item* item) const {
  for (auto elem : items)
    if (elem.second == item) {
      return elem.first;
    }
  FAIL << "Item not in any slot " << item->getAName();
  return EquipmentSlot::WEAPON;
}

void Equipment::equip(Item* item, EquipmentSlot slot) {
  items[slot] = item;
  CHECK(hasItem(item));
}

void Equipment::unequip(EquipmentSlot slot) {
  CHECK(items.count(slot) > 0);
  items.erase(slot);
}

PItem Equipment::removeItem(Item* item) {
  if (isEquiped(item))
    unequip(getSlot(item));
  return Inventory::removeItem(item);
}
  
vector<PItem> Equipment::removeItems(const vector<Item*>& items) {
  vector<PItem> ret;
  for (Item* it : items)
    ret.push_back(removeItem(it));
  return ret;
}

vector<PItem> Equipment::removeAllItems() {
  items.clear();
  return Inventory::removeAllItems();
}

