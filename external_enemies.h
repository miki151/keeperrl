#pragma once

#include "creature_factory.h"
#include "attack_behaviour.h"
#include "task_callback.h"
#include "item_type.h"

struct ExternalEnemy {
  CreatureFactory SERIAL(factory);
  Range SERIAL(groupSize);
  AttackBehaviour SERIAL(behaviour);
  string SERIAL(name);
  SERIALIZE_ALL(factory, groupSize, behaviour, name)
};

struct EnemyEvent {
  EnemyEvent(ExternalEnemy, Range attackTime, EnumMap<ExperienceType, int> = {});
  EnemyEvent(vector<ExternalEnemy>, Range attackTime, EnumMap<ExperienceType, int> = {});
  vector<ExternalEnemy> SERIAL(enemies);
  Range SERIAL(attackTime);
  EnumMap<ExperienceType, int> SERIAL(levelIncrease);
  SERIALIZATION_CONSTRUCTOR(EnemyEvent)
  SERIALIZE_ALL(enemies, attackTime, levelIncrease)
};

class ExternalEnemies {
  public:
  ExternalEnemies(RandomGen&, vector<EnemyEvent>);
  void update(WLevel, double localTime);
  struct CurrentWave {
    string SERIAL(name);
    vector<WCreature> SERIAL(attackers);
    SERIALIZE_ALL(name, attackers)
  };
  struct NextWave {
    NextWave() {}
    NextWave(const NextWave&) = delete;
    NextWave(NextWave&&) = default; // this is to fix a compiler error that I did't understand
    vector<PCreature> SERIAL(attackers);
    string SERIAL(name);
    double SERIAL(attackTime);
    AttackBehaviour SERIAL(behaviour);
    int SERIAL(id);
    SERIALIZE_ALL(attackers, name, attackTime, behaviour, id)
  };
  optional<const NextWave&> getNextWave() const;

  SERIALIZATION_DECL(ExternalEnemies)

  private:
  optional<NextWave> popNextWave(double localTime);
  void updateCurrentWaves(WCollective target);
  OwnerPointer<TaskCallback> callbackDummy = makeOwner<TaskCallback>();
  vector<CurrentWave> SERIAL(currentWaves);
  std::deque<NextWave> SERIAL(waves);
  PTask getAttackTask(WCollective target, AttackBehaviour);
};
