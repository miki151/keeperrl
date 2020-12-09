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
#include "game_event.h"
#include "view.h"
#include "sound.h"

Spectator::Spectator(WLevel l, View* view) : level(l), view(view) {
}

const MapMemory& Spectator::getMemory() const {
  return MapMemory::empty();
}

void Spectator::getViewIndex(Vec2 pos, ViewIndex& index) const {
  Position position(pos, level);
  position.getViewIndex(index, nullptr);
  if (const Creature* c = position.getCreature())
    index.insert(c->getViewObject());
}

void Spectator::refreshGameInfo(GameInfo& gameInfo)  const {
  gameInfo.infoType = GameInfo::InfoType::SPECTATOR;
}

Vec2 Spectator::getScrollCoord() const {
  return level->getBounds().middle();
}

Level* Spectator::getCreatureViewLevel() const {
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

void Spectator::onEvent(const GameEvent& event) {
  using namespace EventInfo;
  event.visit<void>(
      [&](const Projectile& info) {
        if (view) {
          view->animateObject(info.begin.getCoord(), info.end.getCoord(), info.viewId, info.fx);
          if (info.sound)
            view->addSound(*info.sound);
        }
      },
      [&](const FX& info) {
        if (view)
          view->animation(FXSpawnInfo(info.fx, info.position.getCoord(), info.direction.value_or(Vec2(0, 0))));
      },
      [&](const auto&) {}
  );
}
