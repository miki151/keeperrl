#pragma once

#include "util.h"
#include "settlement_info.h"
#include "furniture_type.h"
#include "creature_group.h"
#include "enemy_id.h"

struct FullZLevel {
  optional<EnemyId> SERIAL(enemy);
  SERIALIZE_ALL(NAMED(enemy))
};

struct WaterZLevel {
  FurnitureType SERIAL(waterType);
  CreatureList SERIAL(creatures);
  SERIALIZE_ALL(NAMED(waterType), NAMED(creatures))
};

MAKE_VARIANT2(ZLevelType, FullZLevel, WaterZLevel);

struct ZLevelInfo {
  ZLevelType SERIAL(type);
  optional<int> SERIAL(minDepth);
  optional<int> SERIAL(maxDepth);
  SERIALIZE_ALL(NAMED(type), NAMED(minDepth), NAMED(maxDepth))
};
