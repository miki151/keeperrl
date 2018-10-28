#pragma once

#include "avatar_variant.h"

struct KeeperCreatureInfo {
  CreatureId SERIAL(creatureId);
  TechVariant SERIAL(techVariant);
  TribeAlignment SERIAL(tribeAlignment);
  vector<string> SERIAL(immigrantGroups);
  vector<TechId> SERIAL(initialTech);
  SERIALIZE_ALL(creatureId, techVariant, tribeAlignment, immigrantGroups, initialTech)
};
