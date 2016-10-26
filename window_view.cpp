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


#include "window_view.h"
#include "logging_view.h"
#include "replay_view.h"
#include "level.h"
#include "options.h"
#include "location.h"
#include "renderer.h"
#include "tile.h"
#include "clock.h"
#include "creature_view.h"
#include "view_index.h"
#include "map_memory.h"
#include "progress_meter.h"
#include "version.h"
#include "map_gui.h"
#include "minimap_gui.h"
#include "player_message.h"
#include "position.h"
#include "sound_library.h"

using SDL::SDL_Keysym;
using SDL::SDL_Keycode;

View* WindowView::createDefaultView(ViewParams params) {
  return new WindowView(params);
}

View* WindowView::createLoggingView(OutputArchive& of, ViewParams params) {
  return createDefaultView(params);
  //return new LoggingView(of, new WindowView(params));
}

View* WindowView::createReplayView(InputArchive& ifs, ViewParams params) {
  return createDefaultView(params);
  //return new ReplayView(ifs, new WindowView(params));
}

int rightBarWidthCollective = 344;
int rightBarWidthPlayer = 344;
int bottomBarHeightCollective = 62;
int bottomBarHeightPlayer = 62;

Rectangle WindowView::getMapGuiBounds() const {
  switch (gameInfo.infoType) {
    case GameInfo::InfoType::PLAYER:
      return Rectangle(Vec2(rightBarWidthPlayer, 0), renderer.getSize() - Vec2(0, bottomBarHeightPlayer));
    case GameInfo::InfoType::BAND:
      return Rectangle(Vec2(rightBarWidthCollective, 0), renderer.getSize() - Vec2(0, bottomBarHeightCollective));
    case GameInfo::InfoType::SPECTATOR: {
      Vec2 levelSize = Level::getSplashVisibleBounds().getSize();
      return Rectangle(
          (renderer.getSize() - levelSize.mult(mapLayout->getSquareSize())) / 2,
          (renderer.getSize() + levelSize.mult(mapLayout->getSquareSize())) / 2);
      }
  }
}

Rectangle WindowView::getMinimapBounds() const {
  Vec2 offset(-20, 70);
  Vec2 size(Vec2(renderer.getSize().x, renderer.getSize().x) / 12);
  return Rectangle(Vec2(renderer.getSize().x - size.x, 0), Vec2(renderer.getSize().x, size.y)).translate(offset);
}

void WindowView::resetMapBounds() {
  mapGui->setBounds(getMapGuiBounds());
  minimapDecoration->setBounds(getMinimapBounds().minusMargin(-6));
}

WindowView::WindowView(ViewParams params) : renderer(params.renderer), gui(params.gui), useTiles(params.useTiles),
    options(params.options), clock(params.clock), guiBuilder(renderer, gui, clock, params.options, {
        [this](UserInput input) { inputQueue.push(input);},
        [this](const vector<string>& s) { mapGui->setHint(s);},
        [this](SDL_Keysym ev) { keyboardAction(ev);},
        [this]() { refreshScreen(false);},
        [this](const string& s) { presentText("", s); },
        }), fullScreenTrigger(-1), fullScreenResolution(-1), zoomUI(-1),
    soundLibrary(params.soundLibrary) {}

void WindowView::initialize() {
  renderer.setFullscreen(options->getBoolValue(OptionId::FULLSCREEN));
  renderer.enableCustomCursor(!options->getBoolValue(OptionId::DISABLE_CURSOR));
//  renderer.setFullscreenMode(options->getChoiceValue(OptionId::FULLSCREEN_RESOLUTION));
  renderer.initialize();
  renderer.setZoom(options->getBoolValue(OptionId::ZOOM_UI) ? 2 : 1);
  options->addTrigger(OptionId::FULLSCREEN, [this] (int on) { fullScreenTrigger = on; });
  options->addTrigger(OptionId::DISABLE_CURSOR, [this] (int on) { renderer.enableCustomCursor(!on); });
  //options->addTrigger(OptionId::FULLSCREEN_RESOLUTION, [this] (int index) { fullScreenResolution = index; });
  options->addTrigger(OptionId::ZOOM_UI, [this] (int on) { zoomUI = on; });
  renderThreadId = currentThreadId();
  vector<ViewLayer> allLayers;
  for (auto l : ENUM_ALL(ViewLayer))
    allLayers.push_back(l);
  asciiLayouts = {
    {MapLayout(Vec2(16, 20), allLayers),
    MapLayout(Vec2(8, 10),
        {ViewLayer::FLOOR_BACKGROUND, ViewLayer::FLOOR, ViewLayer::LARGE_ITEM, ViewLayer::CREATURE})}, false};
  spriteLayouts = {{
    MapLayout(Vec2(48, 48), allLayers),
//    MapLayout(Vec2(36, 36), allLayers),
    MapLayout(Vec2(24, 24), allLayers)}, true};
  if (useTiles)
    currentTileLayout = spriteLayouts;
  else
    currentTileLayout = asciiLayouts;
  mapGui = new MapGui({
      bindMethod(&WindowView::mapContinuousLeftClickFun, this),
      [this] (Vec2 pos) {
          if (!guiBuilder.getActiveButton(CollectiveTab::BUILDINGS))
            inputQueue.push(UserInput(UserInputId::TILE_CLICK, pos));},
      bindMethod(&WindowView::mapRightClickFun, this),
      bindMethod(&WindowView::mapCreatureClickFun, this),
      [this] { refreshInput = true;},
      bindMethod(&WindowView::mapCreatureDragFun, this),
      [this] (UniqueEntity<Creature>::Id id, Vec2 pos) {
          inputQueue.push(UserInput(UserInputId::CREATURE_DRAG_DROP, CreatureDropInfo{pos, id})); }}, clock, options );
  minimapGui = new MinimapGui(renderer, [this]() { inputQueue.push(UserInput(UserInputId::DRAW_LEVEL_MAP)); });
  minimapDecoration = gui.stack(gui.rectangle(colors[ColorId::BLACK]), gui.miniWindow(),
      gui.margins(gui.renderInBounds(PGuiElem(minimapGui)), 6));
  resetMapBounds();
  guiBuilder.setMapGui(mapGui);
}

void WindowView::mapCreatureDragFun(UniqueEntity<Creature>::Id id, ViewId viewId, Vec2 origin) {
  gui.getDragContainer().put({DragContentId::CREATURE, id}, gui.viewObject(viewId), origin);
  inputQueue.push(UserInput(UserInputId::CREATURE_DRAG, id));
}

bool WindowView::isKeyPressed(SDL::SDL_Scancode code) {
  return SDL::SDL_GetKeyboardState(nullptr)[code];
}

void WindowView::mapCreatureClickFun(UniqueEntity<Creature>::Id id) {
  if (isKeyPressed(SDL::SDL_SCANCODE_LCTRL) || isKeyPressed(SDL::SDL_SCANCODE_RCTRL)) {
    inputQueue.push(UserInput(UserInputId::CREATURE_BUTTON2, id));
  } else {
    guiBuilder.setCollectiveTab(CollectiveTab::MINIONS);
    inputQueue.push(UserInput(UserInputId::CREATURE_BUTTON, id));
  }
}

