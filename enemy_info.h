#pragma once

#include "settlement_info.h"
#include "villain_type.h"
#include "collective_config.h"
#include "village_behaviour.h"
#include "enemy_id.h"
#include "biome_id.h"

struct EnemyInfo;
class EnemyFactory;

RICH_ENUM(LevelConnectionDir, UP, DOWN);

RICH_ENUM(
    LevelType,
    BASIC,
    TOWER,
    DUNGEON,
    MINETOWN,
    MAZE,
    SOKOBAN,
    BLACK_MARKET,
    OUTBACK
);

struct LevelConnection {
  LevelConnectionDir SERIAL(direction) = LevelConnectionDir::DOWN;
  struct ExtraEnemy {
    EnemyId SERIAL(enemy);
    vector<EnemyInfo> enemyInfo;
    Range SERIAL(numLevels) = Range(1, 2);
    SERIALIZE_ALL(NAMED(enemy), OPTION(numLevels))
  };
  using MainEnemy = EmptyStruct<struct MainEnemyTag>;
  MAKE_VARIANT(EnemyLevelInfo, ExtraEnemy, MainEnemy);
  struct LevelInfo {
    Vec2 SERIAL(levelSize);
    LevelType SERIAL(levelType);
    EnemyLevelInfo SERIAL(enemy);
    bool SERIAL(isLit) = false;
    optional<string> SERIAL(name);
    bool SERIAL(canTransfer) = true;
    template <class Archive>
    void serialize(Archive& ar, unsigned int v);
  };
  EnemyLevelInfo SERIAL(topLevel);
  vector<LevelInfo> SERIAL(levels);
  SERIALIZE_ALL(NAMED(topLevel), NAMED(levels), OPTION(direction))
};

struct BonesInfo {
  double SERIAL(probability);
  vector<EnemyId> SERIAL(enemies);
  SERIALIZE_ALL(probability, enemies)
};

struct EnemyInfo {
  EnemyInfo(SettlementInfo s, CollectiveConfig c, optional<VillageBehaviour> v = none,
      optional<LevelConnection> = none);
  STRUCT_DECLARATIONS(EnemyInfo)
  SERIALIZATION_DECL(EnemyInfo)
  EnemyInfo& setVillainType(VillainType type);
  EnemyInfo& setId(EnemyId);
  EnemyInfo& setImmigrants(vector<ImmigrantInfo>);
  EnemyInfo& setNonDiscoverable();
  void updateCreateOnBones(const EnemyFactory&);
  PCollective buildCollective(ContentFactory*) const;
  SettlementInfo SERIAL(settlement);
  CollectiveConfig SERIAL(config);
  optional<VillageBehaviour> SERIAL(behaviour);
  optional<VillainType> villainType;
  optional<LevelConnection> SERIAL(levelConnection);
  optional<EnemyId> id;
  vector<BiomeId> SERIAL(biomes);
  vector<ImmigrantInfo> SERIAL(immigrants);
  bool SERIAL(discoverable) = true;
  optional<BonesInfo> SERIAL(createOnBones);
  optional<EnemyId> SERIAL(otherEnemy);
};
