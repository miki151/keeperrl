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
#include "square.h"

void MinimapGui::renderMap(Renderer& renderer, Rectangle target) {
  mapBufferTex.update(mapBuffer);
  renderer.drawImage(target, info.bounds, mapBufferTex);
  Vec2 topLeft = target.getTopLeft();
  double scale = min(double(target.getW()) / info.bounds.getW(),
      double(target.getW()) / info.bounds.getH());
  for (Vec2 v : info.roads) {
    Vec2 rrad(1, 1);
    Vec2 pos = topLeft + (v - info.bounds.getTopLeft()) * scale;
    if (pos.inRectangle(target.minusMargin(rrad.x)))
      renderer.drawFilledRectangle(Rectangle(pos - rrad, pos + rrad), colors[ColorId::BROWN]);
  }
  Vec2 rad(3, 3);
  Vec2 player = topLeft + (info.player - info.bounds.getTopLeft()) * scale;
  if (player.inRectangle(target.minusMargin(rad.x)))
    renderer.drawFilledRectangle(Rectangle(player - rad, player + rad), colors[ColorId::BLUE]);
  for (Vec2 pos : info.enemies) {
    Vec2 v = (pos - info.bounds.getTopLeft()) * scale;
    renderer.drawFilledRectangle(Rectangle(topLeft + v - rad, topLeft + v + rad), colors[ColorId::RED]);
  }
  for (auto loc : info.locations) {
    Vec2 v = (loc.pos - info.bounds.getTopLeft()) * scale;
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

MinimapGui::MinimapGui(function<void()> f) : clickFun(f) {
  mapBuffer.create(Level::getMaxBounds().getW(), Level::getMaxBounds().getH());
  mapBufferTex.loadFromImage(mapBuffer);
}

void MinimapGui::clear() {
  refreshBuffer = true;
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
  if (refreshBuffer) {
    for (Vec2 v : Level::getMaxBounds()) {
      if (!v.inRectangle(level->getBounds()) || !memory.hasViewIndex(v))
        mapBuffer.setPixel(v.x, v.y, colors[ColorId::BLACK]);
      else {
        mapBuffer.setPixel(v.x, v.y, Tile::getColor(level->getSafeSquare(v)->getViewObject()));
        if (level->getSafeSquare(v)->getViewObject().hasModifier(ViewObject::Modifier::ROAD))
          info.roads.insert(v);
      }
    }
    refreshBuffer = false;
  }
  for (Vec2 v : memory.getUpdated()) {
    mapBuffer.setPixel(v.x, v.y, Tile::getColor(level->getSafeSquare(v)->getViewObject()));
    if (level->getSafeSquare(v)->getViewObject().hasModifier(ViewObject::Modifier::ROAD))
      info.roads.insert(v);
  }
  memory.clearUpdated();
  info.player = *creature->getPosition(true);
  for (Vec2 pos : creature->getVisibleEnemies())
    if (pos.inRectangle(bounds))
      info.enemies.push_back(pos);
  if (printLocations)
    for (const Location* loc : level->getAllLocations()) {
      bool seen = false;
      for (Vec2 v : loc->getAllSquares())
        if (memory.hasViewIndex(v)) {
          seen = true;
          break;
        }
      if (loc->isMarkedAsSurprise() && !seen) {
        Vec2 pos = loc->getMiddle();
        info.locations.push_back({pos, ""});
      }
      if (loc->hasName() && seen) {
        Vec2 pos = loc->getBottomRight();
        info.locations.push_back({pos, loc->getName()});
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
  double scale = min(double(bounds.getW()) / level->getBounds().getW(),
      double(bounds.getH()) / level->getBounds().getH());
  while (1) {
    update(level, level->getBounds(), creature, true);
    renderMap(r, Rectangle(Vec2(0, 0), embed(level->getBounds().getBottomRight(), bounds.getBottomRight())));
    r.drawAndClearBuffer();
    Event event;
    while (r.pollEvent(event)) {
      if (event.type == Event::KeyPressed)
        return;
      if (event.type == Event::MouseButtonPressed) {
        clickFun(double(event.mouseButton.x) / scale, double(event.mouseButton.y) / scale);
        return;
      }
    }
  }
}


