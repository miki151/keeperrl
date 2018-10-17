#pragma once

#include "util.h"

struct AvatarInfo {
  PCreature playerCreature;
  enum ImmigrationVariant {
    DARK_MAGE,
    DARK_KNIGHT,
    WHITE_KNIGHT
  } impVariant;
};
