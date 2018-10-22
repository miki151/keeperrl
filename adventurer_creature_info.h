#pragma once

#include "avatar_variant.h"

struct AdventurerCreatureInfo {
  CreatureId SERIAL(creatureId);
  TribeAlignment SERIAL(tribeAlignment);
  SERIALIZE_ALL(creatureId, tribeAlignment);
};
