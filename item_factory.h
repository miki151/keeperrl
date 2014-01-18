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
  static ItemFactory workshop();
  static ItemFactory singleType(ItemId);

  static PItem fromId(ItemId);
  static vector<PItem> fromId(ItemId, int num);
  static PItem corpse(const string& name, const string& rottenName, double weight, ItemType = ItemType::CORPSE,
      Item::CorpseInfo corpseInfo = {false, false, false});
  static PItem corpse(CreatureId, ItemType type = ItemType::CORPSE,
      Item::CorpseInfo corpseInfo = {false, false, false});
  static PItem trapItem(PTrigger trigger, string trapName);

  static void init();

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
  vector<ItemId> items;
  vector<double> weights;
  vector<int> minCount;
  vector<int> maxCount;
  vector<ItemId> unique;
};

#endif
