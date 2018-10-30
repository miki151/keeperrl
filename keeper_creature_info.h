#pragma once

#include "avatar_variant.h"

struct KeeperCreatureInfo {
  CreatureId SERIAL(creatureId);
  TechVariant SERIAL(techVariant);
  TribeAlignment SERIAL(tribeAlignment);
  vector<string> SERIAL(immigrantGroups);
  vector<TechId> SERIAL(initialTech);
  vector<string> SERIAL(buildingGroups);
  vector<string> SERIAL(workshopGroups);
  SERIALIZE_ALL(NAMED(creatureId), NAMED(techVariant), NAMED(tribeAlignment), NAMED(immigrantGroups), NAMED(initialTech), NAMED(buildingGroups), NAMED(workshopGroups))
};
