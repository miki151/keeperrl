#pragma once

#include "util.h"

RICH_ENUM(ItemIndex,
  GOLD,
  WOOD,
  IRON,
  STEEL,
  STONE,
  REVIVABLE_CORPSE,
  CORPSE,
  WEAPON,
  TRAP,
  MINION_EQUIPMENT,
  RANGED_WEAPON,
  CAN_EQUIP,
  FOR_SALE
);

extern const char* getName(ItemIndex, int count = 1);
