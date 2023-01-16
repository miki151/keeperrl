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
#include "level.h"
#include "options.h"
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
#include "player_role.h"
#include "file_sharing.h"
#include "fx_manager.h"
#include "fx_renderer.h"
#include "fx_info.h"
#include "fx_view_manager.h"
#include "draw_line.h"
#include "tileset.h"
#include "target_type.h"
#include "keybinding_map.h"
#include "mouse_button_id.h"
#include "steam_input.h"

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

constexpr int rightBarWidthCollective = 330;
constexpr int rightBarWidthPlayer = 330;
constexpr int bottomBarHeightCollective = 66;
constexpr int bottomBarHeightPlayer = 66;

Rectangle WindowView::getMapGuiBounds() const {
  auto type = gameInfo.infoType;
  if (gameInfo.takingScreenshot)
    return Rectangle(renderer.getSize());
  else
    switch (type) {
      case GameInfo::InfoType::PLAYER:
        return Rectangle(Vec2(rightBarWidthPlayer, 0), renderer.getSize() - Vec2(0, bottomBarHeightPlayer));
      case GameInfo::InfoType::BAND:
        return Rectangle(Vec2(rightBarWidthCollective, 0), renderer.getSize() - Vec2(0, bottomBarHeightCollective));
      case GameInfo::InfoType::SPECTATOR:
        return Rectangle(Vec2(0, 0), renderer.getSize());
    }
}

int WindowView::getMinimapWidth() const {
  auto ret = max(149, renderer.getSize().x / 11);
  if (guiBuilder.isEnlargedMinimap())
    ret *= 3;
  return ret;
}

Vec2 WindowView::getMinimapOrigin() const {
  Vec2 offset(-20, 70);
  return Vec2(renderer.getSize().x - getMinimapWidth(), 0) + offset;
}

WindowView::WindowView(ViewParams params) : renderer(params.renderer), gui(params.gui), useTiles(params.useTiles),
    options(params.options), clock(params.clock), guiBuilder(renderer, gui, clock, params.options, {
        [this](UserInput input) { inputQueue.push(input);},
        [this]() { refreshScreen(false);},
        [this](const string& s) { presentText("", s); },
        }), zoomUI(-1),
    soundLibrary(params.soundLibrary), bugreportSharing(params.bugreportSharing), bugreportDir(params.bugreportDir),
    installId(params.installId) {}

void WindowView::initialize(unique_ptr<fx::FXRenderer> fxRenderer, unique_ptr<FXViewManager> fxViewManager) {
  renderer.setFullscreen(options->getBoolValue(OptionId::FULLSCREEN));
  renderer.setVsync(options->getBoolValue(OptionId::VSYNC));
  renderer.enableCustomCursor(!options->getBoolValue(OptionId::DISABLE_CURSOR));
  renderer.initialize();
  renderer.setZoom(options->getBoolValue(OptionId::ZOOM_UI) ? 2 : 1);
  renderer.setFpsLimit(options->getIntValue(OptionId::FPS_LIMIT));
  options->addTrigger(OptionId::FULLSCREEN, [this] (int on) {
    renderer.setFullscreen(on);
    renderer.initialize();
  });
  options->addTrigger(OptionId::VSYNC, [this] (int on) {
    renderer.setVsync(on);
  });
  options->addTrigger(OptionId::FPS_LIMIT, [this] (int fps) {
    renderer.setFpsLimit(fps);
  });
  options->addTrigger(OptionId::DISABLE_CURSOR, [this] (int on) { renderer.enableCustomCursor(!on); });
  options->addTrigger(OptionId::ZOOM_UI, [this] (int on) { zoomUI = on; });
  renderThreadId = currentThreadId();
  vector<ViewLayer> allLayers;
  for (auto l : ENUM_ALL(ViewLayer))
    allLayers.push_back(l);
  asciiLayouts = {
    {MapLayout(Vec2(16, 20), allLayers),
    MapLayout(Vec2(8, 10),
        {ViewLayer::FLOOR_BACKGROUND, ViewLayer::FLOOR, ViewLayer::CREATURE})}, false};
  spriteLayouts = {{
    MapLayout(Vec2(48, 48), allLayers),
//    MapLayout(Vec2(36, 36), allLayers),
    MapLayout(Vec2(24, 24), allLayers)}, true};
  if (useTiles)
    currentTileLayout = spriteLayouts;
  else
    currentTileLayout = asciiLayouts;
  this->fxRenderer = fxRenderer.get();
  mapGui.reset(new MapGui({
      bindMethod(&WindowView::mapContinuousLeftClickFun, this),
      [this] (Vec2 pos) {
        if (!guiBuilder.getActiveButton()) {
          guiBuilder.closeOverlayWindowsAndClearButton();
          inputQueue.push(UserInput(UserInputId::TILE_CLICK, pos));
        }
      },
      bindMethod(&WindowView::mapRightClickFun, this),
      [this] (Vec2) { guiBuilder.toggleEnlargedMinimap(); },
      [this] { refreshInput = true;},
      },
      inputQueue,
      clock,
      options,
      &gui,
      std::move(fxRenderer),
      std::move(fxViewManager)));
  minimapGui.reset(new MinimapGui([this](Vec2 pos) {
    if (guiBuilder.isEnlargedMinimap()) {
      auto bounds = minimapGui->getBounds();
      double x = double(pos.x - bounds.left()) / bounds.width();
      double y = double(pos.y - bounds.top()) / bounds.height();
      mapGui->setCenterRatio(x, y);
    }
    guiBuilder.toggleEnlargedMinimap();
  }));
  rebuildMinimapGui();
  guiBuilder.setMapGui(mapGui);
}

bool WindowView::isKeyPressed(SDL::SDL_Scancode code) {
  auto steamInput = renderer.getSteamInput();
  return SDL::SDL_GetKeyboardState(nullptr)[code] ||
      (code == SDL::SDL_SCANCODE_LSHIFT && steamInput && steamInput->isPressed(C_SHIFT));
}

