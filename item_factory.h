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

#include <map>
#include <string>
#include <functional>

#include "util.h"
#include "corpse_info.h"
#include "item_class.h"

class Item;
class Technology;
class ItemType;
class ItemAttributes;

class ItemFactory {
  public:
  ItemFactory(const ItemFactory&);
  ItemFactory& operator = (const ItemFactory&);
  
  vector<PItem> random();

  static ItemFactory dungeon();
  static ItemFactory chest();
  static ItemFactory armory();
  static ItemFactory potions();
  static ItemFactory scrolls();
  static ItemFactory mushrooms(bool onlyGood = false);
  static ItemFactory amulets();
  static ItemFactory villageShop();
  static ItemFactory dwarfShop();
  static ItemFactory orcShop();
  static ItemFactory gnomeShop();
  static ItemFactory dragonCave();
  static ItemFactory minerals();
  static ItemFactory singleType(ItemType, Range count = Range(1, 2));

  static PItem fromId(ItemType);
  static vector<PItem> fromId(ItemType, int num);
  static PItem corpse(const string& name, const string& rottenName, double weight, ItemClass = ItemClass::CORPSE,
      CorpseInfo corpseInfo = {UniqueEntity<Creature>::Id(), false, false, false});

  static void init();

  SERIALIZATION_DECL(ItemFactory);
  ~ItemFactory();
  
  private:
  struct ItemInfo;
  ItemFactory(const vector<ItemInfo>&);
  static ItemAttributes getAttributes(ItemType);
  ItemFactory& addItem(ItemInfo);
  ItemFactory& addUniqueItem(ItemType, Range count = Range::singleElem(1));
  vector<ItemType> SERIAL(items);
  vector<double> SERIAL(weights);
  vector<Range> SERIAL(count);
  vector<pair<ItemType, Range>> SERIAL(uniqueCounts);
};
