#pragma once

#include "util.h"

RICH_ENUM(ItemIndex,
  WEAPON,
  MINION_EQUIPMENT,
  RANGED_WEAPON,
  FOR_SALE,
  RUNE
);

extern const char* getName(ItemIndex, int count = 1);
extern bool hasIndex(ItemIndex, const Item*);
