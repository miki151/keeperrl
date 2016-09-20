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

#ifndef _INVENTORY_H
#define _INVENTORY_H

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
  vector<PItem> SERIAL(items);
  vector<Item*> SERIAL(itemsCache);
  mutable EnumMap<ItemIndex, optional<vector<Item*>>> indexes;
};

#endif
