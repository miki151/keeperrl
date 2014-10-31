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

#include "ranged_weapon.h"
#include "creature.h"
#include "level.h"

template <class Archive> 
void RangedWeapon::serialize(Archive& ar, const unsigned int version) {
  ar & SUBCLASS(Item);
}

SERIALIZABLE(RangedWeapon);

SERIALIZATION_CONSTRUCTOR_IMPL(RangedWeapon);

RangedWeapon::RangedWeapon(const ItemAttributes& attr) : Item(attr) {}

void RangedWeapon::fire(Creature* c, Level* l, PItem ammo, Vec2 dir) {
  int toHitVariance = 10;
  int attackVariance = 15;
  int toHit = Random.getRandom(-toHitVariance, toHitVariance) + 
    c->getAttr(AttrType::FIRED_ACCURACY) +
    ammo->getModifier(AttrType::FIRED_ACCURACY) +
    getModifier(AttrType::FIRED_ACCURACY);
  int damage = Random.getRandom(-attackVariance, attackVariance) + 
    c->getAttr(AttrType::FIRED_DAMAGE) +
    ammo->getModifier(AttrType::FIRED_DAMAGE) +
    getModifier(AttrType::FIRED_DAMAGE);
  Attack attack(c, chooseRandom({AttackLevel::LOW, AttackLevel::MIDDLE, AttackLevel::HIGH}),
      AttackType::SHOOT, toHit, damage, false, Nothing());
  l->throwItem(std::move(ammo), attack, 20, c->getPosition(), dir, c->getVision());
}

