#include "stdafx.h"
#include "creature_debt.h"
#include "creature.h"

SERIALIZE_DEF(CreatureDebt, debt, total)

int CreatureDebt::getTotal() const {
  return total;
}

vector<Creature*> CreatureDebt::getCreditors() const {
  return getKeys(debt);
}

int CreatureDebt::getAmountOwed(Creature* creditor) const {
  return getValueMaybe(debt, creditor).get_value_or(0);
}

void CreatureDebt::add(Creature* c, int amount) {
  auto& current = debt[c];
  amount = max(-current, amount);
  total += amount;
  if ((current += amount) <= 0)
    debt.erase(c);
}
