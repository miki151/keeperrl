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
  HEALING_ITEM,
  RUNE
);

extern const char* getName(ItemIndex, int count = 1);
extern bool hasIndex(ItemIndex, const Item*);
