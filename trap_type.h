#pragma once

#include "util.h"

RICH_ENUM(TrapType,
  BOULDER,
  POISON_GAS,
  ALARM,
  WEB,
  SURPRISE,
  TERROR,
  FIRE_MINE
);

class EffectType;

extern string getTrapName(TrapType);
extern ViewId getTrapViewId(TrapType);
