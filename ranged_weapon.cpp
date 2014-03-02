#include "stdafx.h"

#include "ranged_weapon.h"
#include "creature.h"
#include "level.h"

template <class Archive> 
void RangedWeapon::serialize(Archive& ar, const unsigned int version) {
  ar & SUBCLASS(Item);
}

SERIALIZABLE(RangedWeapon);

RangedWeapon::RangedWeapon(ViewObject o, const ItemAttributes& attr) : Item(o, attr) {}

void RangedWeapon::fire(Creature* c, Level* l, PItem ammo, Vec2 dir) {
  int toHitVariance = 10;
  int attackVariance = 15;
  int toHit = Random.getRandom(-toHitVariance, toHitVariance) + 
    c->getAttr(AttrType::THROWN_TO_HIT) +
    ammo->getModifier(AttrType::THROWN_TO_HIT) +
    getAccuracy();
  int damage = Random.getRandom(-attackVariance, attackVariance) + 
    c->getAttr(AttrType::THROWN_DAMAGE) +
    ammo->getModifier(AttrType::THROWN_DAMAGE);
  Attack attack(c, chooseRandom({AttackLevel::LOW, AttackLevel::MIDDLE, AttackLevel::HIGH}),
      AttackType::SHOOT, toHit, damage, false, Nothing());
  l->throwItem(std::move(ammo), attack, 20, c->getPosition(), dir);
}

