#pragma once

#include "util.h"
#include "enemy_info.h"

class Position;

RICH_ENUM(
  FurnitureOnBuilt,
  DOWN_STAIRS,
  UP_STAIRS,
  SET_ON_FIRE,
  PORTAL
);

void handleOnBuilt(Position, Furniture*, FurnitureOnBuilt);
optional<Position> getSecondPart(Position, const Furniture*, FurnitureOnBuilt);
