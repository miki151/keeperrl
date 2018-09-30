#pragma once

#include "util.h"

struct Color;

// FXID() will always be invalid
using FXId = pair<int, int>;

namespace fx {

class FXManager;

// Simple validity check; given FX can still be dead even if ID is valid
inline bool valid(FXId id) {
  return id.first >= 0 && id.second > 0;
}

FXId spawnEffect(FXManager*, FXName, float x, float y);
FXId spawnEffect(FXManager*, FXName, float x, float y, Vec2 dir);
FXId spawnSnapshotEffect(FXManager*, FXName, float x, float y, float scalar0 = 0.0f, float scalar1 = 0.0f);

optional<FXName> name(FXManager*, FXId);

void setColor(FXManager*, FXId, Color, int paramIndex = 0);
void setScalar(FXManager*, FXId, float, int paramIndex = 0);
void setPos(FXManager*, FXId, float, float);
}
