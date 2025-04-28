#pragma once

#include "util.h"

RICH_ENUM(VillainType,
  MAIN,
  LESSER,
  MINOR,
  ALLY,
  PLAYER,
  NONE,
  RETIRED
);

extern const char* getName(VillainType);
extern bool blocksInfluence(VillainType);