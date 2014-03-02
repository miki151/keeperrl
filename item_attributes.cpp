#include "stdafx.h"

#include "item_attributes.h"

template <class Archive> 
void ItemAttributes::serialize(Archive& ar, const unsigned int version) {
  ar& BOOST_SERIALIZATION_NVP(name)
    & BOOST_SERIALIZATION_NVP(description)
    & BOOST_SERIALIZATION_NVP(weight)
    & BOOST_SERIALIZATION_NVP(type)
    & BOOST_SERIALIZATION_NVP(realName)
    & BOOST_SERIALIZATION_NVP(realPlural)
    & BOOST_SERIALIZATION_NVP(plural)
    & BOOST_SERIALIZATION_NVP(blindName)
    & BOOST_SERIALIZATION_NVP(firingWeapon)
    & BOOST_SERIALIZATION_NVP(artifactName)
    & BOOST_SERIALIZATION_NVP(trapType)
    & BOOST_SERIALIZATION_NVP(flamability)
    & BOOST_SERIALIZATION_NVP(price)
    & BOOST_SERIALIZATION_NVP(noArticle)
    & BOOST_SERIALIZATION_NVP(damage)
    & BOOST_SERIALIZATION_NVP(toHit)
    & BOOST_SERIALIZATION_NVP(thrownDamage)
    & BOOST_SERIALIZATION_NVP(thrownToHit)
    & BOOST_SERIALIZATION_NVP(rangedWeaponAccuracy)
    & BOOST_SERIALIZATION_NVP(defense)
    & BOOST_SERIALIZATION_NVP(strength)
    & BOOST_SERIALIZATION_NVP(dexterity)
    & BOOST_SERIALIZATION_NVP(speed)
    & BOOST_SERIALIZATION_NVP(twoHanded)
    & BOOST_SERIALIZATION_NVP(attackType)
    & BOOST_SERIALIZATION_NVP(armorType)
    & BOOST_SERIALIZATION_NVP(applyTime)
    & BOOST_SERIALIZATION_NVP(fragile)
    & BOOST_SERIALIZATION_NVP(effect)
    & BOOST_SERIALIZATION_NVP(uses)
    & BOOST_SERIALIZATION_NVP(usedUpMsg)
    & BOOST_SERIALIZATION_NVP(displayUses)
    & BOOST_SERIALIZATION_NVP(identifyOnApply)
    & BOOST_SERIALIZATION_NVP(identifiable)
    & BOOST_SERIALIZATION_NVP(identifyOnEquip);
}

SERIALIZABLE(ItemAttributes);
