#pragma once

#include "util.h"

RICH_ENUM(TeamOrder,
    STAND_GROUND,
    FLEE
);

class TStringId;
extern TStringId getName(TeamOrder);
extern TStringId getDescription(TeamOrder);
extern Keybinding getKeybinding(TeamOrder);
extern bool conflict(TeamOrder, TeamOrder);
