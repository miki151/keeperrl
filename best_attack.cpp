#include "stdafx.h"
#include "best_attack.h"
#include "creature.h"
#include "attr_type.h"

BestAttack::BestAttack(const Creature* c) {
  auto damage = c->getAttr(AttrType::DAMAGE);
  auto spellDamage = c->getAttr(AttrType::SPELL_DAMAGE);
  if (damage > spellDamage) {
    attr = AttrType::DAMAGE;
    value = damage;
  } else {
    attr = AttrType::SPELL_DAMAGE;
    value = spellDamage;
  }
}
