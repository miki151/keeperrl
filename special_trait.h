#pragma once

#include "util.h"
#include "experience_type.h"

struct ExtraTraining {
  ExperienceType HASH(type);
  int HASH(increase);
  COMPARE_ALL(type, increase)
  HASH_ALL(type, increase)
};

struct AttrBonus {
  AttrType HASH(attr);
  int HASH(increase);
  COMPARE_ALL(attr, increase)
  HASH_ALL(attr, increase)
};

using SpecialTrait = variant<ExtraTraining, LastingEffect, SkillId, AttrBonus>;

extern void applySpecialTrait(SpecialTrait, WCreature);
