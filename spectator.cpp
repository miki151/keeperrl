#include "stdafx.h"
#include "spectator.h"
#include "level.h"
#include "map_memory.h"
#include "square.h"
#include "model.h"
#include "game_info.h"
#include "creature.h"
#include "view_index.h"
#include "game.h"
#include "view_object.h"

Spectator::Spectator(WLevel l) : level(l) {
}

const MapMemory& Spectator::getMemory() const {
  return MapMemory::empty();
}

void Spectator::getViewIndex(Vec2 pos, ViewIndex& index) const {
  Position position(pos, getLevel());
  position.getViewIndex(index, nullptr);
  if (WConstCreature c = position.getCreature())
    index.insert(c->getViewObject());
}

void Spectator::refreshGameInfo(GameInfo& gameInfo)  const {
  gameInfo.infoType = GameInfo::InfoType::SPECTATOR;
}

Vec2 Spectator::getPosition() const {
  return level->getBounds().middle();
}

WLevel Spectator::getLevel() const {
  return level;
}

double Spectator::getAnimationTime() const {
  return level->getModel()->getLocalTimeDouble();
}

vector<Vec2> Spectator::getVisibleEnemies() const {
  return {};
}

Spectator::CenterType Spectator::getCenterType() const {
  return CenterType::NONE;
}

const vector<Vec2>& Spectator::getUnknownLocations(WConstLevel) const {
  static vector<Vec2> empty;
  return empty;
}

