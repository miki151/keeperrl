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
#include "body.h"
#include "creature.h"

map<EquipmentSlot, string> Equipment::slotTitles = {
  {EquipmentSlot::WEAPON, "Weapon"},
  {EquipmentSlot::GLOVES, "Gloves"},
  {EquipmentSlot::RANGED_WEAPON, "Ranged weapon"},
  {EquipmentSlot::SHIELD, "Shield"},
  {EquipmentSlot::HELMET, "Helmet"},
  {EquipmentSlot::BODY_ARMOR, "Body armor"},
  {EquipmentSlot::BOOTS, "Boots"},
  {EquipmentSlot::RINGS, "Rings"},
  {EquipmentSlot::AMULET, "Amulet"}};

SERIALIZE_DEF(Equipment, inventory, items, equipped)
SERIALIZATION_CONSTRUCTOR_IMPL(Equipment);

void Equipment::addItem(PItem item, Creature* c) {
  item->onOwned(c);
  inventory.addItem(std::move(item));
}

void Equipment::addItems(vector<PItem> items, Creature* c) {
  for (auto& item : items)
    addItem(std::move(item), c);
}

const vector<Item*>& Equipment::getSlotItems(EquipmentSlot slot) const {
  return items[slot];
}

bool Equipment::hasItem(const Item* it) const {
  return inventory.hasItem(it);
}

const vector<Item*>& Equipment::getAllEquipped() const {
  return equipped;
}

const vector<Item*>& Equipment::getItems() const {
  return inventory.getItems();
}

const vector<Item*>& Equipment::getItems(ItemIndex index) const {
  return inventory.getItems(index);
}

Item* Equipment::getItemById(UniqueEntity<Item>::Id id) const {
  return inventory.getItemById(id);
}

bool Equipment::isEquipped(const Item* item) const {
  return item->canEquip() && items[item->getEquipmentSlot()].contains(item);
}

int Equipment::getMaxItems(EquipmentSlot slot, const Creature* c) const {
  auto& body = c->getBody();
  switch (slot) {
    case EquipmentSlot::WEAPON:
      return min(body.numGood(BodyPart::ARM), c->getMaxSimultaneousWeapons());
    case EquipmentSlot::RINGS:
      return body.numGood(BodyPart::ARM);
    case EquipmentSlot::AMULET:
    case EquipmentSlot::HELMET:
      return body.numGood(BodyPart::HEAD);
    default:
      return 1;
  }
}

bool Equipment::canEquip(const Item* item, const Creature* c) const {
  if (!item->canEquip() || isEquipped(item))
    return false;
  EquipmentSlot slot = item->getEquipmentSlot();
  return items[slot].size() < getMaxItems(slot, c);
}

void Equipment::equip(Item* item, EquipmentSlot slot, Creature* c, const ContentFactory* factory) {
  items[slot].push_back(item);
  equipped.push_back(item);
  item->onEquip(c, true, factory);
  CHECK(inventory.hasItem(item));
}

void Equipment::unequip(Item* item, Creature* c) {
  items[item->getEquipmentSlot()].removeElement(item);
  equipped.removeElement(item);
  item->onUnequip(c);
}

void Equipment::onRemoved(Item* item, Creature* c) {
  if (isEquipped(item))
    unequip(item, c);
  item->onDropped(c);
}

PItem Equipment::removeItem(Item* item, Creature* c) {
  onRemoved(item, c);
  return inventory.removeItem(item);
}
  
vector<PItem> Equipment::removeItems(const vector<Item*>& items, Creature* c) {
  vector<PItem> ret;
  for (Item*& it : copyOf(items))
    ret.push_back(removeItem(it, c));
  return ret;
}

vector<PItem> Equipment::removeAllItems(Creature* c) {
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

void Equipment::tick(Position pos, Creature* c) {
  for (auto& item : inventory.tick(pos, true))
    onRemoved(item.get(), c);
}

bool Equipment::containsAnyOf(const EntitySet<Item>& items) const {
  return inventory.containsAnyOf(items);
}
