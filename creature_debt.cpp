#include "stdafx.h"
#include "creature_debt.h"
#include "creature.h"
#include "field_of_view.h"

template <class Archive>
void CreatureDebt::serialize(Archive& ar, const unsigned int version) {
  ar(debt, total);
}

SERIALIZABLE(CreatureDebt);

int CreatureDebt::getTotal(const Creature* creature) const {
  int ret = 0;
  for (auto c : creature->getPosition().getAllCreatures(FieldOfView::sightRange))
    ret += getAmountOwed(c);
  return ret;
}

vector<Creature*> CreatureDebt::getCreditors(const Creature* creature) const {
  vector<Creature*> ret;
  for (auto c : creature->getPosition().getAllCreatures(FieldOfView::sightRange))
    if (getAmountOwed(c) > 0)
      ret.push_back(c);
  return ret;
}

int CreatureDebt::getAmountOwed(const Creature* creditor) const {
  return debt.getMaybe(creditor).value_or(0);
}

void CreatureDebt::add(const Creature* c, int amount) {
  auto& current = debt.getOrInit(c);
  amount = max(-current, amount);
  total += amount;
  if ((current += amount) <= 0)
    debt.erase(c);
}
