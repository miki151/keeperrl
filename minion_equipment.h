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
  bool needsItem(WConstCreature c, const Item* it, bool noLimit = false) const;
  optional<UniqueEntity<Creature>::Id> getOwner(const Item*) const;
  bool isOwner(const Item*, WConstCreature) const;
  bool tryToOwn(WConstCreature, Item*);
  void discard(const Item*);
  void discard(UniqueEntity<Item>::Id);
  void updateOwners(const vector<WCreature>&);
  vector<Item*> getItemsOwnedBy(WConstCreature, ItemPredicate = nullptr) const;

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);

  void setLocked(WConstCreature, UniqueEntity<Item>::Id, bool locked);
  bool isLocked(WConstCreature, UniqueEntity<Item>::Id) const;
  void sortByEquipmentValue(WConstCreature, vector<Item*>& items) const;
  void autoAssign(WConstCreature, vector<Item*> possibleItems);
  void updateItems(const vector<Item*>& items);

  private:
  enum EquipmentType { ARMOR, HEALING, COMBAT_ITEM, TORCH };

  static optional<EquipmentType> getEquipmentType(const Item* it);
  optional<int> getEquipmentLimit(EquipmentType type) const;
  Item* getWorstItem(WConstCreature, vector<Item*>) const;
  int getItemValue(WConstCreature, const Item*) const;

  EntityMap<Item, UniqueEntity<Creature>::Id> SERIAL(owners);
  EntityMap<Creature, vector<WeakPointer<Item>>> SERIAL(myItems);
  set<pair<UniqueEntity<Creature>::Id, UniqueEntity<Item>::Id>> SERIAL(locked);
};

