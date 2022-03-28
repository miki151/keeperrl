#pragma once

#include "util.h"

RICH_ENUM(ExperienceType,
  MELEE,
  SPELL,
  ARCHERY
);

extern const char* getName(ExperienceType);
extern const char* getNameLowerCase(ExperienceType);
extern const EnumMap<ExperienceType, unordered_set<AttrType, CustomHash<AttrType>>>& getAttrIncreases();
extern optional<ExperienceType> getExperienceType(AttrType);
