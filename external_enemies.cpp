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
#include "territory.h"

SERIALIZE_DEF(ExternalEnemies, currentWaves, waves, nextWave)
SERIALIZATION_CONSTRUCTOR_IMPL(ExternalEnemies)

ExternalEnemies::ExternalEnemies(RandomGen& random, vector<ExternalEnemy> enemies) {
  constexpr int firstAttackDelay = 1800;
  constexpr int attackInterval = 1200;
  constexpr int attackVariation = 450;
  for (int i : Range(500)) {
    int attackTime = firstAttackDelay + max(0, i * attackInterval + random.get(-attackVariation, attackVariation + 1));
    vector<int> indexes(enemies.size());
    for (int i : All(enemies))
      indexes[i] = i;
    for (int index : random.permutation(indexes)) {
      auto& enemy = enemies[index];
      if (enemy.attackTime.contains(attackTime) && enemy.maxOccurences > 0) {
        --enemy.maxOccurences;
        waves.push_back(EnemyEvent{
            enemy,
            LocalTime(attackTime),
            enemy.creatures.getViewId()
        });
        waves.back().enemy.creatures.increaseBaseLevel({{ExperienceType::MELEE, max(0, attackTime / 1000 - 10)}});
        break;
      }
    }
  }
}

PTask ExternalEnemies::getAttackTask(WCollective enemy, AttackBehaviour behaviour) {
  switch (behaviour.getId()) {
    case AttackBehaviourId::KILL_LEADER:
      if (auto leader = enemy->getLeader())
        return Task::attackCreatures({leader});
      else
        return Task::idle();
    case AttackBehaviourId::KILL_MEMBERS:
      return Task::killFighters(enemy, behaviour.get<int>());
    case AttackBehaviourId::STEAL_GOLD:
      return Task::stealFrom(enemy, callbackDummy.get());
    case AttackBehaviourId::CAMP_AND_SPAWN:
      return Task::campAndSpawn(enemy,
            behaviour.get<CreatureFactory>(), Random.get(3, 7), Range(3, 7), Random.get(3, 7));
    case AttackBehaviourId::HALLOWEEN_KIDS: {
      auto nextToDoor = enemy->getTerritory().getExtended(2, 4);
      if (nextToDoor.empty()) {
        if (auto leader = enemy->getLeader())
          return Task::goToTryForever(leader->getPosition());
        else
          return Task::idle();
      } else
        return Task::goToTryForever(Random.choose(nextToDoor));
    }
  }
}

void ExternalEnemies::updateCurrentWaves(WCollective target) {
  auto areAllDead = [](const vector<WCreature>& wave) {
    for (auto c : wave)
      if (!c->isDead() && !c->isAffected(LastingEffect::STUNNED))
        return false;
    return true;
  };
  for (auto wave : Iter(currentWaves))
    if (areAllDead(wave->attackers)) {
      target->onExternalEnemyKilled(wave->name);
      wave.markToErase();
    }
}

void ExternalEnemies::update(WLevel level, LocalTime localTime) {
  WCollective target = level->getModel()->getGame()->getPlayerCollective();
  CHECK(!!target);
  updateCurrentWaves(target);
  if (auto nextWave = popNextWave(localTime)) {
    vector<WCreature> attackers;
    Vec2 landingDir(Random.choose<Dir>());
    auto creatures = nextWave->enemy.creatures.generate(Random, TribeId::getHuman(),
        MonsterAIFactory::singleTask(getAttackTask(target, nextWave->enemy.behaviour),
            nextWave->enemy.behaviour.getId() != AttackBehaviourId::HALLOWEEN_KIDS));
    for (auto& c : creatures) {
      c->getAttributes().setCourage(1);
      auto ref = c.get();
      if (level->landCreature(StairKey::transferLanding(), std::move(c), landingDir))
        attackers.push_back(ref);
    }
    if (nextWave->enemy.behaviour.getId() != AttackBehaviourId::HALLOWEEN_KIDS) {
      target->addAttack(CollectiveAttack(nextWave->enemy.name, attackers));
      currentWaves.push_back(CurrentWave{nextWave->enemy.name, attackers});
    }
  }
}

optional<const EnemyEvent&> ExternalEnemies::getNextWave() const {
  if (nextWave < waves.size())
    return waves[nextWave];
  else
    return none;
}

int ExternalEnemies::getNextWaveIndex() const {
  return nextWave;
}

optional<EnemyEvent> ExternalEnemies::popNextWave(LocalTime localTime) {
  if (nextWave < waves.size() && waves[nextWave].attackTime <= localTime) {
    return waves[nextWave++];
  } else
    return none;
}