void WindowView::mapContinuousLeftClickFun(Vec2 pos) {
  guiBuilder.closeOverlayWindows();
  optional<int> activeLibrary = guiBuilder.getActiveButton(CollectiveTab::TECHNOLOGY);
  optional<int> activeBuilding = guiBuilder.getActiveButton(CollectiveTab::BUILDINGS);
  auto collectiveTab = guiBuilder.getCollectiveTab();
  switch (gameInfo.infoType) {
    case GameInfo::InfoType::SPECTATOR:
    case GameInfo::InfoType::PLAYER:
      inputQueue.push(UserInput(UserInputId::MOVE_TO, pos));
      break;
    case GameInfo::InfoType::BAND:
/*      if (collectiveTab == CollectiveTab::MINIONS)
        inputQueue.push(UserInput(UserInputId::MOVE_TO, pos));
      else*/
      if (collectiveTab == CollectiveTab::TECHNOLOGY && activeLibrary)
        inputQueue.push(UserInput(UserInputId::LIBRARY, BuildingInfo{pos, *activeLibrary}));
      else
      if (collectiveTab == CollectiveTab::BUILDINGS) {
        if (activeBuilding && (isKeyPressed(SDL::SDL_SCANCODE_LSHIFT) || isKeyPressed(SDL::SDL_SCANCODE_RSHIFT)))
          inputQueue.push(UserInput(UserInputId::RECT_SELECTION, pos));
        else if (activeBuilding && (isKeyPressed(SDL::SDL_SCANCODE_LCTRL) || isKeyPressed(SDL::SDL_SCANCODE_RCTRL)))
          inputQueue.push(UserInput(UserInputId::RECT_DESELECTION, pos));
        else if (activeBuilding)
          inputQueue.push(UserInput(UserInputId::BUILD, BuildingInfo{pos, *activeBuilding}));
      }
    default:
      break;
  }
}

void WindowView::mapRightClickFun(Vec2 pos) {
  switch (gameInfo.infoType) {
    case GameInfo::InfoType::SPECTATOR:
    case GameInfo::InfoType::BAND:
      guiBuilder.clearActiveButton();
      break;
    default:
      break;
  }
}

void WindowView::reset() {
  RecursiveLock lock(renderMutex);
  mapLayout = &currentTileLayout.layouts[0];
  gameReady = false;
  wasRendered = false;
  minimapGui->clear();
  mapGui->clearCenter();
  guiBuilder.reset();
  gameInfo = GameInfo{};
  soundQueue.clear();
}

void WindowView::displayOldSplash() {
  Rectangle menuPosition = guiBuilder.getMenuPosition(MenuType::MAIN_NO_TILES, 0);
  int margin = 10;
  renderer.drawImage(renderer.getSize().x / 2 - 415, menuPosition.bottom() + margin,
      gui.get(GuiFactory::TexId::SPLASH1));
  Texture& splash2 = gui.get(GuiFactory::TexId::SPLASH2);
  renderer.drawImage((renderer.getSize().x - splash2.getSize().x) / 2,
      menuPosition.top() - splash2.getSize().y - margin, splash2);
}

void WindowView::displayMenuSplash2() {
  drawMenuBackground(0, 0);
}

void WindowView::drawMenuBackground(double barState, double mouthState) {
  Texture& menuCore = gui.get(GuiFactory::TexId::MENU_CORE);
  Texture& menuMouth = gui.get(GuiFactory::TexId::MENU_MOUTH);
  double scale = double(renderer.getSize().y) / menuCore.getSize().y;
  int width = menuCore.getSize().x * scale;
  double mouthPos1 = 184 * menuCore.getSize().x / 1920;
  double mouthPos2 = 214 * menuCore.getSize().x / 1920;
  double mouthX = (renderer.getSize().x - (menuMouth.getSize().x + 5) * scale) / 2;
  renderer.drawFilledRectangle(mouthX, mouthPos1 * scale, 1 + mouthX + barState * menuMouth.getSize().x * scale,
      (mouthPos2 + menuMouth.getSize().y) * scale, Color(60, 76, 48));
  renderer.drawImage((renderer.getSize().x - width) / 2, 0, menuCore, scale);
  renderer.drawImage(mouthX, scale * (mouthPos1 * (1 - mouthState) + mouthPos2 * mouthState), menuMouth, scale);
  renderer.drawText(colors[ColorId::WHITE], 30, renderer.getSize().y - 40, "Version " BUILD_DATE " " BUILD_VERSION,
      Renderer::NONE, 16);
}

void WindowView::getAutosaveSplash(const ProgressMeter& meter) {
  PGuiElem window = gui.miniWindow(gui.empty(), []{});
  Vec2 windowSize(440, 70);
  Rectangle bounds((renderer.getSize() - windowSize) / 2, (renderer.getSize() + windowSize) / 2);
  Rectangle progressBar(bounds.minusMargin(15));
  window->setBounds(bounds);
  while (!splashDone) {
    refreshScreen(false);
    window->render(renderer);
    double progress = meter.getProgress();
    Rectangle bar(progressBar.topLeft(), Vec2(1 + progressBar.left() * (1.0 - progress) +
          progressBar.right() * progress, progressBar.bottom()));
    renderer.drawFilledRectangle(bar, transparency(colors[ColorId::DARK_GREEN], 50));
    renderer.drawText(colors[ColorId::WHITE], bounds.middle().x, bounds.top() + 20, "Autosaving", Renderer::HOR);
    renderer.drawAndClearBuffer();
    sleep_for(milliseconds(30));
    Event event;
    while (renderer.pollEvent(event)) {
      propagateEvent(event, {});
      considerResizeEvent(event);
    }
  }
}

void WindowView::getSmallSplash(const string& text, function<void()> cancelFun) {
  PGuiElem window = gui.miniWindow(gui.empty(), []{});
  Vec2 windowSize(500, 90);
  string cancelText = "[cancel]";
  Rectangle bounds((renderer.getSize() - windowSize) / 2, (renderer.getSize() + windowSize) / 2);
  Rectangle progressBar(bounds.minusMargin(15));
  window->setBounds(bounds);
  while (!splashDone) {
    refreshScreen(false);
    window->render(renderer);
    renderer.drawText(colors[ColorId::WHITE], bounds.middle().x, bounds.top() + 20, text, Renderer::HOR);
    Rectangle cancelBut(bounds.middle().x - renderer.getTextLength(cancelText) / 2, bounds.top() + 50,
        bounds.middle().x + renderer.getTextLength(cancelText) / 2, bounds.top() + 80);
    if (cancelFun)
      renderer.drawText(colors[ColorId::LIGHT_BLUE], cancelBut.left(), cancelBut.top(), cancelText);
    renderer.drawAndClearBuffer();
    sleep_for(milliseconds(30));
    Event event;
    while (renderer.pollEvent(event)) {
      propagateEvent(event, {});
      considerResizeEvent(event);
      if (event.type == SDL::SDL_MOUSEBUTTONDOWN && cancelFun) {
        if (Vec2(event.button.x, event.button.y).inRectangle(cancelBut))
          cancelFun();
      }
    }
  }
}

