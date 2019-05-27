#pragma once

#include "util.h"
#include "villain_type.h"

struct EnemyInfo;

RICH_ENUM(EnemyId,
  KNIGHTS,
  WARRIORS,
  DWARVES,
  ELVES,
  ELEMENTALIST,
  ELEMENTALIST_ENTRY,
  LIZARDMEN,
  LIZARDMEN_COTTAGE,
  RED_DRAGON,
  GREEN_DRAGON,
  MINOTAUR,

  VILLAGE,
  BANDITS,
  NO_AGGRO_BANDITS,
  COTTAGE_BANDITS,
  ENTS,
  DRIADS,
  CYCLOPS,
  SHELOB,
  HYDRA,
  KRAKEN,
  ANTS_OPEN,
  ANTS_CLOSED,
  ANTS_CLOSED_SMALL,
  CEMETERY,
  CEMETERY_ENTRY,

  DARK_ELVES_ALLY,
  DARK_ELVES_ENEMY,
  DARK_ELVES_ENTRY,
  DARK_ELF_CAVE,
  GNOMES,
  GNOMES_ENTRY,
  OGRE_CAVE,
  HARPY_CAVE,
  ORC_CAVE,
  DEMON_DEN_ABOVE,
  DEMON_DEN,
  ORC_VILLAGE,
  SOKOBAN,
  SOKOBAN_ENTRY,
  WITCH,
  DWARF_CAVE,
  KOBOLD_CAVE,
  HUMAN_COTTAGE,
  UNICORN_HERD,
  ELVEN_COTTAGE,
  ADA_GOLEMS,
  RUINS,
  RAT_CAVE,
  RAT_PEOPLE_CAVE,
  TEMPLE,
  EVIL_TEMPLE,

  TUTORIAL_VILLAGE
);


struct ExternalEnemy;
struct SettlementInfo;
class TribeId;
class NameGenerator;

class EnemyFactory {
  public:
  EnemyFactory(RandomGen&, NameGenerator*, map<EnemyId, EnemyInfo> enemies);
  EnemyFactory(const EnemyFactory&) = delete;
  EnemyFactory(EnemyFactory&&) = default;
  EnemyInfo get(EnemyId) const;
  vector<ExternalEnemy> getExternalEnemies() const;
  vector<ExternalEnemy> getHalloweenKids();
  vector<EnemyInfo> getVaults(TribeAlignment, TribeId allied) const;

  RandomGen& random;

  private:
  NameGenerator* nameGenerator;
  map<EnemyId, EnemyInfo> enemies;
};
