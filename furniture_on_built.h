#pragma once

#include "util.h"

class Position;

RICH_ENUM(FurnitureOnBuilt,
    DOWN_STAIRS
);

void handleOnBuilt(Position, Creature*, FurnitureOnBuilt);
