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
#include "options.h"
#include "drag_and_drop.h"
#include "game_info.h"

using SDL::SDL_Keysym;
using SDL::SDL_Keycode;

MapGui::MapGui(Callbacks call, Clock* c, Options* o, GuiFactory* f) : objects(Level::getMaxBounds()), callbacks(call),
    clock(c), options(o), fogOfWar(Level::getMaxBounds(), false), extraBorderPos(Level::getMaxBounds(), {}),
    lastSquareUpdate(Level::getMaxBounds()), connectionMap(Level::getMaxBounds()), guiFactory(f) {
  clearCenter();
}

static int fireVar = 50;

static Color getFireColor() {
  return Color(200 + Random.get(-fireVar, fireVar), Random.get(fireVar), Random.get(fireVar), 150);
}

MapGui::ViewIdMap::ViewIdMap(Rectangle bounds) : ids(bounds, {}) {
}

void MapGui::ViewIdMap::add(Vec2 pos, ViewId id) {
  if (ids.isDirty(pos))
    ids.getDirtyValue(pos).insert(id);
  else
    ids.setValue(pos, {id});
}

void MapGui::ViewIdMap::remove(Vec2 pos) {
  ids.clear(pos);
}

bool MapGui::ViewIdMap::has(Vec2 pos, ViewId id) {
  return ids.getValue(pos).contains(id);
}

void MapGui::ViewIdMap::clear() {
  ids.clear();
}

void MapGui::setButtonViewId(ViewId id) {
  buttonViewId = id;
}

void MapGui::clearButtonViewId() {
  buttonViewId = none;
}

const MapGui::HighlightedInfo& MapGui::getLastHighlighted() {
  return lastHighlighted;
}

void MapGui::highlightTeam(const vector<UniqueEntity<Creature>::Id>& ids) {
  for (auto& id : ids)
    ++teamHighlight.getOrInit(id);
}

void MapGui::unhighlightTeam(const vector<UniqueEntity<Creature>::Id>& ids) {
  for (auto& id : ids)
    CHECK(--teamHighlight.getOrFail(id) >= 0);
}

Vec2 MapGui::getScreenPos() const {
  return Vec2(
      (int) (min<double>(levelBounds.right(), max(0.0, center.x - mouseOffset.x)) * layout->getSquareSize().x),
      (int) (min<double>(levelBounds.bottom(), max(0.0, center.y - mouseOffset.y)) * layout->getSquareSize().y));
}

void MapGui::setSpriteMode(bool s) {
  spriteMode = s;
}

void MapGui::addAnimation(PAnimation animation, Vec2 pos) {
  animation->setBegin(clock->getRealMillis());
  animations.push_back({std::move(animation), pos});
}

optional<Vec2> MapGui::getMousePos() {
  if (lastMouseMove && lastMouseMove->inRectangle(getBounds()))
    return lastMouseMove;
  else
    return none;
}

optional<Vec2> MapGui::projectOnMap(Vec2 screenCoord) {
  if (screenCoord.inRectangle(getBounds()))
    return layout->projectOnMap(getBounds(), getScreenPos(), screenCoord);
  else
    return none;
}

optional<Vec2> MapGui::getHighlightedTile(Renderer& renderer) {
  if (auto pos = getMousePos())
    return layout->projectOnMap(getBounds(), getScreenPos(), *pos);
  else
    return none;
}

Color MapGui::getHighlightColor(const ViewIndex& index, HighlightType type) {
  double amount = index.getHighlight(type);
  switch (type) {
    case HighlightType::RECT_DESELECTION: return Color::RED.transparency(90);
    case HighlightType::DIG: return Color::YELLOW.transparency(120);
    case HighlightType::FETCH_ITEMS: //return Color(Color::YELLOW).transparency(50);
    case HighlightType::CUT_TREE: return Color::YELLOW.transparency(100);
    case HighlightType::PERMANENT_FETCH_ITEMS: return Color::ORANGE.transparency(50);
    case HighlightType::STORAGE_EQUIPMENT: return Color::BLUE.transparency(buttonViewId ? 120 : 50);
    case HighlightType::STORAGE_RESOURCES: return Color::GREEN.transparency(buttonViewId ? 120 : 50);
    case HighlightType::RECT_SELECTION: return Color::YELLOW.transparency(90);
    case HighlightType::FOG: return Color::WHITE.transparency(120 * amount);
    case HighlightType::POISON_GAS: return Color(0, min<Uint8>(255., amount * 500), 0, (Uint8)(amount * 140));
    case HighlightType::MEMORY: return Color::BLACK.transparency(80);
    case HighlightType::NIGHT: return Color::NIGHT_BLUE.transparency(amount * 160);
    case HighlightType::EFFICIENCY: return Color(255, 0, 0, 120 * (1 - amount));
    case HighlightType::PRIORITY_TASK: return Color(0, 255, 0, 120);
    case HighlightType::CREATURE_DROP:
      if (index.hasObject(ViewLayer::FLOOR) && getHighlightedFurniture() == index.getObject(ViewLayer::FLOOR).id())
        return Color(0, 255, 0);
      else
        return Color(0, 255, 0, 120);
    case HighlightType::CLICKABLE_FURNITURE: return Color(255, 255, 0, 120);
    case HighlightType::CLICKED_FURNITURE: return Color(255, 255, 0);
    case HighlightType::FORBIDDEN_ZONE: return Color(255, 0, 0, 120);
    case HighlightType::UNAVAILABLE: return Color(0, 0, 0, 120);
  }
}

optional<ViewId> getConnectionId(ViewId id) {
  switch (id) {
    case ViewId::BLACK_WALL:
    case ViewId::WOOD_WALL:
    case ViewId::CASTLE_WALL:
    case ViewId::MUD_WALL:
    case ViewId::MOUNTAIN:
    case ViewId::DUNGEON_WALL:
    case ViewId::GOLD_ORE:
    case ViewId::IRON_ORE:
    case ViewId::STONE:
    case ViewId::WALL: return ViewId::WALL;
    default: return id;
  }
}

