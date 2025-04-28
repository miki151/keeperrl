#pragma once

#include "util.h"
#include "view_id.h"

struct AchievementInfo {
  string SERIAL(name);
  string SERIAL(description);
  ViewIdList SERIAL(viewId);
  SERIALIZE_ALL(viewId, name, description)
};