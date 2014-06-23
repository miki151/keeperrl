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

#include "map_gui.h"
#include "view_object.h"
#include "map_layout.h"
#include "view_index.h"
#include "tile.h"
#include "window_view.h"
#include "window_renderer.h"
#include "clock.h"

using namespace colors;

MapGui::MapGui(const Table<Optional<ViewIndex>>& o, function<void(Vec2)> fun) : objects(o), leftClickFun(fun) {
}

void MapGui::setLayout(MapLayout* l) {
  layout = l;
}

void MapGui::setSpriteMode(bool s) {
  spriteMode = s;
}

void MapGui::addAnimation(PAnimation animation, Vec2 pos) {
  animation->setBegin(Clock::get().getRealMillis());
  animations.push_back({std::move(animation), pos});
}


Optional<Vec2> MapGui::getHighlightedTile(WindowRenderer& renderer) {
  Vec2 pos = renderer.getMousePos();
  if (!pos.inRectangle(getBounds()))
    return Nothing();
  return layout->projectOnMap(getBounds(), pos);
}

static Color getBleedingColor(const ViewObject& object) {
  double bleeding = object.getAttribute(ViewObject::Attribute::BLEEDING);
 /* if (object.isPoisoned())
    return Color(0, 255, 0);*/
  if (bleeding > 0)
    bleeding = 0.3 + bleeding * 0.7;
  return Color(255, max(0., (1 - bleeding) * 255), max(0., (1 - bleeding) * 255));
}

Color getHighlightColor(ViewIndex::HighlightInfo info) {
  switch (info.type) {
    case HighlightType::BUILD: return transparency(yellow, 170);
    case HighlightType::RECT_SELECTION: return transparency(yellow, 90);
    case HighlightType::FOG: return transparency(white, 120 * info.amount);
    case HighlightType::POISON_GAS: return Color(0, min(255., info.amount * 500), 0, info.amount * 140);
    case HighlightType::MEMORY: return transparency(black, 80);
    case HighlightType::NIGHT: return transparency(nightBlue, info.amount * 160);
    case HighlightType::EFFICIENCY: return transparency(Color(255, 0, 0) , 120 * (1 - info.amount));
  }
  FAIL << "pokpok";
  return black;
}

enum class ConnectionId {
  ROAD,
  WALL,
  WATER,
  MOUNTAIN2,
};

unordered_map<Vec2, ConnectionId> floorIds;
set<Vec2> shadowed;

Optional<ConnectionId> getConnectionId(ViewId id) {
  switch (id) {
    case ViewId::ROAD: return ConnectionId::ROAD;
    case ViewId::BLACK_WALL:
    case ViewId::YELLOW_WALL:
    case ViewId::HELL_WALL:
    case ViewId::LOW_ROCK_WALL:
    case ViewId::WOOD_WALL:
    case ViewId::CASTLE_WALL:
    case ViewId::MUD_WALL:
    case ViewId::WALL: return ConnectionId::WALL;
    case ViewId::MAGMA:
    case ViewId::WATER: return ConnectionId::WATER;
    case ViewId::MOUNTAIN2: return ConnectionId::MOUNTAIN2;
    default: return Nothing();
  }
}

bool tileConnects(ConnectionId id, Vec2 pos) {
  return floorIds.count(pos) && id == floorIds.at(pos);
}

void MapGui::onLeftClick(Vec2 v) {
  if (optionsGui && v.inRectangle(optionsGui->getBounds()))
    optionsGui->onLeftClick(v);
  if (v.inRectangle(getBounds())) {
    Vec2 pos = layout->projectOnMap(getBounds(), v);
    leftClickFun(pos);
    mouseHeldPos = pos;
  }
}

void MapGui::onRightClick(Vec2) {
}

void MapGui::onMouseMove(Vec2 v) {
  Vec2 pos = layout->projectOnMap(getBounds(), v);
  if (v.inRectangle(getBounds()) && (!optionsGui || !v.inRectangle(optionsGui->getBounds()))) {
    if (mouseHeldPos && *mouseHeldPos != pos) {
      leftClickFun(pos);
      mouseHeldPos = pos;
    }
    highlightedPos = pos;
  } else
    highlightedPos = Nothing();
}

void MapGui::onMouseRelease() {
  mouseHeldPos = Nothing();
}

