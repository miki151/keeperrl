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

#ifndef _ATTACK_H
#define _ATTACK_H

#include "enums.h"
#include "util.h"
#include "effect_type.h"

class Creature;

class Attack {
  public:
  Attack(const Creature* attacker, AttackLevel level, AttackType type, int toHit, int strength,
      bool back, Optional<EffectType> effect = Nothing());

  const Creature* getAttacker() const;
  int getStrength() const;
  int getToHit() const;
  AttackType getType() const;
  AttackLevel getLevel() const;
  bool inTheBack() const;
  Optional<EffectType> getEffect() const;

  private:
  const Creature* attacker;
  AttackLevel level;
  AttackType type;
  int toHit;
  int strength;
  bool back;
  Optional<EffectType> effect;
};

#endif
