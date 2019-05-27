#include "stdafx.h"
#include "furniture_on_built.h"
#include "position.h"
#include "model.h"
#include "level_builder.h"
#include "level_maker.h"
#include "furniture_type.h"
#include "furniture_factory.h"
#include "creature.h"
#include "game.h"
#include "collective.h"
#include "player_control.h"
#include "enemy_factory.h"
#include "settlement_info.h"
#include "enemy_info.h"
#include "collective_builder.h"
#include "creature_group.h"
#include "z_level_info.h"
#include "game_config.h"
#include "resource_counts.h"
#include "content_factory.h"

static SettlementInfo getEnemy(EnemyId id, const ContentFactory* contentFactory) {
  auto enemy = EnemyFactory(Random, nullptr, contentFactory->enemies).get(id);
  enemy.settlement.collective = new CollectiveBuilder(enemy.config, enemy.settlement.tribe);
  return enemy.settlement;
}

static PLevelMaker getLevelMaker(const ZLevelType& level, ResourceCounts resources, int width, TribeId tribe,
    StairKey stairKey, const ContentFactory* contentFactory) {
  return level.visit(
      [&](const WaterZLevel& level) {
        return LevelMaker::getWaterZLevel(Random, level.waterType, width, level.creatures, stairKey);
      },
      [&](const FullZLevel& level) {
        return LevelMaker::getFullZLevel(Random, level.enemy.map([&](auto id) { return getEnemy(id, contentFactory); }), resources,
            width, tribe, stairKey);
      });
}

static optional<ZLevelType> chooseZLevel(RandomGen& random, const vector<ZLevelInfo>& levels, int depth) {
  vector<ZLevelType> available;
  for (auto& l : levels)
    if (l.minDepth.value_or(-100) <= depth && l.maxDepth.value_or(1000000) >= depth)
      available.push_back(l.type);
  if (available.empty())
    return none;
  return random.choose(available);
}

static PLevelMaker getLevelMaker(RandomGen& random, const ContentFactory* contentFactory, TribeAlignment alignment,
    int depth, int width, TribeId tribe, StairKey stairKey) {
  auto& allLevels = contentFactory->zLevels;
  auto& resources = contentFactory->resources;
  vector<ZLevelInfo> levels = concat<ZLevelInfo>({allLevels[0], allLevels[1 + int(alignment)]});
  auto zLevel = *chooseZLevel(random, levels, depth);
  auto res = *chooseResourceCounts(random, resources, depth);
  return getLevelMaker(zLevel, res, width, tribe, stairKey, contentFactory);
}

static void removeOldStairs(Level* level, StairKey stairKey) {
  for (auto pos : level->getAllPositions())
    if (pos.getLandingLink() == stairKey) {
      pos.removeLandingLink();
      pos.removeFurniture(FurnitureLayer::MIDDLE);
      pos.getGame()->getPlayerControl()->addToMemory(pos);
    }
}

template <typename BuildFun>
static WLevel tryBuilding(int numTries, BuildFun buildFun, const string& name) {
  for (int i : Range(numTries)) {
    try {
      return buildFun();
    } catch (LevelGenException) {
      INFO << "Retrying level gen";
    }
  }
  FATAL << "Couldn't generate a level: " << name;
  return nullptr;
}

void handleOnBuilt(Position pos, Furniture* f, Creature* c, FurnitureOnBuilt type) {
  switch (type) {
    case FurnitureOnBuilt::DOWN_STAIRS: {
      auto levels = pos.getModel()->getMainLevels();
      int levelIndex = *levels.findElement(pos.getLevel());
      if (levelIndex == levels.size() - 1) {
        int width = 140;
        auto stairKey = StairKey::getNew();
        auto newLevel = tryBuilding(20, [&]{ return pos.getModel()->buildMainLevel(
            LevelBuilder(Random, pos.getGame()->getContentFactory(), width, width, "", true),
            getLevelMaker(Random, pos.getGame()->getContentFactory(),
                pos.getGame()->getPlayerControl()->getTribeAlignment(),
                levelIndex + 1, width, c->getTribeId(), stairKey)); }, "z-level " + toString(levelIndex));
        Position landing = newLevel->getLandingSquares(stairKey).getOnlyElement();
        landing.addFurniture(pos.getGame()->getContentFactory()->furniture.getFurniture(
            FurnitureType("UP_STAIRS"), TribeId::getMonster()));
        pos.setLandingLink(stairKey);
        pos.getModel()->calculateStairNavigation();
        pos.getGame()->getPlayerCollective()->addKnownTile(landing);
        pos.getGame()->getPlayerControl()->addToMemory(landing);
        for (auto v : landing.neighbors8())
          pos.getGame()->getPlayerControl()->addToMemory(v);
        for (auto pos : newLevel->getAllPositions())
          if (auto f = pos.getFurniture(FurnitureLayer::MIDDLE))
            if (f->isClearFogOfWar())
              pos.getGame()->getPlayerControl()->addToMemory(pos);
      } else {
        auto nextLevel = levels[levelIndex + 1];
        auto oldStairsPos = pos.getModel()->getStairs(pos.getLevel(), nextLevel);
        pos.setLandingLink(*oldStairsPos->getLandingLink());
        oldStairsPos->removeLandingLink();
        oldStairsPos->removeFurniture(FurnitureLayer::MIDDLE);
        pos.getGame()->getPlayerControl()->addToMemory(*oldStairsPos);
      }
      break;
    }
    case FurnitureOnBuilt::SET_ON_FIRE:
      pos.globalMessage(PlayerMessage("A tower of flame errupts from the floor!", MessagePriority::HIGH));
      f->fireDamage(pos, false);
      break;
  }
}
