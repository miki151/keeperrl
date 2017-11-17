#pragma once

#include "util.h"

RICH_ENUM(ExperienceType,
  MELEE,
  SPELL,
  ARCHERY
);

extern const char* getName(ExperienceType);
extern const char* getNameLowerCase(ExperienceType);
extern const EnumMap<ExperienceType, EnumSet<AttrType>>& getAttrIncreases();
extern optional<ExperienceType> getExperienceType(AttrType);
