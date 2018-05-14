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
      [&](const Effect& effect) {
        attr.weaponInfo.attackEffect = effect;
        attr.prefix = effect.getName();
      },
      [&](ItemAttrBonus bonus) {
        attr.modifiers[bonus.attr] += bonus.value;
        attr.prefix = getName(bonus.attr);
      }
  );
}
