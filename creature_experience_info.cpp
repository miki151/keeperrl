#include "stdafx.h"
#include "creature_experience_info.h"
#include "creature.h"
#include "creature_attributes.h"

CreatureExperienceInfo getCreatureExperienceInfo(const Creature* c) {
  return CreatureExperienceInfo {
    c->getAttributes().getExpLevel(),
    c->getAttributes().getMaxExpLevel(),
    EnumMap<ExperienceType, optional<string>>(),
    c->getAttributes().getCombatExperience(),
    0
  };
}

