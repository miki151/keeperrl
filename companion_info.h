#pragma once

#include "util.h"
#include "attr_type.h"

struct CompanionInfo {
  int SERIAL(count);
  vector<CreatureId> SERIAL(creatures);
  bool SERIAL(spawnAway);
  optional<AttrType> SERIAL(statsBase);
  double SERIAL(summonFreq);
  COMPARE_ALL(count, spawnAway, statsBase, summonFreq, creatures)
};
