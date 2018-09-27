#pragma once

#include "util.h"

RICH_ENUM(FXName,
  NO_EFFECT,
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
  FIRE_SPHERE,
  CIRCULAR_SPELL,
  CIRCULAR_BLAST,
  AIR_BLAST,
  MAGIC_MISSILE_OLD,
  MAGIC_MISSILE,
  MAGIC_MISSILE_SPLASH,
  FIREBALL,
  SLEEP,
  BLIND,
  GLITTERING,
  LABORATORY,
  FORGE,
  WORKSHOP,
  JEWELER,
  SPIRAL,
  SPIRAL2,
  SPEED,
  FLYING,
  DEBUFF,
  TELEPORT_OUT,
  TELEPORT_IN,
  SPAWN
);

bool fxesAvailable();
