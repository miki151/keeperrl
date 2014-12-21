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
#include "renderer.h"
#include "clock.h"
#include "view_id.h"
#include "level.h"
#include "creature_view.h"

using sf::Keyboard;

MapGui::MapGui(Callbacks call) : objects(Level::getMaxBounds()), callbacks(call),
    fogOfWar(Level::getMaxBounds(), false), extraBorderPos(Level::getMaxBounds(), {}) {
  clearCenter();
}

static int fireVar = 50;

static Color getFireColor() {
  return Color(200 + Random.get(-fireVar, fireVar), Random.get(fireVar), Random.get(fireVar), 150);
}

void MapGui::updateLayout(MapLayout* l, Rectangle bounds) {
  levelBounds = bounds;
  layout = l;
  Vec2 movePos = Vec2((center.x - mouseOffset.x) * layout->squareWidth(),
      (center.y - mouseOffset.y) * layout->squareHeight());
  movePos.x = max(movePos.x, 0);
  movePos.x = min(movePos.x, int(levelBounds.getKX() * layout->squareWidth()));
  movePos.y = max(movePos.y, 0);
  movePos.y = min(movePos.y, int(levelBounds.getKY() * layout->squareHeight()));
  layout->updatePlayerPos(movePos);
}

void MapGui::setSpriteMode(bool s) {
  spriteMode = s;
}

void MapGui::addAnimation(PAnimation animation, Vec2 pos) {
  animation->setBegin(Clock::get().getRealMillis());
  animations.push_back({std::move(animation), pos});
}


Optional<Vec2> MapGui::getHighlightedTile(Renderer& renderer) {
  Vec2 pos = renderer.getMousePos();
  if (!pos.inRectangle(getBounds()))
    return Nothing();
  return layout->projectOnMap(getBounds(), pos);
}

Color getHighlightColor(HighlightType type, double amount) {
  switch (type) {
    case HighlightType::RECT_DESELECTION: return transparency(colors[ColorId::RED], 90);
    case HighlightType::BUILD: return transparency(colors[ColorId::YELLOW], 170);
    case HighlightType::RECT_SELECTION: return transparency(colors[ColorId::YELLOW], 90);
    case HighlightType::FOG: return transparency(colors[ColorId::WHITE], 120 * amount);
    case HighlightType::POISON_GAS: return Color(0, min(255., amount * 500), 0, amount * 140);
    case HighlightType::MEMORY: return transparency(colors[ColorId::BLACK], 80);
    case HighlightType::NIGHT: return transparency(colors[ColorId::NIGHT_BLUE], amount * 160);
    case HighlightType::EFFICIENCY: return transparency(Color(255, 0, 0) , 120 * (1 - amount));
    case HighlightType::PRIORITY_TASK: return transparency(Color(0, 255, 0), 120);
  }
  FAIL << "pokpok";
  return Color();
}

class ViewIdMap {
  public:
  void add(Vec2 pos, ViewId id) {
    ids[pos].insert(id);
  }

  bool has(Vec2 pos, ViewId id) {
    return ids[pos].count(id);
  }

  void clear() {
    ids.clear();
  }

  private:
  unordered_map<Vec2, unordered_set<ViewId>> ids;
};

ViewIdMap connectionMap;

set<Vec2> shadowed;

Optional<ViewId> getConnectionId(ViewId id) {
  switch (id) {
    case ViewId::BLACK_WALL:
    case ViewId::YELLOW_WALL:
    case ViewId::HELL_WALL:
    case ViewId::LOW_ROCK_WALL:
    case ViewId::WOOD_WALL:
    case ViewId::CASTLE_WALL:
    case ViewId::MUD_WALL:
    case ViewId::MOUNTAIN2:
    case ViewId::GOLD_ORE:
    case ViewId::IRON_ORE:
    case ViewId::STONE:
    case ViewId::WALL: return ViewId::WALL;
    default: return id;
  }
}

Optional<ViewId> getConnectionId(const ViewObject& object) {
  if (object.hasModifier(ViewObject::Modifier::PLANNED))
    return Nothing();
  else
    return getConnectionId(object.id());
}

