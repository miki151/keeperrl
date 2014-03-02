#include "stdafx.h"

#include "creature_attributes.h"
#include "creature.h"


template <class Archive> 
void CreatureAttributes::serialize(Archive& ar, const unsigned int version) {
  ar
    & BOOST_SERIALIZATION_NVP(name)
    & BOOST_SERIALIZATION_NVP(speed)
    & BOOST_SERIALIZATION_NVP(size)
    & BOOST_SERIALIZATION_NVP(strength)
    & BOOST_SERIALIZATION_NVP(dexterity)
    & BOOST_SERIALIZATION_NVP(weight)
    & BOOST_SERIALIZATION_NVP(chatReactionFriendly)
    & BOOST_SERIALIZATION_NVP(chatReactionHostile)
    & BOOST_SERIALIZATION_NVP(firstName)
    & BOOST_SERIALIZATION_NVP(specialMonster)
    & BOOST_SERIALIZATION_NVP(barehandedDamage)
    & BOOST_SERIALIZATION_NVP(barehandedAttack)
    & BOOST_SERIALIZATION_NVP(attackEffect)
    & BOOST_SERIALIZATION_NVP(passiveAttack)
    & BOOST_SERIALIZATION_NVP(legs)
    & BOOST_SERIALIZATION_NVP(arms)
    & BOOST_SERIALIZATION_NVP(heads)
    & BOOST_SERIALIZATION_NVP(innocent)
    & BOOST_SERIALIZATION_NVP(noBody)
    & BOOST_SERIALIZATION_NVP(fireResistant)
    & BOOST_SERIALIZATION_NVP(fireCreature)
    & BOOST_SERIALIZATION_NVP(breathing)
    & BOOST_SERIALIZATION_NVP(humanoid)
    & BOOST_SERIALIZATION_NVP(animal)
    & BOOST_SERIALIZATION_NVP(healer)
    & BOOST_SERIALIZATION_NVP(flyer)
    & BOOST_SERIALIZATION_NVP(undead)
    & BOOST_SERIALIZATION_NVP(notLiving)
    & BOOST_SERIALIZATION_NVP(brain)
    & BOOST_SERIALIZATION_NVP(walker)
    & BOOST_SERIALIZATION_NVP(isFood)
    & BOOST_SERIALIZATION_NVP(stationary)
    & BOOST_SERIALIZATION_NVP(noSleep)
    & BOOST_SERIALIZATION_NVP(courage)
    & BOOST_SERIALIZATION_NVP(maxLevel)
    & BOOST_SERIALIZATION_NVP(carryAnything)
    & BOOST_SERIALIZATION_NVP(permanentlyBlind)
    & BOOST_SERIALIZATION_NVP(invincible)
    & BOOST_SERIALIZATION_NVP(damageMultiplier)
    & BOOST_SERIALIZATION_NVP(skills)
    & BOOST_SERIALIZATION_NVP(skillGain)
    & BOOST_SERIALIZATION_NVP(spells);
}

SERIALIZABLE(CreatureAttributes);
