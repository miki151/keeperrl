#pragma once

#include "util.h"

RICH_ENUM(CreatureStatus,
  CIVILIAN,
  LEADER,
  FIGHTER
);

class Color;

extern Color getColor(CreatureStatus);
extern const char* getDescription(CreatureStatus);
extern const char* getName(CreatureStatus);
