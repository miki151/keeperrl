#pragma once

#include "util.h"
#include "item_type.h"

typedef function<const PositionSet&(WConstCollective)> StorageDestinationFun;
typedef function<bool(WConstCollective, WConstItem)> CollectiveItemPredicate;

struct ItemFetchInfo {
  ItemIndex index;
  CollectiveItemPredicate predicate;
  StorageDestinationFun destinationFun;
  bool oneAtATime;
  CollectiveWarning warning;
};

struct ResourceInfo {
  StorageDestinationFun storageDestination;
  optional<ItemIndex> itemIndex;
  ItemType itemId;
  string name;
  ViewId viewId;
  bool dontDisplay;
  optional<TutorialHighlight> tutorialHighlight;
};