Optional<ViewObject> MapGui::drawObjectAbs(Renderer& renderer, int x, int y, const ViewIndex& index,
    int sizeX, int sizeY, Vec2 tilePos, bool highlighted) {
  vector<ViewObject> objects;
  if (spriteMode) {
    for (ViewLayer layer : layout->getLayers())
      if (index.hasObject(layer))
        objects.push_back(index.getObject(layer));
  } else
    if (auto object = index.getTopObject(layout->getLayers()))
      objects.push_back(*object);
  for (ViewObject& object : objects) {
    if (object.hasModifier(ViewObject::Modifier::PLAYER)) {
      renderer.drawFilledRectangle(x, y, x + sizeX, y + sizeY, Color::Transparent, lightGray);
    }
    if (object.hasModifier(ViewObject::Modifier::TEAM_HIGHLIGHT)) {
      renderer.drawFilledRectangle(x, y, x + sizeX, y + sizeY, Color::Transparent, darkGreen);
    }
    Tile tile = Tile::getTile(object, spriteMode);
    Color color = getBleedingColor(object);
    if (object.hasModifier(ViewObject::Modifier::INVISIBLE) || object.hasModifier(ViewObject::Modifier::HIDDEN))
      color = transparency(color, 70);
    else
    if (tile.translucent > 0)
      color = transparency(color, 255 * (1 - tile.translucent));
    else if (object.hasModifier(ViewObject::Modifier::ILLUSION))
      color = transparency(color, 150);
    if (object.hasModifier(ViewObject::Modifier::PLANNED))
      color = transparency(color, 100);
    double waterDepth = object.getAttribute(ViewObject::Attribute::WATER_DEPTH);
    if (waterDepth > 0) {
      int val = max(0.0, 255.0 - min(2.0, waterDepth) * 60);
      color = Color(val, val, val);
    }
    if (tile.hasSpriteCoord()) {
      int moveY = 0;
      int off = (Renderer::nominalSize -  Renderer::tileSize[tile.getTexNum()]) / 2;
      int sz = Renderer::tileSize[tile.getTexNum()];
      int width = sizeX - 2 * off;
      int height = sizeY - 2 * off;
      set<Dir> dirs;
      if (auto connectionId = getConnectionId(object.id()))
        for (Vec2 dir : Vec2::directions4())
          if (tileConnects(*connectionId, tilePos + dir))
            dirs.insert(dir.getCardinalDir());
      Vec2 coord = tile.getSpriteCoord(dirs);

      if (object.hasModifier(ViewObject::Modifier::MOVE_UP))
        moveY = -6;
      if (object.layer() == ViewLayer::CREATURE || object.hasModifier(ViewObject::Modifier::ROUND_SHADOW)) {
        renderer.drawSprite(x, y - 2, 2 * Renderer::nominalSize, 22 * Renderer::nominalSize, Renderer::nominalSize, Renderer::nominalSize, Renderer::tiles[0], width, height);
        moveY = -6;
      }
      renderer.drawSprite(x + off, y + moveY + off, coord.x * sz,
          coord.y * sz, sz, sz, Renderer::tiles[tile.getTexNum()], width, height, color);
      if (contains({ViewLayer::FLOOR, ViewLayer::FLOOR_BACKGROUND}, object.layer()) && 
          shadowed.count(tilePos) && !tile.stickingOut)
        renderer.drawSprite(x, y, 1 * Renderer::nominalSize, 21 * Renderer::nominalSize, Renderer::nominalSize, Renderer::nominalSize, Renderer::tiles[5], width, height);
      if (object.getAttribute(ViewObject::Attribute::BURNING) > 0) {
        renderer.drawSprite(x, y, Random.getRandom(10, 12) * Renderer::nominalSize, 0 * Renderer::nominalSize,
            Renderer::nominalSize, Renderer::nominalSize, Renderer::tiles[2], width, height);
      }
      if (object.hasModifier(ViewObject::Modifier::LOCKED))
        renderer.drawSprite(x + (Renderer::nominalSize - Renderer::tileSize[3]) / 2, y, 5 * Renderer::tileSize[3], 6 * Renderer::tileSize[3], Renderer::tileSize[3], Renderer::tileSize[3], Renderer::tiles[3], width / 2, height / 2);
    } else {
      renderer.drawText(tile.symFont ? Renderer::SYMBOL_FONT : Renderer::TILE_FONT, sizeY, Tile::getColor(object),
          x + sizeX / 2, y - 3, tile.text, true);
      double burningVal = object.getAttribute(ViewObject::Attribute::BURNING);
      if (burningVal > 0) {
        renderer.drawText(Renderer::SYMBOL_FONT, sizeY, WindowView::getFireColor(),
            x + sizeX / 2, y - 3, L'ѡ', true);
        if (burningVal > 0.5)
          renderer.drawText(Renderer::SYMBOL_FONT, sizeY, WindowView::getFireColor(),
              x + sizeX / 2, y - 3, L'Ѡ', true);
      }
    }
  }
  if (highlighted) {
    renderer.drawFilledRectangle(x, y, x + sizeX, y + sizeY, Color::Transparent, lightGray);
  }
  if (!objects.empty())
    return objects.back();
  else
    return Nothing();
}

void MapGui::setLevelBounds(Rectangle b) {
  levelBounds = b;
}

