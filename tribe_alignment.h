#pragma once

#include "util.h"

RICH_ENUM(TribeAlignment,
  LAWFUL,
  EVIL
);

extern const char* getName(TribeAlignment);
