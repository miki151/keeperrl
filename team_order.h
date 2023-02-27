#pragma once

#include "util.h"

RICH_ENUM(TeamOrder,
    STAND_GROUND,
    FLEE
);

extern const char* getName(TeamOrder);
extern const char* getDescription(TeamOrder);
extern Keybinding getKeybinding(TeamOrder);
extern bool conflict(TeamOrder, TeamOrder);
