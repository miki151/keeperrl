#pragma once

#include "util.h"
#include "special_trait.h"

struct KeeperCreatureInfo {
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
  void serialize(Archive& ar, const unsigned int version) {
    ar(NAMED(creatureId), NAMED(tribeAlignment), NAMED(immigrantGroups), NAMED(technology), NAMED(initialTech), NAMED(buildingGroups), NAMED(workshopGroups), NAMED(description));
    ar(OPTION(specialTraits));
  }
};
