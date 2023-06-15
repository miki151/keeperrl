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
#include "util.h"
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
#include "model.h"
#include "creature_status.h"
#include "game.h"
#include "tileset.h"
#include "mouse_button_id.h"
#include "phylactery_info.h"

#include "fx_manager.h"
#include "fx_view_manager.h"
#include "fx_renderer.h"
#include "keybinding_map.h"

using SDL::SDL_Keysym;
using SDL::SDL_Keycode;

MapGui::MapGui(Callbacks call, SyncQueue<UserInput>& inputQueue, Clock* c, Options* o, GuiFactory* f,
    unique_ptr<fx::FXRenderer> fxRenderer, unique_ptr<FXViewManager> fxViewManager)
    : objects(Level::getMaxBounds()), callbacks(call), inputQueue(inputQueue),
    clock(c), options(o), fogOfWar(Level::getMaxBounds(), false), extraBorderPos(Level::getMaxBounds(), {}),
    lastSquareUpdate(Level::getMaxBounds()), connectionMap(Level::getMaxBounds()), guiFactory(f),
    fxRenderer(std::move(fxRenderer)), fxViewManager(std::move(fxViewManager)) {
  clearCenter();
}

MapGui::~MapGui() = default;

constexpr int fireVar = 50;

static Color getFireColor() {
  return Color(200 + Random.get(-fireVar, fireVar), Random.get(fireVar), Random.get(fireVar), 150);
}

void MapGui::setActiveButton(ViewId id, int index, bool isBuilding) {
  activeButton = ActiveButtonInfo{id, index, isBuilding};
}

void MapGui::clearActiveButton() {
  activeButton = none;
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

static Color blendNightColor(Color color, const ViewIndex& index) {
  color = color.blend(Color(0, 0, 40).transparency(int(index.getNightAmount() * 150)));
  if (index.isHighlight(HighlightType::MEMORY))
    color = color.blend(Color::BLACK.transparency(40));
  return color;
}

static Color getNightColor(const ViewIndex& index) {
  auto color = Color::BLACK.transparency(int(index.getNightAmount() * 150));
  if (index.isHighlight(HighlightType::MEMORY))
    color = color.blend(Color::BLACK.transparency(40));
  return color;
}

void MapGui::addAnimation(const FXSpawnInfo& spawnInfo) {
  auto color = Color::WHITE.transparency(0);
  if (auto index = objects[spawnInfo.position])
    color = getNightColor(*index);
  if (fxViewManager)
    fxViewManager->addUnmanagedFX(spawnInfo, color);
}

optional<Vec2> MapGui::getMousePos() {
  if (lastMouseMove && lastMouseMove->inRectangle(getBounds()))
    return lastMouseMove;
  else
    return none;
}

optional<Vec2> MapGui::projectOnMap(Vec2 screenCoord) {
  if (screenCoord.inRectangle(getBounds())) {
    auto ret = layout->projectOnMap(getBounds(), getScreenPos(), screenCoord);
    if (ret.inRectangle(objects.getBounds()))
      return ret;
  }
  return none;
}

optional<Vec2> MapGui::getHighlightedTile(Renderer&) {
  if (auto pos = getMousePos())
    return layout->projectOnMap(getBounds(), getScreenPos(), *pos);
  else
    return none;
}

Color MapGui::getHighlightColor(Vec2 pos, const ViewIndex& index, HighlightType type) {
  bool buildingSelected = activeButton && activeButton->isBuilding;
  switch (type) {
    case HighlightType::RECT_DESELECTION: return Color::RED.transparency(100);
    case HighlightType::DIG: return Color::YELLOW.transparency(100);
    case HighlightType::FETCH_ITEMS: //return Color(Color::YELLOW).transparency(50);
    case HighlightType::CUT_TREE: return Color::YELLOW.transparency(200);
    case HighlightType::PERMANENT_FETCH_ITEMS: return Color::ORANGE.transparency(100);
    case HighlightType::STORAGE_EQUIPMENT: return Color::BLUE.transparency(100);
    case HighlightType::STORAGE_RESOURCES: return Color::GREEN.transparency(100);
    case HighlightType::QUARTERS: return Color::PINK.transparency((!!quarters && quarters->positions.count(pos)) ? 70 : 30);
    case HighlightType::LEISURE: return Color::DARK_BLUE.transparency(70);
    case HighlightType::RECT_SELECTION: return Color::YELLOW.transparency(100);
    case HighlightType::MEMORY: return Color::BLACK.transparency(80);
    case HighlightType::PRIORITY_TASK: return Color(0, 255, 0, 200);
    case HighlightType::CREATURE_DROP:
      if (index.hasObject(ViewLayer::FLOOR) && getHighlightedFurniture() == index.getObject(ViewLayer::FLOOR).id())
        return Color(0, 255, 0);
      else
        return Color(0, 255, 0, 120);
    case HighlightType::CLICKABLE_FURNITURE: return Color(255, 255, 0, 120);
    case HighlightType::CLICKED_FURNITURE: return Color(255, 255, 0);
    case HighlightType::GUARD_ZONE1: return Color(255, 255, 255, 120);
    case HighlightType::GUARD_ZONE2: return Color::PURPLE.transparency(120);
    case HighlightType::GUARD_ZONE3: return Color::SKY_BLUE.transparency(120);
    case HighlightType::FORBIDDEN_ZONE: return Color(255, 0, 0, 120);
    case HighlightType::TILE_BELOW: return Color(0, 0, 0, 120);
    case HighlightType::UNAVAILABLE: return Color(0, 0, 0, 120);
    case HighlightType::PRISON_NOT_CLOSED:
    case HighlightType::PIGSTY_NOT_CLOSED:
    case HighlightType::TORTURE_UNAVAILABLE: return Color(255, 0, 0);
    case HighlightType::INDOORS: return Color(0, 0, 255, buildingSelected ? 40 : 0);
    default:
      return Color::TRANSPARENT;
  }
}

static ViewId getConnectionId(const ViewId& id, const Tile& tile) {
  return tile.getConnectionId().value_or(id);
}

DirSet MapGui::getConnectionSet(Vec2 tilePos, const ViewId& id, const Tile& tile) {
  DirSet ret;
  int cnt = 0;
  for (Vec2 dir : Vec2::directions8()) {
    Vec2 pos = tilePos + dir;
    if (pos.inRectangle(levelBounds) && connectionMap[pos].count(getConnectionId(id, tile)))
      ret.insert((Dir) cnt);
    ++cnt;
  }
  return ret;
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
  const double scrollDist = 9.0 * 32 / layout->getSquareSize().x;
  if (!keyScrolling)
    return false;
  auto keybindings = guiFactory->getKeybindingMap();
  if (keybindings->matches(Keybinding("SCROLL_MAP_UP"), key))
    softScroll(0, -scrollDist);
  if (keybindings->matches(Keybinding("SCROLL_MAP_DOWN"), key))
    softScroll(0, scrollDist);
  if (keybindings->matches(Keybinding("SCROLL_MAP_LEFT"), key))
    softScroll(-scrollDist, 0);
  if (keybindings->matches(Keybinding("SCROLL_MAP_RIGHT"), key))
    softScroll(scrollDist, 0);
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
    lastRightClick = none;
    scrollingState = ScrollingState::NONE;
    return true;
  }
  lastRightClick = clock->getRealMillis();
  if (pos.inRectangle(getBounds())) {
    lastMousePos = pos;
    scrollingState = ScrollingState::ACTIVE;
    mouseOffset.x = mouseOffset.y = 0;
    return true;
  }
  return false;
}

