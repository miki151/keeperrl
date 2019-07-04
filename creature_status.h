#pragma once

#include "util.h"

RICH_ENUM(CreatureStatus,
  PRISONER,
  CIVILIAN,
  LEADER,
  FIGHTER
);

class Color;

extern Color getColor(CreatureStatus);
extern optional<const char*> getDescription(CreatureStatus);
extern const char* getName(CreatureStatus);
extern EnumSet<CreatureStatus> getDisplayedOnMinions();
