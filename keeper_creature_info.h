#pragma once

#include "util.h"
#include "tech_id.h"
#include "name_generator_id.h"

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
  optional<NameGeneratorId> SERIAL(baseNameGen);
  bool SERIAL(noLeader) = false;
  template <class Archive>
  void serialize(Archive& ar, const unsigned int);
};

static_assert(std::is_nothrow_move_constructible<KeeperCreatureInfo>::value, "T should be noexcept MoveConstructible");
