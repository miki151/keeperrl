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

#ifndef _ITEM_FACTORY
#define _ITEM_FACTORY

#include <map>
#include <string>
#include <functional>

#include "util.h"
#include "name_generator.h"
#include "item.h"

class Item;
class Technology;

class ItemFactory {
  public:
  vector<PItem> random(Optional<int> seed = Nothing());
  vector<PItem> getAll();

  static ItemFactory dungeon();
  static ItemFactory chest();
  static ItemFactory armory();
  static ItemFactory potions();
  static ItemFactory scrolls();
  static ItemFactory mushrooms();
  static ItemFactory amulets();
  static ItemFactory villageShop();
  static ItemFactory dwarfShop();
  static ItemFactory goblinShop();
  static ItemFactory dragonCave();
  static ItemFactory workshop(const vector<Technology*>& techs);
  static ItemFactory laboratory(const vector<Technology*>& techs);
  static ItemFactory singleType(ItemId);

  static PItem fromId(ItemId);
  static vector<PItem> fromId(ItemId, int num);
  static PItem corpse(const string& name, const string& rottenName, double weight, ItemType = ItemType::CORPSE,
      Item::CorpseInfo corpseInfo = {false, false, false});
  static PItem corpse(CreatureId, ItemType type = ItemType::CORPSE,
      Item::CorpseInfo corpseInfo = {false, false, false});
  static PItem trapItem(PTrigger trigger, string trapName);

  static void init();

  template <class Archive>
  static void registerTypes(Archive& ar);

  SERIALIZATION_DECL(ItemFactory);

  private:
  struct ItemInfo {
    ItemInfo(ItemId _id, double _weight) : id(_id), weight(_weight) {}
    ItemInfo(ItemId _id, double _weight, int minC, int maxC)
        : id(_id), weight(_weight), minCount(minC), maxCount(maxC) {}

    ItemId id;
    double weight;
    int minCount = 1;
    int maxCount = 2;
  };
  ItemFactory(const vector<ItemInfo>&, const vector<ItemId>& unique = vector<ItemId>());
  ItemFactory& addItem(ItemInfo);
  ItemFactory& addUniqueItem(ItemId);
  vector<ItemId> SERIAL(items);
  vector<double> SERIAL(weights);
  vector<int> SERIAL(minCount);
  vector<int> SERIAL(maxCount);
  vector<ItemId> SERIAL(unique);
};

#endif
