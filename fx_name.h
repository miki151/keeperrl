#pragma once

#include "util.h"

RICH_ENUM(FXName,
  TEST_SIMPLE,
  TEST_MULTI,
  WOOD_SPLINTERS,
  ROCK_SPLINTERS,
  ROCK_CLOUD,
  EXPLOSION,
  RIPPLE,
  SAND_DUST,
  WATER_SPLASH,
  FIRE,
  CIRCULAR_BLAST,
  AIR_BLAST,
  MAGIC_MISSILE,
  FIREBALL,
  SLEEP,
  INSANITY,
  BLIND,
  PEACEFULNESS,
  SLOW,
  SLOW2,
  SPEED,
  FLYING,
  FLYING2
);

bool fxesAvailable();
