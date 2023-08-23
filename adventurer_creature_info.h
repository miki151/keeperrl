#pragma once

#include "util.h"
#include "villain_group.h"

struct AdventurerCreatureInfo {
  vector<CreatureId> SERIAL(creatureId);
  TribeAlignment SERIAL(tribeAlignment);
  string SERIAL(description);
  optional<string> SERIAL(unlock);
  vector<VillainGroup> SERIAL(villainGroups);
  SERIALIZE_ALL(NAMED(creatureId), NAMED(tribeAlignment), NAMED(description), NAMED(unlock), NAMED(villainGroups))
};
