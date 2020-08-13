#pragma once

#include "util.h"
#include "item_type.h"

class ItemList {
  public:
  ItemList(const ItemList&);
  ItemList(ItemList&&) noexcept;
  ItemList& operator = (const ItemList&);
  ItemList(vector<ItemType>);
  vector<ItemType> getAllItems() const;

  vector<PItem> random(const ContentFactory*) &;

  SERIALIZATION_DECL(ItemList)
  ~ItemList();

  private:
  struct ItemInfo;
  ItemList(vector<ItemInfo>);
  ItemList& addItem(ItemInfo);
  ItemList& addUniqueItem(ItemType, Range count = Range::singleElem(1));
  ItemList& setRandomPrefixes(double chance);
  vector<ItemInfo> SERIAL(items);
  struct MultiItemInfo;
  vector<MultiItemInfo> SERIAL(multiItems);
  vector<pair<ItemType, Range>> SERIAL(unique);
};

static_assert(std::is_nothrow_move_constructible<ItemList>::value, "T should be noexcept MoveConstructible");
