#pragma once

#include "util.h"
#include "item_type.h"

class ItemList {
  public:
  ItemList(const ItemList&);
  ItemList& operator = (const ItemList&);
  ItemList(vector<ItemType>);

  vector<PItem> random() &;

  SERIALIZATION_DECL(ItemList)
  ~ItemList();

  private:
  struct ItemInfo;
  ItemList(vector<ItemInfo>);
  ItemList& addItem(ItemInfo);
  ItemList& addUniqueItem(ItemType, Range count = Range::singleElem(1));
  ItemList& setRandomPrefixes(double chance);
  vector<ItemInfo> SERIAL(items);
  vector<pair<ItemType, Range>> SERIAL(unique);
};
