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

#include "inventory.h"
#include "enums.h"

RICH_ENUM(EquipmentSlot,
  WEAPON,
  RANGED_WEAPON,
  HELMET,
  GLOVES,
  BODY_ARMOR,
  BOOTS,
  AMULET,
  RINGS
);

class Equipment : public Inventory {
  public:
  vector<WItem> getSlotItems(EquipmentSlot slot) const;
  bool isEquipped(const WItem) const;
  bool canEquip(const WItem) const;
  void equip(WItem, EquipmentSlot, WCreature);
  void unequip(WItem, WCreature);
  PItem removeItem(WItem, WCreature);
  int getMaxItems(EquipmentSlot) const;
  const vector<WItem>& getAllEquipped() const;
  vector<PItem> removeItems(const vector<WItem>&, WCreature);
  vector<PItem> removeAllItems(WCreature);

  SERIALIZATION_DECL(Equipment);

  static map<EquipmentSlot, string> slotTitles;

  private:
  EnumMap<EquipmentSlot, vector<WItem>> SERIAL(items);
  vector<WItem> SERIAL(equipped);
};

