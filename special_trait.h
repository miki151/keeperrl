#pragma once

#include "util.h"
#include "experience_type.h"
#include "body_part.h"
#include "color.h"
#include "item_type.h"
#include "pretty_archive.h"
#include "lasting_effect.h"
#include "skill.h"

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

#define VARIANT_TYPES_LIST\
  X(ExtraTraining, 0)\
  X(LastingEffect, 1)\
  X(SkillId, 2)\
  X(AttrBonus, 3)\
  X(OneOfTraits, 4)\
  X(ExtraBodyPart, 5)\
  X(ExtraIntrinsicAttack, 6)

#define VARIANT_NAME SpecialTrait

#include "gen_variant.h"
#include "gen_variant_serialize.h"
inline
#include "gen_variant_serialize_pretty.h"

#undef VARIANT_TYPES_LIST
#undef VARIANT_NAME


extern void applySpecialTrait(SpecialTrait, Creature*, const ContentFactory*);
extern SpecialTrait transformBeforeApplying(SpecialTrait);

struct SpecialTraitInfo {
  double SERIAL(prob);
  vector<SpecialTrait> SERIAL(traits);
  optional<Color> SERIAL(colorVariant);
  SERIALIZE_ALL(NAMED(prob), NAMED(traits), NAMED(colorVariant))
};
