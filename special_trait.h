#pragma once

#include "util.h"
#include "experience_type.h"
#include "body_part.h"
#include "color.h"

struct ExtraTraining {
  ExperienceType HASH(type);
  int HASH(increase);
  COMPARE_ALL(type, increase)
  HASH_ALL(type, increase)
};

struct ExtraBodyPart {
  BodyPart HASH(part);
  int HASH(count);
  COMPARE_ALL(part, count)
  HASH_ALL(part, count)
};

struct AttrBonus {
  AttrType HASH(attr);
  int HASH(increase);
  COMPARE_ALL(attr, increase)
  HASH_ALL(attr, increase)
};

class SpecialTrait;

struct OneOfTraits {
  vector<SpecialTrait> HASH(traits);
  COMPARE_ALL(traits)
  HASH_ALL(traits)
};

MAKE_VARIANT2(SpecialTrait, ExtraTraining, LastingEffect, SkillId, AttrBonus, OneOfTraits, ExtraBodyPart);

extern void applySpecialTrait(SpecialTrait, Creature*);
extern SpecialTrait transformBeforeApplying(SpecialTrait);

struct SpecialTraitInfo {
  double SERIAL(prob);
  vector<SpecialTrait> SERIAL(traits);
  optional<Color> SERIAL(colorVariant);
  SERIALIZE_ALL(NAMED(prob), NAMED(traits), NAMED(colorVariant))
};
