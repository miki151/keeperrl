#include "stdafx.h"
#include "creature_experience_info.h"
#include "creature.h"
#include "creature_attributes.h"
#include "content_factory.h"  

CreatureExperienceInfo getCreatureExperienceInfo(const ContentFactory* f, const Creature* c) {
  return CreatureExperienceInfo {
    c->getAttributes().getExpLevel(),
    c->getAttributes().getMaxExpLevel(),
    EnumMap<ExperienceType, optional<string>>(),
    EnumMap<ExperienceType, vector<pair<string, ViewId>>>([f](ExperienceType type) {
      vector<pair<string, ViewId>> v;
      for (auto attr : getAttrIncreases()[type]) {
        auto& info = f->attrInfo.at(attr);
        v.push_back(make_pair(info.name, info.viewId));
      }
      return v;
    }),
    c->getCombatExperience(),
    0
  };
}

