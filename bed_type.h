#pragma once

#include "util.h"

RICH_ENUM(
    BedType,
    BED,
    COFFIN,
    PRISON,
    CAGE,
    STABLE
);

const char* getName(BedType);