void WindowView::mapContinuousLeftClickFun(Vec2 pos) {
  guiBuilder.closeOverlayWindows();
  optional<int> activeBuilding = guiBuilder.getActiveButton();
  auto collectiveTab = guiBuilder.getCollectiveTab();
  switch (gameInfo.infoType) {
    case GameInfo::InfoType::BAND:
      if (collectiveTab == CollectiveTab::BUILDINGS && activeBuilding)
          inputQueue.push(UserInput(UserInputId::RECT_SELECTION, BuildingClickInfo{pos, *activeBuilding}));
      break;
    default:
      break;
  }
}

void WindowView::mapRightClickFun(Vec2 pos) {
  inputQueue.push(UserInput(UserInputId::CREATURE_MAP_CLICK_EXTENDED, pos));
  switch (gameInfo.infoType) {
    case GameInfo::InfoType::SPECTATOR:
    case GameInfo::InfoType::BAND:
      guiBuilder.closeOverlayWindows();
      guiBuilder.clearActiveButton();
      break;
    default:
      break;
  }
}

void WindowView::reset() {
  mapLayout = &currentTileLayout.layouts[0];
  gameReady = false;
  wasRendered = false;
  minimapGui->clear();
  mapGui->clearCenter();
  guiBuilder.reset();
  gameInfo = GameInfo{};
  soundQueue.clear();
}

void WindowView::getSmallSplash(const ProgressMeter* meter, const string& text, function<void()> cancelFun) {
  renderer.getSteamInput()->pushActionSet(MySteamInput::ActionSet::MENU);
  auto o = OnExit([this] { renderer.getSteamInput()->popActionSet(); });
  auto elems = ScriptedUIDataElems::Record {{
    {"text", text},
    {"barCallback", ScriptedUIDataElems::DynamicWidthCallback { [meter] {
      return meter ? meter->getProgress() : 0.0;}}}
  }};
  if (cancelFun)
    elems.elems["cancelCallback"] = ScriptedUIDataElems::Callback { [cancelFun] { cancelFun(); return true; }};
  ScriptedUIData data = std::move(elems);
  ScriptedUIState state;
  auto elem = gui.scripted([]{}, ScriptedUIId("loading_screen"), data, state);
  auto windowSize = *elem->getPreferredSize();
  Rectangle bounds((renderer.getSize() - windowSize) / 2, (renderer.getSize() + windowSize) / 2);
  Rectangle progressBar(bounds.minusMargin(15));
  elem->setBounds(bounds);
  while (!splashDone) {
    refreshScreen(false);
    elem->render(renderer);
    renderer.drawAndClearBuffer();
    sleep_for(milliseconds(30));
    Event event;
    while (renderer.pollEvent(event)) {
      propagateEvent(event, {elem});
      considerResizeEvent(event);
    }
  }
}

void WindowView::displaySplash(const ProgressMeter* meter, const string& text, function<void()> cancelFun) {
  splashDone = false;
  renderDialog.push([=] {
    getSmallSplash(meter, text, cancelFun);
    splashDone = false;
    renderDialog.pop();
  });
}

void WindowView::clearSplash() {
  splashDone = true;
  if (currentThreadId() != renderThreadId)
    while (splashDone) {
      Progress::checkIfInterrupted();
    }
}

