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

SERIALIZE_DEF(ExternalEnemies, enemies, attackTime)
SERIALIZATION_CONSTRUCTOR_IMPL(ExternalEnemies)

ExternalEnemies::ExternalEnemies(RandomGen& random, vector<ExternalEnemy> e) : enemies(e) {
  for (int i : All(enemies))
    attackTime.push_back(random.get(enemies[i].attackTime));
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

void ExternalEnemies::update(WLevel level, double localTime) {
  WCollective target = level->getModel()->getGame()->getPlayerCollective();
  CHECK(!!target);
  for (int i : All(enemies))
    if (attackTime[i] && *attackTime[i] <= localTime) {
      vector<WCreature> attackers;
      for (int j : Range(Random.get(enemies[i].groupSize))) {
        PCreature c = enemies[i].factory.random(
            MonsterAIFactory::singleTask(getAttackTask(target, enemies[i].behaviour)));
        WCreature ref = c.get();
        if (level->landCreature(StairKey::transferLanding(), std::move(c)))
          attackers.push_back(ref);
      }
      target->addAttack(CollectiveAttack(enemies[i].name, attackers));
      attackTime[i] = none;
    }
}

