#pragma once

#include "util.h"

RICH_ENUM2(unsigned char,
  TrapType,
  BOULDER,
  POISON_GAS,
  ALARM,
  WEB,
  SURPRISE,
  TERROR,
  FIRE
);

class EffectType;

extern string getTrapName(TrapType);
extern ViewId getTrapViewId(TrapType);
