#pragma once

#include "util.h"
#include "experience_type.h"
#include "view_id.h"

class ContentFactory;

struct CreatureExperienceInfo {
  EnumMap<ExperienceType, double> HASH(level);
  EnumMap<ExperienceType, int> HASH(limit);
  EnumMap<ExperienceType, optional<string>> HASH(warning);
  EnumMap<ExperienceType, vector<pair<string, ViewId>>> HASH(attributes);
  double HASH(combatExperience);
  double HASH(teamExperience);
  int HASH(numAvailableUpgrades);
  HASH_ALL(level, limit, warning, combatExperience, teamExperience, numAvailableUpgrades, attributes)
};

extern CreatureExperienceInfo getCreatureExperienceInfo(const ContentFactory*, const Creature*);
