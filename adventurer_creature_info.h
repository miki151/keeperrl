#pragma once

#include "util.h"

struct AdventurerCreatureInfo {
  vector<CreatureId> SERIAL(creatureId);
  TribeAlignment SERIAL(tribeAlignment);
  string SERIAL(description);
  optional<string> SERIAL(unlock);
  SERIALIZE_ALL(NAMED(creatureId), NAMED(tribeAlignment), NAMED(description), NAMED(unlock))
};
