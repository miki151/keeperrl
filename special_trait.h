#pragma once

#include "util.h"
#include "experience_type.h"

struct ExtraTraining {
  ExperienceType SERIAL(type); // HASH(type)
  int SERIAL(increase); // HASH(increase)
  SERIALIZE_ALL(type, increase)
  HASH_ALL(type, increase)
};

struct AttrBonus {
  AttrType SERIAL(attr); // HASH(attr)
  int SERIAL(increase); // HASH(increase)
  SERIALIZE_ALL(attr, increase)
  HASH_ALL(attr, increase)
};

using SpecialTrait = variant<ExtraTraining, LastingEffect, SkillId, AttrBonus>;

extern void applySpecialTrait(SpecialTrait, WCreature);