void WindowView::playVideo(const string& path) {
  renderer.playVideo(path, options->getIntValue(OptionId::SOUND) > 0);
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

void WindowView::rebuildMinimapGui() {
  int width = getMinimapWidth();
  auto icons = guiBuilder.drawMinimapIcons(gameInfo);
  auto iconsHeight = *icons->getPreferredHeight();
  minimapDecoration = gui.margin(std::move(icons),
      gui.stack(gui.rectangle(Color::BLACK), gui.miniWindow(),
      gui.margins(gui.renderInBounds(SGuiElem(minimapGui)), 6)), iconsHeight, GuiFactory::MarginType::BOTTOM);
  auto origin = getMinimapOrigin();
  minimapDecoration->setBounds(Rectangle(origin, origin + Vec2(width, width + iconsHeight)));

}

void WindowView::rebuildGui() {
  INFO << "Rebuilding UI";
  rebuildMinimapGui();
  mapGui->setBounds(getMapGuiBounds());
  SGuiElem bottom, right;
  vector<GuiBuilder::OverlayInfo> overlays;
  int rightBarWidth = 0;
  int bottomBarHeight = 0;
  optional<int> topBarHeight;
  int rightBottomMargin = 30;
  optional<Rectangle> bottomBarBounds;
  tempGuiElems.clear();
  if (!options->getIntValue(OptionId::DISABLE_MOUSE_WHEEL)) {
    tempGuiElems.push_back(gui.mouseWheel([this](bool up) {
      if (renderer.isKeypressed(SDL::SDL_SCANCODE_LCTRL))
        inputQueue.push(UserInput{UserInputId::SCROLL_STAIRS, up ? -1 : 1});
      else
        zoom(up ? -1 : 1);
    }));
    tempGuiElems.back()->setBounds(getMapGuiBounds());
  }
  auto getMovement = [this](int x, int y) {
    inputQueue.push(UserInput(UserInputId::MOVE, Vec2(x, y)));
    mapGui->onMouseGone();
  };
  tempGuiElems.push_back(gui.stack(makeVec(
      gui.keyHandler(bindMethod(&WindowView::keyboardAction, this)),
      gui.keyHandler([this]{ zoom(0); }, Keybinding("ZOOM_MAP"), true),
      gui.keyHandler([this]{ zoom(0); }, {gui.getKey(C_ZOOM)}, true),
      gui.keyHandler([this]{ guiBuilder.toggleEnlargedMinimap(); }, {gui.getKey(C_MINI_MAP)}, true),
      gui.keyHandler([getMovement]{ getMovement(0, -1); }, Keybinding("WALK_NORTH"), false),
      gui.keyHandler([getMovement]{ getMovement(0, 1); }, Keybinding("WALK_SOUTH"), false),
      gui.keyHandler([getMovement]{ getMovement(1, 0); }, Keybinding("WALK_EAST"), false),
      gui.keyHandler([getMovement]{ getMovement(-1, 0); }, Keybinding("WALK_WEST"), false),
      gui.keyHandler([getMovement]{ getMovement(1, -1); }, Keybinding("WALK_NORTH_EAST"), false),
      gui.keyHandler([getMovement]{ getMovement(-1, -1); }, Keybinding("WALK_NORTH_WEST"), false),
      gui.keyHandler([getMovement]{ getMovement(1, 1); }, Keybinding("WALK_SOUTH_EAST"), false),
      gui.keyHandler([getMovement]{ getMovement(-1, 1); }, Keybinding("WALK_SOUTH_WEST"), false),
      gui.keyHandler([getMovement]{ getMovement(0, -1); }, Keybinding("WALK_NORTH2"), false),
      gui.keyHandler([getMovement]{ getMovement(0, 1); }, Keybinding("WALK_SOUTH2"), false),
      gui.keyHandler([getMovement]{ getMovement(1, 0); }, Keybinding("WALK_EAST2"), false),
      gui.keyHandler([getMovement]{ getMovement(-1, 0); }, Keybinding("WALK_WEST2"), false)
  )));
  tempGuiElems.back()->setBounds(getMapGuiBounds());
  if (gameInfo.takingScreenshot) {
    right = gui.empty();
    bottom = gui.empty();
    rightBarWidth = 0;
    bottomBarHeight = 0;
    bottomBarBounds = Rectangle(Vec2(0, 0), renderer.getSize());
  } else
    switch (gameInfo.infoType) {
      case GameInfo::InfoType::SPECTATOR:
          right = gui.empty();
          bottom = gui.empty();
          rightBarWidth = 0;
          bottomBarHeight = 0;
          if (getMapGuiBounds().left() > 0) {
            tempGuiElems.push_back(gui.rectangle(Color::BLACK));
            tempGuiElems.back()->setBounds(Rectangle(0, 0, getMapGuiBounds().left(), renderer.getSize().y));
            tempGuiElems.push_back(gui.rectangle(Color::BLACK));
            tempGuiElems.back()->setBounds(Rectangle(Vec2(getMapGuiBounds().right(), 0), renderer.getSize()));
          }
          if (getMapGuiBounds().top() > 0) {
            tempGuiElems.push_back(gui.rectangle(Color::BLACK));
            tempGuiElems.back()->setBounds(Rectangle(0, 0, renderer.getSize().x, getMapGuiBounds().top()));
            tempGuiElems.push_back(gui.rectangle(Color::BLACK));
            tempGuiElems.back()->setBounds(Rectangle(Vec2(0, getMapGuiBounds().bottom()), renderer.getSize()));
          }
          break;
      case GameInfo::InfoType::PLAYER:
          right = guiBuilder.drawRightPlayerInfo(*gameInfo.playerInfo.getReferenceMaybe<PlayerInfo>());
          rightBarWidth = rightBarWidthPlayer;
          bottomBarHeight = bottomBarHeightPlayer;
          bottomBarBounds = Rectangle(Vec2(rightBarWidth, renderer.getSize().y - bottomBarHeight), renderer.getSize());
          bottom = guiBuilder.drawBottomPlayerInfo(gameInfo);
          break;
      case GameInfo::InfoType::BAND:
          right = guiBuilder.drawRightBandInfo(gameInfo);
          rightBarWidth = rightBarWidthCollective;
          bottomBarHeight = bottomBarHeightCollective;
          topBarHeight = 85;
          bottomBarBounds = Rectangle(Vec2(rightBarWidth, renderer.getSize().y - bottomBarHeight), renderer.getSize());
          bottom = guiBuilder.drawBottomBandInfo(gameInfo, bottomBarBounds->width());
          break;
    }
  if (gameInfo.infoType != GameInfo::InfoType::SPECTATOR) {
    if (!gameInfo.takingScreenshot) {
      tempGuiElems.push_back(gui.mainDecoration(rightBarWidth, bottomBarHeight, topBarHeight));
      tempGuiElems.back()->setBounds(Rectangle(renderer.getSize()));
    }
    tempGuiElems.push_back(gui.margins(std::move(right), 20, 20, 10, 0));
    tempGuiElems.back()->setBounds(Rectangle(Vec2(0, 0),
          Vec2(rightBarWidth, renderer.getSize().y - rightBottomMargin)));
    tempGuiElems.push_back(gui.margins(std::move(bottom), 105, 10, 105, 0));
    tempGuiElems.back()->setBounds(*bottomBarBounds);
    guiBuilder.drawOverlays(overlays, gameInfo);
    overlays.push_back({guiBuilder.drawMessages(gameInfo.messageBuffer, renderer.getSize().x - rightBarWidth),
                       GuiBuilder::OverlayInfo::MESSAGES});
    for (auto& overlay : overlays) {
      Vec2 pos;
      if (auto width = overlay.elem->getPreferredWidth())
        if (auto height = overlay.elem->getPreferredHeight()) {
          pos = getOverlayPosition(overlay.alignment, *height, *width, rightBarWidth, bottomBarHeight);
          tempGuiElems.push_back(std::move(overlay.elem));
          *height = min(*height, renderer.getSize().y - pos.y - bottomBarHeight);
          tempGuiElems.back()->setBounds(Rectangle(pos, pos + Vec2(*width, *height)));
        }
    }
  }
  tempGuiElems.push_back(gui.keyHandlerBool([getMovement, this]{
    auto res = renderer.getDiscreteJoyPos(ControllerJoy::WALKING);
    if (res != Vec2(0, 0)) {
      getMovement(res.x, res.y);
      return true;
    }
    return false;
  }, {gui.getKey(C_WALK)}));
  tempGuiElems.back()->setBounds(getMapGuiBounds());
  propagateMousePosition(getClickableGuiElems());
}

Vec2 WindowView::getOverlayPosition(GuiBuilder::OverlayInfo::Alignment alignment, int height, int width,
    int rightBarWidth, int bottomBarHeight) {
  int bottomOffset = 35;
  int sideOffset = 0;
  int rightWindowHeight = 80;
  switch (alignment) {
    case GuiBuilder::OverlayInfo::MINIONS:
      return Vec2(rightBarWidth - 20, rightWindowHeight - 6);
    case GuiBuilder::OverlayInfo::TOP_LEFT:
      return Vec2(rightBarWidth + sideOffset, rightWindowHeight);
    case GuiBuilder::OverlayInfo::IMMIGRATION:
      return Vec2(rightBarWidth, renderer.getSize().y - bottomBarHeight - height);
    case GuiBuilder::OverlayInfo::VILLAINS:
      return Vec2(rightBarWidth, renderer.getSize().y - bottomBarHeight - height);
    case GuiBuilder::OverlayInfo::BOTTOM_LEFT:
      return Vec2(rightBarWidth + guiBuilder.getImmigrationBarWidth(),
                 max(70, renderer.getSize().y - bottomBarHeight - 27 - height));
    case GuiBuilder::OverlayInfo::LEFT:
      return Vec2(sideOffset,
          renderer.getSize().y - bottomBarHeight - bottomOffset - height);
    case GuiBuilder::OverlayInfo::MESSAGES:
      return Vec2(rightBarWidth, 0);
    case GuiBuilder::OverlayInfo::TUTORIAL:
      return Vec2(rightBarWidth + guiBuilder.getImmigrationBarWidth(),
          renderer.getSize().y - bottomBarHeight - height);
    case GuiBuilder::OverlayInfo::MAP_HINT:
      return Vec2(renderer.getSize().x - width, renderer.getSize().y - bottomBarHeight - height);
    case GuiBuilder::OverlayInfo::CENTER:
      return (renderer.getSize() - Vec2(width, height)) / 2;
  }
}

void WindowView::propagateMousePosition(const vector<SGuiElem>& elems) {
  Event ev;
  ev.type = SDL::SDL_MOUSEMOTION;
  ev.motion.x = renderer.getMousePos().x;
  ev.motion.y = renderer.getMousePos().y;
  ev.motion.xrel = 0;
  ev.motion.yrel = 0;
  gui.propagateEvent(ev, elems);
}

vector<SGuiElem> WindowView::getAllGuiElems() {
  if (!gameReady)
    return blockingElems;
  CHECK(currentThreadId() == renderThreadId);
  if (gameInfo.infoType == GameInfo::InfoType::SPECTATOR)
    return concat({mapGui}, tempGuiElems);
  vector<SGuiElem> ret = concat(tempGuiElems, blockingElems);
  if (gameReady)
    ret = concat({mapGui, minimapDecoration}, ret);
  return ret;
}

vector<SGuiElem> WindowView::getClickableGuiElems() {
  CHECK(currentThreadId() == renderThreadId);
  if (gameInfo.infoType == GameInfo::InfoType::SPECTATOR)
    return {mapGui};
  if (!blockingElems.empty())
    return blockingElems.reverse();
  vector<SGuiElem> ret = tempGuiElems;
  std::reverse(ret.begin(), ret.end());
  if (gameReady) {
    ret.push_back(minimapDecoration);
    ret.push_back(mapGui);
  }
  return ret;
}

void WindowView::setScrollPos(Position pos) {
  mapGui->setCenter(pos.getCoord(), pos.getLevel());
}

void WindowView::resetCenter() {
  mapGui->resetScrolling();
}

void WindowView::updateMinimap(const CreatureView* creature) {
  Vec2 rad(40, 40);
  Vec2 playerPos = mapGui->getScreenPos().div(mapLayout->getSquareSize());
  Rectangle bounds(playerPos - rad, playerPos + rad);
  auto levelBounds = creature->getCreatureViewLevel()->getBounds();
  minimapGui->update(guiBuilder.isEnlargedMinimap() ? levelBounds : bounds, creature, renderer);
}

void WindowView::updateView(CreatureView* view, bool noRefresh) {
  ScopeTimer timer("UpdateView timer");
  if (!wasRendered && currentThreadId() != renderThreadId)
    return;
  gameInfo = {};
  view->refreshGameInfo(gameInfo);
  if (gameInfo.infoType != GameInfo::InfoType::BAND)
    guiBuilder.clearActiveButton();
  wasRendered = false;
  guiBuilder.addUpsCounterTick();
  gameReady = true;
  if (!noRefresh)
    uiLock = false;
  switchTiles();
  rebuildGui();
  mapGui->setSpriteMode(currentTileLayout.sprites);
  bool spectator = gameInfo.infoType == GameInfo::InfoType::SPECTATOR;
  mapGui->updateObjects(view, renderer, mapLayout, true, !spectator, gameInfo.tutorial);
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
        (sound.getPosition()->isSameLevel(view->getCreatureViewLevel()) &&
         sound.getPosition()->getCoord().inRectangle(area)))) {
      soundLibrary->playSound(sound);
      lastPlayed[sound.getId()] = curTime;
    }
  }
  soundQueue.clear();
}

