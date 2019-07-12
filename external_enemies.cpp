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
#include "content_factory.h"
#include "external_enemies_type.h"

SERIALIZE_DEF(ExternalEnemies, currentWaves, waves, nextWave)
SERIALIZATION_CONSTRUCTOR_IMPL(ExternalEnemies)

ExternalEnemies::ExternalEnemies(RandomGen& random, CreatureFactory* factory, vector<ExternalEnemy> enemies, ExternalEnemiesType type) {
  constexpr int firstAttackDelay = 1800;
  constexpr int attackInterval = 1200;
  constexpr int attackVariation = 450;
  constexpr int afterWinTurn = 15000;
  if (type == ExternalEnemiesType::FROM_START)
    startTime = 0_local;
  for (int i : Range(500)) {
    int attackTime = firstAttackDelay + max(0, i * attackInterval + random.get(-attackVariation, attackVariation + 1));
    vector<int> indexes(enemies.size());
    for (int i : All(enemies))
      indexes[i] = i;
    for (int index : random.permutation(indexes)) {
      auto& enemy = enemies[index];
      auto attackRange = enemy.attackTime;
      if (type == ExternalEnemiesType::AFTER_WINNING)
        attackRange = attackRange - afterWinTurn;
      if (attackRange.contains(attackTime) && enemy.maxOccurences > 0) {
        --enemy.maxOccurences;
        waves.push_back(EnemyEvent{
            enemy,
            LocalTime(attackTime),
            enemy.creatures.getViewId(factory)
        });
        waves.back().enemy.creatures.increaseBaseLevel({{ExperienceType::MELEE, max(0, attackTime / 1000 - 10)}});
        break;
      }
    }
  }
}

PTask ExternalEnemies::getAttackTask(WCollective enemy, AttackBehaviour behaviour) {
  return behaviour.visit(
      [&](KillLeader) {
        if (auto leader = enemy->getLeader())
          return Task::attackCreatures({leader});
        else
          return Task::idle();
      },
      [&](KillMembers t) {
        return Task::killFighters(enemy, t.count);
      },
      [&](StealGold) {
        return Task::stealFrom(enemy);
      },
      [&](CampAndSpawn t) {
        return Task::campAndSpawn(enemy, t, Random.get(3, 7));
      },
      [&](HalloweenKids) {
        auto nextToDoor = enemy->getTerritory().getExtended(2, 4);
        if (nextToDoor.empty()) {
          if (auto leader = enemy->getLeader())
            return Task::goToTryForever(leader->getPosition());
          else
            return Task::idle();
        } else
          return Task::goToTryForever(Random.choose(nextToDoor));
      }
  );
}

void ExternalEnemies::updateCurrentWaves(WCollective target) {
  auto areAllDead = [](const vector<Creature*>& wave) {
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
  if (!startTime && level->getGame()->gameWon())
    startTime = localTime;
  if (!startTime)
    return;
  localTime = 0_local + (localTime - *startTime);
  WCollective target = level->getModel()->getGame()->getPlayerCollective();
  CHECK(!!target);
  updateCurrentWaves(target);
  if (auto nextWave = popNextWave(localTime)) {
    vector<Creature*> attackers;
    Vec2 landingDir(Random.choose<Dir>());
    auto attackTask = getAttackTask(target, nextWave->enemy.behaviour);
    auto attackTaskRef = attackTask.get();
    auto creatures = nextWave->enemy.creatures.generate(Random, &level->getGame()->getContentFactory()->getCreatures(),
        TribeId::getMonster(), MonsterAIFactory::singleTask(std::move(attackTask),
            !nextWave->enemy.behaviour.contains<HalloweenKids>()));
    for (auto& c : creatures) {
      c->getAttributes().setCourage(1);
      auto ref = c.get();
      if (level->landCreature(StairKey::transferLanding(), std::move(c), landingDir))
        attackers.push_back(ref);
    }
    if (!nextWave->enemy.behaviour.contains<HalloweenKids>()) {
      target->addAttack(CollectiveAttack({attackTaskRef}, nextWave->enemy.name, attackers));
      currentWaves.push_back(CurrentWave{nextWave->enemy.name, attackers});
    }
  }
}

optional<LocalTime> ExternalEnemies::getStartTime() const {
  return startTime;
}

optional<const EnemyEvent&> ExternalEnemies::getNextWave() const {
  if (!!startTime && nextWave < waves.size())
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
