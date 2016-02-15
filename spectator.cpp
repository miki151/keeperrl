#include "stdafx.h"
#include "spectator.h"
#include "level.h"
#include "map_memory.h"
#include "square.h"
#include "model.h"
#include "game_info.h"
#include "creature.h"
#include "view_index.h"

Spectator::Spectator(const Level* l) : level(l) {
}

const MapMemory& Spectator::getMemory() const {
  return MapMemory::empty();
}

void Spectator::getViewIndex(Vec2 pos, ViewIndex& index) const {
  Position position = getLevel()->getPosition(pos);
  position.getViewIndex(index, TribeId::PEACEFUL);
  if (const Creature* c = position.getCreature())
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

const Level* Spectator::getLevel() const {
  return level;
}

double Spectator::getTime() const {
  return level->getModel()->getTime();
}

vector<Vec2> Spectator::getVisibleEnemies() const {
  return {};
}

bool Spectator::isPlayerView() const {
  return false;
}