vector<Vec2>& getConnectionDirs(ViewId id) {
  static vector<Vec2> v4 = Vec2::directions4();
  static vector<Vec2> v8 = Vec2::directions8();
  switch (id) {
    case ViewId::DORM:
    case ViewId::CEMETERY:
    case ViewId::BEAST_LAIR:
    case ViewId::STOCKPILE1:
    case ViewId::STOCKPILE2:
    case ViewId::STOCKPILE3:
    case ViewId::LIBRARY:
    case ViewId::WORKSHOP:
    case ViewId::FORGE:
    case ViewId::LABORATORY:
    case ViewId::JEWELER:
    case ViewId::TORTURE_TABLE:
    case ViewId::RITUAL_ROOM:
    case ViewId::MOUNTAIN2:
    case ViewId::GOLD_ORE:
    case ViewId::IRON_ORE:
    case ViewId::STONE:
    case ViewId::TRAINING_ROOM: return v8;
    default: return v4;
  }
}

void MapGui::onKeyPressed(Event::KeyEvent key) {
  const double shiftScroll = 10;
  const double normalScroll = 2.5;
  switch (key.code) {
    case Keyboard::Up:
    case Keyboard::Numpad8:
      center.y -= key.shift ? shiftScroll : normalScroll;
      break;
    case Keyboard::Numpad9:
      center.y -= key.shift ? shiftScroll : normalScroll;
      center.x += key.shift ? shiftScroll : normalScroll;
      break;
    case Keyboard::Right: 
    case Keyboard::Numpad6:
      center.x += key.shift ? shiftScroll : normalScroll;
      break;
    case Keyboard::Numpad3:
      center.x += key.shift ? shiftScroll : normalScroll;
      center.y += key.shift ? shiftScroll : normalScroll;
      break;
    case Keyboard::Down:
    case Keyboard::Numpad2:
      center.y += key.shift ? shiftScroll : normalScroll;
      break;
    case Keyboard::Numpad1:
      center.x -= key.shift ? shiftScroll : normalScroll;
      center.y += key.shift ? shiftScroll : normalScroll;
      break;
    case Keyboard::Left:
    case Keyboard::Numpad4:
      center.x -= key.shift ? shiftScroll : normalScroll;
      break;
    case Keyboard::Numpad7:
      center.x -= key.shift ? shiftScroll : normalScroll;
      center.y -= key.shift ? shiftScroll : normalScroll;
      break;
    default: break;
  }
}

bool MapGui::onLeftClick(Vec2 v) {
  if (v.inRectangle(getBounds())) {
    Vec2 pos = layout->projectOnMap(getBounds(), v);
    callbacks.leftClickFun(pos);
    mouseHeldPos = pos;
    return true;
  }
  return false;
}

bool MapGui::onRightClick(Vec2 pos) {
  if (pos.inRectangle(getBounds())) {
    lastMousePos = pos;
    isScrollingNow = true;
    mouseOffset.x = mouseOffset.y = 0;
    return true;
  }
  return false;
}

void MapGui::onMouseMove(Vec2 v) {
  Vec2 pos = layout->projectOnMap(getBounds(), v);
  if (v.inRectangle(getBounds())) {
    if (mouseHeldPos && *mouseHeldPos != pos) {
      callbacks.leftClickFun(pos);
      mouseHeldPos = pos;
    }
    highlightedPos = pos;
  } else
    highlightedPos = Nothing();
  if (isScrollingNow) {
    mouseOffset.x = double(v.x - lastMousePos.x) / layout->squareWidth();
    mouseOffset.y = double(v.y - lastMousePos.y) / layout->squareHeight();
    callbacks.refreshFun();
  }
}

void MapGui::onMouseRelease() {
  if (isScrollingNow) {
    if (fabs(mouseOffset.x) + fabs(mouseOffset.y) < 1)
      callbacks.rightClickFun(layout->projectOnMap(getBounds(), lastMousePos));
    else {
      center.x -= mouseOffset.x;
      center.y -= mouseOffset.y;
    }
    mouseOffset.x = mouseOffset.y = 0;
    isScrollingNow = false;
    callbacks.refreshFun();
  }
  mouseHeldPos = Nothing();
}

