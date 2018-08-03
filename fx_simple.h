#pragma once

#include "util.h"

class Color;

namespace fx {

// This interface is not very efficient, but it's simple to use

pair<int, int> spawnEffect(const char *name, Vec2 pos);
pair<int, int> spawnEffect(const char *name, Vec2 pos, Vec2 dir);
bool isAlive(pair<int, int>);
void kill(pair<int, int>);

void setColor(pair<int, int>, Color, int paramIndex = 0);
void setScalar(pair<int, int>, float, int paramIndex = 0);
void setDir(pair<int, int>, Dir, int paramIndex = 0);
}
