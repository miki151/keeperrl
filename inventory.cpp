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
#include "view_object.h"
#include "resource_id.h"


SERIALIZE_DEF(Inventory, items, itemsCache, weight, counts)
SERIALIZATION_CONSTRUCTOR_IMPL(Inventory);

void Inventory::addViewId(ViewId id, int count) {
  auto& cur = counts[id];
  cur += count;
  if (cur == 0)
    counts.erase(id);
}

void Inventory::addItem(PItem item) {
  CHECK(!!item) << "Null item dropped";
  itemsCache.insert(item.get());
  addViewId(item->getViewObject().id(), 1);
  for (ItemIndex ind : ENUM_ALL(ItemIndex))
    if (indexes[ind] && hasIndex(ind, item.get()))
      indexes[ind]->insert(item.get());
  if (auto id = item->getResourceId()) {
    int index = id->getInternalId();
    if (index < resourceIndexes.size() && resourceIndexes[index])
      resourceIndexes[index]->insert(item.get());
  }
  weight += item->getWeight();
  items.insert(std::move(item));
}

void Inventory::addItems(vector<PItem> v) {
  for (PItem& it : v)
    addItem(std::move(it));
}

PItem Inventory::removeItem(Item* itemRef) {
  PItem item = items.remove(itemRef->getUniqueId());
  weight -= item->getWeight();
  itemsCache.remove(itemRef->getUniqueId());
  addViewId(item->getViewObject().id(), -1);
  for (ItemIndex ind : ENUM_ALL(ItemIndex))
    if (indexes[ind] && hasIndex(ind, item.get()))
      indexes[ind]->remove(itemRef->getUniqueId());
  if (auto id = item->getResourceId()) {
    int index = id->getInternalId();
    if (index < resourceIndexes.size() && resourceIndexes[index])
      resourceIndexes[index]->remove(item.get());
  }
  return item;
}

vector<PItem> Inventory::removeItems(vector<Item*> items) {
  vector<PItem> ret;
  for (Item* item : items)
    ret.push_back(removeItem(item));
  return ret;
}

void Inventory::clearIndex(ItemIndex ind) {
  indexes[ind] = none;
}

vector<PItem> Inventory::removeAllItems() {
  itemsCache.removeAll();
  counts.clear();
  for (ItemIndex ind : ENUM_ALL(ItemIndex))
    indexes[ind] = none;
  for (auto& ind : resourceIndexes)
    ind = none;
  weight = 0;
  return items.removeAll();
}

Item* Inventory::getItemById(UniqueEntity<Item>::Id id) const {
  if (auto item = itemsCache.fetch(id))
    return *item;
  else
    return nullptr;
}


const vector<Item*>& Inventory::getItems(ItemIndex index) const {
  static vector<Item*> empty;
  if (isEmpty()) {
    return empty;
  }
  auto& elems = indexes[index];
  if (!elems) {
    elems.emplace();
    for (auto& item : getItems())
      if (hasIndex(index, item))
        elems->insert(item);
  }
  return elems->getElems();
}

const vector<Item*>& Inventory::getItems(CollectiveResourceId id) const {
  static vector<Item*> empty;
  int index = id.getInternalId();
  if (isEmpty()) {
    return empty;
  }
  if (index >= resourceIndexes.size())
    resourceIndexes.resize(index + 1);
  auto& elems = resourceIndexes[index];
  if (!elems) {
    elems.emplace();
    for (auto& item : getItems())
      if (item->getResourceId() == id)
        elems->insert(item);
  }
  return elems->getElems();
}

const ItemCounts& Inventory::getCounts() const {
  return counts;
}

const vector<Item*>& Inventory::getItems() const {
  return itemsCache.getElems();
}

bool Inventory::hasItem(const Item* itemRef) const {
  return !!itemsCache.fetch(itemRef->getUniqueId());
}

int Inventory::size() const {
  return items.getElems().size();
}

double Inventory::getTotalWeight() const {
  return weight;
}

vector<PItem> Inventory::tick(Position pos, bool carried) {
  PROFILE_BLOCK("Inventory::tick");
  vector<PItem> removed;
  vector<WeakPointer<Item>> itemsCopy;
  for (auto it : getItems())
    if (it->canEverTick(carried))
      itemsCopy.push_back(it->getThis());
  for (auto& item : itemsCopy) {
    auto itemRef = item.get();
    if (itemRef && hasItem(itemRef)) {
      // items might be destroyed or removed from inventory in tick()
      auto oldViewId = itemRef->getViewObject().id();
      itemRef->tick(pos, carried);
      if (!item)
        continue;
      auto newViewId = itemRef->getViewObject().id();
      if (newViewId != oldViewId) {
        addViewId(oldViewId, -1);
        addViewId(newViewId, 1);
      }
      if (itemRef->isDiscarded() && hasItem(itemRef))
        removed.push_back(removeItem(itemRef));
    }
  }
  return removed;
}

bool Inventory::containsAnyOf(const EntitySet<Item>& items) const {
  if (size() > items.getSize()) {
    for (auto& item : items)
      if (!!getItemById(item))
        return true;
  } else {
    for (auto& it : getItems())
      if (items.contains(it))
        return true;
  }
  return false;
}

bool Inventory::isEmpty() const {
  return items.getElems().empty();
}

