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
  bool needs(const Creature* c, const Item* it, bool noLimit = false, bool replacement = false) const;
  optional<UniqueEntity<Creature>::Id> getOwner(const Item*) const;
  bool isOwner(const Item*, const Creature*) const;
  void own(const Creature*, const Item*);
  void discard(const Item*);
  void discard(UniqueEntity<Item>::Id);
  void updateOwners(const vector<Item*>, const vector<Creature*>&);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);

  static int getItemValue(const Item*);

  void setLocked(const Creature*, UniqueEntity<Item>::Id, bool locked);
  bool isLocked(const Creature*, UniqueEntity<Item>::Id) const;

  private:
  enum EquipmentType { ARMOR, HEALING, ARCHERY, COMBAT_ITEM };

  static optional<EquipmentType> getEquipmentType(const Item* it);
  int getEquipmentLimit(EquipmentType type) const;
  bool isItemAppropriate(const Creature*, const Item*) const;

  EntityMap<Item, UniqueEntity<Creature>::Id> SERIAL(owners);
  set<pair<UniqueEntity<Creature>::Id, UniqueEntity<Item>::Id>> SERIAL(locked);
};