void WindowView::getBigSplash(const ProgressMeter& meter, const string& text, function<void()> cancelFun) {
  auto t0 = clock->getRealMillis();
  int mouthMillis = 400;
  Texture& loadingSplash = gui.get(GuiFactory::TexId::LOADING_SPLASH);
  string cancelText = "[cancel]";
  while (!splashDone) {
    Vec2 textPos = useTiles ? Vec2(renderer.getSize().x / 2, renderer.getSize().y * 0.5)
      : Vec2(renderer.getSize().x / 2, renderer.getSize().y - 60);
    Rectangle cancelBut(textPos.x - renderer.getTextLength(cancelText) / 2, textPos.y + 30,
        textPos.x + renderer.getTextLength(cancelText) / 2, textPos.y + 60);
    if (useTiles)
      drawMenuBackground(meter.getProgress(), min(1.0, (double)(clock->getRealMillis() - t0).count() / mouthMillis));
    else
      renderer.drawImage((renderer.getSize().x - loadingSplash.getSize().x) / 2,
          (renderer.getSize().y - loadingSplash.getSize().y) / 2, loadingSplash);
    renderer.drawText(colors[ColorId::WHITE], textPos.x, textPos.y, text, Renderer::HOR);
    if (cancelFun)
      renderer.drawText(colors[ColorId::LIGHT_BLUE], cancelBut.left(), cancelBut.top(), cancelText);
    renderer.drawAndClearBuffer();
    sleep_for(milliseconds(30));
    Event event;
    while (renderer.pollEvent(event)) {
      considerResizeEvent(event);
      if (event.type == SDL::SDL_MOUSEBUTTONDOWN && cancelFun) {
        if (Vec2(event.button.x, event.button.y).inRectangle(cancelBut))
          cancelFun();
      }
    }
  }
}

void WindowView::displaySplash(const ProgressMeter* meter, const string& text, SplashType type,
    function<void()> cancelFun) {
  RecursiveLock lock(renderMutex);
  splashDone = false;
  renderDialog.push([=] {
    switch (type) {
      case SplashType::BIG: getBigSplash(*meter, text, cancelFun); break;
      case SplashType::AUTOSAVING: getAutosaveSplash(*meter); break;
      case SplashType::SMALL: getSmallSplash(text, cancelFun); break;
    }
    splashDone = false;
    renderDialog.pop();
  });
}

void WindowView::clearSplash() {
  splashDone = true;
  if (currentThreadId() != renderThreadId)
    while (splashDone) {}
}

void WindowView::resize(int width, int height) {
  renderer.resize(width, height);
  refreshInput = true;
}

void WindowView::close() {
}

Color getSpeedColor(int value) {
  if (value > 100)
    return Color(max(0, 255 - (value - 100) * 2), 255, max(0, 255 - (value - 100) * 2));
  else
    return Color(255, max(0, 255 + (value - 100) * 4), max(0, 255 + (value - 100) * 4));
}

void WindowView::rebuildGui() {
  int newHash = gameInfo.getHash();
  if (newHash == lastGuiHash)
    return;
  Debug() << "Rebuilding UI";
  lastGuiHash = newHash;
  PGuiElem bottom, right;
  vector<GuiBuilder::OverlayInfo> overlays;
  int rightBarWidth = 0;
  int bottomBarHeight = 0;
  optional<int> topBarHeight;
  int rightBottomMargin = 30;
  tempGuiElems.clear();
  if (!options->getIntValue(OptionId::DISABLE_MOUSE_WHEEL)) {
    tempGuiElems.push_back(gui.mouseWheel([this](bool up) { zoom(up ? -1 : 1); }));
    tempGuiElems.back()->setBounds(getMapGuiBounds());
  }
  tempGuiElems.push_back(gui.keyHandler(bindMethod(&WindowView::keyboardAction, this)));
  tempGuiElems.back()->setBounds(getMapGuiBounds());
  switch (gameInfo.infoType) {
    case GameInfo::InfoType::SPECTATOR:
        right = gui.empty();
        bottom = gui.empty();
        rightBarWidth = 0;
        bottomBarHeight = 0;
        if (getMapGuiBounds().left() > 0) {
          tempGuiElems.push_back(gui.rectangle(colors[ColorId::BLACK]));
          tempGuiElems.back()->setBounds(Rectangle(0, 0, getMapGuiBounds().left(), renderer.getSize().y));
          tempGuiElems.push_back(gui.rectangle(colors[ColorId::BLACK]));
          tempGuiElems.back()->setBounds(Rectangle(Vec2(getMapGuiBounds().right(), 0), renderer.getSize()));
        }
        if (getMapGuiBounds().top() > 0) {
          tempGuiElems.push_back(gui.rectangle(colors[ColorId::BLACK]));
          tempGuiElems.back()->setBounds(Rectangle(0, 0, renderer.getSize().x, getMapGuiBounds().top()));
          tempGuiElems.push_back(gui.rectangle(colors[ColorId::BLACK]));
          tempGuiElems.back()->setBounds(Rectangle(Vec2(0, getMapGuiBounds().bottom()), renderer.getSize()));
        }
        break;
    case GameInfo::InfoType::PLAYER:
        right = guiBuilder.drawRightPlayerInfo(gameInfo.playerInfo);
        bottom = guiBuilder.drawBottomPlayerInfo(gameInfo);
        rightBarWidth = rightBarWidthPlayer;
        bottomBarHeight = bottomBarHeightPlayer;
        guiBuilder.drawPlayerOverlay(overlays, gameInfo.playerInfo);
        break;
    case GameInfo::InfoType::BAND:
        right = guiBuilder.drawRightBandInfo(gameInfo);
        guiBuilder.drawBandOverlay(overlays, gameInfo.collectiveInfo);
        bottom = guiBuilder.drawBottomBandInfo(gameInfo);
        rightBarWidth = rightBarWidthCollective;
        bottomBarHeight = bottomBarHeightCollective;
        topBarHeight = 85;
        break;
  }
  resetMapBounds();
  int bottomOffset = 35;
  int sideOffset = 10;
  int rightWindowHeight = 80;
  if (rightBarWidth > 0) {
    tempGuiElems.push_back(gui.mainDecoration(rightBarWidth, bottomBarHeight, topBarHeight));
    tempGuiElems.back()->setBounds(Rectangle(renderer.getSize()));
    tempGuiElems.push_back(gui.margins(std::move(right), 20, 20, 10, 0));
    tempGuiElems.back()->setBounds(Rectangle(Vec2(0, 0),
          Vec2(rightBarWidth, renderer.getSize().y - rightBottomMargin)));
    tempGuiElems.push_back(gui.margins(std::move(bottom), 105, 10, 105, 0));
    tempGuiElems.back()->setBounds(Rectangle(
          Vec2(rightBarWidth, renderer.getSize().y - bottomBarHeight), renderer.getSize()));
    guiBuilder.drawMessages(overlays, gameInfo.messageBuffer, renderer.getSize().x - rightBarWidth);
    guiBuilder.drawGameSpeedDialog(overlays);
    for (auto& overlay : overlays) {
      Vec2 pos;
      int width = overlay.size.x;
      int height = overlay.size.y;
      switch (overlay.alignment) {
        case GuiBuilder::OverlayInfo::MINIONS:
          pos = Vec2(rightBarWidth - 20, rightWindowHeight - 6);
          break;
        case GuiBuilder::OverlayInfo::TOP_RIGHT:
          pos = Vec2(rightBarWidth + sideOffset, rightWindowHeight);
          break;
        case GuiBuilder::OverlayInfo::BOTTOM_RIGHT:
          pos = Vec2(rightBarWidth + sideOffset,
              renderer.getSize().y - bottomBarHeight - bottomOffset - height);
          break;
        case GuiBuilder::OverlayInfo::LEFT:
          pos = Vec2(sideOffset,
              renderer.getSize().y - bottomBarHeight - bottomOffset - height);
          break;
        case GuiBuilder::OverlayInfo::MESSAGES:
          pos = Vec2(rightBarWidth, 0);
          break;
        case GuiBuilder::OverlayInfo::GAME_SPEED:
          pos = Vec2(26, renderer.getSize().y - height - 50);
          break;
        case GuiBuilder::OverlayInfo::INVISIBLE:
          pos = Vec2(renderer.getSize().x, 0);
          overlay.elem = gui.invisible(std::move(overlay.elem));
          break;
      }
      tempGuiElems.push_back(std::move(overlay.elem));
      height = min(height, renderer.getSize().y - pos.y);
      tempGuiElems.back()->setBounds(Rectangle(pos, pos + Vec2(width, height)));
      Debug() << "Overlay " << overlay.alignment << " bounds " << tempGuiElems.back()->getBounds();
    }
  }
  Event ev;
  ev.type = SDL::SDL_MOUSEMOTION;
  ev.motion.x = renderer.getMousePos().x;
  ev.motion.y = renderer.getMousePos().y;
  propagateEvent(ev, getClickableGuiElems());
}