vector<Vec2>& getConnectionDirs(ViewId id) {
  static vector<Vec2> v4 = Vec2::directions4();
  static vector<Vec2> v8 = Vec2::directions8();
  switch (id) {
    case ViewId::DORM:
    case ViewId::CEMETERY:
    case ViewId::BEAST_LAIR:
    case ViewId::MOUNTAIN:
    case ViewId::DUNGEON_WALL:
    case ViewId::GOLD_ORE:
    case ViewId::IRON_ORE:
    case ViewId::STONE: return v8;
    default: return v4;
  }
}

void MapGui::setSoftCenter(Vec2 pos) {
  setSoftCenter(pos.x, pos.y);
}

void MapGui::setSoftCenter(double x, double y) {
  Coords coords {x, y};
  if (softCenter != coords) {
    softCenter = coords;
    lastScrollUpdate = clock->getRealMillis();
  }
}

void MapGui::softScroll(double x, double y) {
  if (!softCenter)
    softCenter = center;
  softCenter->x = max(0.0, min<double>(softCenter->x + x, levelBounds.right()));
  softCenter->y = max(0.0, min<double>(softCenter->y + y, levelBounds.bottom()));
  lastScrollUpdate = clock->getRealMillis();
}

bool MapGui::onKeyPressed2(SDL_Keysym key) {
  const double scrollDist = 9 * 32 / layout->getSquareSize().x;
  if (!keyScrolling)
    return false;
  switch (key.sym) {
    case SDL::SDLK_w:
      if (!options->getBoolValue(OptionId::WASD_SCROLLING) || GuiFactory::isAlt(key))
        break;
      FALLTHROUGH;
    case SDL::SDLK_UP:
    case SDL::SDLK_KP_8:
      softScroll(0, -scrollDist);
      break;
    case SDL::SDLK_KP_9:
      softScroll(scrollDist, -scrollDist);
      break;
    case SDL::SDLK_d:
      if (!options->getBoolValue(OptionId::WASD_SCROLLING) || GuiFactory::isAlt(key))
        break;
      FALLTHROUGH;
    case SDL::SDLK_RIGHT:
    case SDL::SDLK_KP_6:
      softScroll(scrollDist, 0);
      break;
    case SDL::SDLK_KP_3:
      softScroll(scrollDist, scrollDist);
      break;
    case SDL::SDLK_s:
      if (!options->getBoolValue(OptionId::WASD_SCROLLING) || GuiFactory::isAlt(key))
        break;
      FALLTHROUGH;
    case SDL::SDLK_DOWN:
    case SDL::SDLK_KP_2:
      softScroll(0, scrollDist);
      break;
    case SDL::SDLK_KP_1:
      softScroll(-scrollDist, scrollDist);
      break;
    case SDL::SDLK_a:
      if (!options->getBoolValue(OptionId::WASD_SCROLLING) || GuiFactory::isAlt(key))
        break;
      FALLTHROUGH;
    case SDL::SDLK_LEFT:
    case SDL::SDLK_KP_4:
      softScroll(-scrollDist, 0);
      break;
    case SDL::SDLK_KP_7:
      softScroll(-scrollDist, -scrollDist);
      break;
    default: break;
  }
  return false;
}

bool MapGui::onLeftClick(Vec2 v) {
  if (v.inRectangle(getBounds())) {
    mouseHeldPos = v;
    mouseOffset.x = mouseOffset.y = 0;
    if (auto c = getCreature(v))
      draggedCandidate = c;
    return true;
  }
  return false;
}

bool MapGui::onRightClick(Vec2 pos) {
  if (lastRightClick && clock->getRealMillis() - *lastRightClick < milliseconds{200}) {
    lockedView = true;
    lastRightClick = none;
    return true;
  }
  lastRightClick = clock->getRealMillis();
  if (pos.inRectangle(getBounds())) {
    lastMousePos = pos;
    isScrollingNow = true;
    mouseOffset.x = mouseOffset.y = 0;
    lockedView = false;
    return true;
  }
  return false;
}

void MapGui::onMouseGone() {
  lastMouseMove = none;
}

void MapGui::considerContinuousLeftClick(Vec2 mousePos) {
  Vec2 pos = layout->projectOnMap(getBounds(), getScreenPos(), mousePos);
  if (!lastMapLeftClick || lastMapLeftClick != pos) {
    callbacks.continuousLeftClickFun(pos);
    lastMapLeftClick = pos;
  }
}

bool MapGui::onMouseMove(Vec2 v) {
  lastMouseMove = v;
  auto draggedCreature = getDraggedCreature();
  if (v.inRectangle(getBounds()) && mouseHeldPos && !draggedCreature)
    considerContinuousLeftClick(v);
  if (!draggedCreature && draggedCandidate && mouseHeldPos && mouseHeldPos->distD(v) > 30) {
    callbacks.creatureDragFun(draggedCandidate->id, draggedCandidate->viewId, v);
    setDraggedCreature(draggedCandidate->id, draggedCandidate->viewId, v);
  }
  if (isScrollingNow) {
    mouseOffset.x = double(v.x - lastMousePos.x) / layout->getSquareSize().x;
    mouseOffset.y = double(v.y - lastMousePos.y) / layout->getSquareSize().y;
    callbacks.refreshFun();
  }
  return false;
}

optional<MapGui::CreatureInfo> MapGui::getCreature(Vec2 mousePos) {
  auto info = getHighlightedInfo(layout->getSquareSize(), clock->getRealMillis());
  if (info.creaturePos && info.object && info.object->getCreatureId())
    return CreatureInfo {*info.object->getCreatureId(), info.object->id()};
  else
    return none;
}

