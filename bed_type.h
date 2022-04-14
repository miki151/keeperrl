#pragma once

#include "util.h"

RICH_ENUM(
    BedType,
    BED,
    COFFIN,
    PRISON,
    CAGE
);

const char* getName(BedType);