vector<GuiElem*> WindowView::getAllGuiElems() {
  if (!gameReady)
    return extractRefs(blockingElems);
  CHECK(currentThreadId() == renderThreadId);
  if (gameInfo.infoType == GameInfo::InfoType::SPECTATOR)
    return concat({mapGui}, extractRefs(tempGuiElems));
  vector<GuiElem*> ret = concat(extractRefs(tempGuiElems), extractRefs(blockingElems));
  if (gameReady)
    ret = concat({mapGui, minimapDecoration.get()}, ret);
  return ret;
}

vector<GuiElem*> WindowView::getClickableGuiElems() {
  CHECK(currentThreadId() == renderThreadId);
  if (gameInfo.infoType == GameInfo::InfoType::SPECTATOR)
    return {mapGui};
  vector<GuiElem*> ret = concat(extractRefs(tempGuiElems), extractRefs(blockingElems));
  reverse(ret.begin(), ret.end());
  if (gameReady) {
    ret.push_back(minimapGui);
    ret.push_back(mapGui);
  }
  return ret;
}

void WindowView::setScrollPos(Vec2 pos) {
  mapGui->setCenter(pos);
}

void WindowView::resetCenter() {
  mapGui->resetScrolling();
}

void WindowView::addVoidDialog(function<void()> fun) {
  RecursiveLock lock(renderMutex);
  Semaphore sem;
  renderDialog.push([this, &sem, fun] {
    fun();
    sem.v();
    renderDialog.pop();
  });
  if (currentThreadId() == renderThreadId)
    renderDialog.top()();
  lock.unlock();
  sem.p();
}

void WindowView::drawLevelMap(const CreatureView* creature) {
  TempClockPause pause(clock);
  addVoidDialog([=] {
    minimapGui->presentMap(creature, Rectangle(renderer.getSize()), renderer,
        [this](double x, double y) { mapGui->setCenter(x, y);}); });
}

void WindowView::updateMinimap(const CreatureView* creature) {
  const Level* level = creature->getLevel();
  Vec2 rad(40, 40);
  Vec2 playerPos = mapGui->getScreenPos().div(mapLayout->getSquareSize());
  Rectangle bounds(playerPos - rad, playerPos + rad);
  minimapGui->update(level, bounds, creature, true);
}

void WindowView::updateView(CreatureView* view, bool noRefresh) {
  if (!wasRendered && currentThreadId() != renderThreadId)
    return;
  GameInfo newInfo;
  view->refreshGameInfo(newInfo);
  RecursiveLock lock(renderMutex);
  wasRendered = false;
  guiBuilder.addUpsCounterTick();
  gameReady = true;
  if (!noRefresh)
    uiLock = false;
  switchTiles();
  gameInfo = newInfo;
  rebuildGui();
  mapGui->setSpriteMode(currentTileLayout.sprites);
  bool spectator = gameInfo.infoType == GameInfo::InfoType::SPECTATOR;
  mapGui->updateObjects(view, mapLayout, currentTileLayout.sprites || spectator,
      !spectator, guiBuilder.showMorale());
  updateMinimap(view);
  if (gameInfo.infoType == GameInfo::InfoType::SPECTATOR)
    guiBuilder.setGameSpeed(GuiBuilder::GameSpeed::NORMAL);
  if (soundLibrary)
    playSounds(view);
}

void WindowView::playSounds(const CreatureView* view) {
  Rectangle area = mapLayout->getAllTiles(getMapGuiBounds(), Level::getMaxBounds(), mapGui->getScreenPos());
  auto curTime = clock->getRealMillis();
  const milliseconds soundCooldown {70};
  for (auto& sound : soundQueue) {
    auto lastTime = lastPlayed[sound.getId()];
    if ((!lastTime || curTime > *lastTime + soundCooldown) && (!sound.getPosition() ||
        (sound.getPosition()->isSameLevel(view->getLevel()) && sound.getPosition()->getCoord().inRectangle(area)))) {
      soundLibrary->playSound(sound);
      lastPlayed[sound.getId()] = curTime;
    }
  }
  soundQueue.clear();
}

void WindowView::animateObject(vector<Vec2> trajectory, ViewObject object) {
  RecursiveLock lock(renderMutex);
  if (trajectory.size() >= 2)
    mapGui->addAnimation(
        Animation::thrownObject(
          (trajectory.back() - trajectory.front()).mult(mapLayout->getSquareSize()),
          object,
          currentTileLayout.sprites,
          mapLayout->getSquareSize()),
        trajectory.front());
}

void WindowView::animation(Vec2 pos, AnimationId id) {
  RecursiveLock lock(renderMutex);
  if (currentTileLayout.sprites)
    mapGui->addAnimation(Animation::fromId(id), pos);
}

void WindowView::refreshView() {
  if (currentThreadId() != renderThreadId)
    return;
  {
    RecursiveLock lock(renderMutex);
/*    if (!wasRendered && gameReady)
      rebuildGui();*/
    wasRendered = true;
    CHECK(currentThreadId() == renderThreadId);
    if (gameReady || !blockingElems.empty())
      processEvents();
    if (!renderDialog.empty())
      renderDialog.top()();
    if (uiLock && renderDialog.empty() && blockingElems.empty())
      return;
  }
  if ((renderDialog.empty() && gameReady) || !blockingElems.empty())
    refreshScreen(true);
}

void WindowView::drawMap() {
  for (GuiElem* gui : getAllGuiElems())
    gui->render(renderer);
  Vec2 mousePos = renderer.getMousePos();
  if (GuiElem* dragged = gui.getDragContainer().getGui())
    if (gui.getDragContainer().getOrigin().dist8(mousePos) > 30) {
      dragged->setBounds(Rectangle(mousePos + Vec2(15, 15), mousePos + Vec2(35, 35)));
      dragged->render(renderer);
    }
  guiBuilder.addFpsCounterTick();
}

