#pragma once

#include "util.h"
#include "experience_type.h"

struct CreatureExperienceInfo {
  EnumMap<ExperienceType, double> HASH(level);
  EnumMap<ExperienceType, int> HASH(limit);
  EnumMap<ExperienceType, optional<string>> HASH(warning);
  double HASH(combatExperience);
  int HASH(numAvailableUpgrades);
  HASH_ALL(level, limit, warning, combatExperience, numAvailableUpgrades)
};

extern CreatureExperienceInfo getCreatureExperienceInfo(const Creature*);
