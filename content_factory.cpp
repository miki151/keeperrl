#include "stdafx.h"
#include "content_factory.h"
#include "name_generator.h"
#include "game_config.h"

SERIALIZE_DEF(ContentFactory, creatures, furniture, resources, zLevels, tilePaths)

SERIALIZATION_CONSTRUCTOR_IMPL(ContentFactory)

static bool isZLevel(const vector<ZLevelInfo>& levels, int depth) {
  for (auto& l : levels)
    if (l.minDepth.value_or(-100) <= depth && l.maxDepth.value_or(1000000) >= depth)
      return true;
  return false;
}

bool areResourceCounts(const vector<ResourceDistribution>& resources, int depth) {
  for (auto& l : resources)
    if (l.minDepth.value_or(-100) <= depth && l.maxDepth.value_or(1000000) >= depth)
      return true;
  return false;
}

ContentFactory::ContentFactory(NameGenerator nameGenerator, const GameConfig* config, TilePaths tilePaths)
    : creatures(std::move(nameGenerator), config), furniture(config), tilePaths(std::move(tilePaths)) {
  while (1) {
    if (auto res = config->readObject(zLevels, GameConfigId::Z_LEVELS)) {
      USER_INFO << *res;
      continue;
    }
    if (auto res = config->readObject(resources, GameConfigId::RESOURCE_COUNTS)) {
      USER_INFO << *res;
      continue;
    }
    for (int alignment = 0; alignment < 2; ++alignment) {
      vector<ZLevelInfo> levels = concat<ZLevelInfo>({zLevels[0], zLevels[1 + alignment]});
      for (int depth = 0; depth < 1000; ++depth) {
        if (!isZLevel(levels, depth)) {
          USER_INFO << "No z-level found for depth " << depth << ". Please fix z-level config.";
          continue;
        }
        if (!areResourceCounts(resources, depth)) {
          USER_INFO << "No resource distribution found for depth " << depth << ". Please fix resources config.";
          continue;
        }
      }
    }
    break;
  }
}

void ContentFactory::merge(ContentFactory f) {
  creatures.merge(std::move(f.creatures));
  furniture.merge(std::move(f.furniture));
  tilePaths.merge(std::move(f.tilePaths));
}