bool MapGui::onMiddleClick(Vec2 pos) {
  if (pos.inRectangle(getBounds())) {
    callbacks.middleClickFun(pos);
    return true;
  } else
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

bool MapGui::onMouseMove(Vec2 v, Vec2 rel) {
  if (rel != Vec2(0, 0))
    lastMouseMove = v;
  auto draggedCreature = isDraggedCreature();
  if (v.inRectangle(getBounds()) && mouseHeldPos && !draggedCreature)
    considerContinuousLeftClick(v);
  if (!draggedCreature && draggedCandidate && mouseHeldPos && mouseHeldPos->distD(v) > 30) {
    inputQueue.push(UserInput(UserInputId::CREATURE_DRAG, draggedCandidate->id));
    guiFactory->getDragContainer().put(draggedCandidate->id, guiFactory->viewObject(draggedCandidate->viewId), v);
  }
  if (scrollingState == ScrollingState::ACTIVE) {
    mouseOffset.x = double(v.x - lastMousePos.x) / layout->getSquareSize().x;
    mouseOffset.y = double(v.y - lastMousePos.y) / layout->getSquareSize().y;
    callbacks.refreshFun();
  }
  return false;
}

optional<MapGui::CreatureInfo> MapGui::getCreature(Vec2 mousePos) {
  auto info = getHighlightedInfo(layout->getSquareSize(), clock->getRealMillis());
  if (info.tilePos && info.creaturePos && info.object && info.object->hasModifier(ViewObject::Modifier::CREATURE))
    return CreatureInfo {UniqueEntity<Creature>::Id(*info.object->getGenericId()), info.object->id(), *info.tilePos};
  else
    return none;
}

void MapGui::onMouseRelease(Vec2 v) {
  if (scrollingState == ScrollingState::ACTIVE) {
    if (fabs(mouseOffset.x) + fabs(mouseOffset.y) < 1) {
      if (auto c = getCreature(lastMousePos))
        inputQueue.push(UserInput(UserInputId::CREATURE_MAP_CLICK_EXTENDED, c->position));
      else
        callbacks.rightClickFun(layout->projectOnMap(getBounds(), getScreenPos(), lastMousePos));
    } else {
      center.x = min<double>(levelBounds.right(), max(0.0, center.x - mouseOffset.x));
      center.y = min<double>(levelBounds.bottom(), max(0.0, center.y - mouseOffset.y));
    }
    scrollingState = ScrollingState::AFTER;
    callbacks.refreshFun();
    mouseOffset.x = mouseOffset.y = 0;
  }
  if (auto& draggedElem = guiFactory->getDragContainer().getElement())
    if (v.inRectangle(getBounds()) && guiFactory->getDragContainer().getOrigin().distD(v) > 10)
      draggedElem->visit<void>(
          [&](UniqueEntity<Creature>::Id id) {
            inputQueue.push(UserInput(UserInputId::CREATURE_DRAG_DROP,
               CreatureDropInfo{layout->projectOnMap(getBounds(), getScreenPos(), v), id}));
          },
          [&](const string& group) {
            inputQueue.push(UserInput(UserInputId::CREATURE_GROUP_DRAG_ON_MAP,
               CreatureGroupDropInfo{layout->projectOnMap(getBounds(), getScreenPos(), v), group}));
          },
          [&](TeamId team) {
            inputQueue.push(UserInput(UserInputId::TEAM_DRAG_DROP,
                TeamDropInfo{layout->projectOnMap(getBounds(), getScreenPos(), v), team}));
          }
      );
  if (mouseHeldPos) {
    if (mouseHeldPos->distD(v) > 10) {
      if (!isDraggedCreature())
        considerContinuousLeftClick(v);
    } else if (!!lastMouseMove) {
      if (auto c = getCreature(*mouseHeldPos))
        inputQueue.push(UserInput(UserInputId::CREATURE_MAP_CLICK, c->position));
      else {
        callbacks.leftClickFun(layout->projectOnMap(getBounds(), getScreenPos(), *mouseHeldPos));
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

static Vec2 getAttachmentOffset(Dir dir, Vec2 size) {
  switch (dir) {
    case Dir::N: return Vec2(0, -size.y * 3 / 4);
    case Dir::S: return Vec2(0, size.y / 3);
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

static optional<double> getPartialMovement(MovementInfo::Type type) {
  switch (type) {
    case MovementInfo::Type::ATTACK:
      return 0.8;
    case MovementInfo::Type::WORK:
      return 0.5;
    default:
      return none;
  }
}

Vec2 MapGui::getMovementOffset(const ViewObject& object, Vec2 size, double time, milliseconds curTimeReal,
    bool verticalMovement, Vec2 pos) {
  PROFILE;
  if (auto dir = object.getAttachmentDir())
    return getAttachmentOffset(*dir, size);
  if (!object.hasAnyMovementInfo())
    return Vec2(0, 0);
  double state;
  Vec2 dir;
  auto& movementInfo = object.getLastMovementInfo();
  auto gid = object.getGenericId();
  if (screenMovement &&
      curTimeReal >= screenMovement->startTimeReal &&
      curTimeReal <= screenMovement->endTimeReal) {
    state = (double) (curTimeReal - screenMovement->startTimeReal).count() /
          (double) (screenMovement->endTimeReal - screenMovement->startTimeReal).count();
    dir = object.getMovementInfo(screenMovement->moveCounter);
  }
  else if (!screenMovement) {
    dir = movementInfo.getDir();
/*    if (info.direction.length8() == 0 || time >= info.tEnd + 0.001 || time <= info.tBegin - 0.001)
      return Vec2(0, 0);*/
    state = (time - movementInfo.tBegin) / (movementInfo.tEnd - movementInfo.tBegin);
    const double stopTime = movementInfo.type == MovementInfo::Type::MOVE ? 0.0 : 0.3;
    double stopTime1 = stopTime / 2;
    if (gid)
      // randomize the animation time frame a bit so creatures don't move synchronously
      stopTime1 += -stopTime / 2 + (std::abs(*gid) % 100) * 0.01 * stopTime;
    double stopTime2 = stopTime - stopTime1;
    state = min(1.0, max(0.0, (state - stopTime1) / (1.0 - stopTime1 - stopTime2)));
    INFO << "Anim time b: " << movementInfo.tBegin << " e: " << movementInfo.tEnd << " t: " << time;
  } else
    return Vec2(0, 0);
  double vertical = (verticalMovement && !object.hasModifier(ViewObject::Modifier::FLYING))
      ? getJumpOffset(object, state) : 0;
  if (dir.length8() == 1) {
    if (state >= 0.5 && state < 1.0) {
      if (movementInfo.victim)
        woundedInfo.getOrInit(UniqueEntity<Creature>::Id(movementInfo.victim)) = curTimeReal;
      if (movementInfo.fx && gid && furnitureUsageFX.getOrElse(*gid, -1) < movementInfo.moveCounter) {
        furnitureUsageFX.getOrInit(*gid) = movementInfo.moveCounter;
        addAnimation(FXSpawnInfo(getFXInfo(*movementInfo.fx), pos + dir));
      }
    }
    if (auto mult = getPartialMovement(movementInfo.type)) {
      if (verticalMovement)
        return Vec2(*mult * (state < 0.5 ? state : 1 - state) * dir.x * size.x,
            (*mult * (state < 0.5 ? state : 1 - state)* dir.y - vertical) * size.y);
      else
        return Vec2(0, 0);
    }
  }
  return Vec2((state - 1) * dir.x * size.x, ((state - 1)* dir.y - vertical) * size.y);
}

void MapGui::drawCreatureHighlights(Renderer& renderer, const ViewObject& object, const ViewIndex& index,
    Vec2 pos, Vec2 sz,
    milliseconds curTime) {
  if (spriteMode)
    if (auto mod = object.getAttribute(ViewObject::Attribute::FLANKED_MOD)) {
      auto tmpColor = blendNightColor(Color::RED, index);
      renderer.drawViewObject(pos + Vec2(0, sz.y / 5), ViewId("flanked_highlight"), true, sz, tmpColor);
    }
  for (auto status : object.getCreatureStatus()) {
    drawCreatureHighlight(renderer, pos, sz, getColor(status).transparency(200), index);
    break;
  }
  if (object.getCreatureStatus().isEmpty() && object.hasModifier(ViewObject::Modifier::HOSTILE))
    drawCreatureHighlight(renderer, pos, sz, Color::ORANGE.transparency(200), index);
  if (object.hasModifier(ViewObject::Modifier::PLAYER)) {
      drawCreatureHighlight(renderer, pos, sz, Color::YELLOW.transparency(200), index);
  }
  else if (object.hasModifier(ViewObject::Modifier::PLAYER_BLINK)) {
    if ((curTime.count() / 500) % 2 == 0)
      drawCreatureHighlight(renderer, pos, sz, Color::YELLOW.transparency(200), index);
  }
  else if (object.hasModifier(ViewObject::Modifier::TEAM_HIGHLIGHT))
    drawCreatureHighlight(renderer, pos, sz, Color::YELLOW.transparency(100), index);
  if (object.hasModifier(ViewObject::Modifier::CREATURE))
    if (isCreatureHighlighted(*object.getGenericId()))
      drawCreatureHighlight(renderer, pos, sz, Color::YELLOW.transparency(200), index);
  if (!spriteMode)
    if (auto mod = object.getAttribute(ViewObject::Attribute::FLANKED_MOD)) {
      auto tmpColor = blendNightColor(Color::RED, index);
      renderer.drawText(FontId::SYMBOL_FONT, sz.y - 3, tmpColor, pos + Vec2(sz.x / 2, -2), u8"⛚", Renderer::HOR);
    }
}

bool MapGui::isCreatureHighlighted(UniqueEntity<Creature>::Id creature) {
  return teamHighlight.getMaybe(creature).value_or(0) > 0;
}

bool MapGui::fxesAvailable() const {
  return !!fxRenderer && spriteMode;
}

Color MapGui::getHealthBarColor(double health, bool spirit) {
  auto ret = Color::f(min<double>(1.0, 2 - health * 2), min<double>(1.0, 2 * health), 0);
  if (spirit) {
    swap(ret.g, ret.b);
    ret.g = min(255, ret.g + 100);
  }
  return ret;
}

void MapGui::drawFurnitureCracks(Renderer& renderer, Vec2 tilePos, float state, Vec2 pos, Vec2 size, const ViewIndex& index) {
  const char* tileName = [&] {
    if (state < 0.4)
      return "furniture_cracks2";
    if (state < 0.8)
      return "furniture_cracks1";
    return (const char*)nullptr;
  }();
  if (tileName) {
    auto hash = tilePos.getHash();
    renderer.drawTile(pos, renderer.getTileSet().getTileCoord(tileName), size, blendNightColor(Color::WHITE, index),
        Renderer::SpriteOrientation(hash % 2, (hash / 2) % 2));
  }
}

void MapGui::drawHealthBar(Renderer& renderer, Vec2 tilePos, Vec2 pos, Vec2 size, const ViewObject& object,
    const ViewIndex& index) {
  auto health = object.getAttribute(ViewObject::Attribute::HEALTH);
  if (!health)
    return;
  if (object.hasModifier(ViewObject::Modifier::FURNITURE_CRACKS)) {
    drawFurnitureCracks(renderer, tilePos, *health, pos, size, index);
    return;
  }
  bool capture = object.hasModifier(ViewObject::Modifier::CAPTURE_BAR);
  if (!capture && !object.hasModifier(ViewObject::Modifier::HEALTH_BAR))
    return;
  if (!capture && *health == 1)
    return;
  pos.y -= size.y * 0.2;
  double barWidth = 0.12;
  double barLength = 0.8;
  auto getBar = [&](double state) {
    return Rectangle((int) (pos.x + size.x * (1 - barLength) / 2), pos.y,
        (int) (pos.x + size.x * state * (1 + barLength) / 2), (int) (pos.y + size.y * barWidth));
  };
  auto color = capture ? Color::WHITE : getHealthBarColor(*health, object.hasModifier(ViewObjectModifier::SPIRIT_DAMAGE));
  color = blendNightColor(color, index);
  auto fullRect = getBar(1);
  renderer.drawFilledRectangle(fullRect.minusMargin(-1), Color::TRANSPARENT, Color::BLACK.transparency(100));
  renderer.drawFilledRectangle(fullRect, color.transparency(100));
  if (*health > 0)
    renderer.drawFilledRectangle(getBar(*health), color.transparency(200));
  Rectangle shadowRect(fullRect.bottomLeft() - Vec2(0, 1), fullRect.bottomRight());
  renderer.drawFilledRectangle(shadowRect, Color::BLACK.transparency(100));
}

void MapGui::considerWoundedAnimation(const ViewObject& object, Color& color, milliseconds curTimeReal) {
  const auto woundedAnimLength = milliseconds{40};
  if (object.hasModifier(ViewObject::Modifier::CREATURE))
    if (auto time = woundedInfo.getMaybe(*object.getGenericId()))
      if (*time > curTimeReal - woundedAnimLength)
        color = Color::RED;
}

static double getFlyingMovement(Vec2 size, milliseconds curTimeReal) {
  double range = 0.08;
  double freq = 700;
  return -range * size.y * (3 + (sin(3.1415 / freq * curTimeReal.count()) + 1) / 2);
}

void MapGui::drawObjectAbs(Renderer& renderer, Vec2 pos, const ViewObject& object, Vec2 size, Vec2 movement,
    Vec2 tilePos, milliseconds curTimeReal, const ViewIndex& index) {
  PROFILE;
  auto id = object.id();
  const Tile& tile = renderer.getTileSet().getTile(id, spriteMode);
  Color color = tile.color;
  considerWoundedAnimation(object, color, curTimeReal);
  if (object.hasModifier(ViewObject::Modifier::FROZEN))
    color = Color::SKY_BLUE;
  if (object.hasModifier(ViewObject::Modifier::INVISIBLE) || object.hasModifier(ViewObject::Modifier::HIDDEN))
    color = color.transparency(70);
  else if (tile.translucent > 0)
    color = color.transparency(255 * (1 - tile.translucent));
  else if (object.hasModifier(ViewObject::Modifier::ILLUSION))
    color = color.transparency(150);
  if (object.hasModifier(ViewObject::Modifier::BLOODY) || object.hasModifier(ViewObject::Modifier::UNPAID))
    color = Color(160, 0, 0);
  if (object.hasModifier(ViewObject::Modifier::PLANNED))
    color = color.transparency(100);
  if (auto waterDepth = object.getAttribute(ViewObject::Attribute::WATER_DEPTH))
    if (*waterDepth > 0) {
      Uint8 val = max(0.0, 255.0 - min(2.0f, *waterDepth) * 40);
      color = color * Color(val, val, val);
    }
  color = blendNightColor(color, index);
  const auto zoom = size.y / Renderer::nominalSize;
  if (spriteMode && tile.hasSpriteCoord()) {
    DirSet dirs;
    if (tile.hasAnyConnections() || tile.hasAnyCorners())
      dirs = getConnectionSet(tilePos, id, tile);
    Vec2 move;
    if (object.layer() == ViewLayer::CREATURE)
      drawCreatureHighlights(renderer, object, index, pos + movement, size, curTimeReal);
    if (object.layer() == ViewLayer::CREATURE || tile.roundShadow) {
      auto& coord = renderer.getTileSet().getTileCoord("round_shadow");
      renderer.drawTile(pos + movement, coord, size, blendNightColor(Color(255, 255, 255, 160), index));
    }
    if ((!object.hasModifier(ViewObject::Modifier::STUNNED) && object.layer() == ViewLayer::CREATURE) || tile.moveUp)
      move.y = -4 * zoom;
    renderer.drawTile(pos, tile.getBackgroundCoord(), size, color);
    move += movement;
    if (object.hasModifier(ViewObject::Modifier::FLYING) && !object.hasModifier(ViewObject::Modifier::TURNED_OFF)
        && !object.hasModifier(ViewObject::Modifier::STUNNED))
      move.y += getFlyingMovement(size, curTimeReal);
    if (object.hasModifier(ViewObject::Modifier::IMMOBILE))
      move.y += 11;
    const auto& coord = tile.getSpriteCoord(dirs);
    if (object.hasModifier(ViewObject::Modifier::RIDER)) {
      auto& steedObject = index.getObject(ViewLayer::TORCH2);
      const Tile& steedTile = renderer.getTileSet().getTile(steedObject.id(), spriteMode);
      const auto& steedCoord = steedTile.getSpriteCoord(dirs);
      move.y -= (7 + steedCoord[0].size.y - coord[0].size.y) * zoom;
      move.x -= object.hasModifier(ViewObject::Modifier::FLIPX) ? 2 * zoom : -2 * zoom;
      drawCreatureHighlights(renderer, steedObject, index, pos + movement, size, curTimeReal);
    }
    if (object.weaponViewId && tile.weaponOrigin && !object.hasModifier(ViewObject::Modifier::STUNNED)) {
      const Tile& weaponTile = renderer.getTileSet().getTile(*object.weaponViewId, spriteMode);
      if (weaponTile.weaponOrigin) {
        const auto& weaponCoord = weaponTile.getSpriteCoord(dirs);
        bool flipped = object.hasModifier(ViewObject::Modifier::FLIPX);
        auto orientation = Renderer::SpriteOrientation(false, flipped);
        auto offset = flipped
            ? Vec2(weaponCoord[0].size.x - weaponTile.weaponOrigin->x, weaponTile.weaponOrigin->y) -
              Vec2(coord[0].size.x - tile.weaponOrigin->x, tile.weaponOrigin->y)
            : *weaponTile.weaponOrigin - *tile.weaponOrigin;
        if (coord[0].size.y > Renderer::nominalSize)
          offset.y += 3;
        renderer.drawTile(pos + move - ((coord[0].size - weaponCoord[0].size) / 2 + offset) * zoom, weaponCoord, size, color,
            orientation, object.weaponViewId->getColor());
      }
    }
    if (tile.canMirror)
      renderer.drawTile(pos + move, coord, size, color,
          Renderer::SpriteOrientation((bool)(tilePos.getHash() & 1024), (bool)(tilePos.getHash() & 512)));
    else {
      optional<Color> colorVariant;
      if (!tile.animated)
        colorVariant = object.id().getColor();
      auto orientation = object.hasModifier(ViewObject::Modifier::STUNNED) ? 
          Renderer::SpriteOrientation(Vec2(0, -1), false) :
          Renderer::SpriteOrientation(false, object.hasModifier(ViewObject::Modifier::FLIPX));
      renderer.drawTile(pos + move, coord, size, color, orientation, colorVariant);
      for (auto& id : object.partIds) {
        auto& partTile = renderer.getTileSet().getTile(id, true);
        const auto& partCoord = partTile.getSpriteCoord(dirs);
        renderer.drawTile(pos + move, partCoord, size, color, orientation, id.getColor());
      }
    }
    if (tile.hasAnyCorners()) {
      for (auto coord : tile.getCornerCoords(dirs))
        renderer.drawTile(pos + move, {coord}, size, color);
    }
    if (object.hasModifier(ViewObject::Modifier::AURA))
      renderer.drawTile(pos + move, renderer.getTileSet().getTileCoord("item_aura"), size);
    auto& shortShadow = renderer.getTileSet().getTileCoord("short_shadow");
    if (object.layer() == ViewLayer::FLOOR_BACKGROUND && shadowed.count(tilePos))
      renderer.drawTile(pos, shortShadow, size, Color(255, 255, 255, 170));
    auto burning = object.getAttribute(ViewObject::Attribute::BURNING);
    drawHealthBar(renderer, tilePos, pos + move, size, object, index);
    if (curTimeReal.count() % 2000 < 800 && object.hasModifier(ViewObject::Modifier::TURNED_OFF)) {
      auto& icon = renderer.getTileSet().getTileCoord("power_off");
      renderer.drawTile(pos + move, icon, size);
    }
    if (object.hasModifier(ViewObject::Modifier::LOCKED))
      renderer.drawTile(pos + move, renderer.getTileSet().getTile(ViewId("key"), spriteMode).getSpriteCoord(), size);
    if (fxViewManager)
      if (auto genericId = object.getGenericId()) {
        float fxPosX = tilePos.x + move.x / (float)size.x;
        float fxPosY = tilePos.y + move.y / (float)size.y;

        fxViewManager->addEntity(*genericId, fxPosX, fxPosY);
        if (!object.hasModifier(ViewObject::Modifier::PLANNED))
          if (auto& fxInfo = tile.getFX())
            fxViewManager->addFX(*genericId, *fxInfo);
        if (burning)
          fxViewManager->addFX(*genericId, FXInfo{FXName::FIRE, Color::WHITE, 0.3f + 0.7f * *burning});
        bool bigTile = coord.front().size.x > Renderer::nominalSize;
        for (auto fx : object.particleEffects)
          fxViewManager->addFX(*genericId, fx, bigTile);
        fxViewManager->drawFX(renderer, *genericId, blendNightColor(Color::WHITE, index));
    }
  } else {
    Vec2 tilePos = pos + movement + Vec2(size.x / 2, -3);
    drawCreatureHighlights(renderer, object, index, pos + movement, size, curTimeReal);
    auto color = renderer.getTileSet().getColor(object);
    if (object.id().getColor() != Color::WHITE)
      color = object.id().getColor();
    if (object.hasModifier(ViewObject::Modifier::BLOODY))
      color = Color(160, 0, 0);
    renderer.drawText(tile.symFont ? FontId::SYMBOL_FONT : FontId::TILE_FONT, size.y,
        blendNightColor(color, index), tilePos, tile.text, Renderer::HOR);
    if (object.getAttribute(ViewObject::Attribute::BURNING))
      renderer.drawText(FontId::SYMBOL_FONT, size.y, getFireColor(),
          pos + Vec2(size.x / 2, -3), u8"Ѡ", Renderer::HOR);
    if (object.hasModifier(ViewObject::Modifier::LOCKED))
      renderer.drawText(blendNightColor(Color::YELLOW, index), pos + size / 2, "*", Renderer::CenterType::HOR_VER, size.y);
  }
}

void MapGui::resetScrolling() {
  scrollingState = ScrollingState::NONE;
}

void MapGui::clearCenter() {
  center = mouseOffset = {0.0, 0.0};
  softCenter = none;
  screenMovement = none;
  for (auto v : objects.getBounds())
    objects[v].reset();
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

void MapGui::setCenterRatio(double x, double y) {
  setCenter(x * levelBounds.width(), y * levelBounds.height());
}

void MapGui::setCenter(Vec2 pos, Level* level) {
  previousLevel = level;
  levelBounds = previousLevel->getBounds();
  setCenter(pos.x, pos.y);
}

const static EnumMap<Dir, DirSet> adjacentDirs(
    [](Dir dir) {
      switch (dir) {
        case Dir::NE:
          return DirSet({Dir::N, Dir::E});
        case Dir::SE:
          return DirSet({Dir::S, Dir::E});
        case Dir::SW:
          return DirSet({Dir::S, Dir::W});
        case Dir::NW:
          return DirSet({Dir::N, Dir::W});
        default:
          return DirSet();
      }
    });

void MapGui::drawFoWSprite(Renderer& renderer, Vec2 pos, Vec2 size, DirSet dirs, DirSet diagonalDirs) {
  const Tile& tile = renderer.getTileSet().getTile(ViewId("fog_of_war"), true);
  const Tile& tile2 = renderer.getTileSet().getTile(ViewId("fog_of_war_corner"), true);
  auto coord = tile.getSpriteCoord(dirs);
  renderer.drawTile(pos, coord, size);
  for (Dir dir : diagonalDirs)
    if ((~dirs & adjacentDirs[dir]) == 0)
      renderer.drawTile(pos, tile2.getSpriteCoord(DirSet::oneElement(dir)), size);
}

bool MapGui::isFoW(Vec2 pos) const {
  return !pos.inRectangle(Level::getMaxBounds()) || fogOfWar.getValue(pos);
}

void MapGui::renderExtraBorders(Renderer& renderer, milliseconds currentTimeReal) {
  PROFILE;
  extraBorderPos.clear();
  for (Vec2 wpos : layout->getAllTiles(getBounds(), levelBounds, getScreenPos()))
    if (objects[wpos] && objects[wpos]->hasObject(ViewLayer::FLOOR_BACKGROUND)) {
      ViewId viewId = objects[wpos]->getObject(ViewLayer::FLOOR_BACKGROUND).id();
      if (renderer.getTileSet().getTile(viewId, true).hasExtraBorders())
        for (Vec2 v : wpos.neighbors4())
          if (v.inRectangle(extraBorderPos.getBounds())) {
            if (extraBorderPos.isDirty(v))
              extraBorderPos.getDirtyValue(v).push_back(viewId);
            else
              extraBorderPos.setValue(v, {viewId});
          }
    }
  for (Vec2 wpos : layout->getAllTiles(getBounds(), levelBounds, getScreenPos()))
    if (objects[wpos] && objects[wpos]->hasObject(ViewLayer::FLOOR_BACKGROUND))
      for (auto& id : extraBorderPos.getValue(wpos)) {
        auto color = Color::WHITE;
        if (objects[wpos])
          color = blendNightColor(color, *objects[wpos]);
        const Tile& tile = renderer.getTileSet().getTile(id, true);
        for (auto& borderId : tile.getExtraBorderIds())
          if (connectionMap[wpos].count(borderId)) {
            DirSet dirs = 0;
            for (Vec2 v : Vec2::directions4())
              if ((wpos + v).inRectangle(levelBounds) && connectionMap[wpos + v].count(id))
                dirs.insert(v.getCardinalDir());
            Vec2 pos = projectOnScreen(wpos);
            renderer.drawTile(pos, tile.getExtraBorderCoord(dirs), layout->getSquareSize(), color);
            break;
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
        (objects[curPos]->isHighlight(HighlightType::CLICKABLE_FURNITURE) ||
         (objects[curPos]->isHighlight(HighlightType::CREATURE_DROP) && isDraggedCreature())))
      return objects[curPos]->getObject(ViewLayer::FLOOR).id();
  }
  return none;
}

bool MapGui::isRenderedHighlight(const ViewIndex& index, HighlightType type) {
  if (index.isHighlight(type))
    switch (type) {
      case HighlightType::CLICKABLE_FURNITURE:
        return
            index.hasObject(ViewLayer::FLOOR) &&
            getHighlightedFurniture() == index.getObject(ViewLayer::FLOOR).id() &&
            !isDraggedCreature() &&
            !activeButton;
      case HighlightType::CREATURE_DROP:
        return isDraggedCreature();
      default: return true;
    }
  else
    return false;
}

bool MapGui::isRenderedHighlightLow(Renderer& renderer, const ViewIndex& index, HighlightType type) {
  switch (type) {
    case HighlightType::PRIORITY_TASK:
      return !index.isHighlight(HighlightType::DIG);
    case HighlightType::FORBIDDEN_ZONE:
      if (spriteMode && index.hasObject(ViewLayer::FLOOR)) {
        auto tile = renderer.getTileSet().getTile(index.getObject(ViewLayer::FLOOR).id(), spriteMode);
        return !tile.highlightAbove && !tile.wallShadow;
      }
      return !index.noObjects();
    case HighlightType::CLICKABLE_FURNITURE:
    case HighlightType::CREATURE_DROP:
    case HighlightType::FETCH_ITEMS:
    case HighlightType::PERMANENT_FETCH_ITEMS:
    case HighlightType::STORAGE_EQUIPMENT:
    case HighlightType::STORAGE_RESOURCES:
    case HighlightType::QUARTERS:
    case HighlightType::GUARD_ZONE1:
    case HighlightType::GUARD_ZONE2:
    case HighlightType::GUARD_ZONE3:
    case HighlightType::LEISURE:
    case HighlightType::CLICKED_FURNITURE:
    case HighlightType::CUT_TREE:
    case HighlightType::DIG:
      return true;
    case HighlightType::RECT_DESELECTION:
    case HighlightType::RECT_SELECTION:
    case HighlightType::UNAVAILABLE:
    case HighlightType::MEMORY:
    case HighlightType::TILE_BELOW:
    case HighlightType::INDOORS:
    case HighlightType::INSUFFICIENT_LIGHT:
    case HighlightType::HOSTILE_TOTEM:
    case HighlightType::ALLIED_TOTEM:
    case HighlightType::TORTURE_UNAVAILABLE:
    case HighlightType::PRISON_NOT_CLOSED:
    case HighlightType::PIGSTY_NOT_CLOSED:
      return false;
  }
}

void MapGui::renderTexturedHighlight(Renderer& renderer, Vec2 pos, Vec2 size, Color color, ViewId viewId) {
  if (spriteMode)
    renderer.drawTile(pos, renderer.getTileSet().getTile(viewId, true).getSpriteCoord(), size, color);
  else
    renderer.addQuad(Rectangle(pos, pos + size), color);
}

void MapGui::fxHighlight(Renderer& renderer, const FXInfo& info1, Vec2 tilePos, const ViewIndex& index) {
  if (fxViewManager) {
    GenericId posId = tilePos.x * 1000 + tilePos.y;
    fxViewManager->addEntity(posId, tilePos.x, tilePos.y);
    auto info = info1;
    info.color = blendNightColor(info.color, index);
    fxViewManager->addFX(posId, info);
    fxViewManager->drawFX(renderer, posId, Color::WHITE);
  }
};

void MapGui::renderHighlight(Renderer& renderer, Vec2 pos, Vec2 size, const ViewIndex& index, HighlightType highlight, Vec2 tilePos) {
  auto color = blendNightColor(getHighlightColor(pos, index, highlight), index);
  switch (highlight) {
    case HighlightType::MEMORY:
      break;
    case HighlightType::INDOORS:
      renderTexturedHighlight(renderer, pos, size, color, ViewId("dig_mark2"));
      break;
    case HighlightType::HOSTILE_TOTEM:
      fxHighlight(renderer, FXInfo{FXName::MAGIC_FIELD, Color(255, 100, 100)}, tilePos, index);
      break;
    case HighlightType::ALLIED_TOTEM:
      fxHighlight(renderer, FXInfo{FXName::MAGIC_FIELD, Color(100, 255, 100)}, tilePos, index);
      break;
    case HighlightType::QUARTERS:
    case HighlightType::LEISURE:
    case HighlightType::UNAVAILABLE:
      renderTexturedHighlight(renderer, pos, size, color, ViewId("dig_mark2"));
      break;
    case HighlightType::TILE_BELOW:
      renderTexturedHighlight(renderer, pos, size, color, ViewId("tile_below"));
      break;
    case HighlightType::GUARD_ZONE1:
    case HighlightType::GUARD_ZONE2:
    case HighlightType::GUARD_ZONE3:
      renderTexturedHighlight(renderer, pos, size, color, ViewId("guard_zone"));
      break;
    default:
      renderTexturedHighlight(renderer, pos, size, color, ViewId("dig_mark"));
      break;
  }
}

void MapGui::renderTileGas(Renderer& renderer, Vec2 pos, Vec2 size, const ViewIndex& index, Vec2 tilePos) {
  for (auto& elem : index.getGasAmounts()) {
    auto amount = elem.color.a;
    fxHighlight(renderer, FXInfo{FXName::POISON_CLOUD, elem.color.transparency(255), float(amount) / 255}, tilePos, index);
  }
}

void MapGui::renderHighlights(Renderer& renderer, Vec2 size, milliseconds currentTimeReal, bool lowHighlights) {
  PROFILE;
  Rectangle allTiles = layout->getAllTiles(getBounds(), levelBounds, getScreenPos());
  Vec2 topLeftCorner = projectOnScreen(allTiles.topLeft());
  for (Vec2 wpos : allTiles)
    if (auto& index = objects[wpos])
      if (index->hasAnyHighlight()) {
        Vec2 pos = topLeftCorner + (wpos - allTiles.topLeft()).mult(size);
        for (HighlightType highlight : ENUM_ALL_REVERSE(HighlightType))
          if (isRenderedHighlight(*index, highlight) &&
              isRenderedHighlightLow(renderer, *index, highlight) == lowHighlights)
            renderHighlight(renderer, pos, size, *index, highlight, wpos);
        if (!lowHighlights)
          renderTileGas(renderer, pos, size, *index, wpos);
      }
  for (Vec2 wpos : lowHighlights ? tutorialHighlightLow : tutorialHighlightHigh) {
    Vec2 pos = topLeftCorner + (wpos - allTiles.topLeft()).mult(size);
    if ((currentTimeReal.count() / 1000) % 2 == 0)
      renderTexturedHighlight(renderer, pos, size, Color(255, 255, 0, lowHighlights ? 120 : 80), ViewId("dig_mark"));
  }
}

void MapGui::renderAnimations(Renderer& renderer, milliseconds currentTimeReal) {
  PROFILE;
  animations = std::move(animations).filter([=](const AnimationInfo& elem)
      { return !elem.animation->isDone(currentTimeReal);});
  for (auto& elem : animations) {
    auto color = Color::WHITE;
    if (auto& index = objects[elem.position])
      color = blendNightColor(Color::WHITE, *index);
    elem.animation->render(
        renderer,
        getBounds(),
        projectOnScreen(elem.position) + layout->getSquareSize() / 2,
        layout->getSquareSize(),
        currentTimeReal,
        color);
  }
}

MapGui::HighlightedInfo MapGui::getHighlightedInfo(Vec2 size, milliseconds currentTimeReal) {
  PROFILE;
  HighlightedInfo ret {};
  Rectangle allTiles = layout->getAllTiles(getBounds(), levelBounds, getScreenPos());
  Vec2 topLeftCorner = projectOnScreen(allTiles.topLeft());
  if (auto mousePos = getMousePos())
    if (mouseUI) {
      ret.tilePos = layout->projectOnMap(getBounds(), getScreenPos(), *mousePos);
      ret.tileScreenPos = topLeftCorner + (*ret.tilePos - allTiles.topLeft()).mult(size);
      if (ret.tilePos->inRectangle(objects.getBounds()))
        if (auto& index = objects[*ret.tilePos])
          ret.viewIndex = *index;
      if (!activeButton && ret.tilePos->inRectangle(objects.getBounds()))
        for (Vec2 wpos : Rectangle(*ret.tilePos - Vec2(2, 2), *ret.tilePos + Vec2(2, 2))
            .intersection(objects.getBounds())) {
          Vec2 pos = topLeftCorner + (wpos - allTiles.topLeft()).mult(size);
          if (auto& index = objects[wpos])
            if (objects[wpos]->hasObject(ViewLayer::CREATURE)) {
              const ViewObject& object = objects[wpos]->getObject(ViewLayer::CREATURE);
              Vec2 movement = getMovementOffset(object, size, currentTimeGame, currentTimeReal, true, wpos);
              if (mousePos->inRectangle(Rectangle(pos + movement, pos + movement + size))) {
                ret.tilePos = wpos;
                ret.object = object;
                ret.creaturePos = pos + movement;
                ret.viewIndex = *index;
                break;
              }
          }
        }
    }
  return ret;
}

void MapGui::renderAndInitFoW(Renderer& renderer, Vec2 size) {
  Rectangle allTiles = layout->getAllTiles(getBounds(), levelBounds, getScreenPos());
  Vec2 topLeftCorner = projectOnScreen(allTiles.topLeft());
  fogOfWar.clear();
  for (auto wpos : allTiles) {
    Vec2 pos = topLeftCorner + (wpos - allTiles.topLeft()).mult(size);
    if ((!objects[wpos] || objects[wpos]->noObjects()) && wpos.inRectangle(levelBounds)) {
      renderer.addQuad(Rectangle(pos, pos + size), Color::BLACK);
      fogOfWar.setValue(wpos, true);
    }
  }
}

void MapGui::renderFoWBorders(Renderer& renderer, Vec2 size) {
  Rectangle allTiles = layout->getAllTiles(getBounds(), levelBounds, getScreenPos());
  Vec2 topLeftCorner = projectOnScreen(allTiles.topLeft());   for (auto wpos : allTiles)
  if (!isFoW(wpos)) {
    Vec2 pos = topLeftCorner + (wpos - allTiles.topLeft()).mult(size);
    drawFoWSprite(renderer, pos, size,
        DirSet(
          !isFoW(wpos + Vec2(Dir::N)),
          !isFoW(wpos + Vec2(Dir::S)),
          !isFoW(wpos + Vec2(Dir::E)),
          !isFoW(wpos + Vec2(Dir::W)), false, false, false, false),
        DirSet(false, false, false, false,
          isFoW(wpos + Vec2(Dir::NE)),
          isFoW(wpos + Vec2(Dir::NW)),
          isFoW(wpos + Vec2(Dir::SE)),
          isFoW(wpos + Vec2(Dir::SW))));
  }
}

void MapGui::renderFloorObjects(Renderer& renderer, Vec2 size, milliseconds currentTimeReal) {
  Rectangle allTiles = layout->getAllTiles(getBounds(), levelBounds, getScreenPos());
  Vec2 topLeftCorner = projectOnScreen(allTiles.topLeft());
  auto render = [&] (ViewLayer layer) {
    for (auto wpos : allTiles) {
      Vec2 pos = topLeftCorner + (wpos - allTiles.topLeft()).mult(size);
      if (auto& index = objects[wpos])
        if (index->hasObject(layer)) {
          auto& obj = index->getObject(layer);
          const Tile& tile = renderer.getTileSet().getTile(obj.id(), spriteMode);
          if (layer == ViewLayer::FLOOR_BACKGROUND || !tile.moveUp)
            drawObjectAbs(renderer, pos, obj, size, Vec2(), wpos, currentTimeReal, *index);
        }
    }
  };
  render(ViewLayer::FLOOR_BACKGROUND);
  renderExtraBorders(renderer, currentTimeReal);
  render(ViewLayer::FLOOR);
}

void MapGui::renderAsciiObjects(Renderer& renderer, Vec2 size, milliseconds currentTimeReal) {
  Rectangle allTiles = layout->getAllTiles(getBounds(), levelBounds, getScreenPos());
  Vec2 topLeftCorner = projectOnScreen(allTiles.topLeft());
  auto renderPos = [&] (Vec2 wpos, ViewLayer layer) {
    const ViewIndex& index = *objects[wpos];
    if (index.hasObject(layer)) {
      auto object = index.getObject(layer);
      Vec2 pos = topLeftCorner + (wpos - allTiles.topLeft()).mult(size);
      Vec2 movement = getMovementOffset(object, size, currentTimeGame, currentTimeReal, true, wpos);
      drawObjectAbs(renderer, pos, object, size, movement, wpos, currentTimeReal, index);
      if (lastHighlighted.tilePos == wpos && !lastHighlighted.creaturePos &&
          object.layer() != ViewLayer::CREATURE && object.layer() != ViewLayer::ITEM)
        lastHighlighted.object = object;
      return true;
    }
    return false;
  };
  for (auto wpos : allTiles) {
    if (!objects[wpos] || objects[wpos]->noObjects())
      continue;
    for (ViewLayer layer : {ViewLayer::CREATURE, ViewLayer::ITEM, ViewLayer::FLOOR, ViewLayer::FLOOR_BACKGROUND})
      if (renderPos(wpos, layer)) {
        if (lastHighlighted.tilePos && lastHighlighted.tilePos->y == wpos.y) {
          if (!activeButton && lastHighlighted.creaturePos)
            drawCreatureHighlight(renderer, *lastHighlighted.creaturePos, size, Color::ALMOST_WHITE,
                *objects[*lastHighlighted.tilePos]);
          else if (!getHighlightedFurniture() || !!activeButton)
            drawSquareHighlight(renderer, topLeftCorner + (*lastHighlighted.tilePos - allTiles.topLeft()).mult(size),
                size, squareHighlightColor);
        }
        break;
      }
  }
  for (auto wpos : allTiles) {
    if (!objects[wpos] || objects[wpos]->noObjects())
      continue;
    for (ViewLayer layer : {ViewLayer::TORCH1, ViewLayer::TORCH2})
      renderPos(wpos, layer);
  }
}

void MapGui::renderHighObjects(Renderer& renderer, Vec2 size, milliseconds currentTimeReal) {
  Rectangle allTiles = layout->getAllTiles(getBounds(), levelBounds, getScreenPos());
  Vec2 topLeftCorner = projectOnScreen(allTiles.topLeft());
  for (int ypos : allTiles.getYRange()) {
    for (ViewLayer layer : layout->getLayers())
      if (layer > ViewLayer::FLOOR_BACKGROUND && layer < ViewLayer::CREATURE) {
        for (int xpos : allTiles.getXRange()) {
          auto wpos = Vec2(xpos, ypos);
          if (!objects[wpos] || objects[wpos]->noObjects())
            continue;
          Vec2 pos = topLeftCorner + (wpos - allTiles.topLeft()).mult(size);
          const ViewIndex& index = *objects[wpos];
          const ViewObject* object = nullptr;
          if (index.hasObject(layer))
            object = &index.getObject(layer);
          if (object) {
            const Tile& tile = renderer.getTileSet().getTile(object->id(), spriteMode);
            if (layer != ViewLayer::FLOOR || tile.moveUp) {
              Vec2 movement = getMovementOffset(*object, size, currentTimeGame, currentTimeReal, true, wpos);
              drawObjectAbs(renderer, pos, *object, size, movement, wpos, currentTimeReal, index);
              if (lastHighlighted.tilePos == wpos && !lastHighlighted.creaturePos &&
                  object->layer() != ViewLayer::CREATURE && object->layer() != ViewLayer::ITEM)
                lastHighlighted.object = *object;
              }
          }
        }
        if (lastHighlighted.tilePos && lastHighlighted.tilePos->y == ypos && layer == ViewLayer::FLOOR) {
          if (!activeButton && lastHighlighted.creaturePos)
            drawCreatureHighlight(renderer, *lastHighlighted.creaturePos, size, Color::ALMOST_WHITE,
                *objects[*lastHighlighted.tilePos]);
          else if (!getHighlightedFurniture() || !!activeButton)
            drawSquareHighlight(renderer, topLeftCorner + (*lastHighlighted.tilePos - allTiles.topLeft()).mult(size),
                size, squareHighlightColor);
        }
      }
    for (ViewLayer layer : layout->getLayers())
      if ((int)layer >= (int)ViewLayer::CREATURE) {
        for (int xpos : allTiles.getXRange()) {
          auto wpos = Vec2(xpos, ypos);
          Vec2 pos = topLeftCorner + (wpos - allTiles.topLeft()).mult(size);
          if (!objects[wpos] || objects[wpos]->noObjects()) {
            continue;
          }
          const ViewIndex& index = *objects[wpos];
          const ViewObject* object = nullptr;
          if (index.hasObject(layer))
            object = &index.getObject(layer);
          if (object) {
            Vec2 movement = [&] {
              if (layer == ViewLayer::TORCH2 && index.hasObject(ViewLayer::CREATURE)) {
                auto& riderObj = index.getObject(ViewLayer::CREATURE);
                if (riderObj.hasModifier(ViewObject::Modifier::RIDER))
                  return getMovementOffset(riderObj, size, currentTimeGame, currentTimeReal, true, wpos);
              }
              return getMovementOffset(*object, size, currentTimeGame, currentTimeReal, true, wpos);
            }();
            drawObjectAbs(renderer, pos, *object, size, movement, wpos, currentTimeReal, index);
            if (lastHighlighted.tilePos == wpos && !lastHighlighted.creaturePos && object->layer() != ViewLayer::CREATURE)
              lastHighlighted.object = *object;
          }
        }
      }
  }
}

void MapGui::renderMapObjects(Renderer& renderer, Vec2 size, milliseconds currentTimeReal) {
  PROFILE;
  if (fxViewManager) {
    float zoom = float(layout->getSquareSize().x) / float(Renderer::nominalSize);
    auto offset = projectOnScreen(Vec2(0, 0));
    fxViewManager->beginFrame(renderer, zoom, offset.x, offset.y);
  }
  renderAndInitFoW(renderer, size);
  if (spriteMode) {
    renderFloorObjects(renderer, size, currentTimeReal);
    renderFoWBorders(renderer, size);
  }
  renderHighlights(renderer, size, currentTimeReal, true);
  if (fxViewManager)
    fxViewManager->drawUnorderedBackFX(renderer);
  renderShortestPaths(renderer, size);
  renderPhylacteries(renderer, size, currentTimeReal);
  if (spriteMode)
    renderHighObjects(renderer, size, currentTimeReal);
  else
    renderAsciiObjects(renderer, size, currentTimeReal);
  renderWalkingJoy(renderer, size);
  renderHighlights(renderer, size, currentTimeReal, false);
  if (fxViewManager) {
    fxViewManager->finishFrame();
    fxViewManager->drawUnorderedFrontFX(renderer);
  }
  renderQuarters(renderer);
}

void MapGui::renderQuarters(Renderer& renderer) {
  if (quarters) {
    CHECK(!quarters->positions.empty());
    auto pos = *quarters->positions.begin();
    string name = quarters->name.value_or("Unassigned");
    int length = renderer.getTextLength(name);
    if (quarters->viewId)
      length += 30;
    renderer.drawFilledRectangle(Rectangle(pos, pos + Vec2(length, 20)).minusMargin(-7), Color(0, 0, 0, 150));
    if (quarters->viewId) {
      renderer.drawViewObject(pos, *quarters->viewId, true);
      pos.x += 30;
    }
    renderer.drawText(Color::WHITE, pos, name);
  }

}

void MapGui::renderShortestPaths(Renderer& renderer, Vec2 tileSize) {
  auto renderPath = [&] (const vector<Vec2>& path, Color color, optional<ViewId> target) {
    for (int i : All(path)) {
      auto handle = [&] (Vec2 coord) {
        if (path[i].inRectangle(objects.getBounds()))
          /*if (auto index = objects[path[i]])
            color = blendNightColor(color, *index);*/
        renderer.drawFilledRectangle(Rectangle::centered(coord, tileSize.x / Renderer::nominalSize), color);
      };
      handle(projectOnScreen(path[i]) + tileSize / 2);
      if (i > 0)
        handle((projectOnScreen(path[i]) + projectOnScreen(path[i - 1]) + tileSize) / 2);
      if (target)
        renderer.drawViewObject(projectOnScreen(path[0]), *target, true, tileSize);
    }
  };
  for (auto& path : shortestPath)
    renderPath(path, Color::WHITE.transparency(100), none);
  for (auto& path : permaShortestPath)
    renderPath(path, Color::LIGHT_BLUE.transparency(100), ViewId("guard_post"));
}

void MapGui::renderPhylacteries(Renderer& renderer, Vec2 tileSize, milliseconds currentTimeReal) {
  const double length = tileSize.x * 10;
  const int period = 1000 * length / tileSize.x;
  const double offset = double((-currentTimeReal.count()) % period + period) / period;
  for (auto& info : phylacteries) {
    auto p = projectOnScreen(info.phylactery);
    auto k = projectOnScreen(info.lich);
    if (info.lichObject)
      k += getMovementOffset(*info.lichObject, tileSize, currentTimeGame, currentTimeReal, false, info.lich);
    double dx = k.x - p.x;
    double dy = k.y - p.y;
    double len = sqrt(dx * dx + dy * dy);
    dx = length * dx / len;
    dy = length * dy / len;
    double x = p.x + dx * offset;
    double y = p.y + dy * offset;
    while (((x >= p.x && x <= k.x) || (x <= p.x && x >= k.x))
        && ((y >= p.y && y <= k.y) || (y <= p.y && y >= k.y))) {
      const double fadeDist = tileSize.x;
      auto trans = 255 * min(fadeDist,
          min(sqrt((x - p.x) * (x - p.x) + (y - p.y) * (y - p.y)),
              sqrt((x - k.x) * (x - k.x) + (y - k.y) * (y - k.y)))) / fadeDist;
      renderer.drawViewObject(Vec2(x, y), ViewId("phylactery_tether"), true, tileSize, Color(255, 255, 255, trans));
      x += dx;
      y += dy;
    }
  }
}

void MapGui::drawCreatureHighlight(Renderer& renderer, Vec2 pos, Vec2 size, Color color, const ViewIndex& index) {
  color = blendNightColor(color, index);
  if (spriteMode)
    renderer.drawViewObject(pos + Vec2(0, size.y / 5), ViewId("creature_highlight"), true, size, color);
  else
    renderer.drawFilledRectangle(Rectangle(pos, pos + size), Color::TRANSPARENT, color);
}

void MapGui::drawSquareHighlight(Renderer& renderer, Vec2 pos, Vec2 size, Color color) {
  if (auto wpos = projectOnMap(pos))
    if (auto index = objects[*wpos])
      color = blendNightColor(color, *index);
  if (spriteMode)
    renderer.drawViewObject(pos, ViewId("square_highlight"), true, size, color);
  else
    renderer.drawFilledRectangle(Rectangle(pos, pos + size), Color::TRANSPARENT, color);
}

void MapGui::considerRedrawingSquareHighlight(Renderer& renderer, milliseconds currentTimeReal, Vec2 pos, Vec2 size) {
  PROFILE;
  if (!lastHighlighted.creaturePos) {
    Rectangle allTiles = layout->getAllTiles(getBounds(), levelBounds, getScreenPos());
    Vec2 topLeftCorner = projectOnScreen(allTiles.topLeft());
    for (Vec2 v : concat({pos}, pos.neighbors8()))
      if (v.inRectangle(objects.getBounds()) && (!objects[v] || objects[v]->noObjects())) {
        drawSquareHighlight(renderer, topLeftCorner + (pos - allTiles.topLeft()).mult(size), size, squareHighlightColor);
        break;
      }
  }
}

bool MapGui::onScrollEvent(Vec2 pos, double x, double y, milliseconds timeDiff) {
  if (x != 0.0 || y != 0.0) {
    double diff = double(timeDiff.count()) / layout->getSquareSize().x;
    center.x += diff * x;
    center.y -= diff * y;
    softCenter = none;
    scrollingState = ScrollingState::AFTER;
  }
  return true;
}

void MapGui::processScrolling(milliseconds time) {
  PROFILE;
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

bool MapGui::isDraggedCreature() const {
  if (auto draggedContent = guiFactory->getDragContainer().getElement())
    return draggedContent->visit<bool>(
        [] (UniqueEntity<Creature>::Id) { return true; },
        [] (const string& group) { return true; },
        [] (auto&) { return false; });
  return false;
}

void MapGui::considerScrollingToCreature() {
  PROFILE;
  if (auto pos = centeredCreaturePosition) {
    Vec2 size = layout->getSquareSize();
    Vec2 offset;
    if (auto index = objects[*pos])
      if (index->hasObject(ViewLayer::CREATURE) && !!screenMovement)
        offset = getMovementOffset(index->getObject(ViewLayer::CREATURE), size, 0, clock->getRealMillis(), false, *pos);
    double targetx = pos->x + (double)offset.x / size.x;
    double targety = pos->y + (double)offset.y / size.y;
    setCenter(targetx, targety);
  }
}

void MapGui::updateFX(milliseconds currentTimeReal) {
  if (auto *inst = fx::FXManager::getInstance())
    inst->simulateStableTime(double(currentTimeReal.count()) * 0.001, 60, 60);
}

void MapGui::render(Renderer& renderer) {
  PROFILE;
  auto currentTimeReal = clock->getRealMillis();
  updateFX(currentTimeReal);
  if (scrollingState == ScrollingState::NONE)
    considerScrollingToCreature();
  Vec2 size = layout->getSquareSize();
  lastHighlighted = getHighlightedInfo(size, currentTimeReal);

  renderMapObjects(renderer, size, currentTimeReal);

  renderAnimations(renderer, currentTimeReal);
  if (lastHighlighted.tilePos)
    considerRedrawingSquareHighlight(renderer, currentTimeReal, *lastHighlighted.tilePos, size);
  for (auto& mark : greenMarks)
    drawSquareHighlight(renderer, projectOnScreen(mark), size, Color::GREEN);
  for (auto& mark : redMarks)
    drawSquareHighlight(renderer, projectOnScreen(mark), size, Color::RED);
  if (renderer.getMousePos().inRectangle(getBounds())) {
    int moveSelectionSize = 0;
    if (spriteMode && activeButton) {
      renderer.drawViewObject(renderer.getMousePos() + Vec2(15, 15), activeButton->viewId, spriteMode, size);
      moveSelectionSize = size.y;
    }
    if (selectionSize)
      renderer.drawText(Color::WHITE, renderer.getMousePos() + Vec2(15, 20 + moveSelectionSize),
          toString(abs(selectionSize->x) + 1) + "x" + toString(abs(selectionSize->y) + 1), Renderer::NONE, size.y / 2);
  }
  processScrolling(currentTimeReal);
}

void MapGui::renderWalkingJoy(Renderer& renderer, Vec2 size) {
  if (playerPosition) {
    auto pos = renderer.getDiscreteJoyPos(ControllerJoy::WALKING);
    Vec2 offset;
    if (auto index = objects[*playerPosition])
      if (index->hasObject(ViewLayer::CREATURE) && !!screenMovement)
        offset = getMovementOffset(index->getObject(ViewLayer::CREATURE), size, 0, clock->getRealMillis(), false, *playerPosition);
    if (pos != Vec2(0, 0)) {
      Vec2 wpos = projectOnScreen(*playerPosition + pos);
      auto dir = pos.getBearing();
      auto coord = renderer.getTileSet().getTileCoord("arrow" + toString(int(dir.getCardinalDir()))).getOnlyElement();
      auto color = Color::WHITE;
      if (auto index = objects[*playerPosition])
        color = blendNightColor(Color::WHITE, *index);
      renderer.drawTile(wpos + offset, {coord}, size, color);
    }
  }
}

bool MapGui::onClick(MouseButtonId b, Vec2 v) {
  switch (b) {
    case MouseButtonId::RELEASED:
      onMouseRelease(v);
      return false;
    case MouseButtonId::LEFT:
      return onLeftClick(v);
    case MouseButtonId::RIGHT:
      return onRightClick(v);
    case MouseButtonId::MIDDLE:
      return onMiddleClick(v);
    default:
      return false;
  }
}

void MapGui::updateObject(Vec2 pos, CreatureView* view, Renderer& renderer, milliseconds currentTime) {
  auto level = view->getCreatureViewLevel();
  objects[pos].emplace();
  auto& index = *objects[pos];
  view->getViewIndex(pos, index);
  level->setNeedsRenderUpdate(pos, false);
  if (index.hasObject(ViewLayer::FLOOR) || index.hasObject(ViewLayer::FLOOR_BACKGROUND))
    index.setNightAmount(1.0 - level->getLight(pos));
  lastSquareUpdate[pos] = currentTime;
  connectionMap[pos].clear();
  shadowed.erase(pos + Vec2(0, 1));
  if (index.hasObject(ViewLayer::FLOOR)) {
    auto& object = index.getObject(ViewLayer::FLOOR);
    auto& tile = renderer.getTileSet().getTile(object.id());
    if (tile.wallShadow && !object.hasModifier(ViewObjectModifier::PLANNED)) {
      shadowed.insert(pos + Vec2(0, 1));
    }
    connectionMap[pos].insert(getConnectionId(object.id(), tile));
  }
  if (index.hasObject(ViewLayer::FLOOR_BACKGROUND)) {
    auto& object = index.getObject(ViewLayer::FLOOR_BACKGROUND);
    auto& tile = renderer.getTileSet().getTile(object.id());
    connectionMap[pos].insert(getConnectionId(object.id(), tile));
  }
  if (auto viewId = index.getHiddenId()) {
    auto& tile = renderer.getTileSet().getTile(*viewId);
    connectionMap[pos].insert(getConnectionId(*viewId, tile));
  }
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

void MapGui::updateQuarters(CreatureView* view, Renderer& renderer) {
  quarters = none;
  if (auto pos = projectOnMap(renderer.getMousePos())) {
    quarters = view->getQuarters(*pos);
    if (quarters) {
      unordered_set<Vec2, CustomHash<Vec2>> translated;
      for (auto& v : quarters->positions)
        translated.insert(projectOnScreen(v));
      quarters->positions = translated;
    }
  }
}

void MapGui::updateShortestPaths(CreatureView* view, Renderer& renderer, Vec2 tileSize, milliseconds curTimeReal) {
  shortestPath.clear();
  permaShortestPath = view->getPermanentPaths();
  phylacteries = view->getCreatureViewLevel()->getPhylacteries();
  if (auto pos = projectOnMap(renderer.getMousePos())) {
    auto highlightedInfo = getHighlightedInfo(tileSize, curTimeReal);
    if (!!lastMouseMove && highlightedInfo.tilePos) {
      auto highlightedPath = view->getHighlightedPathTo(*highlightedInfo.tilePos);
        if (!highlightedPath.empty())
          shortestPath.push_back(highlightedPath);
    }
    if (auto draggedContent = guiFactory->getDragContainer().getElement())
      draggedContent->visit<void>(
          [&](UniqueEntity<Creature>::Id id) { shortestPath = view->getPathTo(id, *pos); },
          [&](const string& group) { shortestPath = view->getGroupPathTo(group, *pos); },
          [&](TeamId team) { shortestPath = view->getTeamPathTo(team, *pos); }
      );
  }
}

void MapGui::updateObjects(CreatureView* view, Renderer& renderer, MapLayout* mapLayout, bool smoothMovement, bool ui,
    const optional<TutorialInfo>& tutorial) {
  selectionSize = view->getSelectionSize();
  playerPosition = view->getPlayerPosition();
  renderer.getSteamInput()->setGameActionLayer(!!playerPosition
      ? MySteamInput::GameActionLayer::TURNED_BASED
      : MySteamInput::GameActionLayer::REAL_TIME);
  if (tutorial) {
    tutorialHighlightLow = tutorial->highlightedSquaresLow;
    tutorialHighlightHigh = tutorial->highlightedSquaresHigh;
  } else {
    tutorialHighlightLow.clear();
    tutorialHighlightHigh.clear();
  }
  squareHighlightColor = Color::ALMOST_WHITE;
  greenMarks.clear();
  redMarks.clear();
  if (activeButton)
    if (auto mousePos = getMousePos()) {
      auto pos = layout->projectOnMap(getBounds(), getScreenPos(), *mousePos);
      auto res = view->canPlaceItem(pos, activeButton->index);
      if (!res.isValid)
        squareHighlightColor = Color::RED;
      greenMarks = std::move(res.inGreen);
      redMarks = std::move(res.inRed);
    }
  Level* level = view->getCreatureViewLevel();
  levelBounds = level->getBounds();
  mouseUI = ui;
  layout = mapLayout;
  auto currentTimeReal = clock->getRealMillis();
  updateShortestPaths(view, renderer, layout->getSquareSize(), currentTimeReal);
  updateQuarters(view, renderer);
  // hacky way to detect that we're switching between real-time and turn-based and not between
  // team members in turn-based mode.
  const bool newView = (view->getCenterType() != previousView);
  const bool newLevel = level != previousLevel;
  if (newView || newLevel) {
    if (auto *inst = fx::FXManager::getInstance())
      inst->clearUnorderedEffects();
    for (Vec2 pos : level->getBounds())
      level->setNeedsRenderUpdate(pos, true);
  } else
    for (Vec2 pos : mapLayout->getAllTiles(getBounds(), Level::getMaxBounds(), getScreenPos()))
      if (level->needsRenderUpdate(pos) ||
          !lastSquareUpdate[pos] || *lastSquareUpdate[pos] < currentTimeReal - milliseconds{1000})
        updateObject(pos, view, renderer, currentTimeReal);
  previousView = view->getCenterType();
  auto isGroundOrUpperZlevel = [](auto l) { return l->above || l->below; };
  previousLevel = level;
  keyScrolling = view->getCenterType() == CreatureView::CenterType::NONE;
  currentTimeGame = view->getAnimationTime();
  if (newView) {
    lastMoveCounter = (smoothMovement ? level->getModel()->getMoveCounter() : 1000000000);
    currentMoveCounter = lastMoveCounter;
  }
  bool newTurn = false;
  {
    int newMoveCounter = smoothMovement ? level->getModel()->getMoveCounter() : 1000000000;
    if (currentMoveCounter != newMoveCounter && !newView) {
      lastMoveCounter = currentMoveCounter;
      currentMoveCounter = newMoveCounter;
      newTurn = true;
    }
  }
  if (smoothMovement && view->getCenterType() != CreatureView::CenterType::NONE) {
    if (!screenMovement || newTurn) {
      screenMovement = ScreenMovement {
        clock->getRealMillis(),
        clock->getRealMillis() + milliseconds{100},
        lastMoveCounter
      };
    }
  } else
    screenMovement = none;
  if (view->getCenterType() == CreatureView::CenterType::FOLLOW)
    centeredCreaturePosition = view->getScrollCoord();
  else {
    centeredCreaturePosition = none;
    if (!isCentered() ||
        (view->getCenterType() == CreatureView::CenterType::STAY_ON_SCREEN &&
        getDistanceToEdgeRatio(view->getScrollCoord()) < 0.33 &&
        scrollingState == ScrollingState::NONE)) {
      auto coord = view->getScrollCoord();
      if (newLevel)
        setCenter(coord.x, coord.y);
      else
        setSoftCenter(coord.x, coord.y);
    }
  }
}

