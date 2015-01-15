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

#include "attack.h"



Attack::Attack(const Creature* a, AttackLevel l, AttackType t, int h, int s, bool b, optional<EffectType> _effect)
    : attacker(a), level(l), type(t), accuracy(h), strength(s), back(b), effect(_effect) {}
  
const Creature* Attack::getAttacker() const {
  return attacker;
}

int Attack::getStrength() const {
  return strength;
}

int Attack::getAccuracy() const {
  return accuracy;
}

AttackType Attack::getType() const {
  return type;
}

AttackLevel Attack::getLevel() const {
  return level;
}

bool Attack::inTheBack() const {
  return back;
}
  
optional<EffectType> Attack::getEffect() const {
  return effect;
}
