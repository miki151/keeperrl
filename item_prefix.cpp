#include "stdafx.h"
#include "item_prefix.h"
#include "item_attributes.h"
#include "lasting_effect.h"
#include "effect.h"

void applyPrefix(const ContentFactory* factory, const ItemPrefix& prefix, ItemAttributes& attr) {
  attr.prefixes.push_back(getItemName(factory, prefix));
  prefix.visit(
      [&](LastingEffect effect) {
        attr.equipedEffect.push_back(effect);
      },
      [&](const AttackerEffect& e) {
        attr.weaponInfo.attackerEffect.push_back(e.effect);
      },
      [&](const VictimEffect& e) {
        attr.weaponInfo.victimEffect.push_back(e);
      },
      [&](ItemAttrBonus bonus) {
        attr.modifiers[bonus.attr] += bonus.value;
      },
      [&](const JoinPrefixes& join) {
        for (auto& elem : join.prefixes)
          applyPrefix(factory, elem, attr);
      },
      [&](const SpellId& spell) {
        attr.equipedAbility = spell;
      }
  );
}

vector<string> getEffectDescription(const ContentFactory* factory, const ItemPrefix& prefix) {
  return prefix.visit(
      [&](LastingEffect effect) -> vector<string> {
        return {"grants " + LastingEffects::getName(effect)};
      },
      [&](const AttackerEffect& e) -> vector<string> {
        return {"attacker affected by: " + e.effect.getName(factory)};
      },
      [&](const VictimEffect& e) -> vector<string> {
        return {"victim affected by: " + e.effect.getName(factory) + " (" + toPercentage(e.chance) + " chance)"};
      },
      [&](ItemAttrBonus bonus) -> vector<string> {
        return {"+"_s + toString(bonus.value) + " " + getName(bonus.attr)};
      },
      [&](const JoinPrefixes& join) -> vector<string> {
        vector<string> ret;
        for (auto& e : join.prefixes)
          ret.append(getEffectDescription(factory, e));
        return ret;
      },
      [&](const SpellId& id) -> vector<string> {
        return {"grants "_s + id.data() + " ability"};
      }
  );
}

string getItemName(const ContentFactory* factory, const ItemPrefix& prefix) {
  return prefix.visit(
      [&](LastingEffect effect) {
        return LastingEffects::getName(effect);
      },
      [&](const AttackerEffect& e) {
        return e.effect.getName(factory);
      },
      [&](const VictimEffect& e) {
        return e.effect.getName(factory);
      },
      [&](ItemAttrBonus bonus) {
        return getName(bonus.attr);
      },
      [&](const JoinPrefixes& join) {
        return getItemName(factory, join.prefixes.back());
      },
      [&](const SpellId& id) -> string {
        return id.data();
      }
  );
}

string getGlyphName(const ContentFactory* factory, const ItemPrefix& prefix) {
  return prefix.visit(
      [&](const auto&) {
        return ::getItemName(factory, prefix);
      },
      [&](ItemAttrBonus bonus) {
        return "+"_s + toString(bonus.value) + " " + getName(bonus.attr);
      },
      [&](const JoinPrefixes& join) {
        return getGlyphName(factory, join.prefixes.back());
      }
  );
}
