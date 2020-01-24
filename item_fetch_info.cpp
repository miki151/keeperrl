#include "stdafx.h"
#include "item_fetch_info.h"
#include "item_index.h"
#include "item.h"


bool ItemFetchInfo::applies(const Collective* col, const Item* item) const {
  return index.visit(
        [&] (ItemIndex index) { return hasIndex(index, item); },
        [&] (CollectiveResourceId id) { return item->getResourceId() == id; }
) && predicate(col, item);
}

vector<Item*> ItemFetchInfo::getItems(const Collective* col, Position pos) const {
  return index.visit([&] (auto index) { return pos.getItems(index); })
      .filter([&](auto item) { return predicate(col, item); });
}
