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
#include "window_renderer.h"
#include "creature.h"
#include "square.h"

static Image mapBuffer;
static TextureRenderer renderer;

void MinimapGui::renderMap() {
  renderer.drawImage(info.bounds, mapBuffer, info.scale);
  for (Vec2 v : info.roads) {
    Vec2 rrad(1, 1);
    Vec2 pos = info.bounds.getTopLeft() + v * info.scale;
    renderer.drawFilledRectangle(Rectangle(pos - rrad, pos + rrad), colors[ColorId::BROWN]);
  }
  Vec2 rad(3, 3);
  if (info.player.inRectangle(info.bounds.minusMargin(rad.x)))
    renderer.drawFilledRectangle(Rectangle(info.player - rad, info.player + rad), colors[ColorId::BLUE]);
  for (Vec2 pos : info.enemies)
    renderer.drawFilledRectangle(Rectangle(pos - rad, pos + rad), colors[ColorId::RED]);
  for (auto loc : info.locations)
    if (loc.text.empty())
      renderer.drawText(colors[ColorId::LIGHT_GREEN], loc.pos.x + 5, loc.pos.y, "?");
    else {
      renderer.drawFilledRectangle(loc.pos.x, loc.pos.y,
          loc.pos.x + renderer.getTextLength(loc.text) + 10, loc.pos.y + 25,
          transparency(colors[ColorId::BLACK], 130));
      renderer.drawText(colors[ColorId::WHITE], loc.pos.x + 5, loc.pos.y, loc.text);
    }
}

void MinimapGui::render(Renderer& r) {
  renderMap();
  r.drawSprite(
      getBounds().getTopLeft(), Vec2(0, 0), Vec2(getBounds().getW(), getBounds().getH()), renderer.getTexture());
}

MinimapGui::MinimapGui(function<void()> f) : clickFun(f) {
}

void MinimapGui::onLeftClick(Vec2 v) {
  if (v.inRectangle(getBounds()))
    clickFun();
}

void MinimapGui::initialize() {
  mapBuffer.create(Level::getMaxBounds().getW(), Level::getMaxBounds().getH());
  renderer.initialize(min(2048u, Texture::getMaximumSize()), min(2048u, Texture::getMaximumSize()));
}

void putMapPixel(Vec2 pos, Color col) {
  if (pos.inRectangle(Level::getMaxBounds()))
    mapBuffer.setPixel(pos.x, pos.y, col);
}

void MinimapGui::update(const Level* level, Rectangle levelPart, const CreatureView* creature, bool printLocations) {
  update(level, levelPart, Rectangle(getBounds().getW(), getBounds().getH()), creature, printLocations);
}

void MinimapGui::update(const Level* level, Rectangle levelPart, Rectangle bounds, const CreatureView* creature,
    bool printLocations) {
  double scale = min(double(bounds.getW()) / levelPart.getW(),
      double(bounds.getH()) / levelPart.getH());
  info.bounds = bounds;
  info.scale = scale;
  info.roads.clear();
  info.enemies.clear();
  info.locations.clear();
  for (Vec2 v : Rectangle(bounds.getW() / scale, bounds.getH() / scale)) {
    putMapPixel(v, colors[ColorId::BLACK]);
  }
  for (Vec2 v : levelPart) {
    if (!v.inRectangle(level->getBounds()) || (!creature->getMemory().hasViewIndex(v) && !creature->canSee(v)))
      putMapPixel(v - levelPart.getTopLeft(), colors[ColorId::BLACK]);
    else {
      putMapPixel(v - levelPart.getTopLeft(), Tile::getColor(level->getSquare(v)->getViewObject()));
      if (level->getSquare(v)->getName() == "road")
        info.roads.push_back(v - levelPart.getTopLeft());
    }
  }
  info.player = bounds.getTopLeft() + (creature->getPosition() - levelPart.getTopLeft()) * scale;
  for (const Creature* c : creature->getVisibleEnemies()) {
    Vec2 pos = bounds.getTopLeft() + (c->getPosition() - levelPart.getTopLeft()) * scale;
    if (pos.inRectangle(bounds))
      info.enemies.push_back(pos);
  }
  if (printLocations)
    for (const Location* loc : level->getAllLocations()) {
      bool seen = false;
      for (Vec2 v : loc->getBounds())
        if (creature->getMemory().hasViewIndex(v) || creature->canSee(v)) {
          seen = true;
          break;
        }
      if (loc->isMarkedAsSurprise() && !seen) {
        Vec2 pos = bounds.getTopLeft() + loc->getBounds().middle() * scale;
        info.locations.push_back({pos, ""});
      }
      if (loc->hasName() && seen) {
        Vec2 pos = bounds.getTopLeft() + loc->getBounds().getBottomRight() * scale;
        info.locations.push_back({pos, loc->getName()});
      }
    }
}

void MinimapGui::presentMap(const CreatureView* creature, Rectangle bounds, WindowRenderer& r,
    function<void(double, double)> clickFun) {
  const Level* level = creature->getViewLevel();
  double scale = min(double(bounds.getW()) / level->getBounds().getW(),
      double(bounds.getH()) / level->getBounds().getH());
  while (1) {
    update(level, level->getBounds(), Rectangle(bounds.getW(), bounds.getH()), creature, true);
    renderMap();
    r.drawSprite(
      bounds.getTopLeft(), Vec2(0, 0), Vec2(bounds.getW(), bounds.getH()), renderer.getTexture());
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


