#pragma once

#include "avatar_variant.h"

struct KeeperCreatureInfo {
  CreatureId SERIAL(creatureId);
  TechVariant SERIAL(techVariant);
  WorkerVariant SERIAL(initialWorkerVariant);
  TribeAlignment SERIAL(tribeAlignment);
  ImmigrantsVariant SERIAL(immigrantsVariant);
  vector<TechId> SERIAL(initialTech);
  SERIALIZE_ALL(creatureId, techVariant, initialWorkerVariant, tribeAlignment, immigrantsVariant, initialTech)
};
