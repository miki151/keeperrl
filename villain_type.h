#pragma once

#include "util.h"

RICH_ENUM(VillainType,
  MAIN,
  LESSER,
  ALLY,
  PLAYER,
  NONE
);

extern const char* getName(VillainType);
extern bool blocksInfluence(VillainType);