#include "stdafx.h"
#include "creature_experience_info.h"
#include "creature.h"
#include "creature_attributes.h"
#include "content_factory.h"

int getMaxPromotionLevel(double quartersLuxury) {
  return int(2 + round(10 * quartersLuxury) * 0.1 / 3);
}

int getRequiredLuxury(double combatExp) {
  for (int l = 0;; ++l)
    if (getMaxPromotionLevel(l) >= combatExp / 5)
      return l;
  fail();
}

CreatureExperienceInfo getCreatureExperienceInfo(const ContentFactory* f, const Creature* c) {
  vector<CreatureExperienceInfo::TrainingInfo> training;
  for (auto& elem : c->getAttributes().getMaxExpLevel())
    training.push_back(CreatureExperienceInfo::TrainingInfo{
      elem.first,
      f->attrInfo.at(elem.first).viewId,
      f->attrInfo.at(elem.first).name,
      c->getAttributes().getExpLevel(elem.first),
      elem.second,
      none
    });
  double combatExp = c->getCombatExperience(false, false);
  return CreatureExperienceInfo {
    std::move(training),
    combatExp,
    c->getTeamExperience(),
    c->getCombatExperienceCap(),
    getRequiredLuxury(combatExp),
    0
  };
}

