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


template <class Archive> 
void CreatureAttributes::serialize(Archive& ar, const unsigned int version) {
  ar& SVAR(viewId)
    & SVAR(name)
    & SVAR(speed)
    & SVAR(size)
    & SVAR(strength)
    & SVAR(dexterity)
    & SVAR(weight)
    & SVAR(chatReactionFriendly)
    & SVAR(chatReactionHostile)
    & SVAR(firstName)
    & SVAR(speciesName)
    & SVAR(specialMonster)
    & SVAR(barehandedDamage)
    & SVAR(barehandedAttack)
    & SVAR(attackEffect)
    & SVAR(harmlessApply)
    & SVAR(passiveAttack)
    & SVAR(gender)
    & SVAR(bodyParts)
    & SVAR(innocent)
    & SVAR(uncorporal)
    & SVAR(fireCreature)
    & SVAR(breathing)
    & SVAR(humanoid)
    & SVAR(animal)
    & SVAR(healer)
    & SVAR(undead)
    & SVAR(notLiving)
    & SVAR(brain)
    & SVAR(walker)
    & SVAR(isFood)
    & SVAR(stationary)
    & SVAR(noSleep)
    & SVAR(courage)
    & SVAR(maxLevel)
    & SVAR(carryAnything)
    & SVAR(invincible)
    & SVAR(damageMultiplier)
    & SVAR(attributeGain)
    & SVAR(skills)
    & SVAR(skillGain)
    & SVAR(spells)
    & SVAR(permanentEffects);
  CHECK_SERIAL;
}

SERIALIZABLE(CreatureAttributes);

