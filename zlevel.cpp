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

static EnemyInfo getEnemy(EnemyId id, ContentFactory* contentFactory) {
  auto enemy = EnemyFactory(Random, contentFactory->getCreatures().getNameGenerator(), contentFactory->enemies,
      contentFactory->buildingInfo, {}).get(id);
  enemy.settlement.collective = new CollectiveBuilder(enemy.config, enemy.settlement.tribe);
  return enemy;
}

static LevelMakerResult getLevelMaker(const ZLevelInfo& levelInfo, ResourceCounts resources, TribeId tribe,
    StairKey stairKey, ContentFactory* contentFactory) {
  return levelInfo.type.visit(
      [&](const WaterZLevel& level) {
        return LevelMakerResult{
            LevelMaker::getWaterZLevel(Random, level.waterType, levelInfo.width, level.creatures, stairKey),
            none, levelInfo.width
        };
      },
      [&](const EnemyZLevel& level) {
        auto enemy = getEnemy(level.enemy, contentFactory);
        enemy.settlement.upStairs.push_back(stairKey);
        CHECK(level.attackChance < 0.0001 || !!enemy.behaviour)
            << "Z-level enemy " << level.enemy.data() << " has positive attack chance, but no attack behaviour defined";
        if (Random.chance(level.attackChance)) {
          enemy.behaviour->triggers.push_back(Immediate{});
        }
        return LevelMakerResult{
            LevelMaker::settlementLevel(*contentFactory, Random, enemy.settlement, Vec2(levelInfo.width, levelInfo.width),
                resources, tribe),
            std::move(enemy), levelInfo.width
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
            LevelMaker::getFullZLevel(Random, settlement, resources, levelInfo.width, tribe, stairKey, *contentFactory),
            std::move(enemy), levelInfo.width
        };
      });
}

LevelMakerResult getLevelMaker(RandomGen& random, ContentFactory* contentFactory, const vector<string>& zLevelGroups,
    int depth, TribeId tribe, StairKey stairKey) {
  auto& allLevels = contentFactory->zLevels;
  auto& resources = contentFactory->resources;
  vector<ZLevelInfo> levels;
  for (auto& group : zLevelGroups) 
    levels.append(allLevels.at(group));
  auto zLevel = *chooseZLevel(random, levels, depth);
  auto res = *chooseResourceCounts(random, resources, depth);
  return getLevelMaker(zLevel, res, tribe, stairKey, contentFactory);
}
