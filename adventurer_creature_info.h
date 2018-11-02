#pragma once

#include "util.h"

struct AdventurerCreatureInfo {
  CreatureId SERIAL(creatureId);
  TribeAlignment SERIAL(tribeAlignment);
  string SERIAL(description);
  SERIALIZE_ALL(creatureId, tribeAlignment, description);
};
