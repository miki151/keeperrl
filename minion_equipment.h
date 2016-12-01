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

  static bool isItemUseful(const Item*);
  bool needsItem(const Creature* c, const Item* it, bool noLimit = false) const;
  optional<UniqueEntity<Creature>::Id> getOwner(const Item*) const;
  bool isOwner(const Item*, const Creature*) const;
  void own(const Creature*, Item*);
  void discard(const Item*);
  void discard(UniqueEntity<Item>::Id);
  void updateOwners(const vector<Creature*>&);
  vector<Item*> getItemsOwnedBy(const Creature*, ItemPredicate = nullptr) const;

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);

  void setLocked(const Creature*, UniqueEntity<Item>::Id, bool locked);
  bool isLocked(const Creature*, UniqueEntity<Item>::Id) const;
  void sortByEquipmentValue(vector<Item*>& items) const;
  void autoAssign(const Creature*, vector<Item*> possibleItems);
  void updateItems(const vector<Item*>& items);

  private:
  enum EquipmentType { ARMOR, HEALING, ARCHERY, COMBAT_ITEM };

  static optional<EquipmentType> getEquipmentType(const Item* it);
  optional<int> getEquipmentLimit(EquipmentType type) const;
  bool isItemAppropriate(const Creature*, const Item*) const;
  Item* getWorstItem(const Creature*, vector<Item*>) const;
  int getItemValue(const Item*) const;

  EntityMap<Item, UniqueEntity<Creature>::Id> SERIAL(owners);
  EntityMap<Creature, vector<WItem>> SERIAL(myItems);
  set<pair<UniqueEntity<Creature>::Id, UniqueEntity<Item>::Id>> SERIAL(locked);
};

