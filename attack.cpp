#include "stdafx.h"

using namespace std;

Attack::Attack(const Creature* a, AttackLevel l, AttackType t, int h, int s, bool b, PEffect _effect)
    : attacker(a), level(l), type(t), toHit(h), strength(s), back(b), effect(std::move(_effect)) {}
  
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
  
Effect* Attack::getEffect() const {
  return effect.get();
}
