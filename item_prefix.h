#pragma once

#include "util.h"
#include "effect.h"
#include "spell_id.h"

struct ItemAttrBonus {
  AttrType attr;
  int value;
  COMPARE_ALL(attr, value)
};

class ItemPrefix;

struct JoinPrefixes {
  vector<ItemPrefix> prefixes;
  COMPARE_ALL(prefixes)
};

struct VictimEffect {
  double chance;
  Effect effect;
  COMPARE_ALL(chance, effect)
};

struct AttackerEffect {
  Effect effect;
  COMPARE_ALL(effect)
};

MAKE_VARIANT2(ItemPrefix, LastingEffect, VictimEffect, AttackerEffect, ItemAttrBonus, JoinPrefixes, SpellId);

class ItemAttributes;
extern void applyPrefix(const ItemPrefix&, ItemAttributes&);
extern string getItemName(const ItemPrefix&);
extern string getGlyphName(const ItemPrefix&);
extern vector<string> getEffectDescription(const ItemPrefix&);
