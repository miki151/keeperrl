#pragma once

#include "util.h"
#include "item_type.h"

typedef function<bool(WConstCollective, WConstItem)> CollectiveItemPredicate;

struct ItemFetchInfo {
  ItemIndex index;
  CollectiveItemPredicate predicate;
  StorageId storageId;
  CollectiveWarning warning;
};

struct ResourceInfo {
  optional<StorageId> storageId;
  optional<ItemIndex> itemIndex;
  ItemType itemId;
  string name;
  ViewId viewId;
  bool dontDisplay;
  optional<TutorialHighlight> tutorialHighlight;
};
