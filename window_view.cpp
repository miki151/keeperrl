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

using sf::Color;
using sf::String;
using sf::RenderWindow;
using sf::VideoMode;
using sf::Text;
using sf::Font;
using sf::Event;
using sf::RectangleShape;
using sf::CircleShape;
using sf::Vector2f;
using sf::Vector2u;
using sf::Image;
using sf::Sprite;
using sf::Texture;
using sf::Keyboard;
using sf::Mouse;

View* WindowView::createDefaultView(ViewParams params) {
  return new WindowView(params);
}

View* WindowView::createLoggingView(OutputArchive& of, ViewParams params) {
  return new LoggingView(of, new WindowView(params));
}

View* WindowView::createReplayView(InputArchive& ifs, ViewParams params) {
  return new ReplayView(ifs, new WindowView(params));
}

int rightBarWidthCollective = 330;
int rightBarWidthPlayer = 330;
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
  minimapGui->setBounds(getMinimapBounds());
  minimapDecoration->setBounds(getMinimapBounds().minusMargin(-6));
}

WindowView::WindowView(ViewParams params) : renderer(params.renderer), gui(params.gui), useTiles(params.useTiles),
    options(params.options), clock(params.clock), guiBuilder(renderer, gui, clock, {
        [this](UserInput input) { inputQueue.push(input);},
        [this](const vector<string>& s) { mapGui->setHint(s);},
        [this](sf::Event::KeyEvent ev) { keyboardAction(ev);},
        [this]() { refreshScreen(false);}}), fullScreenTrigger(-1), fullScreenResolution(-1) {}

void WindowView::initialize() {
  renderer.initialize(options->getBoolValue(OptionId::FULLSCREEN), options->getChoiceValue(OptionId::FULLSCREEN_RESOLUTION));
  options->addTrigger(OptionId::FULLSCREEN, [this] (int on) { fullScreenTrigger = on; });
  options->addTrigger(OptionId::FULLSCREEN_RESOLUTION, [this] (int index) { fullScreenResolution = index; });
  renderThreadId = currentThreadId();
  vector<ViewLayer> allLayers;
  for (auto l : ENUM_ALL(ViewLayer))
    allLayers.push_back(l);
  asciiLayouts = {
    MapLayout(Vec2(16, 20), allLayers),
    MapLayout(Vec2(8, 10),
        {ViewLayer::FLOOR_BACKGROUND, ViewLayer::FLOOR, ViewLayer::LARGE_ITEM, ViewLayer::CREATURE}), false};
  spriteLayouts = {
    MapLayout(Vec2(36, 36), allLayers),
    MapLayout(Vec2(18, 18), allLayers), true};
  if (useTiles)
    currentTileLayout = spriteLayouts;
  else
    currentTileLayout = asciiLayouts;
  mapGui = new MapGui({
      [this](Vec2 pos) { mapLeftClickFun(pos); },
      [this](Vec2 pos) { mapRightClickFun(pos); },
      [this](UniqueEntity<Creature>::Id id) { mapCreatureClickFun(id); },
      [this] { refreshInput = true;}}, clock );
  minimapGui = new MinimapGui([this]() { inputQueue.push(UserInput(UserInputId::DRAW_LEVEL_MAP)); });
  minimapDecoration = gui.border2(gui.rectangle(colors[ColorId::BLACK]));
  resetMapBounds();
  guiBuilder.setTilesOk(useTiles);
}

void WindowView::mapCreatureClickFun(UniqueEntity<Creature>::Id id) {
  if (Keyboard::isKeyPressed(Keyboard::LControl) || Keyboard::isKeyPressed(Keyboard::RControl)) {
    inputQueue.push(UserInput(UserInputId::ADD_TO_TEAM, id));
    guiBuilder.setCollectiveTab(GuiBuilder::CollectiveTab::MINIONS);
  } else
    inputQueue.push(UserInput(UserInputId::CREATURE_BUTTON, id));
}