void MapGui::onMouseRelease(Vec2 v) {
  if (isScrollingNow) {
    if (fabs(mouseOffset.x) + fabs(mouseOffset.y) < 1)
      callbacks.rightClickFun(layout->projectOnMap(getBounds(), getScreenPos(), lastMousePos));
    else {
      center.x = min<double>(levelBounds.right(), max(0.0, center.x - mouseOffset.x));
      center.y = min<double>(levelBounds.bottom(), max(0.0, center.y - mouseOffset.y));
    }
    isScrollingNow = false;
    callbacks.refreshFun();
    mouseOffset.x = mouseOffset.y = 0;
  }
  auto draggedCreature = getDraggedCreature();
  if (auto& draggedElem = guiFactory->getDragContainer().getElement())
    if (v.inRectangle(getBounds()) && guiFactory->getDragContainer().getOrigin().distD(v) > 10) {
      switch (draggedElem->getId()) {
        case DragContentId::CREATURE:
          callbacks.creatureDroppedFun(draggedElem->get<UniqueEntity<Creature>::Id>(),
              layout->projectOnMap(getBounds(), getScreenPos(), v));
          break;
        case DragContentId::TEAM:
          callbacks.teamDroppedFun(draggedElem->get<TeamId>(),
              layout->projectOnMap(getBounds(), getScreenPos(), v));
          break;
        default:
          break;
      }
    }
  if (mouseHeldPos) {
    if (mouseHeldPos->distD(v) > 10) {
      if (!draggedCreature)
        considerContinuousLeftClick(v);
    } else {
      if (auto c = getCreature(*mouseHeldPos))
        callbacks.creatureClickFun(c->id);
      else {
        callbacks.leftClickFun(layout->projectOnMap(getBounds(), getScreenPos(), v));
        considerContinuousLeftClick(v);
      }
    }
  }
  mouseHeldPos = none;
  lastMapLeftClick = none;
  draggedCandidate = none;
}

/*void MapGui::drawFloorBorders(Renderer& renderer, DirSet borders, int x, int y) {
  for (const Dir& dir : borders) {
    int coord;
    switch (dir) {
      case Dir::N: coord = 0; break;
      case Dir::E: coord = 1; break;
      case Dir::S: coord = 2; break;
      case Dir::W: coord = 3; break;
      default: continue;
    }
    renderer.drawTile(x, y, {Vec2(coord, 18), 1});
  }
}*/

static Color getMoraleColor(double morale) {
  if (morale < 0)
    return Color(255, 0, 0, -morale * 150);
  else
    return Color(0, 255, 0, morale * 150);
}

static Vec2 getAttachmentOffset(Dir dir, Vec2 size) {
  switch (dir) {
    case Dir::N: return Vec2(0, -size.y * 2 / 3);
    case Dir::S: return Vec2(0, size.y / 4);
    case Dir::E:
    case Dir::W: return Vec2(dir) * size.x / 2;
    default: FATAL << "Bad attachment dir " << int(dir);
  }
  return Vec2();
}

static double getJumpOffset(const ViewObject& object, double state) {
  if (object.hasModifier(ViewObjectModifier::NO_UP_MOVEMENT))
    return 0;
  if (state > 0.5)
    state -= 0.5;
  state *= 2;
  const double maxH = 0.09;
  return maxH * (1.0 - (2.0 * state - 1) * (2.0 * state - 1));
}

Vec2 MapGui::getMovementOffset(const ViewObject& object, Vec2 size, double time, milliseconds curTimeReal,
    bool verticalMovement) {
  if (!object.hasAnyMovementInfo())
    return Vec2(0, 0);
  double state;
  Vec2 dir;
  if (screenMovement &&
      curTimeReal >= screenMovement->startTimeReal &&
      curTimeReal <= screenMovement->endTimeReal) {
    state = (double) (curTimeReal - screenMovement->startTimeReal).count() /
          (double) (screenMovement->endTimeReal - screenMovement->startTimeReal).count();
    dir = object.getMovementInfo(screenMovement->startTimeGame);
  }
  else if (!screenMovement) {
    MovementInfo info = object.getLastMovementInfo();
    dir = info.direction;
/*    if (info.direction.length8() == 0 || time >= info.tEnd + 0.001 || time <= info.tBegin - 0.001)
      return Vec2(0, 0);*/
    state = (time - info.tBegin) / (info.tEnd - info.tBegin);
    double minStopTime = 0.2;
    state = min(1.0, max(0.0, (state - minStopTime) / (1.0 - 2 * minStopTime)));
  } else
    return Vec2(0, 0);
  double vertical = verticalMovement ? getJumpOffset(object, state) : 0;
  if (object.getLastMovementInfo().type == MovementInfo::ATTACK)
    if (dir.length8() == 1) {
      if (verticalMovement)
        return Vec2(0.8 * (state < 0.5 ? state : 1 - state) * dir.x * size.x,
            (0.8 * (state < 0.5 ? state : 1 - state)* dir.y - vertical) * size.y);
      else
        return Vec2(0, 0);
    }
  return Vec2((state - 1) * dir.x * size.x, ((state - 1)* dir.y - vertical) * size.y);
}

void MapGui::drawCreatureHighlights(Renderer& renderer, const ViewObject& object, Vec2 pos, Vec2 sz,
    milliseconds curTime) {
  auto getHighlight = [](Color id) { return Color(id).transparency(200); };
  if (object.hasModifier(ViewObject::Modifier::HOSTILE) && highlightEnemies)
    drawCreatureHighlight(renderer, pos, sz, getHighlight(Color::PURPLE));
  if (object.hasModifier(ViewObject::Modifier::DRAW_MORALE) && highlightMorale)
    if (auto morale = object.getAttribute(ViewObject::Attribute::MORALE))
      drawCreatureHighlight(renderer, pos, sz, getMoraleColor(*morale));
  if (object.hasModifier(ViewObject::Modifier::PLAYER)) {
    if ((curTime.count() / 500) % 2 == 0)
      drawCreatureHighlight(renderer, pos, sz, getHighlight(Color::YELLOW));
  } else
  if (object.hasModifier(ViewObject::Modifier::TEAM_HIGHLIGHT))
    drawCreatureHighlight(renderer, pos, sz, getHighlight(Color::YELLOW));
  if (auto id = object.getCreatureId())
    if (isCreatureHighlighted(*id))
      drawCreatureHighlight(renderer, pos, sz, getHighlight(Color::YELLOW));
}

