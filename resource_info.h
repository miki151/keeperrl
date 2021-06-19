#pragma once

#include "util.h"
#include "item_type.h"
#include "view_id.h"

struct ResourceInfo {
  optional<ItemType> SERIAL(itemId);
  vector<StorageId> SERIAL(storage);
  string SERIAL(name);
  optional<ViewId> SERIAL(viewId);
  optional<TutorialHighlight> SERIAL(tutorialHighlight);
  SERIALIZE_ALL(SKIP(itemId), SKIP(storage), NAMED(name), NAMED(viewId), NAMED(tutorialHighlight))
};