void MapGui::drawFloorBorders(Renderer& renderer, const EnumSet<Dir>& borders, int x, int y) {
  for (const Dir& dir : borders) {
    int coord;
    switch (dir) {
      case Dir::N: coord = 0; break;
      case Dir::E: coord = 1; break;
      case Dir::S: coord = 2; break;
      case Dir::W: coord = 3; break;
      default: continue;
    }
    renderer.drawSprite(x, y, coord * Renderer::nominalSize.x, 18 * Renderer::nominalSize.y,
        Renderer::nominalSize.x, Renderer::nominalSize.y, Renderer::tiles[1],
        Renderer::nominalSize.x, Renderer::nominalSize.y);
  }
}

static void drawMorale(Renderer& renderer, const Rectangle& rect, double morale) {
  Color col;
  if (morale < 0)
    col = Color(255, 0, 0, -morale * 150);
  else
    col = Color(0, 255, 0, morale * 150);
  renderer.drawFilledRectangle(rect, Color::Transparent, col);
}

static Vec2 getAttachmentOffset(Dir dir, int width, int height) {
  switch (dir) {
    case Dir::N: return Vec2(0, -height * 2 / 3);
    case Dir::S: return Vec2(0, height / 4);
    case Dir::E:
    case Dir::W: return Vec2(dir) * width / 2;
    default: FAIL << "Bad attachment dir " << int(dir);
  }
  return Vec2();
}

static double getJumpOffset(double state) {
  if (state > 0.5)
    state -= 0.5;
  state *= 2;
  const double maxH = 0.09;
  return maxH * (1.0 - (2.0 * state - 1) * (2.0 * state - 1));
}

Vec2 MapGui::getMovementOffset(const ViewObject& object, Vec2 size, double time, int curTimeReal) {
  if (!object.hasAnyMovementInfo())
    return Vec2(0, 0);
  double state;
  Vec2 dir;
  if (screenMovement && 
      curTimeReal >= screenMovement->startTimeReal &&
      curTimeReal <= screenMovement->endTimeReal) {
    state = (double(curTimeReal) - screenMovement->startTimeReal) /
          (screenMovement->endTimeReal - screenMovement->startTimeReal);
    dir = object.getMovementInfo(screenMovement->startTimeGame, screenMovement->endTimeGame);
  } else if (object.hasAnyMovementInfo() && !screenMovement) {
    ViewObject::MovementInfo info = object.getLastMovementInfo();
    dir = info.direction;
    if (info.direction.length8() == 0 || time >= info.tEnd || time <= info.tBegin)
      return Vec2(0, 0);
    state = (time - info.tBegin) / (info.tEnd - info.tBegin);
    double minStopTime = 0.2;
    state = min(1.0, max(0.0, (state - minStopTime) / (1.0 - 2 * minStopTime)));
  } else
    return Vec2(0, 0);
  return Vec2((state - 1) * dir.x * size.x, ((state - 1)* dir.y - getJumpOffset(state)) * size.y);
}

void MapGui::drawCreatureHighlights(Renderer& renderer, const ViewObject& object, int x, int y,
    int sizeX, int sizeY) {
  if (object.hasModifier(ViewObject::Modifier::PLAYER)) {
    renderer.drawFilledRectangle(x, y, x + sizeX, y + sizeY, Color::Transparent, colors[ColorId::LIGHT_GRAY]);
  }
  if (object.hasModifier(ViewObject::Modifier::DRAW_MORALE))
    drawMorale(renderer, Rectangle(x, y, x + sizeX, y + sizeY), object.getAttribute(ViewObject::Attribute::MORALE));
  if (object.hasModifier(ViewObject::Modifier::TEAM_HIGHLIGHT)) {
    renderer.drawFilledRectangle(x, y, x + sizeX, y + sizeY, Color::Transparent, colors[ColorId::DARK_GREEN]);
  }
}

