#pragma once

#include "util.h"

class CollectiveAttack {
  public:
  CollectiveAttack(vector<WConstTask> attackTasks, WCollective attacker, const vector<Creature*>& creatures, optional<int> ransom = none);
  CollectiveAttack(vector<WConstTask> attackTasks, const string& name, const vector<Creature*>& creatures);

  WCollective getAttacker() const;
  const string& getAttackerName() const;
  const vector<Creature*>& getCreatures() const;
  optional<int> getRansom() const;
  bool isOngoing() const;

  bool operator == (const CollectiveAttack&) const;

  SERIALIZATION_DECL(CollectiveAttack);

  private:
  optional<int> SERIAL(ransom);
  vector<Creature*> SERIAL(creatures);
  WCollective SERIAL(attacker) = nullptr;
  string SERIAL(attackerName);
  vector<WeakPointer<const Task>> SERIAL(attackTasks);
};

