#pragma once

#include "util.h"

RICH_ENUM(AttrType,
  DAMAGE,
  DEFENSE,
  SPELL_DAMAGE,
  SPELL_DEFENSE,
  RANGED_DAMAGE,
  SPEED
);

extern string getName(AttrType);
extern AttrType getCorrespondingDefense(AttrType);