void WindowView::animateObject(Vec2 begin, Vec2 end, optional<ViewId> object, optional<FXInfo> fx) {
  if (fx && mapGui->fxesAvailable())
    mapGui->addAnimation(FXSpawnInfo(*fx, begin, end - begin));
  else if (object && begin != end)
    mapGui->addAnimation(
        Animation::thrownObject(
          (end - begin).mult(mapLayout->getSquareSize()),
          *object,
          currentTileLayout.sprites),
        begin);
}

void WindowView::animation(Vec2 pos, AnimationId id, Dir orientation) {
  if (currentTileLayout.sprites)
    mapGui->addAnimation(Animation::fromId(id, orientation), pos);
}

void WindowView::animation(const FXSpawnInfo& spawnInfo) {
  mapGui->addAnimation(spawnInfo);
}

void WindowView::refreshView() {
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
  if ((renderDialog.empty() && gameReady) || !blockingElems.empty())
    refreshScreen(true);
}

void WindowView::drawMap() {
  for (auto gui : getAllGuiElems())
    gui->render(renderer);
  Vec2 mousePos = renderer.getMousePos();
  if (GuiElem* dragged = gui.getDragContainer().getGui())
    if (gui.getDragContainer().getOrigin().dist8(mousePos) > 30) {
      dragged->setBounds(Rectangle(mousePos + Vec2(15, 25), mousePos + Vec2(35, 45)));
      dragged->render(renderer);
    }
  guiBuilder.addFpsCounterTick();
}

static Rectangle getBugReportPos(Renderer& renderer) {
  return Rectangle(renderer.getSize() - Vec2(100, 23), renderer.getSize());
}

