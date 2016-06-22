#pragma once

#include "util.h"
#include "level_maker.h"
#include "collective_config.h"
#include "villain_type.h"
#include "village_behaviour.h"

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
  ENTS,
  DRIADS,
  CYCLOPS,
  SHELOB,
  HYDRA,
  ANTS_OPEN,
  ANTS_CLOSED,
  CEMETERY,
  CEMETERY_ENTRY,

  DARK_ELVES,
  DARK_ELVES_ENTRY,
  GNOMES,
  GNOMES_ENTRY,
  FRIENDLY_CAVE,
  ORC_VILLAGE,
  SOKOBAN,
  SOKOBAN_ENTRY,
  WITCH,
  DWARF_CAVE,
  KOBOLD_CAVE,
  HUMAN_COTTAGE,
  ELVEN_COTTAGE
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
      optional<LevelConnection> l = none);
  EnemyInfo& setVillainType(VillainType type);
  EnemyInfo& setSurprise();
  SettlementInfo settlement;
  CollectiveConfig config;
  optional<VillageBehaviour> villain;
  optional<VillainType> villainType;
  optional<LevelConnection> levelConnection;
};

class EnemyFactory {
  public:
  EnemyFactory(RandomGen&);
  EnemyInfo get(EnemyId);
  vector<EnemyInfo> getVaults();

  private:
  RandomGen& random;
};
