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
#include "location.h"
#include "renderer.h"
#include "map_memory.h"
#include "view_index.h"
#include "sdl.h"
#include "view_object.h"

void MinimapGui::renderMap(Renderer& renderer, Rectangle target) {
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
      renderer.drawFilledRectangle(Rectangle(pos - rrad, pos + rrad), colors[ColorId::BROWN]);
  }
  Vec2 rad(3, 3);
  Vec2 player = topLeft + (info.player - info.bounds.topLeft()) * scale;
  if (player.inRectangle(target.minusMargin(rad.x)))
    renderer.drawFilledRectangle(Rectangle(player - rad, player + rad), colors[ColorId::BLUE]);
  for (Vec2 pos : info.enemies) {
    Vec2 v = (pos - info.bounds.topLeft()) * scale;
    renderer.drawFilledRectangle(Rectangle(topLeft + v - rad, topLeft + v + rad), colors[ColorId::RED]);
  }
  for (auto loc : info.locations) {
    Vec2 v = (loc.pos - info.bounds.topLeft()) * scale;
    if (loc.text.empty())
      renderer.drawText(colors[ColorId::LIGHT_GREEN], topLeft.x + v.x + 5, topLeft.y + v.y, "?");
    else {
      renderer.drawFilledRectangle(topLeft.x + v.x, topLeft.y + v.y,
          topLeft.x + v.x + renderer.getTextLength(loc.text) + 10, topLeft.y + v.y + 25,
          transparency(colors[ColorId::BLACK], 130));
      renderer.drawText(colors[ColorId::WHITE], topLeft.x + v.x + 5, topLeft.y + v.y, loc.text);
    }
  }
}

void MinimapGui::render(Renderer& r) {
  renderMap(r, getBounds());
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

MinimapGui::MinimapGui(Renderer& r, function<void()> f) : clickFun(f), renderer(r) {
  auto size = getMapBufferSize();
  mapBuffer = Renderer::createSurface(size.x, size.y);
}

void MinimapGui::clear() {
  currentLevel = nullptr;
  info = MinimapInfo {};
}

bool MinimapGui::onLeftClick(Vec2 v) {
  if (v.inRectangle(getBounds())) {
    clickFun();
    return true;
  }
  return false;
}

void MinimapGui::update(const Level* level, Rectangle bounds, const CreatureView* creature, bool printLocations) {
  info.bounds = bounds;
  info.enemies.clear();
  info.locations.clear();
  const MapMemory& memory = creature->getMemory();
  if (currentLevel != level) {
    int col = SDL_MapRGBA(mapBuffer->format, 0, 0, 0, 1);
    SDL_FillRect(mapBuffer, nullptr, col);
    info.roads.clear();
    for (Position v : level->getAllPositions()) {
      if (memory.getViewIndex(v)) {
        Renderer::putPixel(mapBuffer, v.getCoord(), Tile::getColor(v.getViewObject()));
        if (v.getViewObject().hasModifier(ViewObject::Modifier::ROAD))
          info.roads.insert(v.getCoord());
      }
    }
    currentLevel = level;
  }
  for (Position v : memory.getUpdated(level)) {
    CHECK(v.getCoord().inRectangle(Vec2(mapBuffer->w, mapBuffer->h))) << v.getCoord();
    Renderer::putPixel(mapBuffer, v.getCoord(), Tile::getColor(v.getViewObject()));
    if (v.getViewObject().hasModifier(ViewObject::Modifier::ROAD))
      info.roads.insert(v.getCoord());
  }
  memory.clearUpdated(level);
  info.player = creature->getPosition();
  for (Vec2 pos : creature->getVisibleEnemies())
    if (pos.inRectangle(bounds))
      info.enemies.push_back(pos);
  if (printLocations)
    for (const Location* loc : level->getAllLocations()) {
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
    }
}

static Vec2 embed(Vec2 levelSize, Vec2 screenSize) {
  double s = min(double(screenSize.x) / levelSize.x, double(screenSize.y) / levelSize.y);
  return levelSize * s;
}

void MinimapGui::presentMap(const CreatureView* creature, Rectangle bounds, Renderer& r,
    function<void(double, double)> clickFun) {
  const Level* level = creature->getLevel();
  double scale = min(double(bounds.width()) / level->getBounds().width(),
      double(bounds.height()) / level->getBounds().height());
  while (1) {
    update(level, level->getBounds(), creature, true);
    renderMap(r, Rectangle(Vec2(0, 0), embed(level->getBounds().bottomRight(), bounds.bottomRight())));
    r.drawAndClearBuffer();
    Event event;
    while (r.pollEvent(event)) {
      if (event.type == SDL::SDL_KEYDOWN)
        return;
      if (event.type == SDL::SDL_MOUSEBUTTONDOWN) {
        clickFun(double(event.button.x) / scale, double(event.button.y) / scale);
        return;
      }
    }
  }
}


