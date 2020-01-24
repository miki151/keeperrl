#pragma once

#include "util.h"
#include "resource_id.h"

typedef function<bool(const Collective*, const Item*)> CollectiveItemPredicate;
class Position;
struct ItemFetchInfo {
  variant<ItemIndex, CollectiveResourceId> index;
  bool applies(const Collective*, const Item*) const;
  vector<Item*> getItems(const Collective*, Position) const;
  CollectiveItemPredicate predicate;
  StorageId storageId;
  CollectiveWarning warning;
};
