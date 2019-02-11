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

#pragma once

#include <string>
#include <functional>

#include "util.h"
#include "enums.h"
#include "effect.h"
#include "attack_type.h"
#include "attr_type.h"
#include "ranged_weapon.h"
#include "item_type.h"
#include "game_time.h"
#include "weapon_info.h"
#include "item_prefix.h"
#include "crafting_rune.h"

#define ITATTR(X) ItemAttributes([&](ItemAttributes& i) { X })

class EnemyCheck;

class ItemAttributes {
  public:
  ItemAttributes(function<void(ItemAttributes&)> fun) {
    fun(*this);
  }

  SERIALIZATION_DECL(ItemAttributes);

  MustInitialize<ViewId> SERIAL(viewId);
  MustInitialize<string> SERIAL(name);
  string SERIAL(description);
  optional<string> SERIAL(shortName);
  MustInitialize<double> SERIAL(weight);
  MustInitialize<ItemClass> SERIAL(itemClass);
  optional<string> SERIAL(plural);
  optional<string> SERIAL(blindName);
  optional<string> SERIAL(artifactName);
  optional<TrapType> SERIAL(trapType);
  optional<CollectiveResourceId> SERIAL(resourceId);
  double SERIAL(flamability) = 0;
  int SERIAL(price) = 0;
  bool SERIAL(noArticle) = false;
  EnumMap<AttrType, int> SERIAL(modifiers);
  optional<EquipmentSlot> SERIAL(equipmentSlot);
  TimeInterval SERIAL(applyTime) = 1_visible;
  bool SERIAL(fragile) = false;
  optional<Effect> SERIAL(effect);
  int SERIAL(uses) = -1;
  bool SERIAL(usedUpMsg) = false;
  bool SERIAL(displayUses) = false;
  optional<LastingEffect> SERIAL(equipedEffect);
  optional<LastingEffect> SERIAL(ownedEffect);
  optional<string> SERIAL(applyMsgFirstPerson);
  optional<string> SERIAL(applyMsgThirdPerson);
  optional<SoundId> SERIAL(applySound);
  optional<RangedWeapon> SERIAL(rangedWeapon);
  WeaponInfo SERIAL(weaponInfo);
  vector<pair<int, ItemPrefix>> SERIAL(prefixes);
  optional<string> SERIAL(prefix);
  optional<CraftingRune> SERIAL(craftingRune);
};