void WindowView::mapLeftClickFun(Vec2 pos) {
  guiBuilder.closeOverlayWindows();
  int activeLibrary = guiBuilder.getActiveLibrary();
  int activeBuilding = guiBuilder.getActiveBuilding();
  auto collectiveTab = guiBuilder.getCollectiveTab();
  switch (gameInfo.infoType) {
    case GameInfo::InfoType::SPECTATOR:
    case GameInfo::InfoType::PLAYER:
      inputQueue.push(UserInput(UserInputId::MOVE_TO, pos));
      break;
    case GameInfo::InfoType::BAND:
      if (collectiveTab == GuiBuilder::CollectiveTab::MINIONS)
        inputQueue.push(UserInput(UserInputId::MOVE_TO, pos));
      else
      if (collectiveTab == GuiBuilder::CollectiveTab::TECHNOLOGY && activeLibrary >= 0)
        inputQueue.push(UserInput(UserInputId::LIBRARY, BuildingInfo{pos, activeLibrary}));
      else
      if (collectiveTab == GuiBuilder::CollectiveTab::BUILDINGS) {
        if (Keyboard::isKeyPressed(Keyboard::LShift) || Keyboard::isKeyPressed(Keyboard::RShift))
          inputQueue.push(UserInput(UserInputId::RECT_SELECTION, pos));
        else if (Keyboard::isKeyPressed(Keyboard::LControl) || Keyboard::isKeyPressed(Keyboard::RControl))
          inputQueue.push(UserInput(UserInputId::RECT_DESELECTION, pos));
        else
          inputQueue.push(UserInput(UserInputId::BUILD, BuildingInfo{pos, activeBuilding}));
      }
    default:
      break;
  }
}

void WindowView::mapRightClickFun(Vec2 pos) {
  switch (gameInfo.infoType) {
    case GameInfo::InfoType::SPECTATOR:
    case GameInfo::InfoType::BAND:
      inputQueue.push(UserInput(UserInputId::POSSESS, pos));
      break;
    default:
      break;
  }
}

void WindowView::reset() {
  RenderLock lock(renderMutex);
  mapLayout = &currentTileLayout.normalLayout;
  gameReady = false;
  wasRendered = false;
  minimapGui->clear();
  mapGui->clearCenter();
  guiBuilder.reset();
  gameInfo = GameInfo{};
}

void WindowView::displayOldSplash() {
  Rectangle menuPosition = guiBuilder.getMenuPosition(MenuType::MAIN_NO_TILES);
  int margin = 10;
  renderer.drawImage(renderer.getSize().x / 2 - 415, menuPosition.getKY() + margin,
      gui.get(GuiFactory::TexId::SPLASH1));
  Texture& splash2 = gui.get(GuiFactory::TexId::SPLASH2);
  renderer.drawImage((renderer.getSize().x - splash2.getSize().x) / 2,
      menuPosition.getPY() - splash2.getSize().y - margin, splash2);
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
      (mouthPos2 + menuMouth.getSize().y) * scale, sf::Color(60, 76, 48));
  renderer.drawImage((renderer.getSize().x - width) / 2, 0, menuCore, scale);
  renderer.drawImage(mouthX, scale * (mouthPos1 * (1 - mouthState) + mouthPos2 * mouthState), menuMouth, scale);
  renderer.drawText(colors[ColorId::WHITE], 30, renderer.getSize().y - 40, "Version " BUILD_DATE " " BUILD_VERSION,
      Renderer::NONE, 16);
}

void WindowView::displayAutosaveSplash(const ProgressMeter& meter) {
  splashDone = false;
  renderDialog.push([=, &meter] {
    PGuiElem window = gui.miniWindow(gui.empty());
    Vec2 windowSize(440, 70);
    Rectangle bounds((renderer.getSize() - windowSize) / 2, (renderer.getSize() + windowSize) / 2);
    Rectangle progressBar(bounds.minusMargin(15));
    window->setBounds(bounds);
    while (!splashDone) {
      refreshScreen(false);
      window->render(renderer);
      double progress = meter.getProgress();
      Rectangle bar(progressBar.getTopLeft(), Vec2(1 + progressBar.getPX() * (1.0 - progress) +
            progressBar.getKX() * progress, progressBar.getKY()));
      renderer.drawFilledRectangle(bar, transparency(colors[ColorId::DARK_GREEN], 50));
      renderer.drawText(colors[ColorId::WHITE], bounds.middle().x, bounds.getPY() + 20, "Autosaving", Renderer::HOR);
      renderer.drawAndClearBuffer();
      sf::sleep(sf::milliseconds(30));
      Event event;
      while (renderer.pollEvent(event)) {
        propagateEvent(event, {});
        if (event.type == Event::Resized) {
          resize(event.size.width, event.size.height);
        }
      }
    }
    splashDone = false;
    renderDialog.pop();
  });
}

