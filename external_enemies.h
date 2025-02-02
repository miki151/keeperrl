#pragma once

#include "creature_factory.h"
#include "attack_behaviour.h"
#include "task_callback.h"
#include "inhabitants_info.h"
#include "game_time.h"
#include "view_id.h"

struct ExternalEnemy {
  CreatureList SERIAL(creatures);
  AttackBehaviour SERIAL(behaviour);
  TString SERIAL(name);
  Range SERIAL(attackTime);
  int SERIAL(maxOccurences) = 1;
  SERIALIZE_ALL(NAMED(creatures), NAMED(behaviour), NAMED(name), NAMED(attackTime), OPTION(maxOccurences))
};

static_assert(std::is_nothrow_move_constructible<ExternalEnemy>::value, "T should be noexcept MoveConstructible");


struct EnemyEvent {
  ExternalEnemy SERIAL(enemy);
  LocalTime SERIAL(attackTime);
  ViewIdList SERIAL(viewId);
  SERIALIZE_ALL(enemy, attackTime, viewId)
};

class ExternalEnemies {
  public:
  ExternalEnemies(RandomGen&, CreatureFactory*, vector<ExternalEnemy>, ExternalEnemiesType);
  void update(Level*, LocalTime);
  struct CurrentWave {
    TString SERIAL(name);
    vector<Creature*> SERIAL(attackers);
    SERIALIZE_ALL(name, attackers)
  };
  optional<LocalTime> getStartTime() const;
  optional<const EnemyEvent&> getNextWave() const;
  int getNextWaveIndex() const;
  optional<EnemyEvent> popNextWave(LocalTime localTime);

  SERIALIZATION_DECL(ExternalEnemies)

  private:
  void updateCurrentWaves(Collective* target);
  vector<CurrentWave> SERIAL(currentWaves);
  int SERIAL(nextWave) = 0;
  vector<EnemyEvent> SERIAL(waves);
  PTask getAttackTask(Collective* target, AttackBehaviour);
  optional<LocalTime> SERIAL(startTime);
};
