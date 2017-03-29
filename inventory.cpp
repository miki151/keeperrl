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
#include "resource_id.h"
#include "item_class.h"
#include "corpse_info.h"

template <class Archive> 
void Inventory::serialize(Archive& ar, const unsigned int version) {
  if (version == 0) {
    vector<PItem> SERIAL(oldItems);
    vector<WItem> SERIAL(oldItemsCache);
    serializeAll(ar, oldItems, oldItemsCache);
    items = PItemVector(std::move(oldItems));
    itemsCache = ItemVector(oldItemsCache);
    weight = 0;
    for (auto item : itemsCache.getElems())
      weight += item->getWeight();
  } else
    serializeAll(ar, items, itemsCache, weight);
}

Inventory::~Inventory() {}

SERIALIZABLE(Inventory);

SERIALIZATION_CONSTRUCTOR_IMPL(Inventory);

void Inventory::addItem(PItem item) {
  CHECK(!!item) << "Null item dropped";
  itemsCache.insert(item.get());
  for (ItemIndex ind : ENUM_ALL(ItemIndex))
    if (indexes[ind] && getIndexPredicate(ind)(item.get()))
      indexes[ind]->insert(item.get());
  weight += item->getWeight();
  items.insert(std::move(item));
}

void Inventory::addItems(vector<PItem> v) {
  for (PItem& it : v)
    addItem(std::move(it));
}

PItem Inventory::removeItem(WItem itemRef) {
  PItem item = items.remove(itemRef->getUniqueId());
  weight -= item->getWeight();
  itemsCache.remove(itemRef->getUniqueId());
  for (ItemIndex ind : ENUM_ALL(ItemIndex))
    if (indexes[ind] && getIndexPredicate(ind)(item.get()))
      indexes[ind]->remove(itemRef->getUniqueId());
  return item;
}

vector<PItem> Inventory::removeItems(vector<WItem> items) {
  vector<PItem> ret;
  for (WItem item : items)
    ret.push_back(removeItem(item));
  return ret;
}

void Inventory::clearIndex(ItemIndex ind) {
  indexes[ind] = none;
}

vector<PItem> Inventory::removeAllItems() {
  itemsCache.removeAll();
  for (ItemIndex ind : ENUM_ALL(ItemIndex))
    indexes[ind] = none;
  weight = 0;
  return items.removeAll();
}

vector<WItem> Inventory::getItems(function<bool (WItem)> predicate) const {
  vector<WItem> ret;
  for (const PItem& item : items.getElems())
    if (predicate(item.get()))
      ret.push_back(item.get());
  return ret;
}

WItem Inventory::getItemById(UniqueEntity<Item>::Id id) const {
  if (auto item = itemsCache.fetch(id))
    return *item;
  else
    return nullptr;
}

function<bool(const WItem)> Inventory::getIndexPredicate(ItemIndex index) {
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
  }
}

const vector<WItem>& Inventory::getItems(ItemIndex index) const {
  if (isEmpty()) {
    static vector<WItem> empty;
    return empty;
  }
  auto& elems = indexes[index];
  if (!elems)
    elems = ItemVector(getItems(getIndexPredicate(index)));
  return elems->getElems();
}

const vector<WItem>& Inventory::getItems() const {
  return itemsCache.getElems();
}

bool Inventory::hasItem(const WItem itemRef) const {
  return !!itemsCache.fetch(itemRef->getUniqueId());
}

int Inventory::size() const {
  return items.getElems().size();
}

double Inventory::getTotalWeight() const {
  return weight;
}

bool Inventory::isEmpty() const {
  return items.getElems().empty();
}