void WindowView::refreshScreen(bool flipBuffer) {
  {
    RecursiveLock lock(renderMutex);
    if (fullScreenTrigger > -1) {
      renderer.setFullscreen(fullScreenTrigger);
      renderer.initialize();
      fullScreenTrigger = -1;
    }
    if (fullScreenResolution > -1) {
      if (renderer.isFullscreen()) {
        renderer.setFullscreenMode(fullScreenResolution);
        renderer.initialize();
      }
      fullScreenResolution = -1;
    }
    if (zoomUI > -1) {
      renderer.setZoom(zoomUI ? 2 : 1);
      zoomUI = -1;
    }
    if (!gameReady) {
      if (useTiles)
        displayMenuSplash2();
      else
        displayOldSplash();
    }
    drawMap();
  }
  if (flipBuffer)
    renderer.drawAndClearBuffer();
}

int indexHeight(const vector<ListElem>& options, int index) {
  CHECK(index < options.size() && index >= 0);
  int tmp = 0;
  for (int i : All(options))
    if (options[i].getMod() == ListElem::NORMAL && tmp++ == index)
      return i;
  FAIL << "Bad index " << int(options.size()) << " " << index;
  return -1;
}

optional<int> reverseIndexHeight(const vector<ListElem>& options, int height) {
  if (height < 0 || height >= options.size() || options[height].getMod() != ListElem::NORMAL)
    return none;
  int sub = 0;
  for (int i : Range(height))
    if (options[i].getMod() != ListElem::NORMAL)
      ++sub;
  return height - sub;
}

optional<Vec2> WindowView::chooseDirection(const string& message) {
  RecursiveLock lock(renderMutex);
  TempClockPause pause(clock);
  gameInfo.messageBuffer = { PlayerMessage(message) };
  SyncQueue<optional<Vec2>> returnQueue;
  addReturnDialog<optional<Vec2>>(returnQueue, [=] ()-> optional<Vec2> {
  rebuildGui();
  refreshScreen();
  do {
    Event event;
    renderer.waitEvent(event);
    considerResizeEvent(event);
    if (event.type == SDL::SDL_MOUSEMOTION || event.type == SDL::SDL_MOUSEBUTTONDOWN) {
      if (auto pos = mapGui->projectOnMap(renderer.getMousePos())) {
        refreshScreen(false);
        Vec2 middle = mapLayout->getAllTiles(getMapGuiBounds(), Level::getMaxBounds(), mapGui->getScreenPos())
            .middle();
        if (pos == middle)
          continue;
        Vec2 dir = (*pos - middle).getBearing();
        Vec2 wpos = mapLayout->projectOnScreen(getMapGuiBounds(), mapGui->getScreenPos(),
            middle.x + dir.x, middle.y + dir.y);
        if (currentTileLayout.sprites) {
          static vector<Renderer::TileCoord> coords;
          if (coords.empty())
            for (int i = 0; i < 8; ++i)
              coords.push_back(renderer.getTileCoord("arrow" + toString(i)));
          renderer.drawTile(wpos, coords[int(dir.getCardinalDir())], mapLayout->getSquareSize());
        } else {
          int numArrow = int(dir.getCardinalDir());
          static string arrows[] = { u8"⇑", u8"⇓", u8"⇒", u8"⇐", u8"⇗", u8"⇖", u8"⇘", u8"⇙"};
          renderer.drawText(Renderer::SYMBOL_FONT, mapLayout->getSquareSize().y, colors[ColorId::WHITE],
              wpos.x + mapLayout->getSquareSize().x / 2, wpos.y, arrows[numArrow], Renderer::HOR);
        }
        renderer.drawAndClearBuffer();
        if (event.type == SDL::SDL_MOUSEBUTTONDOWN)
          return dir;
      }
      renderer.flushEvents(SDL::SDL_MOUSEMOTION);
    } else
    refreshScreen();
    if (event.type == SDL::SDL_KEYDOWN)
      switch (event.key.keysym.sym) {
        case SDL::SDLK_UP:
        case SDL::SDLK_KP_8: refreshScreen(); return Vec2(0, -1);
        case SDL::SDLK_KP_9: refreshScreen(); return Vec2(1, -1);
        case SDL::SDLK_RIGHT:
        case SDL::SDLK_KP_6: refreshScreen(); return Vec2(1, 0);
        case SDL::SDLK_KP_3: refreshScreen(); return Vec2(1, 1);
        case SDL::SDLK_DOWN:
        case SDL::SDLK_KP_2: refreshScreen(); return Vec2(0, 1);
        case SDL::SDLK_KP_1: refreshScreen(); return Vec2(-1, 1);
        case SDL::SDLK_LEFT:
        case SDL::SDLK_KP_4: refreshScreen(); return Vec2(-1, 0);
        case SDL::SDLK_KP_7: refreshScreen(); return Vec2(-1, -1);
        case SDL::SDLK_ESCAPE: refreshScreen(); return none;
        default: break;
      }
  } while (1);
  });
  lock.unlock();
  return returnQueue.pop();
}

bool WindowView::yesOrNoPrompt(const string& message, bool defaultNo) {
  int index = defaultNo ? 1 : 0;
  return chooseFromListInternal("", {ListElem(capitalFirst(message), ListElem::TITLE), "Yes", "No"}, index,
      MenuType::YES_NO, nullptr) == 0;
}

optional<int> WindowView::getNumber(const string& title, int min, int max, int increments) {
  CHECK(min < max);
  vector<ListElem> options;
  vector<int> numbers;
  for (int i = min; i <= max; i += increments) {
    options.push_back(toString(i));
    numbers.push_back(i);
  }
  optional<int> res = WindowView::chooseFromList(title, options);
  if (!res)
    return none;
  else
    return numbers[*res];
}

optional<int> WindowView::chooseFromList(const string& title, const vector<ListElem>& options, int index,
    MenuType type, double* scrollPos, optional<UserInputId> exitAction) {
  return chooseFromListInternal(title, options, index, type, scrollPos);
}

optional<string> WindowView::getText(const string& title, const string& value, int maxLength, const string& hint) {
  RecursiveLock lock(renderMutex);
  TempClockPause pause(clock);
  SyncQueue<optional<string>> returnQueue;
  addReturnDialog<optional<string>>(returnQueue, [=] ()-> optional<string> {
    return guiBuilder.getTextInput(title, value, maxLength, hint);
  });
  lock.unlock();
  return returnQueue.pop();
}

optional<int> WindowView::chooseItem(const vector<ItemInfo>& items, double* scrollPos1) {
  RecursiveLock lock(renderMutex);
  uiLock = true;
  TempClockPause pause(clock);
  SyncQueue<optional<int>> returnQueue;
  addReturnDialog<optional<int>>(returnQueue, [=] ()-> optional<int> {
    double* scrollPos = scrollPos1;
    double localScrollPos = 0;
    if (!scrollPos)
      scrollPos = &localScrollPos;
    optional<optional<int>> retVal;
    vector<PGuiElem> lines = guiBuilder.drawItemMenu(items,
      [&retVal] (Rectangle butBounds, optional<int> a) { retVal = a;}, true);
    int menuHeight = lines.size() * guiBuilder.getStandardLineHeight() + 30;
    PGuiElem menu = gui.miniWindow(gui.margins(
            gui.scrollable(gui.verticalList(std::move(lines), guiBuilder.getStandardLineHeight()), scrollPos),
        15), [&retVal] { retVal = optional<int>(none); });
    PGuiElem bg2 = gui.darken();
    bg2->setBounds(renderer.getSize());
    while (1) {
      refreshScreen(false);
      menu->setBounds(guiBuilder.getEquipmentMenuPosition(menuHeight));
      bg2->render(renderer);
      menu->render(renderer);
      renderer.drawAndClearBuffer();
      Event event;
      while (renderer.pollEvent(event)) {
        propagateEvent(event, {menu.get()});
        if (retVal)
          return *retVal;
        if (considerResizeEvent(event))
          continue;
      }
    }
  });
  lock.unlock();
  return returnQueue.pop();
}

