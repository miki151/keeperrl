#pragma once

#include "stdafx.h"
#include "util.h"
#include "enemy_info.h"

struct LevelMakerResult {
  PLevelMaker maker;
  optional<EnemyInfo> enemy;
  int levelWidth;
};

LevelMakerResult getLevelMaker(RandomGen& random, ContentFactory* contentFactory, const vector<string>& zLevelGroups,
    int depth, TribeId tribe, StairKey stairKey);
