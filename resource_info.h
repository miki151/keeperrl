#pragma once

#include "util.h"
#include "item_type.h"
#include "view_id.h"

struct ResourceInfo {
  optional<StorageId> SERIAL(storageId);
  optional<ItemType> SERIAL(itemId);
  string SERIAL(name);
  optional<ViewId> SERIAL(viewId);
  optional<TutorialHighlight> SERIAL(tutorialHighlight);
  SERIALIZE_ALL(NAMED(storageId), SKIP(itemId), NAMED(name), NAMED(viewId), NAMED(tutorialHighlight))
};
