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
#include "name_generator.h"

struct ZLevelResult {
  Level* level;
  vector<PCollective> collective;
};

template <typename BuildFun>
static ZLevelResult tryBuilding(int numTries, BuildFun buildFun, const string& name) {
  for (int i : Range(numTries)) {
    try {
      return buildFun(true);
    } catch (LevelGenException) {
      INFO << "Retrying level gen";
    }
  }
  USER_INFO << "The game had trouble generating " << name << " and will replace it with a generic level";
  for (int i : Range(numTries)) {
    try {
      return buildFun(false);
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
  collective->claimSquare(landing, true);
  for (auto pos : targetLevel->getAllPositions())
    if (auto f = pos.getFurniture(FurnitureLayer::MIDDLE))
      if (f->isClearFogOfWar())
        pos.getGame()->getPlayerControl()->addToMemory(pos);
}

optional<int> getStairDirection(FurnitureOnBuilt type) {
  switch (type) {
    case FurnitureOnBuilt::UP_STAIRS:
      return -1;
    case FurnitureOnBuilt::DOWN_STAIRS:
      return 1;
    default:
      return none;
  }
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
        auto newLevel = tryBuilding(50,
            [&](bool withEnemies) {
              auto contentFactory = pos.getGame()->getContentFactory();
              auto maker = getUpLevel(Random, contentFactory, -levelIndex + 1, pos, withEnemies);
              auto level = pos.getModel()->buildUpLevel(contentFactory,
                  LevelBuilder(Random, contentFactory, levelSize.x, levelSize.y, true), std::move(maker.maker));
              return ZLevelResult{ level, maker.enemies.transform([&](auto& e) { return e.buildCollective(contentFactory); })};
            },
            "z-level " + toString(levelIndex));
        for (auto& c : newLevel.collective)
          pos.getModel()->addCollective(std::move(c));
        targetLevel = newLevel.level;
      } else
        targetLevel = pos.getModel()->getMainLevel(levelIndex - 1);
      addStairs(pos, targetLevel, f->getOtherStairs().value_or(FurnitureType("DOWN_STAIRS")));
      break;
    }
    case FurnitureOnBuilt::DOWN_STAIRS: {
      int levelIndex = *pos.getModel()->getMainLevelDepth(pos.getLevel());
      Level* targetLevel = nullptr;
      if (levelIndex == pos.getModel()->getMainLevelsDepth().getEnd() - 1) {
        auto levelSize = pos.getLevel()->getBounds().getSize();
        auto newLevel = tryBuilding(50,
            [&](bool withEnemies) {
              auto contentFactory = pos.getGame()->getContentFactory();
              auto maker = getLevelMaker(Random, contentFactory, pos.getGame()->zLevelGroups,
                  withEnemies ? levelIndex + 1 : 1, pos.getGame()->getPlayerCollective()->getTribeId(), levelSize,
                  pos.getGame()->getEnemyAggressionLevel());
              auto level = pos.getModel()->buildMainLevel(contentFactory,
                  LevelBuilder(Random, contentFactory, levelSize.x, levelSize.y, true),
                      std::move(maker.maker));
              return ZLevelResult{ level, maker.enemies.transform([&](auto& e) { return e.buildCollective(contentFactory); })};
            },
            "z-level " + toString(levelIndex));
        for (auto& c : newLevel.collective)
          pos.getModel()->addCollective(std::move(c));
        targetLevel = newLevel.level;
        for (auto c : newLevel.level->getAllCreatures())
          c->setCombatExperience(getZLevelCombatExp(levelIndex));
      } else
        targetLevel = pos.getModel()->getMainLevel(levelIndex + 1);
      addStairs(pos, targetLevel, f->getOtherStairs().value_or(FurnitureType("UP_STAIRS")));
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
      pos.globalMessage(PlayerMessage(TStringId("FIRE_SPELL_MESSAGE"), MessagePriority::HIGH));
      f->fireDamage(pos, false);
      break;
    case FurnitureOnBuilt::SAINT_STATUE: {
      if (auto game = pos.getGame()) {
        auto fMod = pos.modFurniture(f->getLayer());
        auto namegen = game->getContentFactory()->getCreatures().getNameGenerator();
        fMod->setName(TSentence("STATUE_OF_SAINT", fMod->getName(), TString(namegen->getNext(
            NameGeneratorId(Random.choose(makeVec<const char*>("FIRST_MALE", "FIRST_FEMALE")))))));
      }
      break;
    }
  }
}

void handleBeforeRemoved(Position pos, const Furniture* f, FurnitureOnBuilt type) {
  switch (type) {
    case FurnitureOnBuilt::UP_STAIRS:
    case FurnitureOnBuilt::DOWN_STAIRS:
      pos.removeLandingLink();
      if (auto otherPos = f->getSecondPart(pos))
        otherPos->removeLandingLink();
      break;
    case FurnitureOnBuilt::PORTAL:
      if (auto otherPos = pos.getOtherPortal())
        for (auto otherPortal : otherPos->modFurniture())
          if (otherPortal->hasUsageType(BuiltinUsageId::PORTAL)) {
            otherPortal->getViewObject()->setColorVariant(Color::WHITE);
            otherPos->setNeedsRenderAndMemoryUpdate(true);
          }
      pos.removePortal();
      break;
    default:
      break;
  }
}