void MapGui::drawObjectAbs(Renderer& renderer, int x, int y, const ViewObject& object,
    int sizeX, int sizeY, Vec2 tilePos, int curTimeReal) {
  const Tile& tile = Tile::getTile(object.id(), spriteMode);
  Color color = Renderer::getBleedingColor(object);
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
    Vec2 move;
    Vec2 sz = Renderer::tileSize[tile.getTexNum()];
    Vec2 off = (Renderer::nominalSize -  sz).mult(Vec2(sizeX, sizeY)).div(Renderer::nominalSize * 2);
    int width = sz.x * sizeX / Renderer::nominalSize.x;
    int height = sz.y* sizeY / Renderer::nominalSize.y;
    if (sz.y > Renderer::nominalSize.y)
      off.y *= 2;
    EnumSet<Dir> dirs;
    EnumSet<Dir> borderDirs;
    if (!object.hasModifier(ViewObject::Modifier::PLANNED))
      if (auto connectionId = getConnectionId(object))
        for (Vec2 dir : getConnectionDirs(object.id())) {
          if (connectionMap.has(tilePos + dir, *connectionId))
            dirs.insert(dir.getCardinalDir());
          else
            borderDirs.insert(dir.getCardinalDir());
        }
    Vec2 movement = getMovementOffset(object, Vec2(sizeX, sizeY), currentTimeGame, curTimeReal);
    drawCreatureHighlights(renderer, object, x + movement.x, y + movement.y, sizeX, sizeY);
    Vec2 coord = tile.getSpriteCoord(dirs);
    if (object.hasModifier(ViewObject::Modifier::MOVE_UP))
      move.y = -4* sizeY / Renderer::nominalSize.y;
    if ((object.layer() == ViewLayer::CREATURE && object.id() != ViewId::BOULDER)
        || object.hasModifier(ViewObject::Modifier::ROUND_SHADOW)) {
      renderer.drawSprite(x + movement.x, y + movement.y - 2, 2 * Renderer::nominalSize.x,
          22 * Renderer::nominalSize.y,
          Renderer::nominalSize.x, Renderer::nominalSize.y, Renderer::tiles[0],
          min(Renderer::nominalSize.x, width), min(Renderer::nominalSize.y, height));
      move.y = -4* sizeY / Renderer::nominalSize.y;
    }
    if (auto background = tile.getBackgroundCoord()) {
      renderer.drawSprite(x + off.x, y + off.y, background->x * sz.x,
          background->y * sz.y, sz.x, sz.y, Renderer::tiles[tile.getTexNum()], width, height, color);
      if (shadowed.count(tilePos))
        renderer.drawSprite(x, y, 1 * Renderer::nominalSize.x, 21 * Renderer::nominalSize.y,
            Renderer::nominalSize.x, Renderer::nominalSize.y, Renderer::tiles[5], width, height);
    }
    if (coord.x < 0)
      return;
    if (auto dir = object.getAttachmentDir())
      move = getAttachmentOffset(*dir, width, height);
    move += movement;
    renderer.drawSprite(x + off.x + move.x, y + move.y + off.y, coord.x * sz.x,
        coord.y * sz.y, sz.x, sz.y, Renderer::tiles[tile.getTexNum()], width, height, color);
    if (tile.hasCorners()) {
      for (Vec2 coord : tile.getCornerCoords(dirs))
        renderer.drawSprite(x + off.x + move.x, y + move.y + off.y, coord.x * sz.x,
            coord.y * sz.y, sz.x, sz.y, Renderer::tiles[tile.getTexNum()], width, height, color);
    }
    if (tile.floorBorders) {
      drawFloorBorders(renderer, borderDirs, x, y);
    }
    if (contains({ViewLayer::FLOOR, ViewLayer::FLOOR_BACKGROUND}, object.layer()) && 
        shadowed.count(tilePos) && !tile.noShadow)
      renderer.drawSprite(x, y, 1 * Renderer::nominalSize.x, 21 * Renderer::nominalSize.y,
          Renderer::nominalSize.x, Renderer::nominalSize.y, Renderer::tiles[5], width, height);
    if (object.getAttribute(ViewObject::Attribute::BURNING) > 0) {
      renderer.drawSprite(x, y, Random.get(10, 12) * Renderer::nominalSize.x, 0 * Renderer::nominalSize.y,
          Renderer::nominalSize.x, Renderer::nominalSize.y, Renderer::tiles[2], width, height);
    }
    if (object.hasModifier(ViewObject::Modifier::LOCKED))
      renderer.drawSprite(x + (Renderer::nominalSize.x - Renderer::tileSize[3].x) / 2, y,
          5 * Renderer::tileSize[3].x, 6 * Renderer::tileSize[3].y,
          Renderer::tileSize[3].x, Renderer::tileSize[3].y, Renderer::tiles[3], width / 2, height / 2);
  } else {
    renderer.drawText(tile.symFont ? Renderer::SYMBOL_FONT : Renderer::TILE_FONT, sizeY, Tile::getColor(object),
        x + sizeX / 2, y - 3, tile.text, true);
    double burningVal = object.getAttribute(ViewObject::Attribute::BURNING);
    if (burningVal > 0) {
      renderer.drawText(Renderer::SYMBOL_FONT, sizeY, getFireColor(), x + sizeX / 2, y - 3, L'ѡ', true);
      if (burningVal > 0.5)
        renderer.drawText(Renderer::SYMBOL_FONT, sizeY, getFireColor(), x + sizeX / 2, y - 3, L'Ѡ', true);
    }
  }
}

