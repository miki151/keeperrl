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

  static bool isItemUseful(const WItem);
  bool needsItem(const Creature* c, const WItem it, bool noLimit = false) const;
  optional<UniqueEntity<Creature>::Id> getOwner(const WItem) const;
  bool isOwner(const WItem, const Creature*) const;
  void own(const Creature*, WItem);
  void discard(const WItem);
  void discard(UniqueEntity<Item>::Id);
  void updateOwners(const vector<Creature*>&);
  vector<WItem> getItemsOwnedBy(const Creature*, ItemPredicate = nullptr) const;

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);

  void setLocked(const Creature*, UniqueEntity<Item>::Id, bool locked);
  bool isLocked(const Creature*, UniqueEntity<Item>::Id) const;
  void sortByEquipmentValue(vector<WItem>& items) const;
  void autoAssign(const Creature*, vector<WItem> possibleItems);
  void updateItems(const vector<WItem>& items);

  private:
  enum EquipmentType { ARMOR, HEALING, ARCHERY, COMBAT_ITEM };

  static optional<EquipmentType> getEquipmentType(const WItem it);
  optional<int> getEquipmentLimit(EquipmentType type) const;
  bool isItemAppropriate(const Creature*, const WItem) const;
  WItem getWorstItem(const Creature*, vector<WItem>) const;
  int getItemValue(const WItem) const;

  EntityMap<Item, UniqueEntity<Creature>::Id> SERIAL(owners);
  EntityMap<Creature, vector<WItem>> SERIAL(myItems);
  set<pair<UniqueEntity<Creature>::Id, UniqueEntity<Item>::Id>> SERIAL(locked);
};

