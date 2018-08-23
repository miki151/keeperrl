#include "stdafx.h"
#include "item_prefix.h"
#include "item_attributes.h"
#include "lasting_effect.h"
#include "effect.h"

void applyPrefix(ItemPrefix prefix, ItemAttributes& attr) {
  prefix.visit(
      [&](LastingEffect effect) {
        attr.equipedEffect = effect;
        attr.prefix = string(LastingEffects::getName(effect));
      },
      [&](const AttackerEffect& e) {
        attr.weaponInfo.attackerEffect = e.effect;
        attr.prefix = e.effect.getName();
      },
      [&](const VictimEffect& e) {
        attr.weaponInfo.victimEffect = e.effect;
        attr.prefix = e.effect.getName();
      },
      [&](ItemAttrBonus bonus) {
        attr.modifiers[bonus.attr] += bonus.value;
        attr.prefix = getName(bonus.attr);
      },
      [&](const JoinPrefixes& join) {
        for (auto& elem : join.prefixes)
          applyPrefix(elem, attr);
      }
  );
}
