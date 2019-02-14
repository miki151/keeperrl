#include "stdafx.h"
#include "item_prefix.h"
#include "item_attributes.h"
#include "lasting_effect.h"
#include "effect.h"

void applyPrefix(const ItemPrefix& prefix, ItemAttributes& attr) {
  attr.prefixes.push_back(getItemName(prefix));
  prefix.visit(
      [&](LastingEffect effect) {
        attr.equipedEffect.push_back(effect);
      },
      [&](const AttackerEffect& e) {
        attr.weaponInfo.attackerEffect.push_back(e.effect);
      },
      [&](const VictimEffect& e) {
        attr.weaponInfo.victimEffect.push_back(e.effect);
      },
      [&](ItemAttrBonus bonus) {
        attr.modifiers[bonus.attr] += bonus.value;
      },
      [&](const JoinPrefixes& join) {
        for (auto& elem : join.prefixes)
          applyPrefix(elem, attr);
      }
  );
}

vector<string> getEffectDescription(const ItemPrefix& prefix) {
  return prefix.visit(
      [&](LastingEffect effect) -> vector<string> {
        return {"grants " + LastingEffects::getName(effect)};
      },
      [&](const AttackerEffect& e) -> vector<string> {
        return {"attacker affected by: " + e.effect.getName()};
      },
      [&](const VictimEffect& e) -> vector<string> {
        return {"victim affected by: " + e.effect.getName()};
      },
      [&](ItemAttrBonus bonus) -> vector<string> {
        return {"+"_s + toString(bonus.value) + " " + getName(bonus.attr)};
      },
      [&](const JoinPrefixes& join) -> vector<string> {
        vector<string> ret;
        for (auto& e : join.prefixes)
          ret.append(getEffectDescription(e));
        return ret;
      }
  );
}

string getItemName(const ItemPrefix& prefix) {
  return prefix.visit(
      [&](LastingEffect effect) {
        return LastingEffects::getName(effect);
      },
      [&](const AttackerEffect& e) {
        return e.effect.getName();
      },
      [&](const VictimEffect& e) {
        return e.effect.getName();
      },
      [&](ItemAttrBonus bonus) {
        return getName(bonus.attr);
      },
      [&](const JoinPrefixes& join) {
        return getItemName(join.prefixes.back());
      }
  );
}

string getGlyphName(const ItemPrefix& prefix) {
  return prefix.visit(
      [&](const auto&) {
        return ::getItemName(prefix);
      },
      [&](ItemAttrBonus bonus) {
        return "+"_s + toString(bonus.value) + " " + getName(bonus.attr);
      },
      [&](const JoinPrefixes& join) {
        return getGlyphName(join.prefixes.back());
      }
  );
}