void MapGui::updateObjects(const CreatureView* view, MapLayout* mapLayout, bool smoothMovement) {
  const Level* level = view->getLevel();
  for (Vec2 pos : mapLayout->getAllTiles(getBounds(), Level::getMaxBounds()))
    objects[pos] = Nothing();
  if (!isCentered())
    setCenter(*view->getPosition(true));
  else if (auto pos = view->getPosition(false))
    setCenter(*pos);
  updateLayout(mapLayout, view->getLevel()->getBounds());
  for (Vec2 pos : mapLayout->getAllTiles(getBounds(), Level::getMaxBounds())) 
    if (level->inBounds(pos)) {
      ViewIndex index;
      view->getViewIndex(pos, index);
      if (!index.isEmpty())
        index.setHighlight(HighlightType::NIGHT, 1.0 - view->getLevel()->getLight(pos));
      objects[pos] = index;
    }
  currentTimeGame = smoothMovement ? view->getTime() : 1000000000;
  if (auto movement = view->getMovementInfo()) {
    if (!screenMovement || screenMovement->startTimeGame != movement->prevTime) {
      screenMovement = {
        movement->from,
        movement->to,
        Clock::get().getRealMillis(),
        Clock::get().getRealMillis() + 100,
        movement->prevTime,
        currentTimeGame
      };
    }
  }
  connectionMap.clear();
  shadowed.clear();
  for (Vec2 wpos : layout->getAllTiles(getBounds(), objects.getBounds()))
    if (auto& index = objects[wpos]) {
      if (index->hasObject(ViewLayer::FLOOR)) {
        const ViewObject& object = index->getObject(ViewLayer::FLOOR);
        if (object.hasModifier(ViewObject::Modifier::CASTS_SHADOW)) {
          shadowed.erase(wpos);
          shadowed.insert(wpos + Vec2(0, 1));
        }
        if (auto id = getConnectionId(object))
          connectionMap.add(wpos, *id);
      }
      if (index->hasObject(ViewLayer::FLOOR_BACKGROUND)) {
        if (auto id = getConnectionId(index->getObject(ViewLayer::FLOOR_BACKGROUND)))
          connectionMap.add(wpos, *id);
      }
      if (auto viewId = index->getHiddenId())
        if (auto id = getConnectionId(*viewId))
          connectionMap.add(wpos, *id);
    }
}

void MapGui::clearCenter() {
  center = mouseOffset = {0.0, 0.0};
}

bool MapGui::isCentered() const {
  return center.x != 0 || center.y != 0;
}

void MapGui::setCenter(double x, double y) {
  center = {x, y};
}

void MapGui::setCenter(Vec2 v) {
  setCenter(v.x, v.y);
}

void MapGui::drawHint(Renderer& renderer, Color color, const string& text) {
  int height = 30;
  int width = renderer.getTextLength(text) + 110;
  Vec2 pos(getBounds().getKX() - width, getBounds().getKY() - height);
  renderer.drawFilledRectangle(pos.x, pos.y, pos.x + width, pos.y + height,
      GuiElem::translucentBgColor);
  renderer.drawText(color, pos.x + 10, pos.y + 1, text);
}

