#pragma once

#include "enemy_id.h"

struct RetiredEnemyInfo {
  EnemyId SERIAL(enemyId);
  VillainType SERIAL(villainType);
  SERIALIZE_ALL(enemyId, villainType)
};
