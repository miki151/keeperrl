#ifndef _ITEM_FACTORY
#define _ITEM_FACTORY

#include <map>
#include <string>
#include <functional>

#include "util.h"
#include "name_generator.h"

class Item;


class ItemFactory {
  public:
  ItemFactory(const vector<pair<ItemId, double>>&,
      const vector<ItemId>& unique = vector<ItemId>());
  PItem random(Optional<int> seed = Nothing());
  vector<PItem> getAll();

  static ItemFactory dungeon();
  static ItemFactory armory();
  static ItemFactory potions();
  static ItemFactory scrolls();
  static ItemFactory mushrooms();
  static ItemFactory amulets();
  static ItemFactory villageShop();
  static ItemFactory dwarfShop();
  static ItemFactory goblinShop();
  static ItemFactory workshop();
  static ItemFactory singleType(ItemId);

  static PItem fromId(ItemId);
  static vector<PItem> fromId(ItemId, int num);
  static PItem corpse(const string& name, const string& rottenName, double weight, ItemType = ItemType::CORPSE,
      Item::CorpseInfo corpseInfo = {false});
  static PItem corpse(CreatureId, ItemType type = ItemType::CORPSE,
      Item::CorpseInfo corpseInfo = {false});
  static PItem trapItem(PTrigger trigger, string trapName);

  static void init();

  private:
  vector<ItemId> items;
  vector<double> weights;
  vector<ItemId> unique;
};

#endif
