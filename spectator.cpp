#include "stdafx.h"
#include "spectator.h"
#include "level.h"
#include "map_memory.h"
#include "square.h"
#include "model.h"
#include "game_info.h"
#include "creature.h"
#include "view_index.h"

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

optional<CreatureView::MovementInfo> Spectator::getMovementInfo() const {
  return none;
}

WLevel Spectator::getLevel() const {
  return level;
}

double Spectator::getLocalTime() const {
  return level->getModel()->getLocalTime();
}

vector<Vec2> Spectator::getVisibleEnemies() const {
  return {};
}

bool Spectator::isPlayerView() const {
  return false;
}

vector<Vec2> Spectator::getUnknownLocations(WConstLevel) const {
  return {};
}

