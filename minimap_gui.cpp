/* Copyright (C) 2013-2014 Michal Brzozowski (rusolis@poczta.fm)

   This file is part of KeeperRL.

   KeeperRL is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   KeeperRL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

#include "stdafx.h"
#include "minimap_gui.h"
#include "level.h"
#include "creature_view.h"
#include "tile.h"
#include "renderer.h"
#include "map_memory.h"
#include "view_index.h"
#include "sdl.h"
#include "view_object.h"
#include "tileset.h"
#include "mouse_button_id.h"

void MinimapGui::render(Renderer& renderer) {
  auto target = getBounds();
  if (!mapBufferTex)
    mapBufferTex.emplace(mapBuffer);
  else if (auto error = mapBufferTex->loadFromMaybe(mapBuffer))
    FATAL << "Failed to render minimap, error: " << toString(*error);
  renderer.drawImage(target, info.bounds, *mapBufferTex);
  Vec2 topLeft = target.topLeft();
  double scale = min(double(target.width()) / info.bounds.width(),
      double(target.width()) / info.bounds.height());
  for (Vec2 v : info.roads) {
    Vec2 rrad(1, 1);
    Vec2 pos = topLeft + (v - info.bounds.topLeft()) * scale;
    if (pos.inRectangle(target.minusMargin(rrad.x)))
      renderer.drawFilledRectangle(Rectangle(pos - rrad, pos + rrad), Color::BROWN);
  }
  Vec2 rad(3, 3);
  Vec2 player = topLeft + (info.player - info.bounds.topLeft()) * scale;
  if (player.inRectangle(target.minusMargin(rad.x)))
    renderer.drawFilledRectangle(Rectangle(player - rad, player + rad), Color::BLUE);
  for (Vec2 pos : info.enemies) {
    Vec2 v = (pos - info.bounds.topLeft()) * scale;
    renderer.drawFilledRectangle(Rectangle(topLeft + v - rad, topLeft + v + rad), Color::RED);
  }
  for (auto loc : info.locations) {
    Vec2 v = (loc - info.bounds.topLeft()) * scale;
//    if (loc.text.empty())
      renderer.drawText(Color::LIGHT_GREEN, topLeft + v + Vec2(5, 0), "?", Renderer::CenterType::HOR_VER);
/*    else {
      renderer.drawFilledRectangle(topLeft.x + v.x, topLeft.y + v.y,
          topLeft.x + v.x + renderer.getTextLength(loc.text) + 10, topLeft.y + v.y + 25,
          transparency(Color::BLACK, 130));
      renderer.drawText(Color::WHITE, topLeft.x + v.x + 5, topLeft.y + v.y, loc.text);
    }*/
  }
}

static Vec2 getMapBufferSize() {
  int w = 1;
  int h = 1;
  while (w < Level::getMaxBounds().width())
    w *= 2;
  while (h < Level::getMaxBounds().height())
    h *= 2;
  return Vec2(w, h);
}

MinimapGui::MinimapGui(function<void(Vec2)> f) : clickFun(f) {
  auto size = getMapBufferSize();
  mapBuffer = Texture::createSurface(size.x, size.y);
}

void MinimapGui::clear() {
  currentLevel = nullptr;
  info = MinimapInfo {};
}

bool MinimapGui::onClick(MouseButtonId id, Vec2 v) {
  if (id == MouseButtonId::LEFT && v.inRectangle(getBounds())) {
    clickFun(v);
    return true;
  }
  return false;
}

constexpr auto visibleLayers = { ViewLayer::FLOOR_BACKGROUND, ViewLayer::FLOOR };

void MinimapGui::update(Rectangle bounds, const CreatureView* creature, Renderer& renderer) {
  PROFILE;
  auto level = creature->getCreatureViewLevel();
  info.bounds = bounds;
  info.enemies.clear();
  info.locations.clear();
  const MapMemory& memory = creature->getMemory();
  auto updatePos = [&] (Position pos) {
    if (auto index = memory.getViewIndex(pos))
      for (auto layer : visibleLayers)
        if (index->hasObject(layer)) {
          auto& object = index->getObject(layer);
          Renderer::putPixel(mapBuffer, pos.getCoord(), renderer.getTileSet().getColor(object));
          if (object.hasModifier(ViewObject::Modifier::ROAD))
            info.roads.insert(pos.getCoord());
        }
  };
  if (currentLevel != level) {
    int col = SDL_MapRGBA(mapBuffer->format, 0, 0, 0, 1);
    SDL_FillRect(mapBuffer, nullptr, col);
    info.roads.clear();
    for (Position v : level->getAllPositions())
      updatePos(v);
    currentLevel = level;
  }
  for (Position v : memory.getUpdated(level)) {
    CHECK(v.getCoord().x < mapBuffer->w && v.getCoord().y < mapBuffer->h) << v.getCoord();
    updatePos(v);
  }
  memory.clearUpdated(level);
  info.player = creature->getScrollCoord();
  for (Vec2 pos : creature->getVisibleEnemies())
    if (pos.inRectangle(bounds))
      info.enemies.push_back(pos);
  info.locations = creature->getUnknownLocations(currentLevel);
  /*for (const Location* loc : level->getAllLocations()) {
    bool seen = false;
    for (Position v : loc->getAllSquares())
      if (memory.getViewIndex(v)) {
        seen = true;
        break;
      }
    if (loc->isMarkedAsSurprise() && !seen)
      info.locations.push_back({loc->getMiddle().getCoord(), ""});
    if (loc->getName() && seen) {
      info.locations.push_back({loc->getBottomRight().getCoord(), *loc->getName()});
    }
  }*/
}
