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

#include "util.h"
#include "unique_entity.h"
#include "entity_map.h"

class Creature;
class Item;

class MinionEquipment {
  public:

  static bool isItemUseful(WConstItem);
  bool needsItem(WConstCreature c, WConstItem it, bool noLimit = false) const;
  optional<UniqueEntity<Creature>::Id> getOwner(WConstItem) const;
  bool isOwner(WConstItem, WConstCreature) const;
  bool tryToOwn(WConstCreature, WItem);
  void discard(WConstItem);
  void discard(UniqueEntity<Item>::Id);
  void updateOwners(const vector<WCreature>&);
  vector<WItem> getItemsOwnedBy(WConstCreature, ItemPredicate = nullptr) const;

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);

  void setLocked(WConstCreature, UniqueEntity<Item>::Id, bool locked);
  bool isLocked(WConstCreature, UniqueEntity<Item>::Id) const;
  void sortByEquipmentValue(WConstCreature, vector<WItem>& items) const;
  void autoAssign(WConstCreature, vector<WItem> possibleItems);
  void updateItems(const vector<WItem>& items);

  private:
  enum EquipmentType { ARMOR, HEALING, COMBAT_ITEM, TORCH };

  static optional<EquipmentType> getEquipmentType(WConstItem it);
  optional<int> getEquipmentLimit(EquipmentType type) const;
  WItem getWorstItem(WConstCreature, vector<WItem>) const;
  int getItemValue(WConstCreature, WConstItem) const;

  EntityMap<Item, UniqueEntity<Creature>::Id> SERIAL(owners);
  EntityMap<Creature, vector<WItem>> SERIAL(myItems);
  set<pair<UniqueEntity<Creature>::Id, UniqueEntity<Item>::Id>> SERIAL(locked);
};