void WindowView::refreshScreen(bool flipBuffer) {
  if (zoomUI > -1) {
    renderer.setZoom(zoomUI ? 2 : 1);
    zoomUI = -1;
  }
  drawMap();
  auto bugReportPos = getBugReportPos(renderer);
  renderer.drawFilledRectangle(bugReportPos, Color::TRANSPARENT, Color::RED);
  renderer.drawText(Color::RED, bugReportPos.middle() - Vec2(0, 2), "report bug", Renderer::CenterType::HOR_VER);
  if (flipBuffer)
    renderer.drawAndClearBuffer();
}

optional<Vec2> WindowView::chooseDirection(Vec2 playerPos, const string& message) {
  TempClockPause pause(clock);
  gameInfo.messageBuffer = makeVec(PlayerMessage(message));
  SyncQueue<optional<Vec2>> returnQueue;
  addReturnDialog<optional<Vec2>>(returnQueue, [=] ()-> optional<Vec2> {
  rebuildGui();
  refreshScreen();
  renderer.getSteamInput()->pushActionSet(MySteamInput::ActionSet::DIRECTION);
  auto o = OnExit([this] { renderer.getSteamInput()->popActionSet(); });
  do {
    optional<Vec2> mousePos;
    Event event;
    auto controllerDir = renderer.getDiscreteJoyPos(ControllerJoy::DIRECTION);
    if (renderer.pollEvent(event)) {
      considerResizeEvent(event);
      if (event.type == SDL::SDL_MOUSEMOTION)
        mousePos = Vec2(event.motion.x, event.motion.y);
      if (event.type == SDL::SDL_KEYDOWN) {
        refreshScreen();
        switch (event.key.keysym.sym) {
          case SDL::SDLK_ESCAPE:
          case C_DIRECTION_CANCEL:
            return none;
          case C_DIRECTION_CONFIRM:
            if (controllerDir != Vec2(0, 0))
              return controllerDir;
            break;
          case C_MENU_UP:
            return Vec2(0, -1);
          case C_MENU_DOWN:
            return Vec2(0, 1);
          default:
            break;
        }
        if (gui.getKeybindingMap()->matches(Keybinding("WALK_NORTH"), event.key.keysym) ||
            gui.getKeybindingMap()->matches(Keybinding("WALK_NORTH2"), event.key.keysym))
          return Vec2(0, -1);
        if (gui.getKeybindingMap()->matches(Keybinding("WALK_SOUTH"), event.key.keysym) ||
            gui.getKeybindingMap()->matches(Keybinding("WALK_SOUTH2"), event.key.keysym))
          return Vec2(0, 1);
        if (gui.getKeybindingMap()->matches(Keybinding("WALK_WEST"), event.key.keysym) ||
            gui.getKeybindingMap()->matches(Keybinding("WALK_WEST2"), event.key.keysym))
          return Vec2(-1, 0);
        if (gui.getKeybindingMap()->matches(Keybinding("WALK_EAST"), event.key.keysym) ||
            gui.getKeybindingMap()->matches(Keybinding("WALK_EAST2"), event.key.keysym))
          return Vec2(1, 0);
        if (gui.getKeybindingMap()->matches(Keybinding("WALK_NORTH_WEST"), event.key.keysym))
          return Vec2(-1, -1);
        if (gui.getKeybindingMap()->matches(Keybinding("WALK_NORTH_EAST"), event.key.keysym))
          return Vec2(1, -1);
        if (gui.getKeybindingMap()->matches(Keybinding("WALK_SOUTH_WEST"), event.key.keysym))
          return Vec2(-1, 1);
        if (gui.getKeybindingMap()->matches(Keybinding("WALK_SOUTH_EAST"), event.key.keysym))
          return Vec2(1, 1);
      }
      if (mousePos && event.type == SDL::SDL_MOUSEBUTTONDOWN) {
        if (event.button.button == SDL_BUTTON_LEFT)
          return (*mousePos - playerPos).getBearing();
        else
          return none;
      }
    }
    refreshScreen(false);
    Vec2 dir = controllerDir;
    if (mousePos && mousePos != playerPos)
      Vec2 dir = (*mousePos - playerPos).getBearing();
    if (dir != Vec2(0, 0)) {
      Vec2 wpos = mapLayout->projectOnScreen(getMapGuiBounds(), mapGui->getScreenPos(),
          playerPos.x + dir.x, playerPos.y + dir.y);
      if (currentTileLayout.sprites) {
        auto coord = renderer.getTileSet().getTileCoord("arrow" + toString(int(dir.getCardinalDir()))).getOnlyElement();
        renderer.drawTile(wpos, {coord}, mapLayout->getSquareSize());
      } else {
        int numArrow = int(dir.getCardinalDir());
        static string arrows[] = { u8"⇑", u8"⇓", u8"⇒", u8"⇐", u8"⇗", u8"⇖", u8"⇘", u8"⇙"};
        renderer.drawText(FontId::SYMBOL_FONT, mapLayout->getSquareSize().y, Color::WHITE,
            wpos + Vec2(mapLayout->getSquareSize().x / 2, 0), arrows[numArrow], Renderer::HOR);
      }
    }
    /*if (auto *inst = fx::FXManager::getInstance())
      inst->simulateStableTime(double(clock->getRealMillis().count()) * 0.001);*/
    renderer.drawAndClearBuffer();
    renderer.flushEvents(SDL::SDL_MOUSEMOTION);
  } while (1);
  });
  return returnQueue.pop();
}

