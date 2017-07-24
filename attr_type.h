#pragma once

#include "util.h"

RICH_ENUM(AttrType,
  DAMAGE,
  DEFENSE,
  SPELL_DAMAGE,
  RANGED_DAMAGE,
  SPEED
);

extern string getName(AttrType);