optional<UniqueEntity<Creature>::Id> WindowView::chooseRecruit(const string& title, const string& warning,
    pair<ViewId, int> budget, const vector<CreatureInfo>& creatures, double* scrollPos) {
  SyncQueue<optional<UniqueEntity<Creature>::Id>> returnQueue;
  return getBlockingGui(returnQueue, guiBuilder.drawRecruitMenu(returnQueue, title, warning, budget, creatures,
        scrollPos), Vec2(rightBarWidthCollective + 30, 80));
}

optional<UniqueEntity<Item>::Id> WindowView::chooseTradeItem(const string& title, pair<ViewId, int> budget,
    const vector<ItemInfo>& items, double* scrollPos) {
  SyncQueue<optional<UniqueEntity<Item>::Id>> returnQueue;
  return getBlockingGui(returnQueue, guiBuilder.drawTradeItemMenu(returnQueue, title, budget, items, scrollPos),
      Vec2(rightBarWidthCollective + 30, 80));
}

optional<Vec2> WindowView::chooseSite(const string& message, const Campaign& campaign, optional<Vec2> current) {
  SyncQueue<optional<Vec2>> returnQueue;
  return getBlockingGui(returnQueue, guiBuilder.drawChooseSiteMenu(returnQueue, message, campaign, current));
}

void WindowView::presentWorldmap(const Campaign& campaign) {
  Semaphore sem;
  return getBlockingGui(sem, guiBuilder.drawWorldmap(sem, campaign));
}

CampaignAction WindowView::prepareCampaign(const Campaign& campaign, Options* options, RetiredGames& retired) {
  SyncQueue<CampaignAction> returnQueue;
  optional<Vec2> embarkPos;
  bool retiredMenu = false;
  bool helpText = false;
  return getBlockingGui(returnQueue, guiBuilder.drawCampaignMenu(returnQueue, campaign, options, retired,
        embarkPos, retiredMenu, helpText));
}

optional<UniqueEntity<Creature>::Id> WindowView::chooseTeamLeader(const string& title,
    const vector<CreatureInfo>& creatures, const string& cancelText) {
  SyncQueue<optional<UniqueEntity<Creature>::Id>> returnQueue;
  return getBlockingGui(returnQueue, guiBuilder.drawTeamLeaderMenu(returnQueue, title, creatures, cancelText));
}

bool WindowView::creaturePrompt(const string& title, const vector<CreatureInfo>& creatures) {
  SyncQueue<bool> returnQueue;
  return getBlockingGui(returnQueue, guiBuilder.drawCreaturePrompt(returnQueue, title, creatures));
}

void WindowView::getBlockingGui(Semaphore& sem, PGuiElem elem, optional<Vec2> origin) {
  RecursiveLock lock(renderMutex);
  TempClockPause pause(clock);
  if (!origin)
    origin = (renderer.getSize() - Vec2(*elem->getPreferredWidth(), *elem->getPreferredHeight())) / 2;
  if (blockingElems.empty()) {
    blockingElems.push_back(gui.darken());
    blockingElems.back()->setBounds(renderer.getSize());
  }
  blockingElems.push_back(std::move(elem));
  blockingElems.back()->setPreferredBounds(*origin);
  lock.unlock();
  if (currentThreadId() == renderThreadId)
    while (!sem.get())
      refreshView();
  else
    sem.p();
  lock.lock();
  blockingElems.clear();
}

void WindowView::presentHighscores(const vector<HighscoreList>& list) {
  Semaphore sem;
  int tabNum = 0;
  bool online = false;
  vector<double> scrollPos(list.size(), 0);
  getBlockingGui(sem, guiBuilder.drawHighscores(list, sem, tabNum, scrollPos, online),
      guiBuilder.getMenuPosition(MenuType::NORMAL, 0).topLeft());
}

PGuiElem WindowView::drawGameChoices(optional<optional<GameTypeChoice>>& choice, optional<GameTypeChoice>& index) {
  return gui.verticalAspect(
      gui.marginFit(
      gui.horizontalListFit(makeVec<PGuiElem>(
            gui.stack(makeVec<PGuiElem>(
              gui.button([&] { choice = GameTypeChoice::KEEPER;}),
              gui.mouseOverAction([&] { index = GameTypeChoice::KEEPER;}),             
              gui.sprite(GuiFactory::TexId::KEEPER_CHOICE, GuiFactory::Alignment::LEFT_STRETCHED),
              gui.marginFit(gui.empty(), gui.mainMenuLabel("keeper          ", -0.08,
                  colors[ColorId::MAIN_MENU_OFF]), 0.94, gui.TOP),
              gui.mouseHighlightGameChoice(gui.stack(
                gui.sprite(GuiFactory::TexId::KEEPER_HIGHLIGHT, GuiFactory::Alignment::LEFT_STRETCHED),
              gui.marginFit(gui.empty(), gui.mainMenuLabel("keeper          ", -0.08),
                  0.94, gui.TOP)),
                GameTypeChoice::KEEPER, index)
              )),
            gui.stack(makeVec<PGuiElem>(
              gui.button([&] { choice = GameTypeChoice::ADVENTURER;}),
              gui.sprite(GuiFactory::TexId::ADVENTURER_CHOICE, GuiFactory::Alignment::RIGHT_STRETCHED),
              gui.marginFit(gui.empty(), gui.mainMenuLabel("          adventurer", -0.08,
                  colors[ColorId::MAIN_MENU_OFF]), 0.94, gui.TOP),
              gui.mouseHighlightGameChoice(gui.stack(
                gui.sprite(GuiFactory::TexId::ADVENTURER_HIGHLIGHT, GuiFactory::Alignment::RIGHT_STRETCHED),
                gui.marginFit(gui.empty(), gui.mainMenuLabel("          adventurer", -0.08),
                  0.94, gui.TOP)),
                GameTypeChoice::ADVENTURER, index)
              ))), 0),
      gui.margins(gui.verticalListFit(makeVec<PGuiElem>(
          gui.stack(
            gui.button([&] { choice = optional<GameTypeChoice>(none);}),
            gui.mainMenuLabelBg("Go back", 0.2),
            gui.mouseHighlightGameChoice(gui.mainMenuLabel("Go back", 0.2), none, index)),
          gui.empty()), 0), 0, 30, 0, 0),
      0.7, gui.TOP), 1);
}

