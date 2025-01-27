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
#include "lasting_or_buff.h"
#include "assembled_minion.h"
#include "t_string.h"

#define ITATTR(X) make_shared<ItemAttributes>([&](ItemAttributes& i) { X })

class EnemyCheck;

class ItemAttributes {
  public:
  ItemAttributes(function<void(ItemAttributes&)> fun) {
    fun(*this);
  }
  template <class Archive>
  void serializeImpl(Archive& ar, const unsigned int);
  SERIALIZATION_DECL(ItemAttributes)

  void scale(double value, const ContentFactory*);

  ViewId SERIAL(viewId);
  optional<ViewId> SERIAL(equipedViewId);
  vector<ViewId> SERIAL(partIds);
  TString SERIAL(name);
  TString SERIAL(description);
  optional<TString> SERIAL(shortName);
  double SERIAL(weight);
  ItemClass SERIAL(itemClass) = ItemClass::OTHER;
  optional<TString> SERIAL(plural);
  optional<TString> SERIAL(blindName);
  optional<TString> SERIAL(artifactName);
  optional<CollectiveResourceId> SERIAL(resourceId);
  int SERIAL(burnTime) = 0;
  int SERIAL(price) = 0;
  bool SERIAL(noArticle) = false;
  HashMap<AttrType, int> SERIAL(modifiers);
  double SERIAL(variationChance) = 0.2;
  optional<EquipmentSlot> SERIAL(equipmentSlot);
  TimeInterval SERIAL(applyTime) = 1_visible;
  bool SERIAL(fragile) = false;
  optional<Effect> SERIAL(effect);
  optional<AssembledMinion> SERIAL(assembledMinion);
  int SERIAL(uses) = -1;
  bool SERIAL(usedUpMsg) = false;
  bool SERIAL(displayUses) = false;
  bool SERIAL(effectDescription) = true;
  vector<LastingOrBuff> SERIAL(equipedEffect);
  optional<CompanionInfo> SERIAL(equipedCompanion);
  vector<LastingOrBuff> SERIAL(ownedEffect);
  optional<TString> SERIAL(applyMsgFirstPerson);
  optional<TString> SERIAL(applyMsgThirdPerson);
  pair<TString, TString> SERIAL(applyVerb) = {TStringId("YOU_APPLY"), TStringId("APPLIES")};
  optional<StatId> SERIAL(producedStat);
  bool SERIAL(effectAppliedWhenThrown) = false;
  optional<CreaturePredicate> SERIAL(applyPredicate);
  optional<SoundId> SERIAL(applySound);
  WeaponInfo SERIAL(weaponInfo);
  vector<pair<int, ItemPrefix>> SERIAL(genPrefixes);
  vector<TString> SERIAL(prefixes);
  vector<TString> SERIAL(suffixes);
  optional<ItemUpgradeInfo> SERIAL(upgradeInfo);
  int SERIAL(maxUpgrades) = 3;
  vector<ItemUpgradeType> SERIAL(upgradeType);
  optional<string> SERIAL(ingredientType);
  Range SERIAL(wishedCount) = Range(1, 2);
  vector<SpellId> SERIAL(equipedAbility);
  HashMap<AttrType, pair<int, CreaturePredicate>> SERIAL(specialAttr);
  vector<StorageId> SERIAL(storageIds);
  optional<Effect> SERIAL(carriedTickEffect);
  CostInfo SERIAL(craftingCost) = CostInfo::noCost();
  optional<TString> SERIAL(equipmentGroup);
  CreaturePredicate SERIAL(autoEquipPredicate) = CreaturePredicates::And{};
  TString SERIAL(equipWarning) = TStringId("ITEM_MAY_HURT_MINION");
};

static_assert(std::is_nothrow_move_constructible<ItemAttributes>::value, "T should be noexcept MoveConstructible");

CEREAL_CLASS_VERSION(ItemAttributes, 2)
