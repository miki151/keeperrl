#include "stdafx.h"
#include "external_enemies.h"
#include "monster_ai.h"
#include "task.h"
#include "level.h"
#include "collective.h"
#include "creature.h"
#include "collective_attack.h"
#include "attack_behaviour.h"
#include "model.h"
#include "game.h"
#include "creature_attributes.h"
#include "creature.h"
#include "container_range.h"
#include "monster.h"
#include "view_object.h"

SERIALIZE_DEF(ExternalEnemies, currentWaves, waves)
SERIALIZATION_CONSTRUCTOR_IMPL(ExternalEnemies)

ExternalEnemies::ExternalEnemies(RandomGen& random, vector<EnemyEvent> events) {
  for (int i : All(events)) {
    auto enemy = Random.choose(events[i].enemies);
    waves.push_back(NextWave {
        enemy.factory.increaseLevel(events[i].levelIncrease),
        Random.get(enemy.groupSize),
        enemy.factory.random()->getViewObject().id(),
        enemy.name,
        random.get(events[i].attackTime),
        enemy.behaviour,
        i
      });
  }
}

PTask ExternalEnemies::getAttackTask(WCollective enemy, AttackBehaviour behaviour) {
  switch (behaviour.getId()) {
    case AttackBehaviourId::KILL_LEADER:
      return Task::attackLeader(enemy);
    case AttackBehaviourId::KILL_MEMBERS:
      return Task::killFighters(enemy, behaviour.get<int>());
    case AttackBehaviourId::STEAL_GOLD:
      return Task::stealFrom(enemy, callbackDummy.get());
    case AttackBehaviourId::CAMP_AND_SPAWN:
      return Task::campAndSpawn(enemy,
            behaviour.get<CreatureFactory>(), Random.get(3, 7), Range(3, 7), Random.get(3, 7));
  }
}

void ExternalEnemies::updateCurrentWaves(WCollective target) {
  auto areAllDead = [](const vector<WCreature>& wave) {
    for (auto c : wave)
      if (!c->isDead())
        return false;
    return true;
  };
  for (auto wave : Iter(currentWaves))
    if (areAllDead(wave->attackers)) {
      target->onExternalEnemyKilled(wave->name);
      wave.markToErase();
    }
}

void ExternalEnemies::update(WLevel level, double localTime) {
  WCollective target = level->getModel()->getGame()->getPlayerCollective();
  CHECK(!!target);
  updateCurrentWaves(target);
  if (auto nextWave = popNextWave(localTime)) {
    vector<WCreature> attackers;
    for (int i : Range(nextWave->numCreatures)) {
      auto c = nextWave->factory.random(MonsterAIFactory::singleTask(getAttackTask(target, nextWave->behaviour)));
      c->getAttributes().setCourage(1);
      auto ref = c.get();
      if (level->landCreature(StairKey::transferLanding(), std::move(c)))
        attackers.push_back(ref);
    }
    target->addAttack(CollectiveAttack(nextWave->name, attackers));
    currentWaves.push_back(CurrentWave{nextWave->name, attackers});
  }
}

optional<const ExternalEnemies::NextWave&> ExternalEnemies::getNextWave() const {
  if (!waves.empty())
    return waves.front();
  else
    return none;
}

optional<ExternalEnemies::NextWave> ExternalEnemies::popNextWave(double localTime) {
  if (!waves.empty() && waves.front().attackTime <= localTime) {
    auto ret = std::move(waves.front());
    waves.pop_front();
    return ret;
  } else
    return none;
}


EnemyEvent::EnemyEvent(ExternalEnemy enemy, Range attackTime, EnumMap<ExperienceType, int> levelIncrease)
  : EnemyEvent(vector<ExternalEnemy>{enemy}, attackTime, levelIncrease) {}

EnemyEvent::EnemyEvent(vector<ExternalEnemy> e, Range attackT, EnumMap<ExperienceType, int> increase)
    : enemies(e), attackTime(attackT), levelIncrease(increase) {}
