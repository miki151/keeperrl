#pragma once

#include "util.h"
#include "view_id.h"

class CollectiveAttack {
  public:
  CollectiveAttack(vector<WConstTask> attackTasks, WCollective attacker, const vector<Creature*>& creatures, optional<int> ransom = none);
  CollectiveAttack(vector<WConstTask> attackTasks, const string& name, ViewId, const vector<Creature*>& creatures);

  WCollective getAttacker() const;
  const string& getAttackerName() const;
  ViewId getAttackerViewId() const;
  const vector<Creature*>& getCreatures() const;
  optional<int> getRansom() const;
  bool isOngoing() const;

  bool operator == (const CollectiveAttack&) const;

  SERIALIZATION_DECL(CollectiveAttack)

  private:
  optional<int> SERIAL(ransom);
  vector<Creature*> SERIAL(creatures);
  WCollective SERIAL(attacker) = nullptr;
  string SERIAL(attackerName);
  ViewId SERIAL(attackerViewId);
  vector<WeakPointer<const Task>> SERIAL(attackTasks);
};

