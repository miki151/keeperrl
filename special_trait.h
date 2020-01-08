#pragma once

#include "util.h"
#include "experience_type.h"
#include "body_part.h"
#include "color.h"
#include "item_type.h"

struct ExtraTraining {
  ExperienceType SERIAL(type);
  int SERIAL(increase);
  SERIALIZE_ALL(type, increase)
};

struct ExtraBodyPart {
  BodyPart SERIAL(part);
  int SERIAL(count);
  SERIALIZE_ALL(part, count)
};

struct ExtraIntrinsicAttack {
  BodyPart SERIAL(part);
  ItemType SERIAL(item);
  SERIALIZE_ALL(part, item)
};

struct AttrBonus {
  AttrType SERIAL(attr);
  int SERIAL(increase);
  SERIALIZE_ALL(attr, increase)
};

class SpecialTrait;

struct OneOfTraits {
  vector<SpecialTrait> SERIAL(traits);
  SERIALIZE_ALL(traits)
};

MAKE_VARIANT2(SpecialTrait, ExtraTraining, LastingEffect, SkillId, AttrBonus, OneOfTraits, ExtraBodyPart, ExtraIntrinsicAttack);

extern void applySpecialTrait(SpecialTrait, Creature*, const ContentFactory*);
extern SpecialTrait transformBeforeApplying(SpecialTrait);

struct SpecialTraitInfo {
  double SERIAL(prob);
  vector<SpecialTrait> SERIAL(traits);
  optional<Color> SERIAL(colorVariant);
  SERIALIZE_ALL(NAMED(prob), NAMED(traits), NAMED(colorVariant))
};