void MapGui::updateObjects(const MapMemory* mem) {
  lastMemory = mem;
  floorIds.clear();
  shadowed.clear();
  for (Vec2 wpos : layout->getAllTiles(getBounds(), objects.getBounds()))
    if (auto& index = objects[wpos])
      if (index->hasObject(ViewLayer::FLOOR)) {
        ViewObject object = index->getObject(ViewLayer::FLOOR);
        if (object.hasModifier(ViewObject::Modifier::CASTS_SHADOW)) {
          shadowed.erase(wpos);
          shadowed.insert(wpos + Vec2(0, 1));
        }
        if (auto id = getConnectionId(object.id()))
          floorIds.insert(make_pair(wpos, *id));
      }
}

const int bgTransparency = 180;

void MapGui::drawHint(Renderer& renderer, Color color, const string& text) {
  int height = 30;
  int width = renderer.getTextLength(text) + 30;
  Vec2 pos(getBounds().getKX() - width, getBounds().getKY() - height);
  renderer.drawFilledRectangle(pos.x, pos.y, pos.x + width, pos.y + height, transparency(black, bgTransparency));
  renderer.drawText(color, pos.x + 10, pos.y + 1, text);
}

void MapGui::render(Renderer& renderer) {
  int sizeX = layout->squareWidth();
  int sizeY = layout->squareHeight();
  renderer.drawFilledRectangle(getBounds(), almostBlack);
  Optional<ViewObject> highlighted;
  for (Vec2 wpos : layout->getAllTiles(getBounds(), levelBounds)) {
    Vec2 pos = layout->projectOnScreen(getBounds(), wpos);
    if (!spriteMode && wpos.inRectangle(levelBounds))
      renderer.drawFilledRectangle(pos.x, pos.y, pos.x + sizeX, pos.y + sizeY, black);
    if (!objects[wpos] || objects[wpos]->isEmpty()) {
      if (wpos.inRectangle(levelBounds))
        renderer.drawFilledRectangle(pos.x, pos.y, pos.x + sizeX, pos.y + sizeY, black);
      if (highlightedPos == wpos) {
        renderer.drawFilledRectangle(pos.x, pos.y, pos.x + sizeX, pos.y + sizeY, Color::Transparent, lightGray);
      }
      continue;
    }
    const ViewIndex& index = *objects[wpos];
    bool isHighlighted = highlightedPos == wpos;
    if (auto topObject = drawObjectAbs(renderer, pos.x, pos.y, index, sizeX, sizeY, wpos, isHighlighted)) {
      if (isHighlighted)
        highlighted = *topObject;
    }
  }
  for (Vec2 wpos : layout->getAllTiles(getBounds(), levelBounds))
    if (auto index = objects[wpos]) {
      Vec2 pos = layout->projectOnScreen(getBounds(), wpos);
      for (ViewIndex::HighlightInfo highlight : index->getHighlight())
        renderer.drawFilledRectangle(pos.x, pos.y, pos.x + sizeX, pos.y + sizeY, getHighlightColor(highlight));
    }
  animations = filter(std::move(animations), [](const AnimationInfo& elem) 
      { return !elem.animation->isDone(Clock::get().getRealMillis());});
  for (auto& elem : animations)
    elem.animation->render(
        renderer,
        getBounds(),
        layout->projectOnScreen(getBounds(), elem.position),
        Clock::get().getRealMillis());
  if (!hint.empty())
    drawHint(renderer, white, hint);
  else
  if (highlightedPos && highlighted) {
    Color col = white;
    if (highlighted->isHostile())
      col = red;
    else if (highlighted->isFriendly())
      col = green;
    drawHint(renderer, col, highlighted->getDescription(true));
  }
  if (optionsGui) {
    int margin = 10;
    optionsGui->setBounds(Rectangle(margin, getBounds().getKY() - optionsHeight - margin, 250, getBounds().getKY() - margin));
    optionsGui->render(renderer);
  }
}

void MapGui::resetHint() {
  hint.clear();
}

PGuiElem MapGui::getHintCallback(const string& s) {
  return GuiElem::mouseOverAction([this, s]() { hint = s; });
}

void MapGui::setOptions(const string& title, vector<PGuiElem> options) {
  int margin = 10;
  vector<PGuiElem> lines = makeVec<PGuiElem>(GuiElem::label(title, white));
  vector<int> heights {40};
  for (auto& elem : options) {
    lines.push_back(GuiElem::margins(std::move(elem), 15, 0, 0, 0));
    heights.push_back(30);
  }
  optionsHeight = 30 * options.size() + 40 + 2 * margin;
  optionsGui = GuiElem::stack(
      GuiElem::rectangle(transparency(black, bgTransparency)),
      GuiElem::margins(GuiElem::verticalList(std::move(lines), heights, 0), margin, margin, margin, margin));
}

void MapGui::clearOptions() {
  optionsGui.reset();
}