View::TargetResult WindowView::chooseTarget(Vec2 playerPos, TargetType targetType, Table<PassableInfo> passable,
      const string& message, optional<Keybinding> cycleKey) {
  TempClockPause pause(clock);
  gameInfo.messageBuffer = makeVec(PlayerMessage(message));
  SyncQueue<TargetResult> returnQueue;
  addReturnDialog<TargetResult>(returnQueue, [=] ()-> TargetResult {
  rebuildGui();
  refreshScreen();
  renderer.getSteamInput()->pushActionSet(MySteamInput::ActionSet::MENU);
  auto o = OnExit([this] { renderer.getSteamInput()->popActionSet(); });
  guiBuilder.disableClickActions = true;
  optional<Vec2> controllerPos;
  if (!gui.getSteamInput()->controllers.empty())
    controllerPos = playerPos;
  do {
    auto pos = controllerPos ? controllerPos : mapGui->projectOnMap(renderer.getMousePos());
    Event event;
    while (renderer.pollEvent(event)) {
      considerResizeEvent(event);
      if (event.type == SDL::SDL_KEYDOWN) {
        if (gui.getKeybindingMap()->matches(Keybinding("EXIT_MENU"), event.key.keysym)) {
          refreshScreen();
          return none;
        }
        else if (cycleKey && gui.getKeybindingMap()->matches(*cycleKey, event.key.keysym))
          return *cycleKey;
        else if (gui.getKeybindingMap()->matches(Keybinding("MENU_SELECT"), event.key.keysym)) {
          if (!!pos && (targetType != TargetType::POSITION ||
              (pos->inRectangle(passable.getBounds()) && passable[*pos] == PassableInfo::PASSABLE)))
            return *pos;
        }
        else if (gui.getKeybindingMap()->matches(Keybinding("MENU_UP"), event.key.keysym))
          controllerPos = controllerPos.value_or(playerPos) + Vec2(0, -1);
        else if (gui.getKeybindingMap()->matches(Keybinding("MENU_DOWN"), event.key.keysym))
          controllerPos = controllerPos.value_or(playerPos) + Vec2(0, 1);
        else if (gui.getKeybindingMap()->matches(Keybinding("MENU_LEFT"), event.key.keysym))
          controllerPos = controllerPos.value_or(playerPos) + Vec2(-1, 0);
        else if (gui.getKeybindingMap()->matches(Keybinding("MENU_RIGHT"), event.key.keysym))
          controllerPos = controllerPos.value_or(playerPos) + Vec2(1, 0);
      }
      if (event.type == SDL::SDL_MOUSEMOTION)
        controllerPos = none;
      if (pos && event.type == SDL::SDL_MOUSEBUTTONDOWN) {
        if (event.button.button == SDL_BUTTON_LEFT && (targetType != TargetType::POSITION ||
            (pos->inRectangle(passable.getBounds()) && passable[*pos] == PassableInfo::PASSABLE)))
          return *pos;
        else
          return none;
      } else
        gui.propagateEvent(event, {mapGui});
    }
    rebuildGui();
    refreshScreen(false);
    if (pos) {
      auto drawPoint = [&] (Vec2 wpos, Color color) {
        if (currentTileLayout.sprites) {
          renderer.drawViewObject(wpos, ViewId("dig_mark"), true, mapLayout->getSquareSize(), color);
        } else {
          renderer.drawText(FontId::SYMBOL_FONT, mapLayout->getSquareSize().y, color,
              wpos + Vec2(mapLayout->getSquareSize().x / 2, 0), "0", Renderer::HOR);
        }
      };
      switch (targetType) {
        case TargetType::SHOW_ALL:
          for (auto v : passable.getBounds()) {
            auto color = [&] {
              switch (passable[v]) {
                case PassableInfo::UNKNOWN:
                  return Color::TRANSPARENT;
                case PassableInfo::PASSABLE:
                  return Color::WHITE.transparency(100);
                case PassableInfo::STOPS_HERE:
                case PassableInfo::NON_PASSABLE:
                  return Color::GREEN;
              }
            }();
            drawPoint(mapLayout->projectOnScreen(getMapGuiBounds(), mapGui->getScreenPos(), v.x, v.y), color);
          }
          break;
        case TargetType::POSITION: {
          auto color = pos->inRectangle(passable.getBounds()) && passable[*pos] == PassableInfo::PASSABLE ? Color::GREEN : Color::RED;
          drawPoint(mapLayout->projectOnScreen(getMapGuiBounds(), mapGui->getScreenPos(), pos->x, pos->y), color);
          break;
        }
        case TargetType::TRAJECTORY: {
          bool wasObstructed = false;
          auto line = drawLine(playerPos, *pos);
          for (auto& pw : line)
            if (pw != playerPos || line.size() == 1) {
              bool obstructed = wasObstructed || !pw.inRectangle(passable.getBounds()) ||
                  passable[pw] == PassableInfo::NON_PASSABLE;
              auto color = obstructed ? Color::RED : Color::GREEN;
              if (!wasObstructed && pw.inRectangle(passable.getBounds()) && passable[pw] == PassableInfo::UNKNOWN)
                color = Color::ORANGE;
              Vec2 wpos = mapLayout->projectOnScreen(getMapGuiBounds(), mapGui->getScreenPos(), pw.x, pw.y);
              wasObstructed = obstructed || passable[pw] == PassableInfo::STOPS_HERE;
              drawPoint(wpos, color);
            }
          break;
        }
      }
    }
    renderer.drawAndClearBuffer();
    // Not sure what this was for but it interfered with switching the mode from keyboard to mouse.
    //renderer.flushEvents(SDL::SDL_MOUSEMOTION);
  } while (1);
  });
  guiBuilder.disableClickActions = false;
  return returnQueue.pop();
}

optional<int> WindowView::getNumber(const string& title, Range range, int initial) {
  return guiBuilder.getNumber(title, renderer.getSize() / 2 - Vec2(225, 0), range, initial);
}

optional<string> WindowView::getText(const string& title, const string& value, int maxLength, const string& hint) {
  TempClockPause pause(clock);
  SyncQueue<optional<string>> returnQueue;
  addReturnDialog<optional<string>>(returnQueue, [=] ()-> optional<string> {
    return guiBuilder.getTextInput(title, value, maxLength, hint);
  });
  return returnQueue.pop();
}

void WindowView::scriptedUI(ScriptedUIId id, const ScriptedUIData& data, ScriptedUIState& state) {
  Semaphore sem;
  return getBlockingGui(sem, gui.stack(gui.stopMouseMovement(), gui.scripted([&sem]{sem.v();}, id, data, state)));
}

optional<Vec2> WindowView::chooseSite(const string& message, const Campaign& campaign, Vec2 current) {
  SyncQueue<optional<Vec2>> returnQueue;
  return getBlockingGui(returnQueue, guiBuilder.drawChooseSiteMenu(returnQueue, message, campaign, current));
}

void WindowView::presentWorldmap(const Campaign& campaign) {
  Semaphore sem;
  return getBlockingGui(sem, guiBuilder.drawWorldmap(sem, campaign));
}

