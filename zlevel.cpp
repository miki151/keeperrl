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

static EnemyInfo getEnemy(EnemyId id, ContentFactory* contentFactory) {
  auto enemy = EnemyFactory(Random, contentFactory->getCreatures().getNameGenerator(), contentFactory->enemies,
      contentFactory->buildingInfo, {}).get(id);
  enemy.settlement.collective = new CollectiveBuilder(enemy.config, enemy.settlement.tribe);
  return enemy;
}

static LevelMakerResult getLevelMaker(const ZLevelInfo& levelInfo, ResourceCounts resources, TribeId tribe,
    ContentFactory* contentFactory, Vec2 size) {
  return levelInfo.type.visit(
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
        if (Random.chance(level.attackChance)) {
          enemy.behaviour->triggers.push_back(Immediate{});
        }
        return LevelMakerResult{
            LevelMaker::settlementLevel(*contentFactory, Random, enemy.settlement, size,
                resources, tribe),
            vector<EnemyInfo>{std::move(enemy)}
        };
      },
      [&](const FullZLevel& level) {
        optional<SettlementInfo> settlement;
        optional<EnemyInfo> enemy;
        if (level.enemy) {
          enemy = getEnemy(*level.enemy, contentFactory);
          settlement = enemy->settlement;
          CHECK(level.attackChance < 0.0001 || !!enemy->behaviour)
              << "Z-level enemy " << level.enemy->data() << " has positive attack chance, but no attack behaviour defined";
          if (Random.chance(level.attackChance)) {
            enemy->behaviour->triggers.push_back(Immediate{});
          }
        }
        return LevelMakerResult{
            LevelMaker::getFullZLevel(Random, settlement, resources, size.x, tribe, *contentFactory),
            vector<EnemyInfo>()
        };
      });
}

LevelMakerResult getLevelMaker(RandomGen& random, ContentFactory* contentFactory, const vector<string>& zLevelGroups,
    int depth, TribeId tribe, Vec2 size) {
  vector<ZLevelInfo> levels;
  for (auto& group : zLevelGroups) 
    levels.append(contentFactory->zLevels.at(group));
  auto zLevel = *chooseZLevel(random, levels, depth);
  auto res = *chooseResourceCounts(random, contentFactory->resources, depth);
  return getLevelMaker(zLevel, res, tribe, contentFactory, size);
}


LevelMakerResult getUpLevel(RandomGen& random, ContentFactory* contentFactory,
    int depth, Position pos) {
  auto& biomeInfo = contentFactory->biomeInfo.at(pos.getModel()->getBiomeId());
  vector<EnemyInfo> enemies;
  for (auto& enemyInfo : biomeInfo.mountainEnemies)
    if (enemyInfo.first.contains(depth) && random.chance(enemyInfo.second.probability))
      for (int it : Range(Random.get(enemyInfo.second.count))) {
        enemies.push_back(getEnemy(enemyInfo.second.id, contentFactory));
        enemies.back().settlement.collective = new CollectiveBuilder(enemies.back().config, enemies.back().settlement.tribe);
      }
  auto res = chooseResourceCounts(random, contentFactory->resources, -depth);
  auto maker = LevelMaker::upLevel(pos, biomeInfo, enemies.transform([](auto e) {return e.settlement; }), res);
  auto size = pos.getModel()->getGroundLevel()->getBounds().getSize();
  return LevelMakerResult { std::move(maker), std::move(enemies)};
}