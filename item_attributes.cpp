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

template <class Archive> 
void ItemAttributes::serialize(Archive& ar, const unsigned int version) {
  ar(name, viewId, description, weight, itemClass, plural, blindName, artifactName, trapType);
  ar(resourceId, flamability, price, noArticle, equipmentSlot, applyTime, ownedEffect);
  ar(fragile, effect, uses, usedUpMsg, displayUses, modifiers, shortName, equipedEffect);
  ar(applyMsgFirstPerson, applyMsgThirdPerson, applySound, rangedWeapon, weaponInfo);
}

SERIALIZABLE(ItemAttributes);
SERIALIZATION_CONSTRUCTOR_IMPL(ItemAttributes);

void ItemAttributes::enableSharpWeapons(int percent) {
    //Make occasional sharp weapons with extra cut.
    //Most common with basic swords, but possible with elven swords and axes
    int spin = rand() % 100 + 1;
    if (spin > percent) return;
    modifiers[AttrType::DAMAGE] = modifiers[AttrType::DAMAGE] + 2;
    name = "sharp " + *name;
}

void ItemAttributes::enableHardenedWeapons(int percent) {
    //Make occasional blunt weapons with extra whack.
    //Most common with clubs but possible with iron hammers.
    int spin = rand() % 100 + 1;
    if (spin > percent) return;
    modifiers[AttrType::DAMAGE] = modifiers[AttrType::DAMAGE] + 2;
    name = "hardened " + *name;
}

void ItemAttributes::enableHardenedArmours(int percent) {
    //Make occasional hardened armour
    //Much more common with leather but possible with iron.
    int spin = rand() % 100 + 1;
    if (spin > percent) return;
    modifiers[AttrType::DEFENSE] = modifiers[AttrType::DEFENSE] + 2;
    name = "hardened " + *name;
}
