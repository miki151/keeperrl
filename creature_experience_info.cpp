#include "stdafx.h"
#include "creature_experience_info.h"
#include "creature.h"
#include "creature_attributes.h"
#include "content_factory.h"

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
  return CreatureExperienceInfo {
    std::move(training),
    c->getCombatExperience(false, false),
    c->getTeamExperience(),
    c->getCombatExperienceCap(),
    0
  };
}

