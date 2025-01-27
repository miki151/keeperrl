#pragma once

#include "effect.h"
#include "view_id.h"
#include "t_string.h"

struct PromotionInfo {
  Effect SERIAL(applied);
  TString SERIAL(name);
  ViewId SERIAL(viewId);
  SERIALIZE_ALL(name, viewId, applied)
};