#pragma once

#include "util.h"
#include "effect.h"
#include "spell_id.h"
#include "pretty_archive.h"
#include "lasting_effect.h"
#include "special_attr.h"
#include "attr_type.h"

class ItemPrefix;

namespace ItemPrefixes {

struct ItemAttrBonus {
  AttrType SERIAL(attr);
  int SERIAL(value);
  COMPARE_ALL(attr, value)
};

struct JoinPrefixes {
  vector<ItemPrefix> SERIAL(prefixes);
  COMPARE_ALL(prefixes)
};

struct VictimEffect {
  double SERIAL(chance);
  Effect SERIAL(effect);
  COMPARE_ALL(chance, effect)
};

struct AttackerEffect {
  Effect SERIAL(effect);
  COMPARE_ALL(effect)
};

struct Scale {
  double SERIAL(value);
  COMPARE_ALL(value)
};

struct Prefix {
  string SERIAL(value);
  HeapAllocated<ItemPrefix> SERIAL(prefix);
  COMPARE_ALL(value, prefix)
};

struct Suffix {
  string SERIAL(value);
  HeapAllocated<ItemPrefix> SERIAL(prefix);
  COMPARE_ALL(value, prefix)
};

using AssembledCreatureEffect = Effect;
using LastingEffect = LastingOrBuff;

#define VARIANT_TYPES_LIST\
  X(LastingEffect, 0)\
  X(VictimEffect, 1)\
  X(AttackerEffect, 2)\
  X(ItemAttrBonus, 3)\
  X(JoinPrefixes, 4)\
  X(SpellId, 5)\
  X(SpecialAttr, 6)\
  X(AssembledCreatureEffect, 7)\
  X(Scale, 8)\
  X(Prefix, 9)\
  X(Suffix, 10)\

#define VARIANT_NAME ItemPrefix

#include "gen_variant.h"
#include "gen_variant_serialize.h"
inline
#include "gen_variant_serialize_pretty.h"

#undef VARIANT_TYPES_LIST
#undef VARIANT_NAME

}

class ItemPrefix : public ItemPrefixes::ItemPrefix {
  public:
  using ItemPrefixes::ItemPrefix::ItemPrefix;
};

class ItemAttributes;
extern void applyPrefix(const ContentFactory*, const ItemPrefix&, ItemAttributes&);
extern optional<TString> getItemName(const ContentFactory*, const ItemPrefix&);
extern TString getGlyphName(const ContentFactory*, const ItemPrefix&);
extern vector<TString> getEffectDescription(const ContentFactory*, const ItemPrefix&);
