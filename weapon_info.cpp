#include "stdafx.h"
#include "weapon_info.h"

bool WeaponInfo::operator ==(const WeaponInfo& o) const {
  return twoHanded == o.twoHanded &&
      attackType == o.attackType &&
      meleeAttackAttr == o.meleeAttackAttr &&
      victimEffect == o.victimEffect &&
      attackerEffect == o.attackerEffect &&
      attackMsg == o.attackMsg;
}

bool WeaponInfo::operator !=(const WeaponInfo& o) const {
  return !(*this == o);
}
