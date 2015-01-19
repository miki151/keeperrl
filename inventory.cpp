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

#include "inventory.h"
#include "item.h"
#include "minion_equipment.h"

template <class Archive> 
void Inventory::serialize(Archive& ar, const unsigned int version) {
  ar& SVAR(items)
    & SVAR(itemsCache);
  CHECK_SERIAL;
}

Inventory::~Inventory() {}

SERIALIZABLE(Inventory);

SERIALIZATION_CONSTRUCTOR_IMPL(Inventory);

void Inventory::addItem(PItem item) {
  itemsCache.push_back(item.get());
  for (ItemIndex ind : ENUM_ALL(ItemIndex))
    if (indexes[ind] && getIndexPredicate(ind)(item.get()))
      indexes[ind]->push_back(item.get()); 
  items.push_back(move(item));
}

void Inventory::addItems(vector<PItem> v) {
  for (PItem& it : v)
    addItem(std::move(it));
}

PItem Inventory::removeItem(Item* itemRef) {
  int ind = -1;
  for (int i : All(items))
    if (items[i].get() == itemRef) {
      ind = i;
      break;
    }
  CHECK(ind > -1) << "Tried to remove unknown item.";
  PItem item = std::move(items[ind]);
  items.erase(items.begin() + ind);
  removeElement(itemsCache, itemRef);
  for (ItemIndex ind : ENUM_ALL(ItemIndex))
    if (indexes[ind] && getIndexPredicate(ind)(item.get()))
      removeElement(*indexes[ind], itemRef);
  return item;
}

vector<PItem> Inventory::removeItems(vector<Item*> items) {
  vector<PItem> ret;
  for (Item* item : items)
    ret.push_back(removeItem(item));
  return ret;
}

vector<PItem> Inventory::removeAllItems() {
  itemsCache.clear();
  for (ItemIndex ind : ENUM_ALL(ItemIndex))
    indexes[ind] = none;
  return move(items);
}

vector<Item*> Inventory::getItems(function<bool (Item*)> predicate) const {
  vector<Item*> ret;
  for (const PItem& item : items)
    if (predicate(item.get()))
      ret.push_back(item.get());
  return ret;
}

function<bool(const Item*)> Inventory::getIndexPredicate(ItemIndex index) {
  switch (index) {
    case ItemIndex::GOLD: return Item::classPredicate(ItemClass::GOLD);
    case ItemIndex::WOOD: return [](const Item* it) {
        return it->getResourceId() == CollectiveResourceId::WOOD; };
    case ItemIndex::IRON: return [](const Item* it) {
        return it->getResourceId() == CollectiveResourceId::IRON; };
    case ItemIndex::STONE: return [](const Item* it) {
        return it->getResourceId() == CollectiveResourceId::STONE; };
    case ItemIndex::REVIVABLE_CORPSE: return [](const Item* it) {
        return it->getClass() == ItemClass::CORPSE && it->getCorpseInfo()->canBeRevived; };
    case ItemIndex::WEAPON: return [](const Item* it) {
        return it->getClass() == ItemClass::WEAPON; };
    case ItemIndex::TRAP: return [](const Item* it) { return !!it->getTrapType(); };
    case ItemIndex::CORPSE: return [](const Item* it) {
        return it->getClass() == ItemClass::CORPSE; };
    case ItemIndex::MINION_EQUIPMENT: return [](const Item* it) {
        return MinionEquipment::isItemUseful(it);};
    case ItemIndex::RANGED_WEAPON: return [](const Item* it) {
        return it->getClass() == ItemClass::RANGED_WEAPON;};
    case ItemIndex::CAN_EQUIP: return [](const Item* it) {return it->canEquip();};
  }
}

const vector<Item*>& Inventory::getItems(ItemIndex index) const {
  if (!indexes[index])
    indexes[index] = getItems(getIndexPredicate(index));
  return *indexes[index];
}

const vector<Item*>& Inventory::getItems() const {
  return itemsCache;
}

bool Inventory::hasItem(const Item* itemRef) const {
  for (const PItem& item : items) 
    if (item.get() == itemRef)
      return true;
  return false;
}

int Inventory::size() const {
  return items.size();
}

bool Inventory::isEmpty() const {
  return items.empty();
}

