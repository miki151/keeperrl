#pragma once

#include "util.h"
#include "experience_type.h"
#include "body_part.h"
#include "color.h"
#include "item_type.h"
#include "pretty_archive.h"
#include "lasting_effect.h"
#include "skill.h"
#include "creature_predicate.h"
#include "special_attr.h"
#include "companion_info.h"

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

struct Lasting {
  LastingEffect SERIAL(effect);
  optional<TimeInterval> SERIAL(time);
  SERIALIZE_ALL(NAMED(effect), NAMED(time))
};

#define VARIANT_TYPES_LIST\
  X(ExtraTraining, 0)\
  X(Lasting, 1)\
  X(WorkshopType, 2)\
  X(AttrBonus, 3)\
  X(SpecialAttr, 4)\
  X(OneOfTraits, 5)\
  X(ExtraBodyPart, 6)\
  X(ExtraIntrinsicAttack, 7)\
  X(CompanionInfo, 8)\
  X(SkillId, 9)

#define VARIANT_NAME SpecialTrait

#include "gen_variant.h"
#include "gen_variant_serialize.h"
inline
#include "gen_variant_serialize_pretty.h"

#undef VARIANT_TYPES_LIST
#undef VARIANT_NAME


extern void applySpecialTrait(GlobalTime, SpecialTrait, Creature*, const ContentFactory*);
extern SpecialTrait transformBeforeApplying(SpecialTrait);

struct SpecialTraitInfo {
  double SERIAL(prob);
  vector<SpecialTrait> SERIAL(traits);
  optional<Color> SERIAL(colorVariant);
  SERIALIZE_ALL(NAMED(prob), NAMED(traits), NAMED(colorVariant))
};
