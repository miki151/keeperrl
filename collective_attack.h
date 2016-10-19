#pragma once

#include "util.h"

class CollectiveAttack {
  public:
  CollectiveAttack(Collective* attacker, const vector<Creature*>& creatures, optional<int> ransom = none);

  Collective* getAttacker() const;
  const vector<Creature*>& getCreatures() const;
  optional<int> getRansom() const;

  bool operator == (const CollectiveAttack&) const;

  SERIALIZATION_DECL(CollectiveAttack);

  private:
  optional<int> SERIAL(ransom);
  vector<Creature*> SERIAL(creatures);
  Collective* SERIAL(attacker);
};

