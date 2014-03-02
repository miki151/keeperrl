#ifndef _RANGED_WEAPON
#define _RANGED_WEAPON

#include "item.h"

class RangedWeapon : public Item {
  public:
  RangedWeapon(ViewObject, const ItemAttributes&);

  virtual void fire(Creature*, Level*, PItem ammo, Vec2 dir);

  SERIALIZATION_DECL(RangedWeapon);
};

#endif
