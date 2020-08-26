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
#include "item_upgrade_info.h"
#include "automaton_part.h"
#include "resource_id.h"
#include "creature_predicate.h"

#define ITATTR(X) ItemAttributes([&](ItemAttributes& i) { X })

class EnemyCheck;

class ItemAttributes {
  public:
  ItemAttributes(function<void(ItemAttributes&)> fun) {
    fun(*this);
  }
  template <class Archive>
  void serializeImpl(Archive& ar, const unsigned int);
  SERIALIZATION_DECL(ItemAttributes)

  ViewId SERIAL(viewId);
  vector<ViewId> SERIAL(partIds);
  string SERIAL(name);
  string SERIAL(description);
  optional<string> SERIAL(shortName);
  double SERIAL(weight);
  ItemClass SERIAL(itemClass);
  optional<string> SERIAL(plural);
  optional<string> SERIAL(blindName);
  optional<string> SERIAL(artifactName);
  optional<CollectiveResourceId> SERIAL(resourceId);
  int SERIAL(burnTime) = 0;
  int SERIAL(price) = 0;
  bool SERIAL(noArticle) = false;
  EnumMap<AttrType, int> SERIAL(modifiers);
  double SERIAL(variationChance) = 0.2;
  EnumMap<AttrType, int> SERIAL(modifierVariation) = EnumMap<AttrType, int>(
      [](AttrType type) {
        switch (type) {
          case AttrType::PARRY:
            return 0;
          default:
            return 1;
        }
        return 0;
      });
  optional<EquipmentSlot> SERIAL(equipmentSlot);
  TimeInterval SERIAL(applyTime) = 1_visible;
  bool SERIAL(fragile) = false;
  optional<Effect> SERIAL(effect);
  int SERIAL(uses) = -1;
  bool SERIAL(usedUpMsg) = false;
  bool SERIAL(displayUses) = false;
  bool SERIAL(effectDescription) = true;
  vector<LastingEffect> SERIAL(equipedEffect);
  optional<LastingEffect> SERIAL(ownedEffect);
  optional<string> SERIAL(applyMsgFirstPerson);
  optional<string> SERIAL(applyMsgThirdPerson);
  optional<SoundId> SERIAL(applySound);
  optional<RangedWeapon> SERIAL(rangedWeapon);
  WeaponInfo SERIAL(weaponInfo);
  vector<pair<int, ItemPrefix>> SERIAL(genPrefixes);
  vector<string> SERIAL(prefixes);
  optional<ItemUpgradeInfo> SERIAL(upgradeInfo);
  int SERIAL(maxUpgrades) = 3;
  optional<string> SERIAL(ingredientType);
  Range SERIAL(wishedCount) = Range(1, 2);
  vector<SpellId> SERIAL(equipedAbility);
  optional<AutomatonPart> SERIAL(automatonPart);
  EnumMap<AttrType, optional<pair<int, CreaturePredicate>>> SERIAL(specialAttr);
};

static_assert(std::is_nothrow_move_constructible<ItemAttributes>::value, "T should be noexcept MoveConstructible");
