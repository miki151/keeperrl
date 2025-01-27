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

class TStringId;

extern TStringId getName(VillainType);
extern bool blocksInfluence(VillainType);