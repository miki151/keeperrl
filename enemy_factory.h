#pragma once

#include "util.h"
#include "level_maker.h"
#include "collective_config.h"
#include "villain_type.h"
#include "village_behaviour.h"
#include "attack_trigger.h"

struct EnemyInfo;

RICH_ENUM(EnemyId,
  KNIGHTS,
  WARRIORS,
  DWARVES,
  ELVES,
  ELEMENTALIST,
  ELEMENTALIST_ENTRY,
  LIZARDMEN,
  RED_DRAGON,
  GREEN_DRAGON,
  MINOTAUR,

  VILLAGE,
  BANDITS,
  NO_AGGRO_BANDITS,
  ENTS,
  DRIADS,
  CYCLOPS,
  SHELOB,
  HYDRA,
  KRAKEN,
  ANTS_OPEN,
  ANTS_CLOSED,
  CEMETERY,
  CEMETERY_ENTRY,

  DARK_ELVES,
  DARK_ELVES_ENTRY,
  GNOMES,
  GNOMES_ENTRY,
  OGRE_CAVE,
  HARPY_CAVE,
  ORC_VILLAGE,
  SOKOBAN,
  SOKOBAN_ENTRY,
  WITCH,
  DWARF_CAVE,
  KOBOLD_CAVE,
  HUMAN_COTTAGE,
  UNICORN_HERD,
  ELVEN_COTTAGE,

  TUTORIAL_VILLAGE
);


struct LevelConnection {
  enum Type {
    CRYPT,
    GNOMISH_MINES,
    TOWER,
    MAZE,
    SOKOBAN,
  };
  Type type;
  HeapAllocated<EnemyInfo> otherEnemy;
};

struct EnemyInfo {
  EnemyInfo(SettlementInfo s, CollectiveConfig c, optional<VillageBehaviour> v = none,
      optional<LevelConnection> = none);
  EnemyInfo& setVillainType(VillainType type);
  EnemyInfo& setId(EnemyId);
  EnemyInfo& setNonDiscoverable();
  SettlementInfo settlement;
  CollectiveConfig config;
  optional<VillageBehaviour> villain;
  optional<VillainType> villainType;
  optional<LevelConnection> levelConnection;
  optional<EnemyId> id;
  bool discoverable = true;
};

struct EnemyEvent;

class EnemyFactory {
  public:
  EnemyFactory(RandomGen&);
  EnemyInfo get(EnemyId);
  vector<EnemyEvent> getExternalEnemies();
  vector<EnemyInfo> getVaults();

  private:
  EnemyInfo getById(EnemyId);
  RandomGen& random;
};
