#ifndef _ATTACK_H
#define _ATTACK_H

#include "enums.h"

class Creature;

class Attack {
  public:
  Attack(const Creature* attacker, AttackLevel level, AttackType type, int toHit, int strength, bool back = false);

  const Creature* getAttacker() const;
  int getStrength() const;
  int getToHit() const;
  AttackType getType() const;
  AttackLevel getLevel() const;
  bool inTheBack() const;

  private:
  const Creature* attacker;
  AttackLevel level;
  AttackType type;
  int toHit;
  int strength;
  bool back;
};

#endif