bool MapGui::isCreatureHighlighted(UniqueEntity<Creature>::Id creature) {
  return teamHighlight.getMaybe(creature).value_or(0) > 0;
}

static bool mirrorSprite(ViewId id) {
  switch (id) {
    case ViewId::GRASS:
    case ViewId::HILL:
      return true;
    default:
      return false;
  }
}

void MapGui::drawHealthBar(Renderer& renderer, Vec2 pos, Vec2 size, double health) {
  if (hideFullHealthBars && health == 1)
    return;
  pos.y -= size.y * 0.2;
  double barWidth = 0.12;
  double barLength = 0.8;
  auto getBar = [&](double state) {
    return Rectangle(pos.x + size.x * (1 - barLength) / 2, pos.y,
        pos.x + size.x * state * (1 + barLength) / 2, pos.y + size.y * barWidth);
  };

  renderer.drawFilledRectangle(getBar(1), Color::BLACK.transparency(150));
  if (health > 0)
    renderer.drawFilledRectangle(getBar(health),
        Color::f(min(1.0, 2 - health * 2), min(1.0, 2 * health), 0).transparency(200));
}

void MapGui::drawObjectAbs(Renderer& renderer, Vec2 pos, const ViewObject& object, Vec2 size,
    Vec2 tilePos, milliseconds curTimeReal) {
  const Tile& tile = Tile::getTile(object.id(), spriteMode);
  Color color = colorWoundedRed ? Renderer::getBleedingColor(object) : Color::WHITE;
  if (object.hasModifier(ViewObject::Modifier::INVISIBLE) || object.hasModifier(ViewObject::Modifier::HIDDEN))
    color = color.transparency(70);
  else
    if (tile.translucent > 0)
      color = color.transparency(255 * (1 - tile.translucent));
    else if (object.hasModifier(ViewObject::Modifier::ILLUSION))
      color = color.transparency(150);
  if (object.hasModifier(ViewObject::Modifier::PLANNED))
    color = color.transparency(100);
  if (auto waterDepth = object.getAttribute(ViewObject::Attribute::WATER_DEPTH))
    if (*waterDepth > 0) {
      Uint8 val = max(0.0, 255.0 - min(2.0f, *waterDepth) * 60);
      color = Color(val, val, val);
    }
  if (spriteMode && tile.hasSpriteCoord()) {
    DirSet dirs;
    DirSet borderDirs;
    if (auto connectionId = getConnectionId(object.id()))
      for (Vec2 dir : getConnectionDirs(object.id())) {
        if ((tilePos + dir).inRectangle(levelBounds) && connectionMap.has(tilePos + dir, *connectionId))
          dirs.insert(dir.getCardinalDir());
        else
          borderDirs.insert(dir.getCardinalDir());
      }
    Vec2 move;
    Vec2 movement = getMovementOffset(object, size, currentTimeGame, curTimeReal, true);
    drawCreatureHighlights(renderer, object, pos + movement, size, curTimeReal);
    if (object.layer() == ViewLayer::CREATURE || tile.roundShadow) {
      static auto coord = renderer.getTileCoord("round_shadow");
      renderer.drawTile(pos + movement, coord, size, Color(255, 255, 255, 160));
      move.y = -4* size.y / renderer.getNominalSize().y;
    }
    if (auto background = tile.getBackgroundCoord())
      renderer.drawTile(pos, *background, size, color);
    if (auto dir = object.getAttachmentDir())
      move = getAttachmentOffset(*dir, size);
    move += movement;
    if (mirrorSprite(object.id()))
      renderer.drawTile(pos + move, tile.getSpriteCoord(dirs), size, color,
          Renderer::SpriteOrientation((bool) (tilePos.getHash() % 2), (bool) (tilePos.getHash() % 4 > 1)));
    else
      renderer.drawTile(pos + move, tile.getSpriteCoord(dirs), size, color);
    if (tile.hasCorners()) {
      for (auto coord : tile.getCornerCoords(dirs))
        renderer.drawTile(pos + move, coord, size, color);
    }
/*    if (tile.floorBorders) {
      drawFloorBorders(renderer, borderDirs, x, y);
    }*/
    static auto shortShadow = renderer.getTileCoord("short_shadow");
    if (object.layer() == ViewLayer::FLOOR_BACKGROUND && shadowed.count(tilePos))
      renderer.drawTile(pos, shortShadow, size, Color(255, 255, 255, 170));
    if (auto burningVal = object.getAttribute(ViewObject::Attribute::BURNING))
      if (*burningVal > 0) {
        static auto fire1 = renderer.getTileCoord("fire1");
        static auto fire2 = renderer.getTileCoord("fire2");
        renderer.drawTile(pos, Random.choose(fire1, fire2), size);
      }
    if (displayAllHealthBars || lastHighlighted.creaturePos == pos + movement)
      if (auto wounded = object.getAttribute(ViewObject::Attribute::WOUNDED))
        drawHealthBar(renderer, pos + move, size, 1 - *wounded);
  } else {
    Vec2 movement = getMovementOffset(object, size, currentTimeGame, curTimeReal, true);
    Vec2 tilePos = pos + movement + Vec2(size.x / 2, -3);
    drawCreatureHighlights(renderer, object, pos, size, curTimeReal);
    renderer.drawText(tile.symFont ? Renderer::SYMBOL_FONT : Renderer::TILE_FONT, size.y, Tile::getColor(object),
        tilePos.x, tilePos.y, tile.text, Renderer::HOR);
    if (auto burningVal = object.getAttribute(ViewObject::Attribute::BURNING))
      if (*burningVal > 0) {
        renderer.drawText(Renderer::SYMBOL_FONT, size.y, getFireColor(), pos.x + size.x / 2, pos.y - 3, u8"ѡ",
            Renderer::HOR);
        if (*burningVal > 0.5)
          renderer.drawText(Renderer::SYMBOL_FONT, size.y, getFireColor(), pos.x + size.x / 2, pos.y - 3, u8"Ѡ",
              Renderer::HOR);
      }
  }
}

