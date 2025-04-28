#pragma once

#include "util.h"
#include "attr_type.h"
#include "view_id.h"

class ContentFactory;

struct CreatureExperienceInfo {
  struct TrainingInfo {
    AttrType HASH(attr);
    ViewId HASH(viewId);
    string HASH(name);
    double HASH(level);
    int HASH(limit);
    optional<string> HASH(warning);
    HASH_ALL(attr, viewId, name, level, limit, warning)
  };
  vector<TrainingInfo> HASH(training);
  double HASH(combatExperience);
  double HASH(teamExperience);
  int HASH(combatExperienceCap);
  int HASH(requiredLuxury);
  int HASH(numAvailableUpgrades);
  HASH_ALL(training, combatExperience, teamExperience, combatExperienceCap, numAvailableUpgrades, requiredLuxury)
};

extern CreatureExperienceInfo getCreatureExperienceInfo(const ContentFactory*, const Creature*);
extern int getMaxPromotionLevel(double quartersLuxury);
extern int getRequiredLuxury(double combatExp);