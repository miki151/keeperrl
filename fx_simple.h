#pragma once

#include "util.h"

struct Color;

RICH_ENUM(FXName, TEST_SIMPLE, TEST_MULTI, WOOD_SPLINTERS, ROCK_SPLINTERS, ROCK_CLOUD, EXPLSION, RIPPLE, FEET_DUST,
          FIRE, CIRCULAR_BLAST, MAGIC_MISSILE, FIREBALL, SLEEP, INSANITY, BLIND, PEACEFULNESS, SLOW, SPEED, FLYING);

// FXID() will always be invalid
using FXId = pair<int, int>;

namespace fx {

// Simple validity check; given FX can still be dead even if ID is valid
inline bool valid(FXId id) {
  return id.first >= 0 && id.second > 0;
}

// This interface is not very efficient, but it's simple to use

FXId spawnEffect(FXName, double x, double y);
FXId spawnEffect(FXName, double x, double y, Vec2 dir);
bool isAlive(FXId);
void kill(FXId, bool immediate = true);
optional<FXName> name(FXId);

void setColor(FXId, Color, int paramIndex = 0);
void setScalar(FXId, float, int paramIndex = 0);
void setDir(FXId, Dir, int paramIndex = 0);
void setPos(FXId, double, double);
}
