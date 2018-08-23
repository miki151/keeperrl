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

SERIALIZE_DEF(Equipment, inventory, items, equipped)
SERIALIZATION_CONSTRUCTOR_IMPL(Equipment);

void Equipment::addItem(PItem item, WCreature c) {
  item->onOwned(c);
  inventory.addItem(std::move(item));
}

void Equipment::addItems(vector<PItem> items, WCreature c) {
  for (auto& item : items)
    addItem(std::move(item), c);
}

vector<WItem> Equipment::getSlotItems(EquipmentSlot slot) const {
  return items[slot];
}

bool Equipment::hasItem(WConstItem it) const {
  return inventory.hasItem(it);
}

const vector<WItem>& Equipment::getAllEquipped() const {
  return equipped;
}

const vector<WItem>& Equipment::getItems() const {
  return inventory.getItems();
}

const vector<WItem>& Equipment::getItems(ItemIndex index) const {
  return inventory.getItems(index);
}

WItem Equipment::getItemById(UniqueEntity<Item>::Id id) const {
  return inventory.getItemById(id);
}

bool Equipment::isEquipped(WConstItem item) const {
  return item->canEquip() && items[item->getEquipmentSlot()].contains(item);
}

int Equipment::getMaxItems(EquipmentSlot slot) const {
  switch (slot) {
    case EquipmentSlot::RINGS: return 2;
    default: return 1;
  }
}

bool Equipment::canEquip(WConstItem item) const {
  if (!item->canEquip() || isEquipped(item))
    return false;
  EquipmentSlot slot = item->getEquipmentSlot();
  return items[slot].size() < getMaxItems(slot);
}

void Equipment::equip(WItem item, EquipmentSlot slot, WCreature c) {
  items[slot].push_back(item);
  equipped.push_back(item);
  item->onEquip(c);
  CHECK(inventory.hasItem(item));
}

void Equipment::unequip(WItem item, WCreature c) {
  items[item->getEquipmentSlot()].removeElement(item);
  equipped.removeElement(item);
  item->onUnequip(c);
}

PItem Equipment::removeItem(WItem item, WCreature c) {
  if (isEquipped(item))
    unequip(item, c);
  item->onDropped(c);
  return inventory.removeItem(item);
}
  
vector<PItem> Equipment::removeItems(const vector<WItem>& items, WCreature c) {
  vector<PItem> ret;
  for (WItem& it : copyOf(items))
    ret.push_back(removeItem(it, c));
  return ret;
}

vector<PItem> Equipment::removeAllItems(WCreature c) {
  return removeItems(inventory.getItems(), c);
}

double Equipment::getTotalWeight() const {
  return inventory.getTotalWeight();
}

bool Equipment::isEmpty() const {
  return inventory.isEmpty();
}

const ItemCounts& Equipment::getCounts() const {
  return inventory.getCounts();
}

void Equipment::tick(Position pos) {
  inventory.tick(pos);
}

