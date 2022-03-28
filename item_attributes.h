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
#include "item_type.h"
#include "game_time.h"
#include "weapon_info.h"
#include "item_prefix.h"
#include "item_upgrade_info.h"
#include "automaton_part.h"
#include "resource_id.h"
#include "creature_predicate.h"
#include "companion_info.h"
#include "storage_id.h"
#include "cost_info.h"

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
  ItemClass SERIAL(itemClass) = ItemClass::OTHER;
  optional<string> SERIAL(plural);
  optional<string> SERIAL(blindName);
  optional<string> SERIAL(artifactName);
  optional<CollectiveResourceId> SERIAL(resourceId);
  int SERIAL(burnTime) = 0;
  int SERIAL(price) = 0;
  bool SERIAL(noArticle) = false;
  map<AttrType, int> SERIAL(modifiers);
  double SERIAL(variationChance) = 0.2;
  optional<EquipmentSlot> SERIAL(equipmentSlot);
  TimeInterval SERIAL(applyTime) = 1_visible;
  bool SERIAL(fragile) = false;
  optional<Effect> SERIAL(effect);
  int SERIAL(uses) = -1;
  bool SERIAL(usedUpMsg) = false;
  bool SERIAL(displayUses) = false;
  bool SERIAL(effectDescription) = true;
  vector<LastingEffect> SERIAL(equipedEffect);
  optional<CompanionInfo> SERIAL(equipedCompanion);
  optional<LastingEffect> SERIAL(ownedEffect);
  optional<string> SERIAL(applyMsgFirstPerson);
  optional<string> SERIAL(applyMsgThirdPerson);
  pair<string, string> SERIAL(applyVerb) = {"apply", "applies"};
  optional<StatId> SERIAL(producedStat);
  bool SERIAL(effectAppliedWhenThrown) = false;
  optional<CreaturePredicate> SERIAL(applyPredicate);
  optional<SoundId> SERIAL(applySound);
  WeaponInfo SERIAL(weaponInfo);
  vector<pair<int, ItemPrefix>> SERIAL(genPrefixes);
  vector<string> SERIAL(prefixes);
  optional<ItemUpgradeInfo> SERIAL(upgradeInfo);
  int SERIAL(maxUpgrades) = 3;
  vector<ItemUpgradeType> SERIAL(upgradeType);
  optional<string> SERIAL(ingredientType);
  Range SERIAL(wishedCount) = Range(1, 2);
  vector<SpellId> SERIAL(equipedAbility);
  map<AttrType, pair<int, CreaturePredicate>> SERIAL(specialAttr);
  vector<StorageId> SERIAL(storageIds);
  optional<Effect> SERIAL(carriedTickEffect);
  CostInfo SERIAL(craftingCost) = CostInfo::noCost();
  optional<string> SERIAL(equipmentGroup);
  CreaturePredicate SERIAL(autoEquipPredicate) = CreaturePredicates::And{};
};

static_assert(std::is_nothrow_move_constructible<ItemAttributes>::value, "T should be noexcept MoveConstructible");
