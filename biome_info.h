#pragma once

#include "util.h"
#include "furniture_type.h"
#include "square_attrib.h"
#include "item_list_id.h"
#include "furniture_list_id.h"
#include "creature_list.h"
#include "enemy_id.h"
#include "view_id.h"

struct MountainInfo {
  int SERIAL(numMountainLevels) = 10;
  double SERIAL(lowlandRatio);
  double SERIAL(hillRatio);
  FurnitureType SERIAL(hill);
  FurnitureType SERIAL(grass);
  FurnitureType SERIAL(mountain);
  FurnitureType SERIAL(mountainDeep);
  FurnitureType SERIAL(mountainFloor);
  SERIALIZE_ALL(NAMED(lowlandRatio), NAMED(hillRatio), NAMED(hill), NAMED(grass), NAMED(mountain), NAMED(mountainDeep), NAMED(mountainFloor), OPTION(numMountainLevels))
};

struct ForestInfo {
  double SERIAL(ratio);
  double SERIAL(density);
  FurnitureType SERIAL(onType);
  FurnitureListId SERIAL(trees);
  SERIALIZE_ALL(NAMED(ratio), NAMED(density), NAMED(onType), NAMED(trees))
};

struct BiomeEnemyInfo {
  EnemyId SERIAL(id);
  Range SERIAL(count) = Range::singleElem(1);
  double SERIAL(probability) = 1;
  SERIALIZE_ALL(NAMED(id), OPTION(count), OPTION(probability))
};

struct KeeperBiomeInfo {
  ViewId SERIAL(viewId);
  string SERIAL(name);
  int SERIAL(priority);
  SERIALIZE_ALL(viewId, name, priority)
};

struct BiomeInfo {
  optional<FurnitureType> SERIAL(overrideWaterType);
  FurnitureType SERIAL(sandType) = FurnitureType("SAND");
  struct LakeInfo {
    Range SERIAL(size);
    Range SERIAL(count);
    optional<FurnitureType> SERIAL(treeType);
    SquareAttrib SERIAL(where) = SquareAttrib::LOWLAND;
    SERIALIZE_ALL(NAMED(size), NAMED(count), OPTION(treeType), OPTION(where))
  };
  optional<LakeInfo> SERIAL(lakes);
  ItemListId SERIAL(items);
  Range SERIAL(itemCount) = Range::singleElem(0);
  MountainInfo SERIAL(mountains);
  vector<ForestInfo> SERIAL(forests);
  CreatureList SERIAL(wildlife);
  vector<BiomeEnemyInfo> SERIAL(darkKeeperEnemies);
  vector<BiomeEnemyInfo> SERIAL(whiteKeeperEnemies);
  vector<BiomeEnemyInfo> SERIAL(darkKeeperBaseEnemies);
  vector<BiomeEnemyInfo> SERIAL(whiteKeeperBaseEnemies);
  vector<pair<Range, BiomeEnemyInfo>> SERIAL(mountainEnemies);
  optional<MusicType> SERIAL(overrideMusic);
  optional<KeeperBiomeInfo> SERIAL(keeperBiome);
  int SERIAL(sightRange) = 100;
  SERIALIZE_ALL(NAMED(overrideWaterType), OPTION(sandType), NAMED(lakes), OPTION(items), OPTION(itemCount), NAMED(mountains), OPTION(forests), OPTION(wildlife), OPTION(darkKeeperEnemies), OPTION(whiteKeeperEnemies), OPTION(darkKeeperBaseEnemies), OPTION(whiteKeeperBaseEnemies), NAMED(overrideMusic), NAMED(keeperBiome), OPTION(sightRange), OPTION(mountainEnemies))
};

static_assert(std::is_nothrow_move_constructible<BiomeInfo>::value, "T should be noexcept MoveConstructible");
