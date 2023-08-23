#pragma once

#include "util.h"

RICH_ENUM(ExperienceType,
  MELEE,
  SPELL,
  ARCHERY,
  FORGE
);

extern const char* getName(ExperienceType);
extern const char* getNameLowerCase(ExperienceType);
extern const EnumMap<ExperienceType, HashMap<AttrType, int>>& getAttrIncreases();
extern optional<pair<ExperienceType, int>> getExperienceType(AttrType);
