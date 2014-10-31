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
  ar& SVAR(name)
    & SVAR(viewId)
    & SVAR(description)
    & SVAR(weight)
    & SVAR(itemClass)
    & SVAR(realName)
    & SVAR(realPlural)
    & SVAR(plural)
    & SVAR(blindName)
    & SVAR(firingWeapon)
    & SVAR(artifactName)
    & SVAR(trapType)
    & SVAR(resourceId)
    & SVAR(flamability)
    & SVAR(price)
    & SVAR(noArticle)
    & SVAR(twoHanded)
    & SVAR(attackType)
    & SVAR(attackTime)
    & SVAR(equipmentSlot)
    & SVAR(applyTime)
    & SVAR(fragile)
    & SVAR(effect)
    & SVAR(attackEffect)
    & SVAR(uses)
    & SVAR(usedUpMsg)
    & SVAR(displayUses)
    & SVAR(identifyOnApply)
    & SVAR(identifiable)
    & SVAR(identifyOnEquip)
    & SVAR(modifiers);
  CHECK_SERIAL;
}

SERIALIZABLE(ItemAttributes);
SERIALIZATION_CONSTRUCTOR_IMPL(ItemAttributes);
