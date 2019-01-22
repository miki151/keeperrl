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

void handleOnBuilt(Position pos, WCreature c, FurnitureOnBuilt type) {
  switch (type) {
    case FurnitureOnBuilt::DOWN_STAIRS:
      auto level = pos.getModel()->buildLevel(
          LevelBuilder(Random, 30, 30, "", true),
          LevelMaker::getZLevel(Random, getDepth(pos) + 1, 30, c->getTribeId()));
      Position landing = [&] {
        for (auto& pos : level->getAllPositions())
          if (pos.getLandingLink())
            return pos;
        FATAL << "No landing position found in subterranean level";
        fail();
      }();
      auto destructedType = landing.getFurniture(FurnitureLayer::MIDDLE)->getType();
      landing.removeFurniture(landing.getFurniture(FurnitureLayer::MIDDLE),
          FurnitureFactory::get(FurnitureType::UP_STAIRS, c->getTribeId()));
      auto stairKey = *landing.getLandingLink();
      pos.setLandingLink(stairKey);
      pos.getModel()->calculateStairNavigation();
      pos.getGame()->getPlayerCollective()->onDestructed(landing, destructedType, DestroyAction::Type::DIG);
      pos.getGame()->getPlayerCollective()->claimSquare(landing);
      for (auto pos : level->getAllPositions())
        if (auto f = pos.getFurniture(FurnitureLayer::MIDDLE))
          if (f->isClearFogOfWar())
            pos.getGame()->getPlayerControl()->addToMemory(pos);
      break;
  }
}
