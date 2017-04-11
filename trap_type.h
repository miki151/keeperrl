#pragma once

#include "util.h"

RICH_ENUM(TrapType,
  BOULDER,
  POISON_GAS,
  ALARM,
  WEB,
  SURPRISE,
  TERROR
);

class EffectType;

extern string getTrapName(TrapType);
extern ViewId getTrapViewId(TrapType);
