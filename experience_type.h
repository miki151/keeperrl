#pragma once

#include "util.h"

RICH_ENUM(ExperienceType,
  MELEE,
  SPELL,
  ARCHERY
);

extern const char* getName(ExperienceType);
extern const char* getNameLowerCase(ExperienceType);
extern const EnumMap<ExperienceType, unordered_map<AttrType, int, CustomHash<AttrType>>>& getAttrIncreases();
extern optional<pair<ExperienceType, int>> getExperienceType(AttrType);
