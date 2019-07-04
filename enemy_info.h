#pragma once

#include "settlement_info.h"
#include "villain_type.h"
#include "collective_config.h"
#include "village_behaviour.h"
#include "attack_trigger.h"
#include "immigrant_info.h"
#include "enemy_id.h"

struct EnemyInfo;
class EnemyFactory;

struct LevelConnection {
  enum class Type;
  Type SERIAL(type);
  HeapAllocated<EnemyInfo> otherEnemy;
  EnemyId SERIAL(enemyId);
  bool SERIAL(deadInhabitants) = false;
  SERIALIZE_ALL(NAMED(type), NAMED(enemyId), OPTION(deadInhabitants))
};

RICH_ENUM(
    LevelConnection::Type,
    CRYPT,
    GNOMISH_MINES,
    TOWER,
    MAZE,
    SOKOBAN
);

struct BonesInfo {
  double SERIAL(probability);
  vector<EnemyId> SERIAL(enemies);
  SERIALIZE_ALL(probability, enemies)
};

struct EnemyInfo {
  EnemyInfo(SettlementInfo s, CollectiveConfig c, optional<VillageBehaviour> v = none,
      optional<LevelConnection> = none);
  SERIALIZATION_CONSTRUCTOR(EnemyInfo)
  EnemyInfo& setVillainType(VillainType type);
  EnemyInfo& setId(EnemyId);
  EnemyInfo& setImmigrants(vector<ImmigrantInfo>);
  EnemyInfo& setNonDiscoverable();
  void updateCreateOnBones(const EnemyFactory&);
  SettlementInfo SERIAL(settlement);
  CollectiveConfig SERIAL(config);
  optional<VillageBehaviour> SERIAL(behaviour);
  optional<VillainType> villainType;
  optional<LevelConnection> SERIAL(levelConnection);
  optional<EnemyId> id;
  vector<ImmigrantInfo> SERIAL(immigrants);
  bool SERIAL(discoverable) = true;
  optional<BonesInfo> SERIAL(createOnBones);
  SERIALIZE_ALL(NAMED(settlement), OPTION(config), NAMED(behaviour), NAMED(levelConnection), OPTION(immigrants), OPTION(discoverable), NAMED(createOnBones))
};
