#pragma once

#include "stdafx.h"
#include "util.h"
#include "enemy_info.h"

struct LevelMakerResult {
  PLevelMaker maker;
  optional<EnemyInfo> enemy;
  int levelWidth;
};

LevelMakerResult getLevelMaker(RandomGen& random, ContentFactory* contentFactory, TribeAlignment alignment,
    int depth, TribeId tribe, StairKey stairKey);
