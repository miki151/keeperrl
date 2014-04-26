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

#ifndef _EQUIPMENT_H
#define _EQUIPMENT_H

#include "inventory.h"
#include "enums.h"


class Equipment : public Inventory {
  public:
  Item* getItem(EquipmentSlot slot) const;
  bool isEquiped(const Item*) const;
  EquipmentSlot getSlot(const Item*) const;
  void equip(Item*, EquipmentSlot);
  void unequip(EquipmentSlot slot);
  PItem removeItem(Item*);
  vector<PItem> removeItems(const vector<Item*>&);
  vector<PItem> removeAllItems();

  SERIALIZATION_DECL(Equipment);

  static map<EquipmentSlot, string> slotTitles;

  private:
  map<EquipmentSlot, Item*> SERIAL(items);
};

#endif
