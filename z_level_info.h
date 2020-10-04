#pragma once

#include "cereal/cereal.hpp"
#include "util.h"
#include "settlement_info.h"
#include "furniture_type.h"
#include "creature_group.h"
#include "enemy_id.h"

struct FullZLevel {
  optional<EnemyId> SERIAL(enemy);
  double SERIAL(attackChance) = 0;
  SERIALIZE_ALL(NAMED(enemy), OPTION(attackChance))
};

struct WaterZLevel {
  FurnitureType SERIAL(waterType);
  CreatureList SERIAL(creatures);
  SERIALIZE_ALL(NAMED(waterType), NAMED(creatures))
};

struct EnemyZLevel {
  EnemyId SERIAL(enemy);
  double SERIAL(attackChance) = 0;
  SERIALIZE_ALL(NAMED(enemy), OPTION(attackChance))
};

MAKE_VARIANT2(ZLevelType, FullZLevel, WaterZLevel, EnemyZLevel);

struct ZLevelInfo {
  ZLevelType SERIAL(type);
  optional<int> SERIAL(minDepth);
  optional<int> SERIAL(maxDepth);
  int SERIAL(width) = 140;
  bool SERIAL(guarantee) = false;
  template <class Archive>
  void serialize(Archive& ar1, const unsigned int version) {
    ar1(NAMED(type), NAMED(minDepth), NAMED(maxDepth), OPTION(width));
    if (version >= 1)
      ar1(OPTION(guarantee));
  }
};

CEREAL_CLASS_VERSION(ZLevelInfo, 1)

optional<ZLevelInfo> chooseZLevel(RandomGen&, const vector<ZLevelInfo>&, int depth);