void MapGui::drawFoWSprite(Renderer& renderer, Vec2 pos, int sizeX, int sizeY, EnumSet<Dir> dirs) {
  const Tile& tile = Tile::fromViewId(ViewId::FOG_OF_WAR); 
  const Tile& tile2 = Tile::fromViewId(ViewId::FOG_OF_WAR_CORNER); 
  Vec2 coord = tile.getSpriteCoord(dirs.intersection({Dir::N, Dir::S, Dir::E, Dir::W}));
  Vec2 sz = Renderer::tileSize[tile.getTexNum()];
  renderer.drawSprite(pos.x, pos.y, coord.x * sz.x, coord.y * sz.y, sz.x, sz.y,
      Renderer::tiles[tile.getTexNum()], sizeX, sizeY);
  for (Dir dir : dirs.intersection({Dir::NE, Dir::SE, Dir::NW, Dir::SW})) {
    switch (dir) {
      case Dir::NE: if (!dirs[Dir::N] || !dirs[Dir::E]) continue;
      case Dir::SE: if (!dirs[Dir::S] || !dirs[Dir::E]) continue;
      case Dir::NW: if (!dirs[Dir::N] || !dirs[Dir::W]) continue;
      case Dir::SW: if (!dirs[Dir::S] || !dirs[Dir::W]) continue;
      default: break;
    }
    Vec2 coord = tile2.getSpriteCoord({dir});
    renderer.drawSprite(pos.x, pos.y, coord.x * sz.x, coord.y * sz.y, sz.x, sz.y,
        Renderer::tiles[tile2.getTexNum()], sizeX, sizeY);
  }
}

bool MapGui::isFoW(Vec2 pos) const {
  return !pos.inRectangle(Level::getMaxBounds()) || fogOfWar.getValue(pos);
}

void MapGui::renderExtraBorders(Renderer& renderer, int currentTimeReal) {
  for (Vec2 wpos : layout->getAllTiles(getBounds(), levelBounds))
    if (objects[wpos] && objects[wpos]->hasObject(ViewLayer::FLOOR_BACKGROUND)) {
      ViewId viewId = objects[wpos]->getObject(ViewLayer::FLOOR_BACKGROUND).id();
      if (Tile::fromViewId(viewId).hasExtraBorders())
        for (Vec2 v : wpos.neighbors4())
          if (v.inRectangle(extraBorderPos.getBounds())) {
            if (extraBorderPos.isDirty(v))
              extraBorderPos.getDirtyValue(v).insert(viewId);
            else
              extraBorderPos.setValue(v, {viewId});
          }
    }
  for (Vec2 wpos : layout->getAllTiles(getBounds(), levelBounds))
    for (ViewId id : extraBorderPos.getValue(wpos)) {
      const Tile& tile = Tile::fromViewId(id);
      for (ViewId underId : tile.getExtraBorderIds())
        if (connectionMap.has(wpos, underId)) {
          EnumSet<Dir> dirs;
          for (Vec2 v : Vec2::directions4())
            if (connectionMap.has(wpos + v, id))
              dirs.insert(v.getCardinalDir());
          if (auto coord = tile.getExtraBorderCoord(dirs)) {
            Vec2 sz = Renderer::tileSize[tile.getTexNum()];
            Vec2 pos = projectOnScreen(wpos, currentTimeReal);
            renderer.drawSprite(pos.x, pos.y, coord->x * sz.x, coord->y * sz.y, sz.x, sz.y,
                Renderer::tiles[tile.getTexNum()], layout->squareWidth(), layout->squareHeight());
          }
        }
    }
}

Vec2 MapGui::projectOnScreen(Vec2 wpos, int curTime) {
  double x = wpos.x;
  double y = wpos.y;
  if (screenMovement) {
    if (curTime >= screenMovement->startTimeReal && curTime <= screenMovement->endTimeReal) {
      double state = (double(curTime) - screenMovement->startTimeReal) /
          (screenMovement->endTimeReal - screenMovement->startTimeReal);
      x += (1 - state) * (screenMovement->to.x - screenMovement->from.x);
      y += (1 - state) * (screenMovement->to.y - screenMovement->from.y);
    }
  }
  return layout->projectOnScreen(getBounds(), x, y);
}

