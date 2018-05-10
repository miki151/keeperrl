#pragma once

#include "util.h"

struct AvatarInfo {
  PCreature playerCreature;
  enum ImpVariant {
    IMPS,
    GOBLINS,
  } impVariant;
};
