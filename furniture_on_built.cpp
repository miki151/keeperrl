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

static SettlementInfo getEnemy(EnemyId id) {
  auto enemy = EnemyFactory(Random, nullptr).get(id);
  enemy.settlement.collective = new CollectiveBuilder(enemy.config, enemy.settlement.tribe);
  return enemy.settlement;
}

static PLevelMaker getLevelMaker(const ZLevelType& level, int width, TribeId tribe) {
  return level.visit(
      [&](const WaterZLevel& level) {
        return LevelMaker::getWaterZLevel(Random, level.waterType, width, level.creatures, StairKey::getNew());
      },
      [&](const FullZLevel& level) {
        return LevelMaker::getFullZLevel(Random, level.enemy.map([](auto id) { return getEnemy(id); }),
            width, tribe, StairKey::getNew());
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

static PLevelMaker getLevelMaker(RandomGen& random, const GameConfig* config, TribeAlignment alignment,
    int depth, int width, TribeId tribe) {
  array<vector<ZLevelInfo>, 3> allLevels;
  while (1) {
    if (auto res = config->readObject(allLevels, GameConfigId::Z_LEVELS))
      USER_INFO << *res;
    vector<ZLevelInfo> levels = concat<ZLevelInfo>({allLevels[0], allLevels[1 + int(alignment)]});
    if (auto res = chooseZLevel(random, levels, depth))
      return getLevelMaker(*res, width, tribe);
    else
      USER_INFO << "No z-level found for depth " << depth << ". Please fix z-level config.";
  }
  fail();
}

static void removeOldStairs(Level* level, StairKey stairKey) {
  for (auto pos : level->getAllPositions())
    if (pos.getLandingLink() == stairKey) {
      pos.removeLandingLink();
      pos.removeFurniture(FurnitureLayer::MIDDLE);
    }
}

void handleOnBuilt(Position pos, Creature* c, FurnitureOnBuilt type) {
  switch (type) {
    case FurnitureOnBuilt::DOWN_STAIRS:
      auto levels = pos.getModel()->getMainLevels();
      WLevel level = nullptr;
      int levelIndex = *levels.findElement(pos.getLevel());
      if (levelIndex == levels.size() - 1) {
        int width = 140;
        level = pos.getModel()->buildMainLevel(
            LevelBuilder(Random, pos.getGame()->getCreatureFactory(), width, width, "", true),
            getLevelMaker(Random, pos.getGame()->getGameConfig(),
                pos.getGame()->getPlayerControl()->getKeeperCreatureInfo().tribeAlignment,
                levelIndex + 1, width, c->getTribeId()));
      } else {
        level = levels[levelIndex + 1];
      }
      Position landing = [&]() -> Position {
        for (auto& pos : level->getAllPositions())
          if (pos.getLandingLink())
            return pos;
        FATAL << "No landing position found in subterranean level";
        fail();
      }();
      landing.addFurniture(FurnitureFactory::get(FurnitureType::UP_STAIRS, c->getTribeId()));
      auto stairKey = *landing.getLandingLink();
      removeOldStairs(pos.getLevel(), stairKey);
      pos.setLandingLink(stairKey);
      pos.getModel()->calculateStairNavigation();
      pos.getGame()->getPlayerCollective()->claimSquare(landing);
      for (auto v : landing.neighbors8())
        pos.getGame()->getPlayerControl()->addToMemory(v);
      for (auto pos : level->getAllPositions())
        if (auto f = pos.getFurniture(FurnitureLayer::MIDDLE))
          if (f->isClearFogOfWar())
            pos.getGame()->getPlayerControl()->addToMemory(pos);
      break;
  }
}
