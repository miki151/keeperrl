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
#include "creature_group.h"
#include "game_config.h"
#include "content_factory.h"
#include "zlevel.h"

struct ZLevelResult {
  Level* level;
  PCollective collective;
};

template <typename BuildFun>
static ZLevelResult tryBuilding(int numTries, BuildFun buildFun, const string& name) {
  for (int i : Range(numTries)) {
    try {
      return buildFun();
    } catch (LevelGenException) {
      INFO << "Retrying level gen";
    }
  }
  FATAL << "Couldn't generate a level: " << name;
  fail();
}

void handleOnBuilt(Position pos, Furniture* f, FurnitureOnBuilt type) {
  switch (type) {
    case FurnitureOnBuilt::DOWN_STAIRS: {
      auto levels = pos.getModel()->getMainLevels();
      int levelIndex = *levels.findElement(pos.getLevel());
      if (levelIndex == levels.size() - 1) {
        auto stairKey = StairKey::getNew();
        auto newLevel = tryBuilding(20,
            [&]{
              auto contentFactory = pos.getGame()->getContentFactory();
              auto maker = getLevelMaker(Random, contentFactory, pos.getGame()->zLevelGroups,
                  levelIndex + 1, pos.getGame()->getPlayerCollective()->getTribeId(), stairKey);
              auto level = pos.getModel()->buildMainLevel(contentFactory,
                  LevelBuilder(Random, contentFactory, maker.levelWidth, maker.levelWidth, true),
                      std::move(maker.maker));
              return ZLevelResult{ level, maker.enemy ? maker.enemy->buildCollective(contentFactory) : nullptr};
            },
            "z-level " + toString(levelIndex));
        if (newLevel.collective)
          pos.getModel()->addCollective(std::move(newLevel.collective));
        Position landing = newLevel.level->getLandingSquares(stairKey).getOnlyElement();
        landing.addFurniture(pos.getGame()->getContentFactory()->furniture.getFurniture(
            FurnitureType("UP_STAIRS"), TribeId::getMonster()));
        pos.setLandingLink(stairKey);
        pos.getModel()->calculateStairNavigation();
        // Add known tiles around the stairs so it's possible to build bridge on water/lava levels.
        for (auto v : landing.getRectangle(Rectangle::centered(1)))
          pos.getGame()->getPlayerCollective()->addKnownTile(v);
        pos.getGame()->getPlayerControl()->addToMemory(landing);
        for (auto v : landing.neighbors8())
          pos.getGame()->getPlayerControl()->addToMemory(v);
        for (auto pos : newLevel.level->getAllPositions())
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