void WindowView::displaySplash(const ProgressMeter& meter, SplashType type, function<void()> cancelFun) {
  RenderLock lock(renderMutex);
  string text;
  switch (type) {
    case SplashType::CREATING: text = "Creating a new world, just for you..."; break;
    case SplashType::LOADING: text = "Loading the game..."; break;
    case SplashType::SAVING: text = "Saving the game..."; break;
    case SplashType::UPLOADING: text = "Uploading the map..."; break;
    case SplashType::DOWNLOADING: text = "Downloading the map..."; break;
    case SplashType::AUTOSAVING: displayAutosaveSplash(meter); return;
  }
  splashDone = false;
  renderDialog.push([=, &meter] {
    int t0 = clock->getRealMillis();
    int mouthMillis = 400;
    Texture& loadingSplash = gui.get(GuiFactory::TexId::LOADING_SPLASH);
    string cancelText = "[cancel]";
    while (!splashDone) {
      Vec2 textPos = useTiles ? Vec2(renderer.getSize().x / 2, renderer.getSize().y * 0.5)
        : Vec2(renderer.getSize().x / 2, renderer.getSize().y - 60);
      Rectangle cancelBut(textPos.x - renderer.getTextLength(cancelText) / 2, textPos.y + 30,
        textPos.x + renderer.getTextLength(cancelText) / 2, textPos.y + 60);
      if (useTiles)
        drawMenuBackground(meter.getProgress(), min(1.0, double(clock->getRealMillis() - t0) / mouthMillis));
      else
        renderer.drawImage((renderer.getSize().x - loadingSplash.getSize().x) / 2,
            (renderer.getSize().y - loadingSplash.getSize().y) / 2, loadingSplash);
      renderer.drawText(colors[ColorId::WHITE], textPos.x, textPos.y, text, Renderer::HOR);
      if (cancelFun)
        renderer.drawText(colors[ColorId::LIGHT_BLUE], cancelBut.getPX(), cancelBut.getPY(), cancelText);
      renderer.drawAndClearBuffer();
      sf::sleep(sf::milliseconds(30));
      Event event;
      while (renderer.pollEvent(event)) {
        if (event.type == Event::Resized) {
          resize(event.size.width, event.size.height);
        }
        if (event.type == Event::MouseButtonPressed && cancelFun) {
          if (Vec2(event.mouseButton.x, event.mouseButton.y).inRectangle(cancelBut))
            cancelFun();
        }
      }
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

struct KeyInfo {
  string keyDesc;
  string action;
  Event::KeyEvent event;
};

void WindowView::close() {
}

Color getSpeedColor(int value) {
  if (value > 100)
    return Color(max(0, 255 - (value - 100) * 2), 255, max(0, 255 - (value - 100) * 2));
  else
    return Color(255, max(0, 255 + (value - 100) * 4), max(0, 255 + (value - 100) * 4));
}

void WindowView::rebuildGui() {
  PGuiElem bottom, right;
  vector<GuiBuilder::OverlayInfo> overlays;
  int rightBarWidth = 0;
  int bottomBarHeight = 0;
  int rightBottomMargin = 30;
  tempGuiElems.clear();
  tempGuiElems.push_back(gui.mouseWheel([this](bool up) { zoom(!up); }));
  tempGuiElems.back()->setBounds(getMapGuiBounds());
  tempGuiElems.push_back(gui.keyHandler(bindMethod(&WindowView::keyboardAction, this)));
  tempGuiElems.back()->setBounds(getMapGuiBounds());
  switch (gameInfo.infoType) {
    case GameInfo::InfoType::SPECTATOR:
        right = gui.empty();
        bottom = gui.empty();
        rightBarWidth = 0;
        bottomBarHeight = 0;
        if (getMapGuiBounds().getPX() > 0) {
          tempGuiElems.push_back(gui.rectangle(colors[ColorId::BLACK]));
          tempGuiElems.back()->setBounds(Rectangle(0, 0, getMapGuiBounds().getPX(), renderer.getSize().y));
          tempGuiElems.push_back(gui.rectangle(colors[ColorId::BLACK]));
          tempGuiElems.back()->setBounds(Rectangle(Vec2(getMapGuiBounds().getKX(), 0), renderer.getSize()));
        }
        if (getMapGuiBounds().getPY() > 0) {
          tempGuiElems.push_back(gui.rectangle(colors[ColorId::BLACK]));
          tempGuiElems.back()->setBounds(Rectangle(0, 0, renderer.getSize().x, getMapGuiBounds().getPY()));
          tempGuiElems.push_back(gui.rectangle(colors[ColorId::BLACK]));
          tempGuiElems.back()->setBounds(Rectangle(Vec2(0, getMapGuiBounds().getKY()), renderer.getSize()));
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
        right = guiBuilder.drawRightBandInfo(gameInfo.collectiveInfo, gameInfo.villageInfo);
        guiBuilder.drawBandOverlay(overlays, gameInfo.collectiveInfo);
        bottom = guiBuilder.drawBottomBandInfo(gameInfo);
        rightBarWidth = rightBarWidthCollective;
        bottomBarHeight = bottomBarHeightCollective;
        break;
  }
  resetMapBounds();
  int bottomOffset = 35;
  int sideOffset = 10;
  int rightWindowHeight = 80;
  if (rightBarWidth > 0) {
    tempGuiElems.push_back(gui.mainDecoration(rightBarWidth, bottomBarHeight));
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
      tempGuiElems.back()->setBounds(Rectangle(pos, pos + Vec2(width, height)));
      Debug() << "Overlay " << overlay.alignment << " bounds " << tempGuiElems.back()->getBounds();
    }
  }
}

vector<GuiElem*> WindowView::getAllGuiElems() {
  if (!gameReady)
    return extractRefs(blockingElems);
  CHECK(currentThreadId() == renderThreadId);
  if (gameInfo.infoType == GameInfo::InfoType::SPECTATOR)
    return concat({mapGui}, extractRefs(tempGuiElems));
  vector<GuiElem*> ret = concat(extractRefs(tempGuiElems), extractRefs(blockingElems));
  if (gameReady)
    ret = concat(concat({mapGui}, ret), {minimapDecoration.get(), minimapGui});
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

void WindowView::resetCenter() {
  mapGui->clearCenter();
}

void WindowView::addVoidDialog(function<void()> fun) {
  RenderLock lock(renderMutex);
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
  minimapGui->update(level, bounds, creature);
}

void WindowView::updateView(const CreatureView* collective, bool noRefresh) {
  if (!wasRendered && currentThreadId() != renderThreadId)
    return;
  GameInfo newInfo;
  collective->refreshGameInfo(newInfo);
  RenderLock lock(renderMutex);
  wasRendered = false;
  guiBuilder.addUpsCounterTick();
  gameReady = true;
  if (!noRefresh)
    uiLock = false;
  switchTiles();
  gameInfo = newInfo;
  mapGui->setSpriteMode(currentTileLayout.sprites);
  bool spectator = gameInfo.infoType == GameInfo::InfoType::SPECTATOR;
  mapGui->updateObjects(collective, mapLayout, currentTileLayout.sprites || spectator,
      !spectator, guiBuilder.showMorale());
  updateMinimap(collective);
  if (gameInfo.infoType == GameInfo::InfoType::SPECTATOR)
    guiBuilder.setGameSpeed(GuiBuilder::GameSpeed::NORMAL);
}

void WindowView::animateObject(vector<Vec2> trajectory, ViewObject object) {
  RenderLock lock(renderMutex);
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
  RenderLock lock(renderMutex);
  if (currentTileLayout.sprites)
    mapGui->addAnimation(Animation::fromId(id), pos);
}

void WindowView::refreshView() {
  if (currentThreadId() != renderThreadId)
    return;
  {
    RenderLock lock(renderMutex);
    if (!wasRendered && gameReady)
      rebuildGui();
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
  guiBuilder.addFpsCounterTick();
}

void WindowView::refreshScreen(bool flipBuffer) {
  {
    RenderLock lock(renderMutex);
    if (fullScreenTrigger > -1) {
      renderer.initialize(fullScreenTrigger);
      fullScreenTrigger = -1;
    }
    if (fullScreenResolution > -1) {
      if (renderer.isFullscreen())
        renderer.initialize(true, fullScreenResolution);
      fullScreenResolution = -1;
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
  RenderLock lock(renderMutex);
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
    if (event.type == Event::MouseMoved || event.type == Event::MouseButtonPressed) {
      if (auto pos = mapGui->projectOnMap(renderer.getMousePos())) {
        refreshScreen(false);
        Vec2 middle = mapLayout->getAllTiles(getMapGuiBounds(), Level::getMaxBounds(), mapGui->getScreenPos())
            .middle();
        if (pos == middle)
          continue;
        Vec2 dir = (*pos - middle).getBearing();
        Vec2 wpos = mapLayout->projectOnScreen(getMapGuiBounds(), mapGui->getScreenPos(),
            middle.x + dir.x, middle.y + dir.y);
        if (currentTileLayout.sprites)
          renderer.drawTile(wpos, {Vec2(17, 5) + dir, 4}, mapLayout->getSquareSize());
        else {
          int numArrow = int(dir.getCardinalDir());
          static sf::Uint32 arrows[] = { L'⇑', L'⇓', L'⇒', L'⇐', L'⇗', L'⇖', L'⇘', L'⇙'};
          renderer.drawText(Renderer::SYMBOL_FONT, mapLayout->getSquareSize().y, colors[ColorId::WHITE],
              wpos.x + mapLayout->getSquareSize().x / 2, wpos.y, arrows[numArrow], Renderer::HOR);
        }
        renderer.drawAndClearBuffer();
        if (event.type == Event::MouseButtonPressed)
          return dir;
      }
      renderer.flushEvents(Event::MouseMoved);
    } else
    refreshScreen();
    if (event.type == Event::KeyPressed)
      switch (event.key.code) {
        case Keyboard::Up:
        case Keyboard::Numpad8: refreshScreen(); return Vec2(0, -1);
        case Keyboard::Numpad9: refreshScreen(); return Vec2(1, -1);
        case Keyboard::Right:
        case Keyboard::Numpad6: refreshScreen(); return Vec2(1, 0);
        case Keyboard::Numpad3: refreshScreen(); return Vec2(1, 1);
        case Keyboard::Down:
        case Keyboard::Numpad2: refreshScreen(); return Vec2(0, 1);
        case Keyboard::Numpad1: refreshScreen(); return Vec2(-1, 1);
        case Keyboard::Left:
        case Keyboard::Numpad4: refreshScreen(); return Vec2(-1, 0);
        case Keyboard::Numpad7: refreshScreen(); return Vec2(-1, -1);
        case Keyboard::Escape: refreshScreen(); return none;
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
      MenuType::YES_NO, nullptr, none, none, {}) == 0;
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
  return chooseFromListInternal(title, options, index, type, scrollPos, exitAction, none, {});
}

optional<string> WindowView::getText(const string& title, const string& value, int maxLength, const string& hint) {
  RenderLock lock(renderMutex);
  TempClockPause pause(clock);
  SyncQueue<optional<string>> returnQueue;
  addReturnDialog<optional<string>>(returnQueue, [=] ()-> optional<string> {
    return guiBuilder.getTextInput(title, value, maxLength, hint);
  });
  lock.unlock();
  return returnQueue.pop();
}

optional<MinionAction> WindowView::getMinionAction(const vector<PlayerInfo>& minions,
    UniqueEntity<Creature>::Id& currentMinion) {
  RenderLock lock(renderMutex);
  uiLock = true;
  TempClockPause pause(clock);
  SyncQueue<optional<MinionAction>> returnQueue;
  addReturnDialog<optional<MinionAction>>(returnQueue, [=, &currentMinion] ()-> optional<MinionAction> {
    optional<optional<MinionAction>> ret;
    tempGuiElems.push_back(gui.stack(gui.reverseButton([&ret] { ret = optional<MinionAction>(none); }),
        gui.window(guiBuilder.drawMinionMenu(minions, currentMinion, [&] (optional<MinionAction> r) { ret = r; }),
          [&ret] { ret = optional<MinionAction>(none); })));
    GuiElem* menu = tempGuiElems.back().get();
    menu->setBounds(guiBuilder.getMinionMenuPosition());
    PGuiElem bg = gui.darken();
    bg->setBounds(getMapGuiBounds());
    while (1) {
      refreshScreen(false);
      bg->render(renderer);
      menu->render(renderer);
      renderer.drawAndClearBuffer();
      Event event;
      while (renderer.pollEvent(event)) {
        propagateEvent(event, {menu});
        if (ret) {
          tempGuiElems.pop_back();
          return *ret;
        }
        if (considerResizeEvent(event))
          continue;
      }
    }
  });
  lock.unlock();
  return returnQueue.pop();
}

optional<int> WindowView::chooseItem(const vector<PlayerInfo>& minions, UniqueEntity<Creature>::Id& cur,
    const vector<ItemInfo>& items, double* scrollPos1) {
  RenderLock lock(renderMutex);
  uiLock = true;
  TempClockPause pause(clock);
  SyncQueue<optional<int>> returnQueue;
  addReturnDialog<optional<int>>(returnQueue, [=, &cur] ()-> optional<int> {
    double* scrollPos = scrollPos1;
    double localScrollPos;
    if (!scrollPos)
      scrollPos = &localScrollPos;
    optional<optional<int>> retVal;
    vector<PGuiElem> lines = guiBuilder.drawItemMenu(items,
      [&retVal] (Rectangle butBounds, optional<int> a) { retVal = a;}, true);
    int menuHeight = lines.size() * guiBuilder.getStandardLineHeight() + 30;
    PGuiElem menu = gui.stack(gui.reverseButton([&retVal] { retVal = optional<int>(none); }),
        gui.miniWindow(gui.margins(
            gui.scrollable(gui.verticalList(std::move(lines), guiBuilder.getStandardLineHeight()), scrollPos),
        15, 15, 15, 15)));
    PGuiElem bg1 = gui.window(guiBuilder.drawMinionMenu(minions, cur, [&] (optional<MinionAction>) { }),
          [&retVal] { retVal = optional<int>(none); });
    bg1->setBounds(guiBuilder.getMinionMenuPosition());
    PGuiElem bg2 = gui.darken();
    bg2->setBounds(renderer.getSize());
    while (1) {
      refreshScreen(false);
      menu->setBounds(guiBuilder.getEquipmentMenuPosition(menuHeight));
      bg1->render(renderer);
      bg2->render(renderer);
      menu->render(renderer);
      renderer.drawAndClearBuffer();
      Event event;
      while (renderer.pollEvent(event)) {
        propagateEvent(event, {menu.get(), bg1.get()});
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

optional<UniqueEntity<Creature>::Id> WindowView::chooseRecruit(const string& title, pair<ViewId, int> budget,
    const vector<CreatureInfo>& creatures, double* scrollPos) {
  SyncQueue<optional<UniqueEntity<Creature>::Id>> returnQueue;
  return getBlockingGui(returnQueue, guiBuilder.drawRecruitMenu(returnQueue, title, budget, creatures, scrollPos),
      Vec2(rightBarWidthCollective + 30, 80));
}

optional<UniqueEntity<Item>::Id> WindowView::chooseTradeItem(const string& title, pair<ViewId, int> budget,
    const vector<ItemInfo>& items, double* scrollPos) {
  SyncQueue<optional<UniqueEntity<Item>::Id>> returnQueue;
  return getBlockingGui(returnQueue, guiBuilder.drawTradeItemMenu(returnQueue, title, budget, items, scrollPos),
      Vec2(rightBarWidthCollective + 30, 80));
}

void WindowView::getBlockingGui(Semaphore& sem, PGuiElem elem, Vec2 origin) {
  RenderLock lock(renderMutex);
  TempClockPause pause(clock);
  if (blockingElems.empty()) {
    blockingElems.push_back(gui.darken());
    blockingElems.back()->setBounds(renderer.getSize());
  }
  blockingElems.push_back(std::move(elem));
  blockingElems.back()->setPreferredBounds(origin);
  lock.unlock();
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
      guiBuilder.getMenuPosition(MenuType::NORMAL).getTopLeft());
}

PGuiElem WindowView::drawGameChoices(optional<GameTypeChoice>& choice,optional<GameTypeChoice>& index) {
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
            gui.button([&] { choice = GameTypeChoice::LOAD;}),
            gui.mainMenuLabelBg("Load game", 0.2),
            gui.mouseHighlightGameChoice(gui.mainMenuLabel("Load game", 0.2), GameTypeChoice::LOAD, index)),
          gui.stack(
            gui.button([&] { choice = GameTypeChoice::BACK;}),
            gui.mainMenuLabelBg("Go back", 0.2),
            gui.mouseHighlightGameChoice(gui.mainMenuLabel("Go back", 0.2), GameTypeChoice::BACK, index)
          )), 0), 0, 30, 0, 0),
      0.7, gui.TOP), 1);
}

GameTypeChoice WindowView::chooseGameType() {
  if (!useTiles) {
    if (auto ind = chooseFromListInternal("", {
          ListElem("Choose your role:", ListElem::TITLE),
          "Keeper",
          "Adventurer",
          ListElem("Or simply:", ListElem::TITLE),
          "Load game",
          "Go back",
          },
        0, MenuType::MAIN, nullptr, none, none, {}))
      return (GameTypeChoice)(*ind);
    else
      return GameTypeChoice::BACK;
  }
  RenderLock lock(renderMutex);
  uiLock = true;
  TempClockPause pause(clock);
  SyncQueue<GameTypeChoice> returnQueue;
  addReturnDialog<GameTypeChoice>(returnQueue, [=] ()-> GameTypeChoice {
  optional<GameTypeChoice> choice;
  optional<GameTypeChoice> index = GameTypeChoice::KEEPER;
  PGuiElem stuff = drawGameChoices(choice, index);
  while (1) {
    refreshScreen(false);
    stuff->setBounds(guiBuilder.getMenuPosition(MenuType::GAME_CHOICE));
    stuff->render(renderer);
    renderer.drawAndClearBuffer();
    Event event;
    while (renderer.pollEvent(event)) {
      propagateEvent(event, {stuff.get()});
      if (choice)
        return *choice;
      if (considerResizeEvent(event))
        continue;
      if (event.type == Event::KeyPressed)
        switch (event.key.code) {
          case Keyboard::Numpad4:
          case Keyboard::Left:
            if (index != GameTypeChoice::KEEPER)
              index = GameTypeChoice::KEEPER;
            else
              index = GameTypeChoice::ADVENTURER;
            break;
          case Keyboard::Numpad6:
          case Keyboard::Right:
            if (index != GameTypeChoice::ADVENTURER)
              index = GameTypeChoice::ADVENTURER;
            else
              index = GameTypeChoice::KEEPER;
            break;
          case Keyboard::Numpad8:
          case Keyboard::Up:
            if (!index)
              index = GameTypeChoice::KEEPER;
            else
              switch (*index) {
                case GameTypeChoice::LOAD: index = GameTypeChoice::KEEPER; break;
                case GameTypeChoice::BACK: index = GameTypeChoice::LOAD; break;
                case GameTypeChoice::KEEPER: 
                case GameTypeChoice::ADVENTURER: index = GameTypeChoice::BACK; break;
                default: break;
              }
            break;
          case Keyboard::Numpad2:
          case Keyboard::Down:
            if (!index)
              index = GameTypeChoice::KEEPER;
            else
              switch (*index) {
                case GameTypeChoice::LOAD: index = GameTypeChoice::BACK; break;
                case GameTypeChoice::BACK: index = GameTypeChoice::KEEPER; break;
                case GameTypeChoice::KEEPER: 
                case GameTypeChoice::ADVENTURER: index = GameTypeChoice::LOAD; break;
                default: break;
              }
            break;
          case Keyboard::Numpad5:
          case Keyboard::Return: if (index) return *index;
          case Keyboard::Escape: return GameTypeChoice::BACK;
          default: break;
        }
    }
  }
  });
  lock.unlock();
  return returnQueue.pop();
}

optional<int> WindowView::chooseFromListInternal(const string& title, const vector<ListElem>& options, int index1,
    MenuType menuType, double* scrollPos1, optional<UserInputId> exitAction, optional<Event::KeyEvent> exitKey,
    vector<Event::KeyEvent> shortCuts) {
  if (!useTiles && menuType == MenuType::MAIN)
    menuType = MenuType::MAIN_NO_TILES;
  if (options.size() == 0)
    return none;
  RenderLock lock(renderMutex);
  uiLock = true;
  inputQueue.push(UserInputId::REFRESH);
  TempClockPause pause(clock);
  SyncQueue<optional<int>> returnQueue;
  addReturnDialog<optional<int>>(returnQueue, [=] ()-> optional<int> {
  renderer.flushEvents(Event::KeyPressed);
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
      gui.keyHandler([&] (Event::KeyEvent event) {
        switch (event.code) {
          case Keyboard::Numpad8:
          case Keyboard::Up:
            if (count > 0) {
              index = (index - 1 + count) % count;
              *scrollPos = guiBuilder.getScrollPos(optionIndexes[index], options.size());
            } else
              *scrollPos = max<double>(0, *scrollPos - 1);
            break;
          case Keyboard::Numpad2:
          case Keyboard::Down:
            if (count > 0) {
              index = (index + 1 + count) % count;
              *scrollPos = guiBuilder.getScrollPos(optionIndexes[index], options.size());
            } else
              *scrollPos = min<int>(options.size() - 1, *scrollPos + 1);
            break;
          case Keyboard::Numpad5:
          case Keyboard::Return: 
            if (count > 0 && index > -1) {
              CHECK(index < indexes.size()) <<
                  index << " " << indexes.size() << " " << count << " " << options.size();
              callbackRet = indexes[index];
              break;
            }
          case Keyboard::Escape: callbackRet = optional<int>(none);
          default: break;
        }        
      }, true));
  while (1) {
    refreshScreen(false);
    stuff->setBounds(guiBuilder.getMenuPosition(menuType));
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
  chooseFromListInternal(title, conv, -1, menu, &scrollPos, exitAction, none, {});
}

void WindowView::switchZoom() {
  refreshInput = true;
  if (mapLayout != &currentTileLayout.normalLayout)
    mapLayout = &currentTileLayout.normalLayout;
  else
    mapLayout = &currentTileLayout.unzoomLayout;
}

void WindowView::zoom(bool out) {
  refreshInput = true;
  if (mapLayout != &currentTileLayout.normalLayout && !out)
    mapLayout = &currentTileLayout.normalLayout;
  else if (out)
    mapLayout = &currentTileLayout.unzoomLayout;
}

void WindowView::switchTiles() {
  bool normal = (mapLayout == &currentTileLayout.normalLayout);
  if (options->getBoolValue(OptionId::ASCII) || !useTiles)
    currentTileLayout = asciiLayouts;
  else
    currentTileLayout = spriteLayouts;
  if (gameInfo.infoType == GameInfo::InfoType::SPECTATOR && useTiles &&
      renderer.getSize().x < Level::getSplashVisibleBounds().getW() * mapLayout->getSquareSize().x)
    normal = false;
  if (normal)
    mapLayout = &currentTileLayout.normalLayout;
  else
    mapLayout = &currentTileLayout.unzoomLayout;
}

bool WindowView::travelInterrupt() {
  return lockKeyboard;
}

int WindowView::getTimeMilli() {
  return clock->getMillis();
}

int WindowView::getTimeMilliAbsolute() {
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

bool WindowView::considerResizeEvent(sf::Event& event) {
  if (event.type == Event::Resized) {
    resize(event.size.width, event.size.height);
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
        case Event::KeyPressed:
        case Event::MouseWheelMoved:
        case Event::MouseButtonPressed:
          inputQueue.push(UserInput(UserInputId::EXIT));
          return;
        default:break;
      }
    else
      propagateEvent(event, getClickableGuiElems());
    switch (event.type) {
      case Event::KeyPressed:
        renderer.flushEvents(Event::KeyPressed);
        break;
      case Event::MouseButtonPressed :
        if (event.mouseButton.button == sf::Mouse::Right)
          guiBuilder.closeOverlayWindows();
        if (event.mouseButton.button == sf::Mouse::Middle)
          inputQueue.push(UserInput(UserInputId::DRAW_LEVEL_MAP));
        break;
      case Event::MouseButtonReleased :
        if (event.mouseButton.button == sf::Mouse::Left)
          inputQueue.push(UserInput(UserInputId::BUTTON_RELEASE, BuildingInfo{Vec2(0, 0),
              guiBuilder.getActiveBuilding()}));
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
    case Event::MouseButtonReleased:
      // MapGui needs this event otherwise it will sometimes lock the mouse button
      mapGui->onMouseRelease(Vec2(event.mouseButton.x, event.mouseButton.y));
      break;
    case Event::MouseButtonPressed:
      lockKeyboard = true;
      break;
    default:break;
  }
  GuiElem::propagateEvent(event, guiElems);
}

UserInputId getDirActionId(const Event::KeyEvent& key) {
  if (key.control)
    return UserInputId::TRAVEL;
  if (key.alt)
    return UserInputId::FIRE;
  else
    return UserInputId::MOVE;
}

void WindowView::keyboardAction(Event::KeyEvent key) {
  if (lockKeyboard)
    return;
  lockKeyboard = true;
  switch (key.code) {
    case Keyboard::Z: switchZoom(); break;
    case Keyboard::F2: options->handle(this, OptionSet::GENERAL); refreshScreen(); break;
    case Keyboard::Space:
      inputQueue.push(UserInput(UserInputId::WAIT));
      break;
    case Keyboard::Escape:
      if (!renderer.isMonkey())
        inputQueue.push(UserInput(UserInputId::EXIT));
      break;
    case Keyboard::Up:
    case Keyboard::Numpad8:
      inputQueue.push(UserInput(getDirActionId(key), Vec2(0, -1)));
      mapGui->onMouseGone();
      break;
    case Keyboard::Numpad9:
      inputQueue.push(UserInput(getDirActionId(key), Vec2(1, -1)));
      mapGui->onMouseGone();
      break;
    case Keyboard::Right: 
    case Keyboard::Numpad6:
      inputQueue.push(UserInput(getDirActionId(key), Vec2(1, 0)));
      mapGui->onMouseGone();
      break;
    case Keyboard::Numpad3:
      inputQueue.push(UserInput(getDirActionId(key), Vec2(1, 1)));
      mapGui->onMouseGone();
      break;
    case Keyboard::Down:
    case Keyboard::Numpad2:
      inputQueue.push(UserInput(getDirActionId(key), Vec2(0, 1)));
      mapGui->onMouseGone();
      break;
    case Keyboard::Numpad1:
      inputQueue.push(UserInput(getDirActionId(key), Vec2(-1, 1)));
      mapGui->onMouseGone();
      break;
    case Keyboard::Left:
    case Keyboard::Numpad4:
      inputQueue.push(UserInput(getDirActionId(key), Vec2(-1, 0)));
      mapGui->onMouseGone();
      break;
    case Keyboard::Numpad7:
      inputQueue.push(UserInput(getDirActionId(key), Vec2(-1, -1)));
      mapGui->onMouseGone();
      break;
    case Keyboard::M: inputQueue.push(UserInput(UserInputId::SHOW_HISTORY)); break;
    case Keyboard::H: inputQueue.push(UserInput(UserInputId::HIDE)); break;
    case Keyboard::P: inputQueue.push(UserInput(UserInputId::PAY_DEBT)); break;
    case Keyboard::C: inputQueue.push(UserInput(key.shift ? UserInputId::CONSUME : UserInputId::CHAT)); break;
    case Keyboard::U: inputQueue.push(UserInput(UserInputId::UNPOSSESS)); break;
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
