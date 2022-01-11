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
            none
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
            std::move(enemy)
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
            std::move(enemy)
        };
      });
}

LevelMakerResult getLevelMaker(RandomGen& random, ContentFactory* contentFactory, const vector<string>& zLevelGroups,
    int depth, TribeId tribe, Vec2 size) {
  auto& allLevels = contentFactory->zLevels;
  auto& resources = contentFactory->resources;
  vector<ZLevelInfo> levels;
  for (auto& group : zLevelGroups) 
    levels.append(allLevels.at(group));
  auto zLevel = *chooseZLevel(random, levels, depth);
  auto res = *chooseResourceCounts(random, resources, depth);
  return getLevelMaker(zLevel, res, tribe, contentFactory, size);
}


LevelMakerResult getUpLevel(RandomGen& random, ContentFactory* contentFactory,
    int depth, Position pos) {
  auto& biomeInfo = contentFactory->biomeInfo.at(pos.getModel()->getBiomeId());
  optional<EnemyInfo> enemy;
  if (depth == biomeInfo.mountains.numMountainLevels && biomeInfo.mountainTopEnemy) {
    enemy = getEnemy(*biomeInfo.mountainTopEnemy, contentFactory);
    enemy->settlement.collective = new CollectiveBuilder(enemy->config, enemy->settlement.tribe);
  }
  auto maker = LevelMaker::upLevel(pos, biomeInfo, enemy ? &enemy->settlement : nullptr);
  auto size = pos.getModel()->getGroundLevel()->getBounds().getSize();
  return LevelMakerResult { std::move(maker), std::move(enemy)};
}