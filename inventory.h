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
#include "indexed_vector.h"
#include "item_index.h"
#include "item_counts.h"
#include "entity_set.h"

class Item;
class Position;


class Inventory {
  public:
  void addItem(PItem);
  void addItems(vector<PItem>);
  PItem removeItem(Item* item);
  vector<PItem> removeItems(vector<Item*> items);
  vector<PItem> removeAllItems();
  void clearIndex(ItemIndex);

  const vector<Item*>& getItems() const;
  vector<Item*> getItems(function<bool (const Item*)> predicate) const;
  const vector<Item*>& getItems(ItemIndex) const;
  const vector<Item*>& getItems(CollectiveResourceId) const;
  const ItemCounts& getCounts() const;

  bool hasItem(const Item*) const;
  Item* getItemById(UniqueEntity<Item>::Id) const;
  int size() const;
  double getTotalWeight() const;
  // returns removed items
  vector<PItem> tick(Position, bool carried);
  bool containsAnyOf(const EntitySet<Item>&) const;

  bool isEmpty() const;

  SERIALIZATION_DECL(Inventory)

  private:

  typedef IndexedVector<Item*, UniqueEntity<Item>::Id> ItemVector;
  typedef IndexedVector<PItem, UniqueEntity<Item>::Id> PItemVector;

  ItemCounts SERIAL(counts);
  PItemVector SERIAL(items);
  ItemVector SERIAL(itemsCache);
  double SERIAL(weight) = 0;
  mutable EnumMap<ItemIndex, optional<ItemVector>> indexes;
  mutable vector<optional<ItemVector>> resourceIndexes;
  void addViewId(ViewId, int count);
};
