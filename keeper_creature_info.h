#pragma once

#include "util.h"
#include "tech_id.h"

class SpecialTrait;

struct KeeperCreatureInfo {
  KeeperCreatureInfo();
  STRUCT_DECLARATIONS(KeeperCreatureInfo)
  vector<CreatureId> SERIAL(creatureId);
  TribeAlignment SERIAL(tribeAlignment);
  vector<string> SERIAL(immigrantGroups);
  vector<TechId> SERIAL(technology);
  vector<TechId> SERIAL(initialTech);
  vector<string> SERIAL(buildingGroups);
  vector<string> SERIAL(workshopGroups);
  string SERIAL(description);
  vector<SpecialTrait> SERIAL(specialTraits);
  template <class Archive>
  void serialize(Archive& ar, const unsigned int);
};
