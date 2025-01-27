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

class TStringId;
TStringId getName(BedType);
TStringId getPlural(BedType);
