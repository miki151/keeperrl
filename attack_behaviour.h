#pragma once

#include "creature_list.h"

EMPTY_STRUCT(KillLeader);
EMPTY_STRUCT(StealGold);
EMPTY_STRUCT(HalloweenKids);

struct KillMembers {
  int SERIAL(count);
  COMPARE_ALL(count)
};

using CampAndSpawn = CreatureList;


#define VARIANT_TYPES_LIST\
  X(KillLeader, 0)\
  X(StealGold, 1)\
  X(HalloweenKids, 2)\
  X(KillMembers, 3)\
  X(CampAndSpawn, 4)

#define VARIANT_NAME AttackBehaviour

#include "gen_variant.h"
#include "gen_variant_serialize.h"
inline
#include "gen_variant_serialize_pretty.h"

#undef VARIANT_TYPES_LIST
#undef VARIANT_NAME
