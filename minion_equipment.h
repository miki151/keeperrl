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

#ifndef _MINION_EQUIPMENT_H
#define _MINION_EQUIPMENT_H

#include "util.h"
#include "unique_entity.h"

class Creature;
class Item;

class MinionEquipment {
  public:

  static bool isItemUseful(const Item*);
  bool needs(const Creature* c, const Item* it, bool noLimit = false, bool replacement = false) const;
  const Creature* getOwner(const Item*) const;
  void own(const Creature*, const Item*);
  void discard(const Item*);
  void discard(UniqueEntity<Item>::Id);
  void updateOwners(const vector<Item*>);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);

  static int getItemValue(const Item*);

  private:
  enum EquipmentType { ARMOR, HEALING, ARCHERY, COMBAT_ITEM };

  static optional<EquipmentType> getEquipmentType(const Item* it);
  int getEquipmentLimit(EquipmentType type) const;
  bool isItemAppropriate(const Creature*, const Item*) const;

  map<UniqueEntity<Item>::Id, const Creature*> SERIAL(owners);
};

#endif
