#include "stdafx.h"
#include "creature_debt.h"
#include "creature.h"


template <class Archive>
void CreatureDebt::serialize(Archive& ar, const unsigned int version) {
  ar(debt, total);
}

SERIALIZABLE(CreatureDebt);

int CreatureDebt::getTotal() const {
  return total;
}

vector<Creature::Id> CreatureDebt::getCreditors() const {
  return debt.getKeys();
}

int CreatureDebt::getAmountOwed(Creature* creditor) const {
  return debt.getMaybe(creditor).value_or(0);
}

void CreatureDebt::add(Creature* c, int amount) {
  auto& current = debt.getOrInit(c);
  amount = max(-current, amount);
  total += amount;
  if ((current += amount) <= 0)
    debt.erase(c);
}
