#pragma once

#include "util.h"
#include "unique_entity.h"
#include "entity_map.h"

class Creature;

class CreatureDebt {
  public:
  int getTotal(const Creature*) const;
  vector<Creature*> getCreditors(const Creature* creature) const;
  int getAmountOwed(const Creature* creditor) const;
  void add(const Creature*, int);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);

  private:
  EntityMap<Creature, int> SERIAL(debt);
  int SERIAL(total);
};
