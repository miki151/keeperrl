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

#include "stdafx.h"
#include "inventory.h"
#include "equipment_slot.h"

class Body;
class ContentFactory;

class Equipment {
  public:
  void addItem(PItem, Creature*, const ContentFactory* = nullptr);
  void addItems(vector<PItem>, Creature*);
  const vector<Item*>& getSlotItems(EquipmentSlot slot) const;
  bool hasItem(const Item*) const;
  bool isEquipped(const Item*) const;
  bool canEquip(const Item*, const Creature*) const;
  void equip(Item*, EquipmentSlot, Creature*, const ContentFactory* = nullptr);
  void unequip(Item*, Creature*, const ContentFactory* = nullptr);
  PItem removeItem(Item*, Creature*);
  int getMaxItems(EquipmentSlot, const Creature*) const;
  const vector<Item*>& getAllEquipped() const;
  const vector<Item*>& getItems() const;
  const vector<Item*>& getItems(ItemIndex) const;
  Item* getItemById(UniqueEntity<Item>::Id) const;
  vector<PItem> removeItems(const vector<Item*>&, Creature*);
  vector<PItem> removeAllItems(Creature*);
  double getTotalWeight() const;
  bool isEmpty() const;
  const ItemCounts& getCounts() const;
  void tick(Position, Creature*);
  bool containsAnyOf(const EntitySet<Item>&) const;

  SERIALIZATION_DECL(Equipment)

  static map<EquipmentSlot, string> slotTitles;

  private:
  Inventory SERIAL(inventory);
  EnumMap<EquipmentSlot, vector<Item*>> SERIAL(items);
  vector<Item*> SERIAL(equipped);
  void onRemoved(Item*, Creature*);
};

