#include "zlevel.h"
#include "content_factory.h"
#include "resource_counts.h"
#include "z_level_info.h"
#include "enemy_factory.h"
#include "collective_builder.h"
#include "level_maker.h"
#include "attack_trigger.h"
#include "tribe_alignment.h"
#include "external_enemies.h"
#include "model.h"
#include "level.h"
#include "enemy_aggression_level.h"

int getZLevelCombatExp(int depth) {
  return depth * 3 / 2;
}

static EnemyInfo getEnemy(EnemyId id, ContentFactory* contentFactory) {
  auto enemy = EnemyFactory(Random, contentFactory->getCreatures().getNameGenerator(), contentFactory->enemies,
      contentFactory->buildingInfo, {}).get(id);
  enemy.settlement.collective = new CollectiveBuilder(enemy.config, enemy.settlement.tribe);
  return enemy;
}

static double modifyAggression(double value, EnemyAggressionLevel aggressionLevel) {
  switch (aggressionLevel) {
    case EnemyAggressionLevel::NONE: return 0;
    case EnemyAggressionLevel::MODERATE: return value;
    case EnemyAggressionLevel::EXTREME: return min(1.0, 2 * value);
  }
}

static LevelMakerResult getLevelMaker(const ZLevelType& levelInfo, ResourceCounts resources, TribeId tribe,
    ContentFactory* contentFactory, Vec2 size, EnemyAggressionLevel aggressionLevel) {
  return levelInfo.visit(
      [&](const WaterZLevel& level) {
        return LevelMakerResult{
            LevelMaker::getWaterZLevel(Random, level.waterType, size.x, level.creatures),
            vector<EnemyInfo>()
        };
      },
      [&](const EnemyZLevel& level) {
        auto enemy = getEnemy(level.enemy, contentFactory);
        CHECK(level.attackChance < 0.0001 || !!enemy.behaviour)
            << "Z-level enemy " << level.enemy.data() << " has positive attack chance, but no attack behaviour defined";
        if (Random.chance(modifyAggression(level.attackChance, aggressionLevel))) {
          enemy.behaviour->triggers.push_back(Immediate{});
        }
        return LevelMakerResult{
            LevelMaker::settlementLevel(*contentFactory, Random, enemy.settlement, size,
                resources, tribe, level.mountainType),
            vector<EnemyInfo>{std::move(enemy)}
        };
      },
      [&](const FullZLevel& level) {
        optional<SettlementInfo> settlement;
        vector<EnemyInfo> enemy;
        if (level.enemy) {
          enemy.push_back(getEnemy(*level.enemy, contentFactory));
          settlement = enemy[0].settlement;
          CHECK(level.attackChance < 0.0001 || !!enemy[0].behaviour)
              << "Z-level enemy " << level.enemy->data() << " has positive attack chance, but no attack behaviour defined";
          if (Random.chance(modifyAggression(level.attackChance, aggressionLevel))) {
            enemy[0].behaviour->triggers.push_back(Immediate{});
          }
        }
        return LevelMakerResult{
            LevelMaker::getFullZLevel(Random, settlement, resources, size.x, tribe, *contentFactory),
            std::move(enemy)
        };
      });
}

LevelMakerResult getLevelMaker(RandomGen& random, ContentFactory* contentFactory, const vector<string>& zLevelGroups,
    int depth, TribeId tribe, Vec2 size, EnemyAggressionLevel aggressionLevel) {
  vector<ZLevelInfo> levels;
  for (auto& group : zLevelGroups)
    levels.append(contentFactory->zLevels.at(group));
  auto zLevel = *chooseZLevel(random, levels, depth);
  auto res = *chooseResourceCounts(random, contentFactory->resources, depth);
  return getLevelMaker(zLevel, res, tribe, contentFactory, size, aggressionLevel);
}


LevelMakerResult getUpLevel(RandomGen& random, ContentFactory* contentFactory,
    int depth, Position pos, bool withEnemy) {
  auto& biomeInfo = contentFactory->biomeInfo.at(pos.getModel()->getBiomeId());
  vector<EnemyInfo> enemies;
  if (withEnemy)
    for (auto& enemyInfo : biomeInfo.mountainEnemies)
      if (enemyInfo.first.contains(depth) && random.chance(enemyInfo.second.probability))
        for (int it : Range(Random.get(enemyInfo.second.count))) {
          enemies.push_back(getEnemy(enemyInfo.second.id, contentFactory));
          enemies.back().settlement.collective = new CollectiveBuilder(enemies.back().config,
              enemies.back().settlement.tribe);
        }
  auto res = chooseResourceCounts(random, contentFactory->resources, -depth);
  auto maker = LevelMaker::upLevel(pos, biomeInfo, enemies.transform([](auto e) {return e.settlement; }), res);
  auto size = pos.getModel()->getGroundLevel()->getBounds().getSize();
  return LevelMakerResult { std::move(maker), std::move(enemies)};
}
