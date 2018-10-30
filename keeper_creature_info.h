#pragma once

#include "avatar_variant.h"

struct KeeperCreatureInfo {
  CreatureId SERIAL(creatureId);
  TribeAlignment SERIAL(tribeAlignment);
  vector<string> SERIAL(immigrantGroups);
  vector<TechId> SERIAL(technology);
  vector<TechId> SERIAL(initialTech);
  vector<string> SERIAL(buildingGroups);
  vector<string> SERIAL(workshopGroups);
  SERIALIZE_ALL(NAMED(creatureId), NAMED(tribeAlignment), NAMED(immigrantGroups), NAMED(technology), NAMED(initialTech), NAMED(buildingGroups), NAMED(workshopGroups))
};