optional<GameTypeChoice> WindowView::chooseGameType() {
  if (!useTiles) {
    if (auto ind = chooseFromListInternal("", {
          ListElem("Choose your role:", ListElem::TITLE),
          "Keeper",
          "Adventurer"
          },
        0, MenuType::MAIN, nullptr))
      return (GameTypeChoice)(*ind);
    else
      return none;
  }
  RecursiveLock lock(renderMutex);
  uiLock = true;
  TempClockPause pause(clock);
  SyncQueue<optional<GameTypeChoice>> returnQueue;
  addReturnDialog<optional<GameTypeChoice>>(returnQueue, [=] ()-> optional<GameTypeChoice> {
  optional<optional<GameTypeChoice>> choice;
  optional<GameTypeChoice> index = GameTypeChoice::KEEPER;
  PGuiElem stuff = drawGameChoices(choice, index);
  while (1) {
    refreshScreen(false);
    stuff->setBounds(guiBuilder.getMenuPosition(MenuType::GAME_CHOICE, 0));
    stuff->render(renderer);
    renderer.drawAndClearBuffer();
    Event event;
    while (renderer.pollEvent(event)) {
      propagateEvent(event, {stuff.get()});
      if (choice)
        return *choice;
      if (considerResizeEvent(event))
        continue;
      if (event.type == SDL::SDL_KEYDOWN)
        switch (event.key.keysym.sym) {
          case SDL::SDLK_KP_4:
          case SDL::SDLK_LEFT:
            if (index != GameTypeChoice::KEEPER)
              index = GameTypeChoice::KEEPER;
            else
              index = GameTypeChoice::ADVENTURER;
            break;
          case SDL::SDLK_KP_6:
          case SDL::SDLK_RIGHT:
            if (index != GameTypeChoice::ADVENTURER)
              index = GameTypeChoice::ADVENTURER;
            else
              index = GameTypeChoice::KEEPER;
            break;
          case SDL::SDLK_KP_5:
          case SDL::SDLK_KP_ENTER:
          case SDL::SDLK_RETURN: if (index) return *index;
          case SDL::SDLK_ESCAPE: return none;
          default: break;
        }
    }
  }
  });
  lock.unlock();
  return returnQueue.pop();
}

optional<int> WindowView::chooseFromListInternal(const string& title, const vector<ListElem>& options, int index1,
    MenuType menuType, double* scrollPos1) {
  if (!useTiles && menuType == MenuType::MAIN)
    menuType = MenuType::MAIN_NO_TILES;
  if (options.size() == 0)
    return none;
  RecursiveLock lock(renderMutex);
  uiLock = true;
  inputQueue.push(UserInputId::REFRESH);
  TempClockPause pause(clock);
  SyncQueue<optional<int>> returnQueue;
  addReturnDialog<optional<int>>(returnQueue, [=] ()-> optional<int> {
  renderer.flushEvents(SDL::SDL_KEYDOWN);
  int contentHeight;
  int choice = -1;
  int count = 0;
  double* scrollPos = scrollPos1;
  int index = index1;
  vector<int> indexes(options.size());
  vector<int> optionIndexes;
  int elemCount = 0;
  for (int i : All(options)) {
    if (options[i].getMod() == ListElem::NORMAL) {
      indexes[count] = elemCount;
      optionIndexes.push_back(i);
      ++count;
    }
    if (options[i].getMod() != ListElem::TITLE)
      ++elemCount;
  }
  if (optionIndexes.empty())
    optionIndexes.push_back(0);
  double localScrollPos = index >= 0 ? guiBuilder.getScrollPos(optionIndexes[index], options.size()) : 0;
  if (scrollPos == nullptr)
    scrollPos = &localScrollPos;
  PGuiElem stuff = guiBuilder.drawListGui(capitalFirst(title), options, menuType, &contentHeight, &index, &choice);
  PGuiElem dismissBut = gui.margins(gui.stack(makeVec<PGuiElem>(
        gui.button([&](){ choice = -100; }),
        gui.mouseHighlight2(gui.mainMenuHighlight()),
        gui.centeredLabel(Renderer::HOR, "Dismiss"))), 0, 5, 0, 0);
  switch (menuType) {
    case MenuType::MAIN: break;
    case MenuType::YES_NO:
      stuff = gui.window(std::move(stuff), [&choice] { choice = -100;}); break;
    default:
      stuff = gui.scrollable(std::move(stuff), scrollPos);
      stuff = gui.margins(std::move(stuff), 0, 15, 0, 0);
      stuff = gui.margin(gui.centerHoriz(std::move(dismissBut), renderer.getTextLength("Dismiss") + 100),
          std::move(stuff), 30, gui.BOTTOM);
      stuff = gui.window(std::move(stuff), [&choice] { choice = -100;});
      break;
  }
  optional<optional<int>> callbackRet;
  stuff = gui.stack(
      std::move(stuff),
      gui.keyHandler([&] (SDL_Keysym key) {
        switch (key.sym) {
          case SDL::SDLK_KP_8:
          case SDL::SDLK_UP:
            if (count > 0) {
              index = (index - 1 + count) % count;
              *scrollPos = guiBuilder.getScrollPos(optionIndexes[index], options.size());
            } else
              *scrollPos = max<double>(0, *scrollPos - 1);
            break;
          case SDL::SDLK_KP_2:
          case SDL::SDLK_DOWN:
            if (count > 0) {
              index = (index + 1 + count) % count;
              *scrollPos = guiBuilder.getScrollPos(optionIndexes[index], options.size());
            } else
              *scrollPos = min<int>(options.size() - 1, *scrollPos + 1);
            break;
          case SDL::SDLK_KP_5:
          case SDL::SDLK_KP_ENTER:
          case SDL::SDLK_RETURN:
            if (count > 0 && index > -1) {
              CHECK(index < indexes.size()) <<
                  index << " " << indexes.size() << " " << count << " " << options.size();
              callbackRet = indexes[index];
              break;
            }
          case SDL::SDLK_ESCAPE: callbackRet = optional<int>(none);
          default: break;
        }        
      }, true));
  while (1) {
    refreshScreen(false);
    stuff->setBounds(guiBuilder.getMenuPosition(menuType, options.size()));
    stuff->render(renderer);
    renderer.drawAndClearBuffer();
    Event event;
    while (renderer.pollEvent(event)) {
      propagateEvent(event, concat({stuff.get()}, getClickableGuiElems()));
      if (choice > -1) {
        CHECK(choice < indexes.size()) << choice;
        return indexes[choice];
      }
      if (choice == -100)
        return none;
      if (callbackRet)
        return *callbackRet;
      if (considerResizeEvent(event))
        continue;
    }
  }
  });
  lock.unlock();
  return returnQueue.pop();
}

void WindowView::presentText(const string& title, const string& text) {
  TempClockPause pause(clock);
  presentList(title, ListElem::convert({text}), false);
}

void WindowView::presentList(const string& title, const vector<ListElem>& options, bool scrollDown, MenuType menu,
    optional<UserInputId> exitAction) {
  vector<ListElem> conv(options);
  for (ListElem& e : conv)
    if (e.getMod() == ListElem::NORMAL)
      e.setMod(ListElem::TEXT);
  double scrollPos = scrollDown ? options.size() - 1 : 0;
  chooseFromListInternal(title, conv, -1, menu, &scrollPos);
}

void WindowView::zoom(int dir) {
  refreshInput = true;
  auto& layouts = currentTileLayout.layouts;
  int index = *findAddress(layouts, mapLayout);
  if (dir != 0 )
    index += dir;
  else {
    CHECK(index == 0 || index == 1) << index;
    index = 1 - index;
  }
  index = max(0, min<int>(layouts.size() - 1, index));
  mapLayout = &currentTileLayout.layouts[index];
}