void MapGui::resetScrolling() {
  lockedView = true;
}

void MapGui::clearCenter() {
  center = mouseOffset = {0.0, 0.0};
  softCenter = none;
  screenMovement = none;
}

bool MapGui::isCentered() const {
  return center.x != 0 || center.y != 0;
}

void MapGui::setCenter(double x, double y) {
  center = {x, y};
  center.x = max(0.0, min<double>(center.x, levelBounds.right()));
  center.y = max(0.0, min<double>(center.y, levelBounds.bottom()));
  softCenter = none;
}

void MapGui::setCenter(Vec2 v) {
  setCenter(v.x, v.y);
}

void MapGui::drawFoWSprite(Renderer& renderer, Vec2 pos, Vec2 size, DirSet dirs) {
  const Tile& tile = Tile::getTile(ViewId::FOG_OF_WAR, true);
  const Tile& tile2 = Tile::getTile(ViewId::FOG_OF_WAR_CORNER, true);
  static DirSet fourDirs = DirSet({Dir::N, Dir::S, Dir::E, Dir::W});
  auto coord = tile.getSpriteCoord(dirs & fourDirs);
  renderer.drawTile(pos, coord, size);
  for (Dir dir : dirs.intersection(fourDirs.complement())) {
    static DirSet ne({Dir::N, Dir::E});
    static DirSet se({Dir::S, Dir::E});
    static DirSet nw({Dir::N, Dir::W});
    static DirSet sw({Dir::S, Dir::W});
    switch (dir) {
      case Dir::NE: if (!dirs.contains(ne)) continue;
        FALLTHROUGH;
      case Dir::SE: if (!dirs.contains(se)) continue;
        FALLTHROUGH;
      case Dir::NW: if (!dirs.contains(nw)) continue;
        FALLTHROUGH;
      case Dir::SW: if (!dirs.contains(sw)) continue;
        FALLTHROUGH;
      default: break;
    }
    renderer.drawTile(pos, tile2.getSpriteCoord(DirSet::oneElement(dir)), size);
  }
}

bool MapGui::isFoW(Vec2 pos) const {
  return !pos.inRectangle(Level::getMaxBounds()) || fogOfWar.getValue(pos);
}

void MapGui::renderExtraBorders(Renderer& renderer, milliseconds currentTimeReal) {
  extraBorderPos.clear();
  for (Vec2 wpos : layout->getAllTiles(getBounds(), levelBounds, getScreenPos()))
    if (objects[wpos] && objects[wpos]->hasObject(ViewLayer::FLOOR_BACKGROUND)) {
      ViewId viewId = objects[wpos]->getObject(ViewLayer::FLOOR_BACKGROUND).id();
      if (Tile::getTile(viewId, true).hasExtraBorders())
        for (Vec2 v : wpos.neighbors4())
          if (v.inRectangle(extraBorderPos.getBounds())) {
            if (extraBorderPos.isDirty(v))
              extraBorderPos.getDirtyValue(v).push_back(viewId);
            else
              extraBorderPos.setValue(v, {viewId});
          }
    }
  for (Vec2 wpos : layout->getAllTiles(getBounds(), levelBounds, getScreenPos()))
    for (ViewId id : extraBorderPos.getValue(wpos)) {
      const Tile& tile = Tile::getTile(id, true);
      for (ViewId underId : tile.getExtraBorderIds())
        if (connectionMap.has(wpos, underId)) {
          DirSet dirs = 0;
          for (Vec2 v : Vec2::directions4())
            if ((wpos + v).inRectangle(levelBounds) && connectionMap.has(wpos + v, id))
              dirs.insert(v.getCardinalDir());
          if (auto coord = tile.getExtraBorderCoord(dirs)) {
            Vec2 pos = projectOnScreen(wpos);
            renderer.drawTile(pos, *coord, layout->getSquareSize());
          }
        }
    }
}

Vec2 MapGui::projectOnScreen(Vec2 wpos) {
  double x = wpos.x;
  double y = wpos.y;
  /*if (screenMovement) {
    if (curTime >= screenMovement->startTimeReal && curTime <= screenMovement->endTimeReal) {
      double state = (double)(curTime - screenMovement->startTimeReal).count() /
          (double) (screenMovement->endTimeReal - screenMovement->startTimeReal).count();
      x += (1 - state) * (screenMovement->to.x - screenMovement->from.x);
      y += (1 - state) * (screenMovement->to.y - screenMovement->from.y);
    }
  }*/
  return layout->projectOnScreen(getBounds(), getScreenPos(), x, y);
}

optional<ViewId> MapGui::getHighlightedFurniture() {
  if (auto mousePos = getMousePos()) {
    Vec2 curPos = layout->projectOnMap(getBounds(), getScreenPos(), *mousePos);
    if (curPos.inRectangle(objects.getBounds()) &&
        objects[curPos] &&
        objects[curPos]->hasObject(ViewLayer::FLOOR) &&
        (objects[curPos]->getHighlight(HighlightType::CLICKABLE_FURNITURE) > 0 ||
         (objects[curPos]->getHighlight(HighlightType::CREATURE_DROP) > 0 && !!getDraggedCreature())))
      return objects[curPos]->getObject(ViewLayer::FLOOR).id();
  }
  return none;
}

