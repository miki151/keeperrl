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

#include "creature_attributes.h"
#include "creature.h"

CreatureAttributes::CreatureAttributes(function<void(CreatureAttributes&)> fun) {
  fun(*this);
}

CreatureAttributes::~CreatureAttributes() {}

template <class Archive> 
void CreatureAttributes::serialize(Archive& ar, const unsigned int version) {
  ar& SVAR(viewId)
    & SVAR(undeadViewId)
    & SVAR(illusionViewObject)
    & SVAR(spawnType)
    & SVAR(name)
    & SVAR(undeadName)
    & SVAR(size)
    & SVAR(attr)
    & SVAR(weight)
    & SVAR(chatReactionFriendly)
    & SVAR(chatReactionHostile)
    & SVAR(firstName)
    & SVAR(speciesName)
    & SVAR(barehandedDamage)
    & SVAR(barehandedAttack)
    & SVAR(attackEffect)
    & SVAR(harmlessApply)
    & SVAR(passiveAttack)
    & SVAR(gender)
    & SVAR(bodyParts)
    & SVAR(injuredBodyParts)
    & SVAR(lostBodyParts)
    & SVAR(innocent)
    & SVAR(uncorporal)
    & SVAR(fireCreature)
    & SVAR(breathing)
    & SVAR(humanoid)
    & SVAR(animal)
    & SVAR(undead)
    & SVAR(notLiving)
    & SVAR(brain)
    & SVAR(isFood)
    & SVAR(stationary)
    & SVAR(noSleep)
    & SVAR(cantEquip)
    & SVAR(courage)
    & SVAR(carryAnything)
    & SVAR(invincible)
    & SVAR(worshipped)
    & SVAR(dontChase)
    & SVAR(attributeGain)
    & SVAR(skills)
    & SVAR(spells)
    & SVAR(permanentEffects)
    & SVAR(lastingEffects)
    & SVAR(minionTasks)
    & SVAR(groupName)
    & SVAR(attrIncrease);
}

SERIALIZABLE(CreatureAttributes);

SERIALIZATION_CONSTRUCTOR_IMPL(CreatureAttributes);
