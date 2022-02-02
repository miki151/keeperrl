#pragma once

#include "util.h"
#include "view_id.h"

struct EquipmentGroup {
  string SERIAL(name);
  ViewId SERIAL(viewId);
  bool SERIAL(autoLocked) = false;
  SERIALIZE_ALL(NAMED(name), NAMED(viewId), OPTION(autoLocked))
};