#pragma once

#include "util.h"

struct AdventurerCreatureInfo {
  vector<CreatureId> SERIAL(creatureId);
  TribeAlignment SERIAL(tribeAlignment);
  string SERIAL(description);
  SERIALIZE_ALL(creatureId, tribeAlignment, description)
};
