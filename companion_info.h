#pragma once

#include "util.h"
#include "attr_type.h"

struct CompanionInfo {
  int SERIAL(count) = 1;
  bool SERIAL(spawnAway) = true;
  optional<AttrType> SERIAL(statsBase);
  double SERIAL(summonFreq);
  vector<CreatureId> SERIAL(creatures);
  bool SERIAL(hostile) = false;
  bool SERIAL(getsKillCredit) = false;
  SERIALIZE_ALL(OPTION(count), OPTION(spawnAway), OPTION(statsBase), NAMED(summonFreq), NAMED(creatures), OPTION(hostile), OPTION(getsKillCredit))
};
