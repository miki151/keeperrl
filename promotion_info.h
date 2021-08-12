#pragma once

#include "effect.h"
#include "view_id.h"

struct PromotionInfo {
  Effect SERIAL(applied);
  string SERIAL(name);
  ViewId SERIAL(viewId);
  SERIALIZE_ALL(name, viewId, applied)
};