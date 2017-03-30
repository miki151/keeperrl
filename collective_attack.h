#pragma once

#include "util.h"

class CollectiveAttack {
  public:
  CollectiveAttack(Collective* attacker, const vector<WCreature>& creatures, optional<int> ransom = none);
  CollectiveAttack(const string& name, const vector<WCreature>& creatures);

  Collective* getAttacker() const;
  const string& getAttackerName() const;
  const vector<WCreature>& getCreatures() const;
  optional<int> getRansom() const;

  bool operator == (const CollectiveAttack&) const;

  SERIALIZATION_DECL(CollectiveAttack);

  private:
  optional<int> SERIAL(ransom);
  vector<WCreature> SERIAL(creatures);
  Collective* SERIAL(attacker) = nullptr;
  string SERIAL(attackerName);
};

