#pragma once

#include "util.h"
#include "experience_type.h"
#include "body_part.h"
#include "color.h"
#include "item_type.h"

struct ExtraTraining {
  ExperienceType type;
  int increase;
  COMPARE_ALL(type, increase)
};

struct ExtraBodyPart {
  BodyPart part;
  int count;
  COMPARE_ALL(part, count)
};

struct ExtraIntrinsicAttack {
  BodyPart part;
  ItemType item;
  COMPARE_ALL(part, item)
};

struct AttrBonus {
  AttrType attr;
  int increase;
  COMPARE_ALL(attr, increase)
};

class SpecialTrait;

struct OneOfTraits {
  vector<SpecialTrait> traits;
  COMPARE_ALL(traits)
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
