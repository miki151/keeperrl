/* Copyright (C) 2013-2014 Michal Brzozowski (rusolis@poczta.fm)

   This file is part of KeeperRL.

   KeeperRL is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   KeeperRL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

#include "stdafx.h"

#include "equipment.h"
#include "item.h"

map<EquipmentSlot, string> Equipment::slotTitles = {
  {EquipmentSlot::WEAPON, "Weapon"},
  {EquipmentSlot::GLOVES, "Gloves"},
  {EquipmentSlot::RANGED_WEAPON, "Ranged weapon"},
  {EquipmentSlot::HELMET, "Helmet"},
  {EquipmentSlot::BODY_ARMOR, "Body armor"},
  {EquipmentSlot::BOOTS, "Boots"},
  {EquipmentSlot::RINGS, "Rings"},
  {EquipmentSlot::AMULET, "Amulet"}};

template <class Archive> 
void Equipment::serialize(Archive& ar, const unsigned int version) {
  ar & SUBCLASS(Inventory) & SVAR(items);
  CHECK_SERIAL;
}

SERIALIZABLE(Equipment);
SERIALIZATION_CONSTRUCTOR_IMPL(Equipment);

vector<Item*> Equipment::getItem(EquipmentSlot slot) const {
  if (items.count(slot) > 0)
    return items.at(slot);
  else
    return {};
}

bool Equipment::isEquiped(const Item* item) const {
  if (!item->canEquip())
    return false;
  EquipmentSlot slot = item->getEquipmentSlot();
  return items.count(slot) && contains(items.at(slot), item);
}

int Equipment::getMaxItems(EquipmentSlot slot) const {
  switch (slot) {
    case EquipmentSlot::RINGS: return 2;
    default: return 1;
  }
}

bool Equipment::canEquip(const Item* item) const {
  if (!item->canEquip())
    return false;
  EquipmentSlot slot = item->getEquipmentSlot();
  return !items.count(slot) || items.at(slot).size() < getMaxItems(slot);
}

void Equipment::equip(Item* item, EquipmentSlot slot) {
  items[slot].push_back(item);
  CHECK(hasItem(item));
}

void Equipment::unequip(const Item* item) {
  EquipmentSlot slot = item->getEquipmentSlot();
  CHECK(items.count(slot));
  removeElement(items.at(slot), item);
}

PItem Equipment::removeItem(Item* item) {
  if (isEquiped(item))
    unequip(item);
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