variant<View::AvatarChoice, AvatarMenuOption> WindowView::chooseAvatar(const vector<AvatarData>& avatars) {
  SyncQueue<variant<AvatarChoice, AvatarMenuOption>> returnQueue;
  return getBlockingGui(returnQueue, guiBuilder.drawAvatarMenu(returnQueue, avatars), none, false);
}

CampaignAction WindowView::prepareCampaign(CampaignOptions campaign, CampaignMenuState& state) {
  SyncQueue<CampaignAction> returnQueue;
  return getBlockingGui(returnQueue, guiBuilder.drawCampaignMenu(returnQueue, campaign, state));
}

vector<int> WindowView::prepareWarlordGame(RetiredGames& games, const vector<PlayerInfo>& minions,
    int maxTeam, int maxDungeons) {
  string searchString;
  vector<int> chosen;
  /*for (int i : Range(maxCount))
    chosen.push_back(i);*/
  int page = 0;
  while (1) {
    if (page == 0) {
      SyncQueue<variant<int, bool>> queue;
      sort(chosen.begin(), chosen.end());
      auto res = getBlockingGui(queue,
          guiBuilder.drawWarlordMinionsMenu(queue, minions, chosen, maxTeam));
      if (auto value = res.getValueMaybe<bool>()) {
        if (*value) {
          ++page;
          continue;
        } else
          return vector<int>();
      } else
      if (auto value = res.getValueMaybe<int>()) {
        if (chosen.contains(*value))
          chosen.removeElement(*value);
        else if (chosen.size() < maxTeam)
          chosen.push_back(*value);
      }
    } else {
      SyncQueue<variant<string, bool, none_t>> queue;
      auto res = getBlockingGui(queue,
          guiBuilder.drawRetiredDungeonMenu(queue, games, searchString, maxDungeons));
      if (auto value = res.getValueMaybe<bool>()) {
        if (*value)
          return chosen;
        else {
          --page;
          continue;
        }
      } else
      if (auto value = res.getValueMaybe<string>())
        searchString = *value;
    }
  }
}

optional<UniqueEntity<Creature>::Id> WindowView::chooseCreature(const string& title,
    const vector<PlayerInfo>& creatures, const string& cancelText) {
  SyncQueue<optional<UniqueEntity<Creature>::Id>> returnQueue;
  return getBlockingGui(returnQueue, guiBuilder.drawChooseCreatureMenu(returnQueue, title, creatures, cancelText));
}

bool WindowView::creatureInfo(const string& title, bool prompt, const vector<PlayerInfo>& creatures) {
  SyncQueue<bool> returnQueue;
  return getBlockingGui(returnQueue, guiBuilder.drawCreatureInfo(returnQueue, title, prompt, creatures));
}

void WindowView::logMessage(const std::string& message) {
  RecursiveLock lock(logMutex);
  messageLog.push_back(message);
  if (messageLog.size() > 10000)
    messageLog.pop_front();
}

void WindowView::getBlockingGui(Semaphore& sem, SGuiElem elem, optional<Vec2> origin) {
  TempClockPause pause(clock);
  renderer.getSteamInput()->pushActionSet(MySteamInput::ActionSet::MENU);
  auto o = OnExit([this] { renderer.getSteamInput()->popActionSet(); });
  bool origOrigin = !!origin;
  if (!origin)
    origin = (renderer.getSize() - Vec2(*elem->getPreferredWidth(), *elem->getPreferredHeight())) / 2;
  origin->y = max(0, origin->y);
  int origElemCount = blockingElems.size();
  if (blockingElems.empty()) {
    /*blockingElems.push_back(gui.darken());
    blockingElems.back()->setBounds(Rectangle(renderer.getSize()));*/
    blockingElems.push_back(gui.keyHandler(bindMethod(&WindowView::keyboardActionAlways, this)));
  }
  //propagateMousePosition({elem});
  blockingElems.push_back(elem);
  if (currentThreadId() == renderThreadId)
    while (!sem.get()) {
      Vec2 size(*elem->getPreferredWidth(), min(renderer.getSize().y - origin->y, *elem->getPreferredHeight()));
      if (!origOrigin)
        origin = (renderer.getSize() - Vec2(*elem->getPreferredWidth(), *elem->getPreferredHeight())) / 2;
      elem->setBounds(Rectangle(*origin, *origin + size));
      refreshView();
    }
  else
    sem.p();
  while (blockingElems.size() > origElemCount)
    blockingElems.pop_back();
}

