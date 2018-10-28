#pragma once

#include "avatar_variant.h"

struct KeeperCreatureInfo {
  CreatureId SERIAL(creatureId);
  TechVariant SERIAL(techVariant);
  TribeAlignment SERIAL(tribeAlignment);
  vector<string> SERIAL(immigrantGroups);
  vector<TechId> SERIAL(initialTech);
  vector<string> SERIAL(buildingGroups);
  SERIALIZE_ALL(creatureId, techVariant, tribeAlignment, immigrantGroups, initialTech, buildingGroups)
};