bool MapGui::isRenderedHighlight(const ViewIndex& index, HighlightType type) {
  if (index.getHighlight(type) > 0)
    switch (type) {
      case HighlightType::CLICKABLE_FURNITURE:
        return
            index.hasObject(ViewLayer::FLOOR) &&
            getHighlightedFurniture() == index.getObject(ViewLayer::FLOOR).id() &&
            !getDraggedCreature() &&
            !buttonViewId;
      case HighlightType::CREATURE_DROP:
        return !!getDraggedCreature();
      default: return true;
    }
  else
    return false;
}

bool MapGui::isRenderedHighlightLow(const ViewIndex& index, HighlightType type) {
  switch (type) {
    case HighlightType::PRIORITY_TASK:
      return index.getHighlight(HighlightType::DIG) == 0;
    case HighlightType::CLICKABLE_FURNITURE:
    case HighlightType::CREATURE_DROP:
    case HighlightType::FORBIDDEN_ZONE:
    case HighlightType::FETCH_ITEMS:
    case HighlightType::PERMANENT_FETCH_ITEMS:
    case HighlightType::STORAGE_EQUIPMENT:
    case HighlightType::STORAGE_RESOURCES:
    case HighlightType::CLICKED_FURNITURE:
    case HighlightType::CUT_TREE:
      return true;
    default: return false;
  }
}

void MapGui::renderTexturedHighlight(Renderer& renderer, Vec2 pos, Vec2 size, Color color) {
  if (spriteMode)
    renderer.drawTile(pos, Tile::getTile(ViewId::DIG_MARK, true).getSpriteCoord(), size, color);
  else
    renderer.addQuad(Rectangle(pos, pos + size), color);
}

void MapGui::renderHighlight(Renderer& renderer, Vec2 pos, Vec2 size, const ViewIndex& index, HighlightType highlight) {
  auto color = getHighlightColor(index, highlight);
  switch (highlight) {
    case HighlightType::MEMORY:
    case HighlightType::POISON_GAS:
    case HighlightType::NIGHT:
      renderer.addQuad(Rectangle(pos, pos + size), color);
      break;
/*    case HighlightType::CUT_TREE:
      if (spriteMode && index.hasObject(ViewLayer::FLOOR))
        break;
      FALLTHROUGH;*/
    default:
      renderTexturedHighlight(renderer, pos, size, color);
      break;
  }
}

void MapGui::renderHighlights(Renderer& renderer, Vec2 size, milliseconds currentTimeReal, bool lowHighlights) {
  Rectangle allTiles = layout->getAllTiles(getBounds(), levelBounds, getScreenPos());
  Vec2 topLeftCorner = projectOnScreen(allTiles.topLeft());
  for (Vec2 wpos : allTiles)
    if (auto& index = objects[wpos])
      if (index->hasAnyHighlight()) {
        Vec2 pos = topLeftCorner + (wpos - allTiles.topLeft()).mult(size);
        for (HighlightType highlight : ENUM_ALL(HighlightType))
          if (isRenderedHighlight(*index, highlight)  && isRenderedHighlightLow(*index, highlight) == lowHighlights)
            renderHighlight(renderer, pos, size, *index, highlight);
      }
  for (Vec2 wpos : lowHighlights ? tutorialHighlightLow : tutorialHighlightHigh) {
    Vec2 pos = topLeftCorner + (wpos - allTiles.topLeft()).mult(size);
    if ((currentTimeReal.count() / 1000) % 2 == 0)
      renderTexturedHighlight(renderer, pos, size, Color(255, 255, 0, lowHighlights ? 120 : 40));
  }
  renderer.drawQuads();
}

void MapGui::renderAnimations(Renderer& renderer, milliseconds currentTimeReal) {
  animations = std::move(animations).filter([=](const AnimationInfo& elem)
      { return !elem.animation->isDone(currentTimeReal);});
  for (auto& elem : animations)
    elem.animation->render(
        renderer,
        getBounds(),
        projectOnScreen(elem.position),
        currentTimeReal);
}

MapGui::HighlightedInfo MapGui::getHighlightedInfo(Vec2 size, milliseconds currentTimeReal) {
  HighlightedInfo ret {};
  Rectangle allTiles = layout->getAllTiles(getBounds(), levelBounds, getScreenPos());
  Vec2 topLeftCorner = projectOnScreen(allTiles.topLeft());
  if (auto mousePos = getMousePos())
    if (mouseUI) {
      ret.tilePos = layout->projectOnMap(getBounds(), getScreenPos(), *mousePos);
      if (!buttonViewId && ret.tilePos->inRectangle(objects.getBounds()))
        for (Vec2 wpos : Rectangle(*ret.tilePos - Vec2(2, 2), *ret.tilePos + Vec2(2, 2))
            .intersection(objects.getBounds())) {
          Vec2 pos = topLeftCorner + (wpos - allTiles.topLeft()).mult(size);
          if (objects[wpos] && objects[wpos]->hasObject(ViewLayer::CREATURE)) {
            const ViewObject& object = objects[wpos]->getObject(ViewLayer::CREATURE);
            Vec2 movement = getMovementOffset(object, size, currentTimeGame, currentTimeReal, true);
            if (mousePos->inRectangle(Rectangle(pos + movement, pos + movement + size))) {
              ret.tilePos = none;
              ret.object = object;
              ret.creaturePos = pos + movement;
              break;
            }
          }
        }
    }
  return ret;
}

