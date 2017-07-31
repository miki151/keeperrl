#pragma once

#include "creature_factory.h"
#include "attack_behaviour.h"
#include "task_callback.h"

struct ExternalEnemy {
  CreatureFactory SERIAL(factory);
  Range SERIAL(groupSize);
  AttackBehaviour SERIAL(behaviour);
  string SERIAL(name);
  SERIALIZE_ALL(factory, groupSize, behaviour, name)
};

struct EnemyEvent {
  EnemyEvent(ExternalEnemy, Range attackTime, double levelIncrease = 0);
  EnemyEvent(vector<ExternalEnemy>, Range attackTime, double levelIncrease = 0);
  vector<ExternalEnemy> SERIAL(enemies);
  Range SERIAL(attackTime);
  double SERIAL(levelIncrease);
  SERIALIZATION_CONSTRUCTOR(EnemyEvent)
  SERIALIZE_ALL(enemies, attackTime, levelIncrease)
};

class ExternalEnemies {
  public:
  ExternalEnemies(RandomGen&, vector<EnemyEvent>);
  void update(WLevel, double localTime);

  SERIALIZATION_DECL(ExternalEnemies)

  private:
  OwnerPointer<TaskCallback> callbackDummy = makeOwner<TaskCallback>();
  vector<EnemyEvent> SERIAL(events);
  vector<optional<int>> SERIAL(attackTime);
  PTask getAttackTask(WCollective target, AttackBehaviour);
};
