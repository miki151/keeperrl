#include "stdafx.h"

#include "attack.h"



Attack::Attack(const Creature* a, AttackLevel l, AttackType t, int h, int s, bool b, Optional<EffectType> _effect)
    : attacker(a), level(l), type(t), toHit(h), strength(s), back(b), effect(_effect) {}
  
const Creature* Attack::getAttacker() const {
  return attacker;
}

int Attack::getStrength() const {
  return strength;
}

int Attack::getToHit() const {
  return toHit;
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
  
Optional<EffectType> Attack::getEffect() const {
  return effect;
}
