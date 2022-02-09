#pragma once

#include "stdafx.h"
#include "util.h"
#include "enemy_info.h"

struct LevelMakerResult {
  PLevelMaker maker;
  vector<EnemyInfo> enemies;
};

LevelMakerResult getLevelMaker(RandomGen& random, ContentFactory* contentFactory, const vector<string>& zLevelGroups,
    int depth, TribeId tribe, Vec2 size);

LevelMakerResult getUpLevel(RandomGen& random, ContentFactory* contentFactory, int depth, Position);
