#include "stdafx.h"
#include "spectator.h"
#include "level.h"
#include "map_memory.h"
#include "square.h"
#include "model.h"
#include "game_info.h"
#include "creature.h"

Spectator::Spectator(const Level* l) : level(l) {
}

const MapMemory& Spectator::getMemory() const {
  return MapMemory::empty();
}

void Spectator::getViewIndex(Vec2 pos, ViewIndex& index) const {
  const Square* square = getLevel()->getSafeSquare(pos);
  square->getViewIndex(index, nullptr);
  if (const Creature* c = square->getCreature())
    index.insert(c->getViewObject());
}

void Spectator::refreshGameInfo(GameInfo& gameInfo)  const {
  gameInfo.infoType = GameInfo::InfoType::SPECTATOR;
}

optional<Vec2> Spectator::getPosition(bool force) const {
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

vector<const Creature*> Spectator::getVisibleEnemies() const {
  return {};
}


