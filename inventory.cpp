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
  return move(items);
}

vector<Item*> Inventory::getItems(function<bool (Item*)> predicate) const {
  vector<Item*> ret;
  for (const PItem& item : items)
    if (predicate(item.get()))
      ret.push_back(item.get());
  return ret;
}

vector<Item*> Inventory::getItems() const {
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