void MapGui::render(Renderer& renderer) {
  int sizeX = layout->squareWidth();
  int sizeY = layout->squareHeight();
  int currentTimeReal = Clock::get().getRealMillis();
  renderer.drawFilledRectangle(getBounds(), colors[ColorId::ALMOST_BLACK]);
  renderer.drawFilledRectangle(Rectangle(
        projectOnScreen(levelBounds.getTopLeft(), currentTimeReal),
        projectOnScreen(levelBounds.getBottomRight(), currentTimeReal)), colors[ColorId::BLACK]);
  Optional<ViewObject> highlighted;
  fogOfWar.clear();
  extraBorderPos.clear();
  for (ViewLayer layer : layout->getLayers()) {
    for (Vec2 wpos : layout->getAllTiles(getBounds(), levelBounds)) {
      Vec2 pos = projectOnScreen(wpos, currentTimeReal);
      if (!objects[wpos] || objects[wpos]->noObjects()) {
        if (layer == layout->getLayers().back()) {
          if (wpos.inRectangle(levelBounds))
            renderer.drawFilledRectangle(pos.x, pos.y, pos.x + sizeX, pos.y + sizeY, colors[ColorId::BLACK]);
        }
        fogOfWar.setValue(wpos, true);
        continue;
      }
      const ViewIndex& index = *objects[wpos];
      const ViewObject* object = nullptr;
      if (spriteMode) {
        if (index.hasObject(layer))
          object = &index.getObject(layer);
      } else
        object = index.getTopObject(layout->getLayers());
      if (object) {
        drawObjectAbs(renderer, pos.x, pos.y, *object, sizeX, sizeY, wpos, currentTimeReal);
        if (highlightedPos == wpos)
          highlighted = *object;
      }
      if (contains({ViewLayer::FLOOR, ViewLayer::FLOOR_BACKGROUND}, layer) && highlightedPos == wpos) {
        renderer.drawFilledRectangle(pos.x, pos.y, pos.x + sizeX, pos.y + sizeY, Color::Transparent,
            colors[ColorId::LIGHT_GRAY]);
      }
      if (layer == layout->getLayers().back())
        if (!isFoW(wpos))
          drawFoWSprite(renderer, pos, sizeX, sizeY, {
              !isFoW(wpos + Vec2(Dir::N)),
              !isFoW(wpos + Vec2(Dir::S)),
              !isFoW(wpos + Vec2(Dir::E)),
              !isFoW(wpos + Vec2(Dir::W)),
              isFoW(wpos + Vec2(Dir::NE)),
              isFoW(wpos + Vec2(Dir::NW)),
              isFoW(wpos + Vec2(Dir::SE)),
              isFoW(wpos + Vec2(Dir::SW))});
    }
    if (!spriteMode)
      break;
    if (layer == ViewLayer::FLOOR_BACKGROUND)
      renderExtraBorders(renderer, currentTimeReal);
  }
  for (Vec2 wpos : layout->getAllTiles(getBounds(), levelBounds))
    if (auto index = objects[wpos]) {
      Vec2 pos = projectOnScreen(wpos, currentTimeReal);
      for (HighlightType highlight : ENUM_ALL(HighlightType))
        if (index->getHighlight(highlight) > 0)
          renderer.drawFilledRectangle(pos.x, pos.y, pos.x + sizeX, pos.y + sizeY,
              getHighlightColor(highlight, index->getHighlight(highlight)));
      if (highlightedPos == wpos) {
        renderer.drawFilledRectangle(pos.x, pos.y, pos.x + sizeX, pos.y + sizeY, Color::Transparent,
            colors[ColorId::LIGHT_GRAY]);
      }
    }
  animations = filter(std::move(animations), [](const AnimationInfo& elem) 
      { return !elem.animation->isDone(Clock::get().getRealMillis());});
  for (auto& elem : animations)
    elem.animation->render(
        renderer,
        getBounds(),
        projectOnScreen(elem.position, currentTimeReal),
        Clock::get().getRealMillis());
  if (!hint.empty())
    drawHint(renderer, colors[ColorId::WHITE], hint);
  else
  if (highlightedPos && highlighted) {
    Color col = colors[ColorId::WHITE];
    if (highlighted->isHostile())
      col = colors[ColorId::RED];
    else if (highlighted->isFriendly())
      col = colors[ColorId::GREEN];
    drawHint(renderer, col, highlighted->getDescription(true));
  }
}

void MapGui::setHint(const string& h) {
  hint = h;
}
