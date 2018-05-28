#pragma once

#include "util.h"

class CollectiveAttack {
  public:
  CollectiveAttack(vector<WConstTask> attackTasks, WCollective attacker, const vector<WCreature>& creatures, optional<int> ransom = none);
  CollectiveAttack(vector<WConstTask> attackTasks, const string& name, const vector<WCreature>& creatures);

  WCollective getAttacker() const;
  const string& getAttackerName() const;
  const vector<WCreature>& getCreatures() const;
  optional<int> getRansom() const;
  bool isOngoing() const;

  bool operator == (const CollectiveAttack&) const;

  SERIALIZATION_DECL(CollectiveAttack);

  private:
  optional<int> SERIAL(ransom);
  vector<WCreature> SERIAL(creatures);
  WCollective SERIAL(attacker) = nullptr;
  string SERIAL(attackerName);
  vector<WConstTask> SERIAL(attackTasks);
};