void MapGui::renderMapObjects(Renderer& renderer, Vec2 size, milliseconds currentTimeReal) {
  Rectangle allTiles = layout->getAllTiles(getBounds(), levelBounds, getScreenPos());
  Vec2 topLeftCorner = projectOnScreen(allTiles.topLeft());
  renderer.drawFilledRectangle(getBounds(), Color::BLACK);
  renderer.drawFilledRectangle(Rectangle(
        projectOnScreen(levelBounds.topLeft()),
        projectOnScreen(levelBounds.bottomRight())), Color::BLACK);
  fogOfWar.clear();
  for (ViewLayer layer : layout->getLayers()) {
    for (Vec2 wpos : allTiles) {
      Vec2 pos = topLeftCorner + (wpos - allTiles.topLeft()).mult(size);
      if (!objects[wpos] || objects[wpos]->noObjects()) {
        if (layer == layout->getLayers().back()) {
          if (wpos.inRectangle(levelBounds))
            renderer.addQuad(Rectangle(pos, pos + size), Color::BLACK);
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
        drawObjectAbs(renderer, pos, *object, size, wpos, currentTimeReal);
        if (lastHighlighted.tilePos == wpos && object->layer() != ViewLayer::CREATURE)
          lastHighlighted.object = *object;
      }
      if (spriteMode && layer == layout->getLayers().back())
        if (!isFoW(wpos))
          drawFoWSprite(renderer, pos, size, DirSet(
              !isFoW(wpos + Vec2(Dir::N)),
              !isFoW(wpos + Vec2(Dir::S)),
              !isFoW(wpos + Vec2(Dir::E)),
              !isFoW(wpos + Vec2(Dir::W)),
              isFoW(wpos + Vec2(Dir::NE)),
              isFoW(wpos + Vec2(Dir::NW)),
              isFoW(wpos + Vec2(Dir::SE)),
              isFoW(wpos + Vec2(Dir::SW))));
    }
    if (layer == ViewLayer::FLOOR || !spriteMode) {
      if (!buttonViewId && lastHighlighted.creaturePos)
        drawCreatureHighlight(renderer, *lastHighlighted.creaturePos, size, Color::ALMOST_WHITE);
      if (lastHighlighted.tilePos && (!getHighlightedFurniture() || !!buttonViewId))
        drawSquareHighlight(renderer, topLeftCorner + (*lastHighlighted.tilePos - allTiles.topLeft()).mult(size),
            size);
    }
    if (layer == ViewLayer::FLOOR_BACKGROUND)
      renderHighlights(renderer, size, currentTimeReal, true);
    if (!spriteMode)
      break;
    if (layer == ViewLayer::FLOOR_BACKGROUND)
      renderExtraBorders(renderer, currentTimeReal);
  }
  renderer.drawQuads();
}

void MapGui::drawCreatureHighlight(Renderer& renderer, Vec2 pos, Vec2 size, Color color) {
  if (spriteMode)
    renderer.drawViewObject(pos + Vec2(0, size.y / 5), ViewId::CREATURE_HIGHLIGHT, true, size, color);
  else
    renderer.drawFilledRectangle(Rectangle(pos, pos + size), Color::TRANSPARENT, color);
}

void MapGui::drawSquareHighlight(Renderer& renderer, Vec2 pos, Vec2 size) {
  if (spriteMode)
    renderer.drawViewObject(pos, ViewId::SQUARE_HIGHLIGHT, true, size, Color::ALMOST_WHITE);
  else
    renderer.drawFilledRectangle(Rectangle(pos, pos + size), Color::TRANSPARENT, Color::LIGHT_GRAY);
}

void MapGui::considerRedrawingSquareHighlight(Renderer& renderer, milliseconds currentTimeReal, Vec2 pos, Vec2 size) {
  Rectangle allTiles = layout->getAllTiles(getBounds(), levelBounds, getScreenPos());
  Vec2 topLeftCorner = projectOnScreen(allTiles.topLeft());
  for (Vec2 v : concat({pos}, pos.neighbors8()))
    if (v.inRectangle(objects.getBounds()) && (!objects[v] || objects[v]->noObjects())) {
      drawSquareHighlight(renderer, topLeftCorner + (pos - allTiles.topLeft()).mult(size), size);
      break;
    }
}

void MapGui::processScrolling(milliseconds time) {
  if (!!softCenter && !!lastScrollUpdate) {
    double offsetx = softCenter->x - center.x;
    double offsety = softCenter->y - center.y;
    double offset = sqrt(offsetx * offsetx + offsety * offsety);
    if (offset < 0.1)
      softCenter = none;
    else {
      double timeDiff = (time - *lastScrollUpdate).count();
      double moveDist = min(offset, max(offset, 4.0) * 10 * timeDiff / 1000);
      offsetx /= offset;
      offsety /= offset;
      center.x += offsetx * moveDist;
      center.y += offsety * moveDist;
    }
    lastScrollUpdate = time;
  }
}

optional<UniqueEntity<Creature>::Id> MapGui::getDraggedCreature() const {
  if (auto draggedContent = guiFactory->getDragContainer().getElement())
    switch (draggedContent->getId()) {
      case DragContentId::CREATURE:
        return draggedContent->get<UniqueEntity<Creature>::Id>();
      default:
        break;
    }
  return none;
}

void MapGui::setDraggedCreature(UniqueEntity<Creature>::Id id, ViewId viewId, Vec2 origin) {
  guiFactory->getDragContainer().put({DragContentId::CREATURE, id}, guiFactory->viewObject(viewId), origin);
}

void MapGui::considerScrollingToCreature() {
  if (auto& info = centeredCreaturePosition) {
    Vec2 size = layout->getSquareSize();
    Vec2 offset;
    if (auto index = objects[info->pos])
      if (index->hasObject(ViewLayer::CREATURE))
        offset = getMovementOffset(index->getObject(ViewLayer::CREATURE), size, 0, clock->getRealMillis(), false);
    double targetx = info->pos.x + (double)offset.x / size.x;
    double targety = info->pos.y + (double)offset.y / size.y;
    if (info->softScroll)
      setSoftCenter(targetx, targety);
    else
      setCenter(targetx, targety);
  }
}

void MapGui::render(Renderer& renderer) {
  considerScrollingToCreature();
  Vec2 size = layout->getSquareSize();
  auto currentTimeReal = clock->getRealMillis();
  lastHighlighted = getHighlightedInfo(size, currentTimeReal);
  renderMapObjects(renderer, size, currentTimeReal);
  renderHighlights(renderer, size, currentTimeReal, false);
  renderAnimations(renderer, currentTimeReal);
  if (lastHighlighted.tilePos)
    considerRedrawingSquareHighlight(renderer, currentTimeReal, *lastHighlighted.tilePos, size);
  if (spriteMode && buttonViewId && renderer.getMousePos().inRectangle(getBounds()))
    renderer.drawViewObject(renderer.getMousePos() + Vec2(15, 15), *buttonViewId, spriteMode, size);
  processScrolling(currentTimeReal);
}

void MapGui::updateObject(Vec2 pos, CreatureView* view, milliseconds currentTime) {
  WLevel level = view->getLevel();
  objects[pos].emplace();
  auto& index = *objects[pos];
  view->getViewIndex(pos, index);
  level->setNeedsRenderUpdate(pos, false);
  if (index.hasObject(ViewLayer::FLOOR) || index.hasObject(ViewLayer::FLOOR_BACKGROUND))
    index.setHighlight(HighlightType::NIGHT, 1.0 - view->getLevel()->getLight(pos));
  lastSquareUpdate[pos] = currentTime;
  connectionMap.remove(pos);
  shadowed.erase(pos + Vec2(0, 1));
  if (index.hasObject(ViewLayer::FLOOR)) {
    auto& object = index.getObject(ViewLayer::FLOOR);
    auto& tile = Tile::getTile(object.id());
    if (tile.wallShadow) {
      shadowed.insert(pos + Vec2(0, 1));
    }
    if (auto id = getConnectionId(object.id()))
      connectionMap.add(pos, *id);
  }
  if (index.hasObject(ViewLayer::FLOOR_BACKGROUND)) {
    if (auto id = getConnectionId(index.getObject(ViewLayer::FLOOR_BACKGROUND).id()))
      connectionMap.add(pos, *id);
  }
  if (auto viewId = index.getHiddenId())
    if (auto id = getConnectionId(*viewId))
      connectionMap.add(pos, *id);
}

double MapGui::getDistanceToEdgeRatio(Vec2 pos) {
  Vec2 v = projectOnScreen(pos);
  double ret = 100000;
  auto bounds = getBounds();
  ret = min(ret, fabs((double) v.x - bounds.left()) / bounds.width());
  ret = min(ret, fabs((double) v.x - bounds.right()) / bounds.width());
  ret = min(ret, fabs((double) v.y - bounds.top()) / bounds.height());
  ret = min(ret, fabs((double) v.y - bounds.bottom()) / bounds.height());
  if (!v.inRectangle(bounds))
    ret = -ret;
  return ret;
}

void MapGui::updateObjects(CreatureView* view, MapLayout* mapLayout, bool smoothMovement, bool ui,
    const optional<TutorialInfo>& tutorial) {
  if (tutorial) {
    tutorialHighlightLow = tutorial->highlightedSquaresLow;
    tutorialHighlightHigh = tutorial->highlightedSquaresHigh;
  } else {
    tutorialHighlightLow.clear();
    tutorialHighlightHigh.clear();
  }
  WLevel level = view->getLevel();
  levelBounds = view->getLevel()->getBounds();
  mouseUI = ui;
  layout = mapLayout;
  auto currentTimeReal = clock->getRealMillis();
  if (view != previousView || level != previousLevel)
    for (Vec2 pos : level->getBounds())
      level->setNeedsRenderUpdate(pos, true);
  else
    for (Vec2 pos : mapLayout->getAllTiles(getBounds(), Level::getMaxBounds(), getScreenPos()))
      if (level->needsRenderUpdate(pos) || lastSquareUpdate[pos] < currentTimeReal - milliseconds{1000})
        updateObject(pos, view, currentTimeReal);
  previousView = view;
  if (previousLevel != level) {
    screenMovement = none;
    clearCenter();
    setCenter(view->getPosition());
    previousLevel = level;
    mouseOffset = {0, 0};
  }
  keyScrolling = view->getCenterType() == CreatureView::CenterType::NONE;
  bool newTurn = false;
  {
    double newCurrentTimeGame = smoothMovement ? view->getLocalTime() : 1000000000;
    if (currentTimeGame != newCurrentTimeGame) {
      lastEndTimeGame = currentTimeGame;
      currentTimeGame = newCurrentTimeGame;
      newTurn = true;
    }
  }
  if (smoothMovement && view->getCenterType() != CreatureView::CenterType::NONE) {
    if (!screenMovement || newTurn) {
      screenMovement = ScreenMovement {
        clock->getRealMillis(),
        clock->getRealMillis() + milliseconds{100},
        lastEndTimeGame
      };
    }
  } else
    screenMovement = none;
  if (view->getCenterType() == CreatureView::CenterType::FOLLOW) {
    if (centeredCreaturePosition) {
      centeredCreaturePosition->pos = view->getPosition();
      if (newTurn)
        centeredCreaturePosition->softScroll = false;
    } else
      centeredCreaturePosition = CenteredCreatureInfo { view->getPosition(), true };
  } else {
    centeredCreaturePosition = none;
    if (!isCentered() ||
        (view->getCenterType() == CreatureView::CenterType::STAY_ON_SCREEN && getDistanceToEdgeRatio(view->getPosition()) < 0.33)) {
      setSoftCenter(view->getPosition());
    }
  }
}

