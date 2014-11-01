/* Copyright (C) 2013-2014 Michal Brzozowski (rusolis@poczta.fm)

   This file is part of KeeperRL.

   KeeperRL is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   KeeperRL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

#ifndef _ITEM_ATTRIBUTES_H
#define _ITEM_ATTRIBUTES_H

#include <string>
#include <functional>

#include "util.h"
#include "enums.h"
#include "effect_type.h"

#define ITATTR(X) ItemAttributes([&](ItemAttributes& i) { X })

class EnemyCheck;

RICH_ENUM(ModifierType,
  DAMAGE,
  ACCURACY,
  THROWN_DAMAGE,
  THROWN_ACCURACY,
  FIRED_DAMAGE,
  FIRED_ACCURACY,
  DEFENSE,
  INV_LIMIT
);

RICH_ENUM(AttrType,
  STRENGTH,
  DEXTERITY,
  SPEED
);


enum class AttackType { CUT, STAB, CRUSH, PUNCH, BITE, EAT, HIT, SHOOT, SPELL, POSSESS};

class ItemAttributes {
  public:
  ItemAttributes(function<void(ItemAttributes&)> fun) {
    fun(*this);
  }

  SERIALIZATION_DECL(ItemAttributes);

  MustInitialize<ViewId> SERIAL(viewId);
  MustInitialize<string> SERIAL(name);
  string SERIAL(description);
  MustInitialize<double> SERIAL(weight);
  MustInitialize<ItemClass> SERIAL(itemClass);
  Optional<string> SERIAL(realName);
  Optional<string> SERIAL(realPlural);
  Optional<string> SERIAL(plural);
  Optional<string> SERIAL(blindName);
  Optional<ItemId> SERIAL(firingWeapon);
  Optional<string> SERIAL(artifactName);
  Optional<TrapType> SERIAL(trapType);
  Optional<CollectiveResourceId> SERIAL(resourceId);
  double SERIAL2(flamability, 0);
  int SERIAL2(price, 0);
  bool SERIAL2(noArticle, false);
  EnumMap<ModifierType, int> SERIAL(modifiers);
  EnumMap<AttrType, int> SERIAL(attrs);
  bool SERIAL2(twoHanded, false);
  AttackType SERIAL2(attackType, AttackType::HIT);
  double SERIAL2(attackTime, 1);
  Optional<EquipmentSlot> SERIAL(equipmentSlot);
  double SERIAL2(applyTime, 1);
  bool SERIAL2(fragile, false);
  Optional<EffectType> SERIAL(effect);
  Optional<EffectType> SERIAL(attackEffect);
  int SERIAL2(uses, -1);
  bool SERIAL2(usedUpMsg, false);
  bool SERIAL2(displayUses, false);
  bool SERIAL2(identifyOnApply, true);
  bool SERIAL2(identifiable, false);
  bool SERIAL2(identifyOnEquip, true);

};

#endif
