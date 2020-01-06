#pragma once

#include "util.h"

RICH_ENUM(
    AutomatonSlot,
    HEAD,
    ARMS,
    LEGS,
    WINGS
);

const char* getName(AutomatonSlot);
