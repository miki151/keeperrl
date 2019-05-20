#pragma once

#include "util.h"
#include "view_id.h"

struct SavedGameInfo {
  struct MinionInfo {
    ViewId SERIAL(viewId);
    int SERIAL(level);
    SERIALIZE_ALL(viewId, level)
  };
  ViewId getViewId() const;
  vector<MinionInfo> SERIAL(minions);
  double SERIAL(dangerLevel);
  string SERIAL(name);
  int SERIAL(progressCount);
  SERIALIZE_ALL(minions, dangerLevel, name, progressCount)
};


