#pragma once

#include "util.h"

RICH_ENUM(CreatureStatus,
  PRISONER,
  CIVILIAN,
  LEADER,
  FIGHTER
);

class Color;
class TStringId;

extern Color getColor(CreatureStatus);
extern optional<TStringId> getDescription(CreatureStatus);
extern TStringId getName(CreatureStatus);
extern EnumSet<CreatureStatus> getDisplayedOnMinions();
