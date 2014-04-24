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
    & SVAR(legs)
    & SVAR(arms)
    & SVAR(wings)
    & SVAR(heads)
    & SVAR(innocent)
    & SVAR(noBody)
    & SVAR(fireResistant)
    & SVAR(poisonResistant)
    & SVAR(fireCreature)
    & SVAR(breathing)
    & SVAR(humanoid)
    & SVAR(animal)
    & SVAR(healer)
    & SVAR(flyer)
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
    & SVAR(permanentlyBlind)
    & SVAR(invincible)
    & SVAR(damageMultiplier)
    & SVAR(attributeGain)
    & SVAR(skills)
    & SVAR(skillGain)
    & SVAR(spells);
  CHECK_SERIAL;
}

SERIALIZABLE(CreatureAttributes);
