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

SERIALIZE_DEF(ExternalEnemies, events, attackTime, waves)
SERIALIZATION_CONSTRUCTOR_IMPL(ExternalEnemies)

ExternalEnemies::ExternalEnemies(RandomGen& random, vector<EnemyEvent> e) : events(e) {
  for (int i : All(events))
    attackTime.push_back(random.get(events[i].attackTime));
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

void ExternalEnemies::updateWaves(WCollective target) {
  auto areAllDead = [](const vector<WCreature>& wave) {
    for (auto c : wave)
      if (!c->isDead())
        return false;
    return true;
  };
  for (auto wave : Iter(waves))
    if (areAllDead(wave->attackers)) {
      target->onExternalEnemyKilled(wave->name);
      wave.markToErase();
    }
}

void ExternalEnemies::update(WLevel level, double localTime) {
  WCollective target = level->getModel()->getGame()->getPlayerCollective();
  CHECK(!!target);
  updateWaves(target);
  for (int i : All(events))
    if (attackTime[i] && *attackTime[i] <= localTime) {
      auto enemy = Random.choose(events[i].enemies);
      enemy.factory.increaseLevel(events[i].levelIncrease);
      vector<WCreature> attackers;
      for (int j : Range(Random.get(enemy.groupSize))) {
        PCreature c = enemy.factory.random(
            MonsterAIFactory::singleTask(getAttackTask(target, enemy.behaviour)));
        WCreature ref = c.get();
        if (level->landCreature(StairKey::transferLanding(), std::move(c)))
          attackers.push_back(ref);
        ref->getAttributes().setCourage(1);
      }
      target->addAttack(CollectiveAttack(enemy.name, attackers));
      waves.push_back(Wave{enemy.name, attackers});
      attackTime[i] = none;
    }
}


EnemyEvent::EnemyEvent(ExternalEnemy enemy, Range attackTime, EnumMap<ExperienceType, int> levelIncrease)
  : EnemyEvent(vector<ExternalEnemy>{enemy}, attackTime, levelIncrease) {}

EnemyEvent::EnemyEvent(vector<ExternalEnemy> e, Range attackT, EnumMap<ExperienceType, int> increase)
    : enemies(e), attackTime(attackT), levelIncrease(increase) {}
