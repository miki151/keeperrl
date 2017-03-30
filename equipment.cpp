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
  ar & SUBCLASS(Inventory);
  serializeAll(ar, items, equipped);
}

SERIALIZABLE(Equipment);
SERIALIZATION_CONSTRUCTOR_IMPL(Equipment);

vector<WItem> Equipment::getItem(EquipmentSlot slot) const {
  return items[slot];
}

const vector<WItem>& Equipment::getAllEquipped() const {
  return equipped;
}

bool Equipment::isEquipped(const WItem item) const {
  return item->canEquip() && contains(items[item->getEquipmentSlot()], item);
}

int Equipment::getMaxItems(EquipmentSlot slot) const {
  switch (slot) {
    case EquipmentSlot::RINGS: return 2;
    default: return 1;
  }
}

bool Equipment::canEquip(const WItem item) const {
  if (!item->canEquip() || isEquipped(item))
    return false;
  EquipmentSlot slot = item->getEquipmentSlot();
  return items[slot].size() < getMaxItems(slot);
}

void Equipment::equip(WItem item, EquipmentSlot slot, WCreature c) {
  items[slot].push_back(item);
  equipped.push_back(item);
  item->onEquip(c);
  CHECK(hasItem(item));
}

void Equipment::unequip(WItem item, WCreature c) {
  removeElement(items[item->getEquipmentSlot()], item);
  removeElement(equipped, item);
  item->onUnequip(c);
}

PItem Equipment::removeItem(WItem item, WCreature c) {
  if (isEquipped(item))
    unequip(item, c);
  return Inventory::removeItem(item);
}
  
vector<PItem> Equipment::removeItems(const vector<WItem>& items, WCreature c) {
  vector<PItem> ret;
  for (WItem it : items)
    ret.push_back(removeItem(it, c));
  return ret;
}

vector<PItem> Equipment::removeAllItems(WCreature c) {
  return removeItems(getItems(), c);
}

