#pragma once

#include "util.h"

RICH_ENUM(AttrType,
  DAMAGE,
  DEFENSE,
  SPELL_DAMAGE,
  RANGED_DAMAGE
);

extern string getName(AttrType);
