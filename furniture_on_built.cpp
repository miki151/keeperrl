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

static int getDepth(Position pos) {
  unordered_set<Level*> visited { pos.getLevel() };
  function<optional<int>(Position)> search = [&](Position pos) -> optional<int> {
    if (pos.getLevel() == pos.getModel()->getTopLevel())
      return 0;
    for (auto& key : pos.getLevel()->getAllStairKeys()) {
      auto next = pos.getLandingAtNextLevel(key)[0];
      if (!visited.count(next.getLevel())) {
        visited.insert(next.getLevel());
        if (auto res = search(next))
          return 1 + *res;
      }
    }
    return none;
  };
  return *search(pos);
}

static optional<SettlementInfo> getSettlement(int depth) {
  auto enemy = EnemyFactory(Random).get(EnemyId::RED_DRAGON);
  enemy.settlement.collective = new CollectiveBuilder(enemy.config, enemy.settlement.tribe);
  return enemy.settlement;

}

static void removeOldStairs(Level* level, StairKey stairKey) {
  for (auto pos : level->getAllPositions())
    if (pos.getLandingLink() == stairKey) {
      pos.removeLandingLink();
      pos.removeFurniture(FurnitureLayer::MIDDLE);
    }
}

void handleOnBuilt(Position pos, WCreature c, FurnitureOnBuilt type) {
  switch (type) {
    case FurnitureOnBuilt::DOWN_STAIRS:
      auto levels = pos.getModel()->getMainLevels();
      WLevel level;
      int levelIndex = *levels.findElement(pos.getLevel());
      if (levelIndex == levels.size() - 1) {
        level = pos.getModel()->buildMainLevel(
            LevelBuilder(Random, 30, 30, "", true),
            LevelMaker::getZLevel(Random, getSettlement(getDepth(pos) + 1), 30, c->getTribeId()));
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
