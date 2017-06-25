#pragma once

#include "util.h"

RICH_ENUM(ExperienceType,
  TRAINING,
  STUDY
);

extern const char* getName(ExperienceType);
extern const char* getNameLowerCase(ExperienceType);
extern char getSymbol(ExperienceType);
