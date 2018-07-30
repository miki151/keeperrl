#pragma once

#include "util.h"

class Color;

namespace fx {

// This interface is not very efficient, but it's simple to use

pair<int, int> spawnEffect(const char *name, Vec2);
bool isAlive(pair<int, int>);
void kill(pair<int, int>);

void setColor(pair<int, int>, Color, int param_index = 0);
void setScalar(pair<int, int>, float, int param_index = 0);
void setDir(pair<int, int>, Dir, int param_index = 0);
}
