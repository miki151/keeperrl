#pragma once

#include "util.h"
#include "experience_type.h"

struct ExtraTraining {
  ExperienceType SERIAL(type); // HASH(type)
  int SERIAL(increase); // HASH(increase)
  SERIALIZE_ALL(type, increase)
  HASH_ALL(type, increase)
};

using SpecialTrait = variant<ExtraTraining, LastingEffect, SkillId>;

string getSpecialTraitAdjective(LastingEffect effect);
string getSpecialTraitAdjective(ExperienceType type);
string getSpecialTraitJobName(SkillId skill);
extern void applySpecialTrait(SpecialTrait, WCreature);
