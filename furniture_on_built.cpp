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
#include "known_tiles.h"

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

static void addStairs(Position pos, Level* targetLevel, FurnitureType stairsType) {
  Position landing(pos.getCoord(), targetLevel);
  if (!landing.canEnterEmpty({MovementTrait::WALK}))
    landing.addFurniture(pos.getGame()->getContentFactory()->furniture.getFurniture(FurnitureType("FLOOR"),
        TribeId::getMonster()));
  landing.addFurniture(pos.getGame()->getContentFactory()->furniture.getFurniture(stairsType, TribeId::getMonster()));
  auto stairKey = StairKey::getNew();
  landing.setLandingLink(stairKey);
  pos.setLandingLink(stairKey);
  pos.getModel()->calculateStairNavigation();
  auto collective = pos.getGame()->getPlayerCollective();
  for (auto v : concat({landing}, landing.neighbors8())) {
    collective->addKnownTile(v);
    pos.getGame()->getPlayerControl()->addToMemory(v);
  }
  collective->claimSquare(landing);
  for (auto pos : targetLevel->getAllPositions())
    if (auto f = pos.getFurniture(FurnitureLayer::MIDDLE))
      if (f->isClearFogOfWar())
        pos.getGame()->getPlayerControl()->addToMemory(pos);
}

optional<Position> getSecondPart(Position pos, const Furniture* f, FurnitureOnBuilt type) {
  auto getOtherStairs = [] (Position pos, int dir) {
    int index = *pos.getModel()->getMainLevelDepth(pos.getLevel());
    index = pos.getModel()->getMainLevelsDepth().clamp(index + dir);
    return Position(pos.getCoord(), pos.getModel()->getMainLevel(index));
  };
  switch (type) {
    case FurnitureOnBuilt::UP_STAIRS:
      if (pos.getLandingLink())
        return getOtherStairs(pos, -1);
      break;
    case FurnitureOnBuilt::DOWN_STAIRS:
      if (pos.getLandingLink())
        return getOtherStairs(pos, 1);
      break;
    default:
      break;
  }
  return none;
}

static Color getPortalColor(int index) {
  CHECK(index >= 0);
  index += 1 + 2 * (index / 6);
  return Color(255 * (index % 2), 255 * ((index / 2) % 2), 255 * ((index / 4) % 2));
}

void handleOnBuilt(Position pos, Furniture* f, FurnitureOnBuilt type) {
  switch (type) {
    case FurnitureOnBuilt::UP_STAIRS: {
      int levelIndex = *pos.getModel()->getMainLevelDepth(pos.getLevel());
      Level* targetLevel = nullptr;
      if (levelIndex == pos.getModel()->getMainLevelsDepth().getStart()) {
        auto levelSize = pos.getLevel()->getBounds().getSize();
        auto newLevel = tryBuilding(20,
            [&]{
              auto contentFactory = pos.getGame()->getContentFactory();
              auto maker = getUpLevel(Random, contentFactory, -levelIndex + 1, pos);
              auto level = pos.getModel()->buildUpLevel(contentFactory,
                  LevelBuilder(Random, contentFactory, levelSize.x, levelSize.y, true), std::move(maker.maker));
              return ZLevelResult{ level, maker.enemy ? maker.enemy->buildCollective(contentFactory) : nullptr};
            },
            "z-level " + toString(levelIndex));
        if (newLevel.collective)
          pos.getModel()->addCollective(std::move(newLevel.collective));
        targetLevel = newLevel.level;
      } else
        targetLevel = pos.getModel()->getMainLevel(levelIndex - 1);
      addStairs(pos, targetLevel, FurnitureType("DOWN_STAIRS"));
      break;
    }
    case FurnitureOnBuilt::DOWN_STAIRS: {
      int levelIndex = *pos.getModel()->getMainLevelDepth(pos.getLevel());
      Level* targetLevel = nullptr;
      if (levelIndex == pos.getModel()->getMainLevelsDepth().getEnd() - 1) {
        auto levelSize = pos.getLevel()->getBounds().getSize();
        auto newLevel = tryBuilding(20,
            [&]{
              auto contentFactory = pos.getGame()->getContentFactory();
              auto maker = getLevelMaker(Random, contentFactory, pos.getGame()->zLevelGroups,
                  levelIndex + 1, pos.getGame()->getPlayerCollective()->getTribeId(), levelSize);
              auto level = pos.getModel()->buildMainLevel(contentFactory,
                  LevelBuilder(Random, contentFactory, levelSize.x, levelSize.y, true),
                      std::move(maker.maker));
              return ZLevelResult{ level, maker.enemy ? maker.enemy->buildCollective(contentFactory) : nullptr};
            },
            "z-level " + toString(levelIndex));
        if (newLevel.collective)
          pos.getModel()->addCollective(std::move(newLevel.collective));
        targetLevel = newLevel.level;
      } else
        targetLevel = pos.getModel()->getMainLevel(levelIndex + 1);
      addStairs(pos, targetLevel, FurnitureType("UP_STAIRS"));
      break;
    }
    case FurnitureOnBuilt::PORTAL:
      pos.registerPortal();
      f->getViewObject()->setColorVariant(Color::WHITE);
      if (auto otherPos = pos.getOtherPortal())
        for (auto otherPortal : otherPos->modFurniture())
          if (otherPortal->hasUsageType(BuiltinUsageId::PORTAL)) {
            auto color = getPortalColor(*pos.getPortalIndex());
            otherPortal->getViewObject()->setColorVariant(color);
            f->getViewObject()->setColorVariant(color);
            pos.setNeedsRenderAndMemoryUpdate(true);
            otherPos->setNeedsRenderAndMemoryUpdate(true);
          }
      break;
    case FurnitureOnBuilt::SET_ON_FIRE:
      pos.globalMessage(PlayerMessage("A tower of flame errupts from the floor!", MessagePriority::HIGH));
      f->fireDamage(pos, false);
      break;
  }
}
