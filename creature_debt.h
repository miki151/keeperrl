#pragma once

#include "util.h"

class Creature;

class CreatureDebt {
  public:
  int getTotal() const;
  vector<Creature*> getCreditors() const;
  int getAmountOwed(Creature* creditor) const;
  void add(Creature*, int);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);

  private:
  map<Creature*, int> SERIAL(debt);
  int SERIAL(total);
};
