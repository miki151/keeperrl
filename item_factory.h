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
#include "item_list_id.h"
#include "item_list.h"

class Item;
class Technology;
class ItemType;
class ItemAttributes;
class ItemList;
class GameConfig;

class ItemFactory {
  public:
  ItemFactory(const GameConfig*);
  ItemList get(ItemListId);

  static PItem corpse(const string& name, const string& rottenName, double weight, bool instantlyRotten = false,
      ItemClass = ItemClass::CORPSE,
      CorpseInfo corpseInfo = {UniqueEntity<Creature>::Id(), false, false, false});

  SERIALIZATION_DECL(ItemFactory)
  private:
  map<ItemListId, ItemList> SERIAL(lists);
};
