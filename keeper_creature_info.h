#pragma once

#include "util.h"
#include "tech_id.h"
#include "name_generator_id.h"
#include "minion_trait.h"
#include "cost_info.h"
#include "villain_group.h"
#include "keeper_base_info.h"

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
  vector<string> SERIAL(zLevelGroups);
  string SERIAL(description);
  vector<SpecialTrait> SERIAL(specialTraits);
  optional<NameGeneratorId> SERIAL(baseNameGen);
  EnumSet<MinionTrait> SERIAL(minionTraits) = {MinionTrait::LEADER};
  int SERIAL(maxPopulation) = 10;
  int SERIAL(immigrantInterval) = 140;
  string SERIAL(populationString) = "population";
  bool SERIAL(noLeader) = false;
  bool SERIAL(prisoners) = true;
  vector<string> SERIAL(endlessEnemyGroups) = {"basic"};
  optional<string> SERIAL(unlock);
  vector<CostInfo> SERIAL(credit);
  vector<string> SERIAL(flags);
  vector<VillainGroup> SERIAL(villainGroups);
  bool SERIAL(requireQuartersForExp) = true;
  optional<KeeperBaseInfo> SERIAL(startingBase) = none;
  template <class Archive>
  void serialize(Archive& ar, const unsigned int);
};

static_assert(std::is_nothrow_move_constructible<KeeperCreatureInfo>::value, "T should be noexcept MoveConstructible");
