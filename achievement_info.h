#pragma once

#include "util.h"
#include "view_id.h"
#include "t_string.h"

struct AchievementInfo {
  TString SERIAL(name);
  TString SERIAL(description);
  ViewIdList SERIAL(viewId);
  SERIALIZE_ALL(viewId, name, description)
};