#include "stdafx.h"

#include "item_attributes.h"

template <class Archive> 
void ItemAttributes::serialize(Archive& ar, const unsigned int version) {
  ar& SVAR(name)
    & SVAR(description)
    & SVAR(weight)
    & SVAR(type)
    & SVAR(realName)
    & SVAR(realPlural)
    & SVAR(plural)
    & SVAR(blindName)
    & SVAR(firingWeapon)
    & SVAR(artifactName)
    & SVAR(trapType)
    & SVAR(flamability)
    & SVAR(price)
    & SVAR(noArticle)
    & SVAR(damage)
    & SVAR(toHit)
    & SVAR(thrownDamage)
    & SVAR(thrownToHit)
    & SVAR(rangedWeaponAccuracy)
    & SVAR(defense)
    & SVAR(strength)
    & SVAR(dexterity)
    & SVAR(speed)
    & SVAR(twoHanded)
    & SVAR(attackType)
    & SVAR(attackTime)
    & SVAR(armorType)
    & SVAR(applyTime)
    & SVAR(fragile)
    & SVAR(effect)
    & SVAR(uses)
    & SVAR(usedUpMsg)
    & SVAR(displayUses)
    & SVAR(identifyOnApply)
    & SVAR(identifiable)
    & SVAR(identifyOnEquip);
  CHECK_SERIAL;
}

SERIALIZABLE(ItemAttributes);
