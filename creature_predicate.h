#pragma once

#include "util.h"
#include "lasting_effect.h"
#include "furniture_type.h"
#include "creature_id.h"
#include "gender.h"
#include "lasting_or_buff.h"
#include "body_material_id.h"
#include "attr_type.h"
#include "tile_gas_type.h"
#include "t_string.h"

#define SIMPLE_PREDICATE(Name) \
  struct Name { \
    SERIALIZE_EMPTY()\
  }

struct CreaturePredicate;

namespace CreaturePredicates {
SIMPLE_PREDICATE(Enemy);
SIMPLE_PREDICATE(Automaton);
SIMPLE_PREDICATE(Rider);
SIMPLE_PREDICATE(Hidden);
SIMPLE_PREDICATE(Indoors);
SIMPLE_PREDICATE(Night);
SIMPLE_PREDICATE(AIAfraidOf);
SIMPLE_PREDICATE(InTerritory);
SIMPLE_PREDICATE(Spellcaster);
SIMPLE_PREDICATE(Humanoid);
SIMPLE_PREDICATE(PopLimitReached);
SIMPLE_PREDICATE(IsClosedOffPigsty);
SIMPLE_PREDICATE(CanCreatureEnter);
SIMPLE_PREDICATE(SameTribe);
SIMPLE_PREDICATE(HasAnyHealth);
SIMPLE_PREDICATE(IsPlayer);

struct HatedBy {
  BuffId SERIAL(effect);
  SERIALIZE_ALL(effect)
};

struct Ingredient {
  string SERIAL(name);
  SERIALIZE_ALL(name)
};

struct EquipedIngredient {
  string SERIAL(name);
  SERIALIZE_ALL(name)
};

struct OnTheGround {
  string SERIAL(name);
  SERIALIZE_ALL(name)
};

struct Attacker {
  HeapAllocated<CreaturePredicate> SERIAL(pred);
  SERIALIZE_ALL(pred)
};

struct Name {
  TString SERIAL(name);
  HeapAllocated<CreaturePredicate> SERIAL(pred);
  SERIALIZE_ALL(name, pred)
};

struct Flag {
  string SERIAL(name);
  SERIALIZE_ALL(name)
};

struct CreatureFlag {
  string SERIAL(name);
  SERIALIZE_ALL(name)
};

struct Unlocked {
  UnlockId SERIAL(id);
  SERIALIZE_ALL(id)
};

struct Health {
  double SERIAL(from);
  double SERIAL(to);
  SERIALIZE_ALL(from, to)
};

struct Kills {
  int SERIAL(cnt);
  SERIALIZE_ALL(cnt)
};

struct Not {
  HeapAllocated<CreaturePredicate> SERIAL(pred);
  SERIALIZE_ALL(pred)
};

struct And {
  vector<CreaturePredicate> SERIAL(pred);
  SERIALIZE_ALL(pred)
};

struct Or {
  vector<CreaturePredicate> SERIAL(pred);
  SERIALIZE_ALL(pred)
};

struct Distance {
  optional<int> SERIAL(min);
  optional<int> SERIAL(max);
  SERIALIZE_ALL(min, max)
};

struct DistanceD {
  optional<double> SERIAL(min);
  optional<double> SERIAL(max);
  SERIALIZE_ALL(min, max)
};

struct Translate {
  Vec2 SERIAL(dir);
  HeapAllocated<CreaturePredicate> SERIAL(pred);
  SERIALIZE_ALL(dir, pred)
};

struct Area {
  int SERIAL(radius);
  HeapAllocated<CreaturePredicate> SERIAL(pred);
  SERIALIZE_ALL(radius, pred)
};

struct Frequency {
  int SERIAL(value);
  SERIALIZE_ALL(value)
};

using LastingEffect = LastingOrBuff;
using BodyMaterial = BodyMaterialId;

struct AttributeAtLeast {
  AttrType SERIAL(attr);
  int SERIAL(value);
  SERIALIZE_ALL(attr, value)
};

struct TimeOfDay {
  int SERIAL(min);
  int SERIAL(max);
  SERIALIZE_ALL(min, max);
};

struct MaxLevelBelow {
  AttrType SERIAL(type);
  int SERIAL(value);
  SERIALIZE_ALL(type, value)
};

struct ExperienceBelow {
  int SERIAL(value);
  SERIALIZE_ALL(value)
};

using ContainsGas = TileGasType;

#define CREATURE_PREDICATE_LIST\
  X(Enemy, 0)\
  X(Automaton, 1)\
  X(LastingEffect, 2)\
  X(CreatureStatus, 3)\
  X(BodyMaterial, 4)\
  X(HatedBy, 5)\
  X(Ingredient, 6)\
  X(OnTheGround, 7)\
  X(Flag, 8)\
  X(CreatureFlag, 9)\
  X(Name, 10)\
  X(Night, 11)\
  X(Indoors, 12)\
  X(Attacker, 13)\
  X(FurnitureType, 14)\
  X(Not, 15)\
  X(And, 16)\
  X(Or, 17)\
  X(Health, 18)\
  X(CreatureId, 19)\
  X(Distance, 20)\
  X(DistanceD, 21)\
  X(AIAfraidOf, 22)\
  X(InTerritory, 23)\
  X(Spellcaster, 24)\
  X(Humanoid, 25)\
  X(PopLimitReached, 26)\
  X(Translate, 27)\
  X(Unlocked, 28)\
  X(Gender, 29)\
  X(Rider, 30)\
  X(Kills, 31)\
  X(IsClosedOffPigsty, 32)\
  X(CanCreatureEnter, 33)\
  X(Area, 34)\
  X(Frequency, 35)\
  X(SameTribe, 36)\
  X(Hidden, 37)\
  X(AttributeAtLeast, 38)\
  X(ContainsGas, 39)\
  X(HasAnyHealth, 40)\
  X(TimeOfDay, 41)\
  X(MaxLevelBelow, 42)\
  X(EquipedIngredient, 43)\
  X(ExperienceBelow, 44)\
  X(IsPlayer, 45)\

#define VARIANT_NAME CreaturePredicate
#define VARIANT_TYPES_LIST CREATURE_PREDICATE_LIST

#include "gen_variant.h"

#undef VARIANT_TYPES_LIST
#undef VARIANT_NAME

template <class Archive>
void serialize(Archive& ar1, CreaturePredicate&);

}
class Position;
struct CreaturePredicate : CreaturePredicates::CreaturePredicate {
  using CreaturePredicates::CreaturePredicate::CreaturePredicate;
  bool apply(Position, const Creature* attacker) const;
  bool apply(const Creature*, const Creature* attacker) const;
  TString getName(const ContentFactory*) const;
  TString getNameInternal(const ContentFactory*, bool negated = false) const;
};
