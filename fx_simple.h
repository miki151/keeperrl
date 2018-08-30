#pragma once

#include "util.h"

struct Color;

// FXID() will always be invalid
using FXId = pair<int, int>;

namespace fx {

// Simple validity check; given FX can still be dead even if ID is valid
inline bool valid(FXId id) {
  return id.first >= 0 && id.second > 0;
}

FXId spawnEffect(FXName, float x, float y);
FXId spawnEffect(FXName, float x, float y, Vec2 dir);
FXId spawnSnapshotEffect(FXName, float x, float y, float scalar0 = 0.0f, float scalar1 = 0.0f);

bool isAlive(FXId);
void kill(FXId, bool immediate = true);
optional<FXName> name(FXId);

void setColor(FXId, Color, int paramIndex = 0);
void setScalar(FXId, float, int paramIndex = 0);
void setDir(FXId, Dir, int paramIndex = 0);
void setPos(FXId, float, float);
}
