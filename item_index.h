#pragma once

#include "util.h"

RICH_ENUM(ItemIndex,
  GOLD,
  WOOD,
  IRON,
  ADA,
  STONE,
  REVIVABLE_CORPSE,
  CORPSE,
  WEAPON,
  TRAP,
  MINION_EQUIPMENT,
  RANGED_WEAPON,
  CAN_EQUIP,
  FOR_SALE,
  RUNE,
  ASSEMBLED_MINION
);

extern const char* getName(ItemIndex, int count = 1);
extern bool hasIndex(ItemIndex, const Item*);
