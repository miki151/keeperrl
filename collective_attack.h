#pragma once

#include "util.h"
#include "view_id.h"

class CollectiveAttack {
  public:
  CollectiveAttack(vector<const Task*> attackTasks, Collective* attacker, const vector<Creature*>&,
      optional<int> ransom = none);
  CollectiveAttack(vector<const Task*> attackTasks, const string& name, ViewIdList, const vector<Creature*>&);

  Collective* getAttacker() const;
  const string& getAttackerName() const;
  ViewIdList getAttackerViewId() const;
  const vector<Creature*>& getCreatures() const;
  optional<int> getRansom() const;
  bool isOngoing() const;

  bool operator == (const CollectiveAttack&) const;

  SERIALIZATION_DECL(CollectiveAttack)

  private:
  optional<int> SERIAL(ransom);
  vector<Creature*> SERIAL(creatures);
  Collective* SERIAL(attacker) = nullptr;
  string SERIAL(attackerName);
  ViewIdList SERIAL(attackerViewId);
  vector<WeakPointer<const Task>> SERIAL(attackTasks);
};

