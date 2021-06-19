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

#include "stdafx.h"

#include "item_attributes.h"
#include "item_class.h"
#include "resource_id.h"
#include "equipment.h"
#include "lasting_effect.h"
#include "sound.h"
#include "statistics.h"

template <class Archive> 
void ItemAttributes::serializeImpl(Archive& ar, const unsigned int version) {
  ar(NAMED(name), NAMED(viewId), OPTION(description), NAMED(weight), OPTION(itemClass), NAMED(plural), NAMED(blindName));
  ar(NAMED(artifactName), NAMED(resourceId), OPTION(burnTime), OPTION(price), OPTION(noArticle), NAMED(equipmentSlot), OPTION(equipedAbility));
  ar(OPTION(applyTime), NAMED(ownedEffect), OPTION(maxUpgrades), OPTION(fragile), NAMED(effect), OPTION(uses), OPTION(usedUpMsg));
  ar(OPTION(displayUses), OPTION(modifiers), NAMED(shortName), OPTION(equipedEffect), NAMED(upgradeInfo), OPTION(effectDescription));
  ar(NAMED(applyMsgFirstPerson), NAMED(applyMsgThirdPerson), NAMED(applySound), OPTION(weaponInfo), OPTION(applyVerb));
  ar(OPTION(prefixes), OPTION(genPrefixes), NAMED(ingredientType), OPTION(modifierVariation), OPTION(variationChance), OPTION(wishedCount));
  ar(NAMED(automatonPart), OPTION(specialAttr), OPTION(partIds), OPTION(equipedCompanion), OPTION(upgradeType), OPTION(producedStat));
  ar(OPTION(effectAppliedWhenThrown), OPTION(applyPredicate), NAMED(storageIds));
}

template <class Archive>
void ItemAttributes::serialize(Archive& ar, const unsigned int version) {
  serializeImpl(ar, version);
}

SERIALIZABLE(ItemAttributes);
SERIALIZATION_CONSTRUCTOR_IMPL(ItemAttributes);

#include "pretty_archive.h"
template<>
void ItemAttributes::serialize(PrettyInputArchive& ar1, unsigned version) {
  serializeImpl(ar1, version);
  ar1(endInput());
  if (automatonPart) {
    automatonPart->viewId = viewId;
    automatonPart->name = shortName.value_or(name);
  }
}