void WindowView::switchTiles() {
  int index = *findAddress(currentTileLayout.layouts, mapLayout);
  if (options->getBoolValue(OptionId::ASCII) || !useTiles)
    currentTileLayout = asciiLayouts;
  else
    currentTileLayout = spriteLayouts;
  if (currentTileLayout.layouts.size() <= index)
    index = 0;
  while (gameInfo.infoType == GameInfo::InfoType::SPECTATOR && useTiles &&
      renderer.getSize().x < Level::getSplashVisibleBounds().width() *
      currentTileLayout.layouts[index].getSquareSize().x && index < currentTileLayout.layouts.size() - 1)
    ++index;
  mapLayout = &currentTileLayout.layouts[index];
}

bool WindowView::travelInterrupt() {
  return lockKeyboard;
}

milliseconds WindowView::getTimeMilli() {
  return clock->getMillis();
}

milliseconds WindowView::getTimeMilliAbsolute() {
  return clock->getRealMillis();
}

void WindowView::stopClock() {
  clock->pause();
}

void WindowView::continueClock() {
  clock->cont();
}

bool WindowView::isClockStopped() {
  return clock->isPaused();
}

bool WindowView::considerResizeEvent(Event& event) {
  if (event.type == SDL::SDL_QUIT)
    throw GameExitException();
  if (event.type == SDL::SDL_WINDOWEVENT && event.window.event == SDL::SDL_WINDOWEVENT_RESIZED) {
    resize(event.window.data1, event.window.data2);
    return true;
  }
  return false;
}

Vec2 lastGoTo(-1, -1);

void WindowView::processEvents() {
  Event event;
  while (renderer.pollEvent(event)) {
    considerResizeEvent(event);
    if (gameInfo.infoType == GameInfo::InfoType::SPECTATOR)
      switch (event.type) {
        case SDL::SDL_KEYDOWN:
        case SDL::SDL_MOUSEWHEEL:
        case SDL::SDL_MOUSEBUTTONDOWN:
          inputQueue.push(UserInput(UserInputId::EXIT));
          return;
        default:break;
      }
    else
      propagateEvent(event, getClickableGuiElems());
    switch (event.type) {
      case SDL::SDL_KEYDOWN:
        if (gameInfo.infoType == GameInfo::InfoType::PLAYER)
          renderer.flushEvents(SDL::SDL_KEYDOWN);
        break;
      case SDL::SDL_MOUSEBUTTONDOWN:
        if (event.button.button == SDL_BUTTON_RIGHT)
          guiBuilder.closeOverlayWindows();
        if (event.button.button == SDL_BUTTON_MIDDLE)
          inputQueue.push(UserInput(UserInputId::DRAW_LEVEL_MAP));
        break;
      case SDL::SDL_MOUSEBUTTONUP:
        if (event.button.button == SDL_BUTTON_LEFT) {
          if (auto building = guiBuilder.getActiveButton(CollectiveTab::BUILDINGS))
            inputQueue.push(UserInput(UserInputId::BUTTON_RELEASE, BuildingInfo{Vec2(0, 0), *building}));
          if (auto building = guiBuilder.getActiveButton(CollectiveTab::TECHNOLOGY))
            inputQueue.push(UserInput(UserInputId::BUTTON_RELEASE, BuildingInfo{Vec2(0, 0), *building}));
        }
        break;
      default: break;
    }
  }
}

void WindowView::propagateEvent(const Event& event, vector<GuiElem*> guiElems) {
  CHECK(currentThreadId() == renderThreadId);
  if (gameReady)
    mapGui->setHint({});
  switch (event.type) {
    case SDL::SDL_MOUSEBUTTONUP:
      // MapGui needs this event otherwise it will sometimes lock the mouse button
      mapGui->onMouseRelease(Vec2(event.button.x, event.button.y));
      break;
    case SDL::SDL_MOUSEBUTTONDOWN:
      lockKeyboard = true;
      break;
    default:break;
  }
  gui.propagateEvent(event, guiElems);
}

UserInputId getDirActionId(const SDL_Keysym& key) {
  if (GuiFactory::isCtrl(key))
    return UserInputId::TRAVEL;
  if (GuiFactory::isAlt(key))
    return UserInputId::FIRE;
  else
    return UserInputId::MOVE;
}

void WindowView::keyboardAction(const SDL_Keysym& key) {
  if (lockKeyboard)
    return;
  lockKeyboard = true;
  switch (key.sym) {
    case SDL::SDLK_z: zoom(0); break;
    case SDL::SDLK_F2: options->handle(this, OptionSet::GENERAL); refreshScreen(); break;
    case SDL::SDLK_ESCAPE:
      if (!guiBuilder.clearActiveButton() && !renderer.isMonkey())
        inputQueue.push(UserInput(UserInputId::EXIT));
      break;
    case SDL::SDLK_UP:
    case SDL::SDLK_KP_8:
      inputQueue.push(UserInput(getDirActionId(key), Vec2(0, -1)));
      mapGui->onMouseGone();
      break;
    case SDL::SDLK_KP_9:
      inputQueue.push(UserInput(getDirActionId(key), Vec2(1, -1)));
      mapGui->onMouseGone();
      break;
    case SDL::SDLK_RIGHT: 
    case SDL::SDLK_KP_6:
      inputQueue.push(UserInput(getDirActionId(key), Vec2(1, 0)));
      mapGui->onMouseGone();
      break;
    case SDL::SDLK_KP_3:
      inputQueue.push(UserInput(getDirActionId(key), Vec2(1, 1)));
      mapGui->onMouseGone();
      break;
    case SDL::SDLK_DOWN:
    case SDL::SDLK_KP_2:
      inputQueue.push(UserInput(getDirActionId(key), Vec2(0, 1)));
      mapGui->onMouseGone();
      break;
    case SDL::SDLK_KP_1:
      inputQueue.push(UserInput(getDirActionId(key), Vec2(-1, 1)));
      mapGui->onMouseGone();
      break;
    case SDL::SDLK_LEFT:
    case SDL::SDLK_KP_4:
      inputQueue.push(UserInput(getDirActionId(key), Vec2(-1, 0)));
      mapGui->onMouseGone();
      break;
    case SDL::SDLK_KP_7:
      inputQueue.push(UserInput(getDirActionId(key), Vec2(-1, -1)));
      mapGui->onMouseGone();
      break;
    default: break;
  }
}

UserInput WindowView::getAction() {
  lockKeyboard = false;
  if (refreshInput) {
    refreshInput = false;
    return UserInputId::REFRESH;
  }
  if (auto input = inputQueue.popAsync())
    return *input;
  else
    return UserInput(UserInputId::IDLE);
}

double WindowView::getGameSpeed() {
  switch (guiBuilder.getGameSpeed()) {
    case GuiBuilder::GameSpeed::SLOW: return 0.015;
    case GuiBuilder::GameSpeed::NORMAL: return 0.025;
    case GuiBuilder::GameSpeed::FAST: return 0.04;
    case GuiBuilder::GameSpeed::VERY_FAST: return 0.06;
  }
}

void WindowView::addSound(const Sound& sound) {
  soundQueue.push_back(sound);
}
