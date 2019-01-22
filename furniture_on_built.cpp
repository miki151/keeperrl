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

void handleOnBuilt(Position pos, WCreature c, FurnitureOnBuilt type) {
  switch (type) {
    case FurnitureOnBuilt::DOWN_STAIRS:
      auto level = pos.getModel()->buildLevel(
          LevelBuilder(Random, 30, 30, "", true),
          LevelMaker::emptyLevel(FurnitureType::MOUNTAIN2));
      Position landing(level->getBounds().middle(), level);
      auto destructedType = landing.getFurniture(FurnitureLayer::MIDDLE)->getType();
      landing.removeFurniture(landing.getFurniture(FurnitureLayer::MIDDLE),
          FurnitureFactory::get(FurnitureType::UP_STAIRS, c->getTribeId()));
      auto stairKey = StairKey::getNew();
      landing.setLandingLink(stairKey);
      pos.setLandingLink(stairKey);
      pos.getModel()->calculateStairNavigation();
      pos.getGame()->getPlayerCollective()->onDestructed(landing, destructedType, DestroyAction::Type::DIG);
      pos.getGame()->getPlayerCollective()->claimSquare(landing);
      break;
  }
}
