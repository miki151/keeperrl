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

#pragma once

#include <functional>

#include "util.h"
#include "unique_entity.h"

class Item;

RICH_ENUM(ItemIndex,
  GOLD,
  WOOD,
  IRON,
  STEEL,
  STONE,
  REVIVABLE_CORPSE,
  CORPSE,
  WEAPON,
  TRAP,
  MINION_EQUIPMENT,
  RANGED_WEAPON,
  CAN_EQUIP,
  FOR_SALE
);

template <typename T, typename Id>
class IndexedVector {
  public:
  IndexedVector() {}
  IndexedVector(vector<T>&& e) : elems(std::move(e)) {
    for (int i: All(elems))
      indexes.emplace(elems[i]->getUniqueId(), i);
  }

  IndexedVector(const vector<T>& e) : elems(e) {
    for (int i: All(elems))
      indexes.emplace(elems[i]->getUniqueId(), i);
  }

  const vector<T>& getElems() const {
    return elems;
  }

  void insert(T&& t) {
    indexes.emplace(t->getUniqueId(), elems.size());
    elems.push_back(std::move(t));
  }

  T remove(Id id) {
    int index = indexes.at(id);
    T ret = std::move(elems[index]);
    indexes.erase(ret->getUniqueId());
    elems[index] = std::move(elems.back());
    elems.pop_back();
    if (index < elems.size()) // checks if we haven't just removed the last element
      indexes[elems[index]->getUniqueId()] = index;
    return ret;
  }

  const T& operator[] (int index) const {
    return elems[index];
  }

  T& operator[] (int) {
    return elems[index];
  }

  optional<const T&> fetch(Id id) const {
    auto iter = indexes.find(id);
    if (iter == indexes.end())
      return none;
    else
      return elems[iter->second];
  }

  optional<T&> fetch(Id id) {
    auto iter = indexes.find(id);
    if (iter == indexes.end())
      return none;
    else
      return elems[iter->second];
  }

  vector<T> removeAll() {
    indexes.clear();
    return std::move(elems);
  }

  SERIALIZE_ALL(elems, indexes)

  private:
  vector<T> SERIAL(elems);
  unordered_map<Id, int, CustomHash<Id>> SERIAL(indexes);
};

typedef IndexedVector<Item*, UniqueEntity<Item>::Id> ItemVector;
typedef IndexedVector<PItem, UniqueEntity<Item>::Id> PItemVector;

class Inventory {
  public:
  void addItem(PItem);
  void addItems(vector<PItem>);
  static function<bool(const Item*)> getIndexPredicate(ItemIndex);
  PItem removeItem(Item* item);
  vector<PItem> removeItems(vector<Item*> items);
  vector<PItem> removeAllItems();
  void clearIndex(ItemIndex);

  const vector<Item*>& getItems() const;
  vector<Item*> getItems(function<bool (Item*)> predicate) const;
  const vector<Item*>& getItems(ItemIndex) const;

  bool hasItem(const Item*) const;
  Item* getItemById(UniqueEntity<Item>::Id);
  int size() const;

  bool isEmpty() const;

  ~Inventory();

  SERIALIZATION_DECL(Inventory);

  private:
  PItemVector SERIAL(items);
  ItemVector SERIAL(itemsCache);
  mutable EnumMap<ItemIndex, optional<ItemVector>> indexes;
};

BOOST_CLASS_VERSION(Inventory, 1)
