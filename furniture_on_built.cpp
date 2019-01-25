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

static SettlementInfo getEnemy(EnemyId id) {
  auto enemy = EnemyFactory(Random).get(id);
  enemy.settlement.collective = new CollectiveBuilder(enemy.config, enemy.settlement.tribe);
  return enemy.settlement;
}

static optional<SettlementInfo> getSettlement(int depth) {
  if (depth == 0)
    return none;
  if (depth == 1)
    return getEnemy(EnemyId::KOBOLD_CAVE);
  if (Random.roll(3))
    return getEnemy(EnemyId::DWARF_CAVE);
  return none;
}

static PLevelMaker getLevelMaker(int depth, int width, TribeId tribe) {
  if (depth <= 4)
    return LevelMaker::getFullZLevel(Random, getSettlement(depth), width, tribe, StairKey::getNew());
  return LevelMaker::getWaterZLevel(Random, FurnitureType::MAGMA, width,
      CreatureFactory::lavaCreatures(TribeId::getMonster()), StairKey::getNew());
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
      WLevel level = nullptr;
      int levelIndex = *levels.findElement(pos.getLevel());
      if (levelIndex == levels.size() - 1) {
        int width = 140;
        level = pos.getModel()->buildMainLevel(
            LevelBuilder(Random, width, width, "", true), getLevelMaker(levelIndex, width, c->getTribeId()));
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
