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
  FOR_SALE,
  HEALING_ITEM
);

extern const char* getName(ItemIndex, int count = 1);
extern function<bool(const WItem)> getIndexPredicate(ItemIndex);