void WindowView::zoom(int dir) {
  refreshInput = true;
  auto& layouts = currentTileLayout.layouts;
  int index = *layouts.findAddress(mapLayout);
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
  int index = *currentTileLayout.layouts.findAddress(mapLayout);
  if (options->getBoolValue(OptionId::ASCII) || !useTiles)
    currentTileLayout = asciiLayouts;
  else
    currentTileLayout = spriteLayouts;
  if (currentTileLayout.layouts.size() <= index)
    index = 0;
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

bool WindowView::considerResizeEvent(const Event& event, bool withBugReportEvent) {
  if (withBugReportEvent && considerBugReportEvent(event))
    return true;
  if (event.type == SDL::SDL_QUIT)
    throw GameExitException();
  if (event.type == SDL::SDL_WINDOWEVENT && event.window.event == SDL::SDL_WINDOWEVENT_RESIZED) {
    resize(event.window.data1, event.window.data2);
    return true;
  }
  return false;
}

void WindowView::setBugReportSaveCallback(BugReportSaveCallback f) {
  bugReportSaveCallback = f;
}

void WindowView::dungeonScreenshot(Vec2 size) {
  mapGui->render(renderer);
  renderer.drawAndClearBuffer();
  renderer.makeScreenshot(DirectoryPath::current().file(retiredScreenshotFilename),
                          Rectangle((renderer.getSize() - size) / 2, (renderer.getSize() + size) / 2));
}

bool WindowView::zoomUIAvailable() const {
  return renderer.getWindowSize().x >= 2 * renderer.getMinResolution().x
      && renderer.getWindowSize().y >= 2 * renderer.getMinResolution().y;
}

bool WindowView::considerBugReportEvent(const Event& event) {
  if (event.type == SDL::SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT &&
      Vec2(event.button.x, event.button.y).inRectangle(getBugReportPos(renderer))) {
    bool exit = false;
    optional<GuiBuilder::BugReportInfo> bugreportInfo;
    auto elem = guiBuilder.drawBugreportMenu(!!bugReportSaveCallback,
        [&exit, &bugreportInfo] (optional<GuiBuilder::BugReportInfo> info) {
      bugreportInfo = info;
      exit = true;
    });
    Vec2 size(*elem->getPreferredWidth(), *elem->getPreferredHeight());
    do {
      Event event;
      while (renderer.pollEvent(event)) {
        considerResizeEvent(event, false);
        gui.propagateEvent(event, {elem});
      }
      elem->setBounds(Rectangle((renderer.getSize() - size) / 2, (renderer.getSize() + size) / 2));
      refreshScreen(false);
      elem->render(renderer);
      renderer.drawAndClearBuffer();
    } while (!exit);
    if (bugreportInfo) {
      optional<FilePath> savefile;
      optional<FilePath> screenshot;
      if (bugreportInfo->includeSave) {
        savefile = bugreportDir.file("bugreport.sav");
        bugReportSaveCallback(*savefile);
      }
      if (bugreportInfo->includeScreenshot) {
        screenshot = bugreportDir.file("bugreport.png");
        renderer.makeScreenshot(*screenshot, Rectangle(renderer.getSize()));
      }
      if (!savefile && !screenshot && bugreportInfo->text.size() < 5)
        return true;
      ProgressMeter meter(1.0);
      optional<string> result;
      FileSharing::CancelFlag cancel;
      displaySplash(&meter, "Uploading bug report", [&cancel]{ cancel.cancel(); });
      thread t([&] {
        result = bugreportSharing->uploadBugReport(cancel, bugreportInfo->text, savefile, screenshot, meter);
        clearSplash();
      });
      refreshView();
      t.join();
      if (result)
        presentText("Error", "There was an error while sending the bug report: " + *result);
      if (savefile)
        remove(savefile->getPath());
      if (screenshot)
        remove(screenshot->getPath());
    }
    return true;
  }
  return false;
}

void WindowView::processEvents() {
  guiBuilder.disableTooltip = !blockingElems.empty();
  auto clickableGuiElems = getClickableGuiElems();
  gui.propagateScrollEvent(clickableGuiElems);
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
    else {
      if (event.type == SDL::SDL_KEYDOWN && renderDialog.empty() && blockingElems.empty()) {
        if (lockKeyboard)
          return;
        lockKeyboard = true;
      }
      propagateEvent(event, clickableGuiElems);
    }
    switch (event.type) {
      case SDL::SDL_KEYDOWN:
        if (gameInfo.infoType == GameInfo::InfoType::PLAYER)
          renderer.flushEvents(SDL::SDL_KEYDOWN);
        break;
      case SDL::SDL_MOUSEBUTTONDOWN:
        if (event.button.button == SDL_BUTTON_RIGHT)
          gui.getDragContainer().pop();
        break;
      case SDL::SDL_MOUSEBUTTONUP:
        if (event.button.button == SDL_BUTTON_LEFT) {
          if (auto building = guiBuilder.getActiveButton())
            inputQueue.push(UserInput(UserInputId::RECT_CONFIRM, BuildingClickInfo{Vec2(0, 0), *building}));
          else if (gameInfo.infoType == GameInfo::InfoType::BAND)
            inputQueue.push(UserInputId::RECT_CANCEL);
        }
        break;
      default: break;
    }
  }
}

void WindowView::propagateEvent(const Event& event, vector<SGuiElem> guiElems) {
  CHECK(currentThreadId() == renderThreadId);
  if (gameReady)
    guiBuilder.clearHint();
  switch (event.type) {
    case SDL::SDL_MOUSEBUTTONUP:
      // MapGui needs this event otherwise it will sometimes lock the mouse button
      mapGui->onClick(MouseButtonId::RELEASED, Vec2(event.button.x, event.button.y));
      break;
    case SDL::SDL_MOUSEBUTTONDOWN:
      lockKeyboard = true;
      break;
    default:break;
  }
  gui.propagateEvent(event, guiElems);
}

// These commands will run even in blocking gui.
void WindowView::keyboardActionAlways(const SDL_Keysym& key) {
  switch (key.sym) {
#ifndef RELEASE
    case SDL::SDLK_F8:
      //renderer.startMonkey();
      renderer.loadAnimations();
      renderer.getTileSet().clear();
      renderer.getTileSet().reload();
      renderer.getTileSet().loadTextures();
      if (fxRenderer)
        fxRenderer->loadTextures();
      gui.loadImages();
      break;
#endif
    default:
      break;
  }
}

// These commands will run only when the map is in focus (I think)
void WindowView::keyboardAction(const SDL_Keysym& key) {
  keyboardActionAlways(key);
  switch (key.sym) {
#ifndef RELEASE
    case SDL::SDLK_F10:
      if (auto input = getText("Enter effect", "", 100, ""))
        inputQueue.push({UserInputId::APPLY_EFFECT, *input});
      break;
    case SDL::SDLK_F11:
      if (auto input = getText("Enter item type", "", 100, ""))
        inputQueue.push({UserInputId::CREATE_ITEM, *input});
      break;
    case SDL::SDLK_F12:
      if (auto input = getText("Enter creature id", "", 100, ""))
        inputQueue.push({UserInputId::SUMMON_ENEMY, *input});
      break;
    case SDL::SDLK_F9:
      inputQueue.push(UserInputId::CHEAT_ATTRIBUTES);
      break;
#endif
    /*case SDL::SDLK_F7:
      presentList("", vector<string>(messageLog.begin(), messageLog.end()), true);
      break;
    case SDL::SDLK_F2:
      if (!renderer.isMonkey()) {
        options->handle(this, OptionSet::GENERAL);
        refreshScreen();
      }
      break;*/
    case C_OPEN_MENU:
    case SDL::SDLK_ESCAPE:
      if (!guiBuilder.clearActiveButton() && !renderer.isMonkey())
        inputQueue.push(UserInput(UserInputId::EXIT));
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

optional<int> WindowView::chooseAtMouse(const vector<string>& elems) {
  return guiBuilder.chooseAtMouse(elems);
}

void WindowView::addSound(const Sound& sound) {
  soundQueue.push_back(sound);
}
