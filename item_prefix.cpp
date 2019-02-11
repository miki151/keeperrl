#include "stdafx.h"
#include "item_prefix.h"
#include "item_attributes.h"
#include "lasting_effect.h"
#include "effect.h"

void applyPrefix(ItemPrefix prefix, ItemAttributes& attr) {
  attr.prefix = getName(prefix);
  prefix.visit(
      [&](LastingEffect effect) {
        attr.equipedEffect = effect;
      },
      [&](const AttackerEffect& e) {
        attr.weaponInfo.attackerEffect = e.effect;
      },
      [&](const VictimEffect& e) {
        attr.weaponInfo.victimEffect = e.effect;
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

string getName(const ItemPrefix& prefix) {
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
        return getName(join.prefixes.back());
      }
  );
}
