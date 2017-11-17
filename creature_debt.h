#pragma once

#include "util.h"
#include "unique_entity.h"
#include "entity_map.h"

class Creature;

class CreatureDebt {
  public:
  int getTotal() const;
  vector<UniqueEntity<Creature>::Id> getCreditors() const;
  int getAmountOwed(WCreature creditor) const;
  void add(WCreature, int);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);

  private:
  EntityMap<Creature, int> SERIAL(debt);
  int SERIAL(total);
};
