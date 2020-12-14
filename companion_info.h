#pragma once

#include "util.h"

struct CompanionInfo {
  int SERIAL(count);
  vector<CreatureId> SERIAL(creatures);
  bool SERIAL(spawnAway);
  bool SERIAL(updateStats);
  SERIALIZE_ALL(count, spawnAway, updateStats, creatures)
};
