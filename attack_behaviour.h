#pragma once

#include "enum_variant.h"
#include "creature_list.h"

EMPTY_STRUCT(KillLeader);
EMPTY_STRUCT(StealGold);
EMPTY_STRUCT(HalloweenKids);

struct KillMembers {
  int count;
  COMPARE_ALL(count)
};

using CampAndSpawn = CreatureList;

MAKE_VARIANT2(AttackBehaviour, KillLeader, StealGold, HalloweenKids, KillMembers, CampAndSpawn);
