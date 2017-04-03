#pragma once

#include "creature_factory.h"
#include "attack_behaviour.h"
#include "task_callback.h"

struct ExternalEnemy {
  CreatureFactory SERIAL(factory);
  Range SERIAL(groupSize);
  Range SERIAL(attackTime);
  AttackBehaviour SERIAL(behaviour);
  string SERIAL(name);
  SERIALIZE_ALL(factory, groupSize, attackTime, behaviour, name)
};

class ExternalEnemies {
  public:
  ExternalEnemies(RandomGen&, vector<ExternalEnemy>);
  void update(Level*, double localTime);

  SERIALIZATION_DECL(ExternalEnemies)

  private:
  vector<ExternalEnemy> SERIAL(enemies);
  vector<optional<int>> SERIAL(attackTime);
  PTask getAttackTask(WCollective target, AttackBehaviour);
};
