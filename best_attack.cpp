#include "stdafx.h"
#include "best_attack.h"
#include "creature.h"
#include "attr_type.h"

BestAttack::BestAttack(const Creature* c) {
  attr = AttrType::DAMAGE;
  value = 0;
  for (auto a : {AttrType::DAMAGE, AttrType::SPELL_DAMAGE, AttrType::RANGED_DAMAGE}) {
    auto damage = c->getAttr(a);
    if (damage > value) {
      value = damage;
      attr = a;
    }
  }
}
