#pragma once

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
  SERIALIZE_ALL(NAMED(type), NAMED(minDepth), NAMED(maxDepth), OPTION(width))
};

optional<ZLevelInfo> chooseZLevel(RandomGen&, const vector<ZLevelInfo>&, int depth);
