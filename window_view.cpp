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
  return new LoggingView<WindowView>(of, params);
}

View* WindowView::createReplayView(InputArchive& ifs, ViewParams params) {
  return new ReplayView<WindowView>(ifs, params);
}

class TempClockPause {
  public:
  TempClockPause(Clock* c) : clock(c) {
    if (!clock->isPaused()) {
      clock->pause();
      cont = true;
    }
  }

  ~TempClockPause() {
    if (cont)
      clock->cont();
  }

  private:
  Clock* clock;
  bool cont = false;
};

int rightBarWidthCollective = 330;
int rightBarWidthPlayer = 290;
int bottomBarHeightCollective = 62;
int bottomBarHeightPlayer = 62;

Rectangle WindowView::getMapGuiBounds() const {
  switch (gameInfo.infoType) {
    case GameInfo::InfoType::PLAYER:
      return Rectangle(0, 0, renderer.getWidth() - rightBarWidthPlayer,
          renderer.getHeight() - bottomBarHeightPlayer);
    case GameInfo::InfoType::BAND:
      return Rectangle(0, 0, renderer.getWidth() - rightBarWidthCollective,
          renderer.getHeight() - bottomBarHeightCollective);
    case GameInfo::InfoType::SPECTATOR: {
      int x = Level::getSplashVisibleBounds().getW();
      int y = Level::getSplashVisibleBounds().getH();
      return Rectangle((renderer.getWidth() - x * mapLayout->squareWidth()) / 2,
          (renderer.getHeight() - y * mapLayout->squareHeight()) / 2,
          (renderer.getWidth() + x * mapLayout->squareWidth()) / 2,
          (renderer.getHeight() + y * mapLayout->squareHeight()) / 2);
      }
  }
  FAIL << "wfw";
  return Rectangle();
}

Rectangle WindowView::getMinimapBounds() const {
  int x = 20;
  int y = 70;
  return Rectangle(x, y, x + renderer.getWidth() / 12, y + renderer.getWidth() / 12);
}

void WindowView::resetMapBounds() {
  mapGui->setBounds(getMapGuiBounds());
  minimapGui->setBounds(getMinimapBounds());
  minimapDecoration->setBounds(getMinimapBounds().minusMargin(-6));
}

WindowView::WindowView(ViewParams params) : renderer(params.renderer), useTiles(params.useTiles),
    options(params.options), clock(params.clock), guiBuilder(renderer, clock, {
        [this](UserInput input) { inputQueue.push(input);},
        [this](const string& s) { mapGui->setHint(s);},
        [this](sf::Event::KeyEvent ev) { keyboardAction(ev);}}) {}

void WindowView::initialize() {
  renderer.initialize(options->getBoolValue(OptionId::FULLSCREEN));
  options->addTrigger(OptionId::FULLSCREEN, [this] (bool on) { renderer.initialize(on); });
  renderThreadId = currentThreadId();
  vector<ViewLayer> allLayers;
  for (auto l : ENUM_ALL(ViewLayer))
    allLayers.push_back(l);
  asciiLayouts = {
    MapLayout(16, 20, allLayers),
    MapLayout(8, 10,
        {ViewLayer::FLOOR_BACKGROUND, ViewLayer::FLOOR, ViewLayer::LARGE_ITEM, ViewLayer::CREATURE}), false};
  spriteLayouts = {
    MapLayout(36, 36, allLayers),
    MapLayout(18, 18, allLayers), true};
  if (useTiles)
    currentTileLayout = spriteLayouts;
  else
    currentTileLayout = asciiLayouts;
  mapGui = new MapGui({
      [this](Vec2 pos) { mapLeftClickFun(pos); },
      [this](Vec2 pos) { mapRightClickFun(pos); },
      [this] { refreshInput = true;}}, clock );
  minimapGui = new MinimapGui([this]() { inputQueue.push(UserInput(UserInputId::DRAW_LEVEL_MAP)); });
  minimapDecoration = GuiElem::border2(GuiElem::rectangle(colors[ColorId::BLACK]));
  resetMapBounds();
  guiBuilder.setTilesOk(useTiles);
  if (useTiles) {
    CHECK(menuCore.loadFromFile("menu_core.png"));
    CHECK(menuMouth.loadFromFile("menu_mouth.png"));
    menuCore.setSmooth(true);
    menuMouth.setSmooth(true);
  } else {
    CHECK(splash1.loadFromFile("splash2f.png"));
    CHECK(splash2.loadFromFile("splash2e.png"));
    CHECK(loadingSplash.loadFromFile(chooseRandom(LIST(
        "splash2a.png",
        "splash2b.png",
        "splash2c.png",
        "splash2d.png"))));
  }

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
      if (collectiveTab == GuiBuilder::CollectiveTab::TECHNOLOGY && activeLibrary >= 0)
        inputQueue.push(UserInput(UserInputId::LIBRARY, BuildingInfo(pos, activeLibrary)));
      if (collectiveTab == GuiBuilder::CollectiveTab::BUILDINGS) {
        if (Keyboard::isKeyPressed(Keyboard::LShift))
          inputQueue.push(UserInput(UserInputId::RECT_SELECTION, pos));
        else if (Keyboard::isKeyPressed(Keyboard::LControl))
          inputQueue.push(UserInput(UserInputId::RECT_DESELECTION, pos));
        else
          inputQueue.push(UserInput(UserInputId::BUILD, BuildingInfo(pos, activeBuilding)));
      }
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
  minimapGui->clear();
  mapGui->clearCenter();
  guiBuilder.reset();
  gameInfo = GameInfo{};
}

void WindowView::displayOldSplash() {
  Rectangle menuPosition = getMenuPosition(View::MAIN_MENU_NO_TILES);
  int margin = 10;
  renderer.drawImage(renderer.getWidth() / 2 - 415, menuPosition.getKY() + margin, splash1);
  renderer.drawImage((renderer.getWidth() - splash2.getSize().x) / 2,
      menuPosition.getPY() - splash2.getSize().y - margin, splash2);
}

void WindowView::displayMenuSplash2() {
  drawMenuBackground(0, 0);
}

void WindowView::drawMenuBackground(double barState, double mouthState) {
  double scale = double(renderer.getHeight()) / menuCore.getSize().y;
  int width = menuCore.getSize().x * scale;
  double mouthPos1 = 184;
  double mouthPos2 = 214;
  double mouthX = (renderer.getWidth() - (menuMouth.getSize().x + 5) * scale) / 2;
  renderer.drawFilledRectangle(mouthX, mouthPos1 * scale, 1 + mouthX + barState * menuMouth.getSize().x * scale,
      (mouthPos2 + menuMouth.getSize().y) * scale, sf::Color(60, 76, 48));
  renderer.drawImage((renderer.getWidth() - width) / 2, 0, menuCore, scale);
  renderer.drawImage(mouthX, scale * (mouthPos1 * (1 - mouthState) + mouthPos2 * mouthState), menuMouth, scale);
}

void WindowView::displaySplash(const ProgressMeter& meter, View::SplashType type) {
  RenderLock lock(renderMutex);
  string text;
  switch (type) {
    case View::CREATING: text = "Creating a new world, just for you..."; break;
    case View::LOADING: text = "Loading the game..."; break;
    case View::SAVING: text = "Saving the game..."; break;
  }
  splashDone = false;
  renderDialog = [=, &meter] {
    int t0 = clock->getRealMillis();
    int mouthMillis = 400;
    while (!splashDone) {
      if (useTiles) {
        drawMenuBackground(meter.getProgress(), min(1.0, double(clock->getRealMillis() - t0) / mouthMillis));
        renderer.drawText(colors[ColorId::WHITE], renderer.getWidth() / 2, renderer.getHeight() * 0.5, text, true);
      } else {
        renderer.drawImage((renderer.getWidth() - loadingSplash.getSize().x) / 2,
            (renderer.getHeight() - loadingSplash.getSize().y) / 2, loadingSplash);
        renderer.drawText(colors[ColorId::WHITE], renderer.getWidth() / 2, renderer.getHeight() - 60, text, true);
      }
      renderer.drawAndClearBuffer();
      sf::sleep(sf::milliseconds(30));
      Event event;
      while (renderer.pollEvent(event)) {
        if (event.type == Event::Resized) {
          resize(event.size.width, event.size.height, {});
        }
      }
    }
    renderDialog = nullptr;
  };
}

void WindowView::clearSplash() {
  splashDone = true;
}

void WindowView::resize(int width, int height, vector<GuiElem*> gui) {
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
  tempGuiElems.clear();
  switch (gameInfo.infoType) {
    case GameInfo::InfoType::SPECTATOR:
        right = GuiElem::empty();
        bottom = GuiElem::empty();
        rightBarWidth = 0;
        bottomBarHeight = 0;
        if (getMapGuiBounds().getPX() > 0) {
          tempGuiElems.push_back(GuiElem::rectangle(colors[ColorId::BLACK]));
          tempGuiElems.back()->setBounds(Rectangle(0, 0, getMapGuiBounds().getPX(), renderer.getHeight()));
          tempGuiElems.push_back(GuiElem::rectangle(colors[ColorId::BLACK]));
          tempGuiElems.back()->setBounds(Rectangle(getMapGuiBounds().getKX(), 0,
                renderer.getWidth(), renderer.getHeight()));
        }
        if (getMapGuiBounds().getPY() > 0) {
          tempGuiElems.push_back(GuiElem::rectangle(colors[ColorId::BLACK]));
          tempGuiElems.back()->setBounds(Rectangle(0, 0, renderer.getWidth(), getMapGuiBounds().getPY()));
          tempGuiElems.push_back(GuiElem::rectangle(colors[ColorId::BLACK]));
          tempGuiElems.back()->setBounds(Rectangle(0, getMapGuiBounds().getKY(),
                renderer.getWidth(), renderer.getHeight()));
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
        right = guiBuilder.drawRightBandInfo(gameInfo.bandInfo, gameInfo.villageInfo);
        guiBuilder.drawBandOverlay(overlays, gameInfo.bandInfo);
        bottom = guiBuilder.drawBottomBandInfo(gameInfo);
        rightBarWidth = rightBarWidthCollective;
        bottomBarHeight = bottomBarHeightCollective;
        break;
  }
  resetMapBounds();
  CHECK(currentThreadId() != renderThreadId);
  int bottomOffset = 15;
  int leftMargin = 20;
  int rightMargin = 20;
  int sideOffset = 10;
  int rightWindowHeight = 80;
  if (rightBarWidth > 0) {
    tempGuiElems.push_back(GuiElem::mainDecoration(rightBarWidth, bottomBarHeight));
    tempGuiElems.back()->setBounds(Rectangle(renderer.getWidth(), renderer.getHeight()));
    tempGuiElems.push_back(GuiElem::margins(std::move(right), 20, 20, 10, 0));
    tempGuiElems.back()->setBounds(Rectangle(
          renderer.getWidth() - rightBarWidth, 0, renderer.getWidth(), renderer.getHeight()));
    tempGuiElems.push_back(GuiElem::margins(std::move(bottom), 80, 10, 80, 0));
    tempGuiElems.back()->setBounds(Rectangle(
          0, renderer.getHeight() - bottomBarHeight,
          renderer.getWidth() - rightBarWidth, renderer.getHeight()));
    guiBuilder.drawMessages(overlays, gameInfo.messageBuffer,
        renderer.getWidth() - rightBarWidth - leftMargin - rightMargin);
    guiBuilder.drawGameSpeedDialog(overlays);
    for (auto& overlay : overlays) {
      Vec2 pos;
      int topMargin = 20;
      int bottomMargin = 20;
      int width = leftMargin + rightMargin + overlay.size.x;
      int height = overlay.size.y + bottomMargin + topMargin;
      switch (overlay.alignment) {
        case GuiBuilder::OverlayInfo::RIGHT:
          pos = Vec2(renderer.getWidth() - rightBarWidth - sideOffset - width,
              rightWindowHeight);
          break;
        case GuiBuilder::OverlayInfo::LEFT:
          pos = Vec2(sideOffset,
              renderer.getHeight() - bottomBarHeight - bottomOffset - height);
          break;
        case GuiBuilder::OverlayInfo::MESSAGES:
          width -= rightMargin - 10;
          height -= bottomMargin + topMargin + 6;
          topMargin = 2;
          pos = Vec2(0, 0);
          break;
        case GuiBuilder::OverlayInfo::GAME_SPEED:
          pos = Vec2(renderer.getWidth() - overlay.size.x - 90, renderer.getHeight() - overlay.size.y - 90);
          break;
        case GuiBuilder::OverlayInfo::INVISIBLE:
          pos = Vec2(renderer.getWidth(), 0);
          overlay.elem = GuiElem::invisible(std::move(overlay.elem));
          break;
      }
      PGuiElem elem = GuiElem::margins(std::move(overlay.elem), leftMargin, topMargin, rightMargin, bottomMargin);
      switch (overlay.alignment) {
        case GuiBuilder::OverlayInfo::MESSAGES:
          elem = GuiElem::translucentBackground(std::move(elem));
          break;
        default:
          elem = GuiElem::miniWindow(std::move(elem));
          break;
      }
      tempGuiElems.push_back(std::move(elem));
      tempGuiElems.back()->setBounds(Rectangle(pos, pos + Vec2(width, height)));
      Debug() << "Overlay " << overlay.alignment << " bounds " << tempGuiElems.back()->getBounds();
    }
  }
}

vector<GuiElem*> WindowView::getAllGuiElems() {
  CHECK(currentThreadId() == renderThreadId);
  if (gameInfo.infoType == GameInfo::InfoType::SPECTATOR)
    return concat({mapGui}, extractRefs(tempGuiElems));
  vector<GuiElem*> ret = extractRefs(tempGuiElems);
  if (gameReady)
    ret = concat(concat({mapGui}, ret), {minimapDecoration.get(), minimapGui});
  return ret;
}

vector<GuiElem*> WindowView::getClickableGuiElems() {
  CHECK(currentThreadId() == renderThreadId);
  if (gameInfo.infoType == GameInfo::InfoType::SPECTATOR)
    return {mapGui};
  vector<GuiElem*> ret = extractRefs(tempGuiElems);
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
  renderDialog = [this, &sem, fun] {
    fun();
    sem.v();
    renderDialog = nullptr;
  };
  if (currentThreadId() == renderThreadId)
    renderDialog();
  lock.unlock();
  sem.p();
}

void WindowView::drawLevelMap(const CreatureView* creature) {
  TempClockPause pause(clock);
  addVoidDialog([=] {
    minimapGui->presentMap(creature, getMapGuiBounds(), renderer,
        [this](double x, double y) { mapGui->setCenter(x, y);}); });
}

void WindowView::updateMinimap(const CreatureView* creature) {
  const Level* level = creature->getLevel();
  Vec2 rad(40, 40);
  Vec2 playerPos = mapGui->getScreenPos().div(Vec2(mapLayout->squareWidth(), mapLayout->squareHeight()));
  Rectangle bounds(playerPos - rad, playerPos + rad);
  minimapGui->update(level, bounds, creature);
}

void WindowView::updateView(const CreatureView* collective, bool noRefresh) {
  if (!wasRendered)
    return;
  RenderLock lock(renderMutex);
  wasRendered = false;
  guiBuilder.addUpsCounterTick();
  gameReady = true;
  if (!noRefresh)
    uiLock = false;
  switchTiles();
  collective->refreshGameInfo(gameInfo);
  mapGui->setSpriteMode(currentTileLayout.sprites);
  bool spectator = gameInfo.infoType == GameInfo::InfoType::SPECTATOR;
  mapGui->updateObjects(collective, mapLayout, options->getBoolValue(OptionId::SMOOTH_MOVEMENT)
      && (currentTileLayout.sprites || spectator), !spectator);
  updateMinimap(collective);
  rebuildGui();
  if (gameInfo.infoType == GameInfo::InfoType::SPECTATOR)
    guiBuilder.setGameSpeed(GuiBuilder::GameSpeed::NORMAL);
}

void WindowView::animateObject(vector<Vec2> trajectory, ViewObject object) {
  RenderLock lock(renderMutex);
  if (trajectory.size() >= 2)
    mapGui->addAnimation(
        Animation::thrownObject(
          (trajectory.back() - trajectory.front())
              .mult(Vec2(mapLayout->squareWidth(), mapLayout->squareHeight())),
          object,
          currentTileLayout.sprites),
        trajectory.front());
}

void WindowView::animation(Vec2 pos, AnimationId id) {
  RenderLock lock(renderMutex);
  if (currentTileLayout.sprites)
    mapGui->addAnimation(Animation::fromId(id), pos);
}

void WindowView::refreshView() {
  {
    RenderLock lock(renderMutex);
    wasRendered = true;
    CHECK(currentThreadId() == renderThreadId);
    if (gameReady)
      processEvents();
    if (renderDialog)
      renderDialog();
    if (uiLock && !renderDialog)
      return;
  }
  if (!renderDialog && gameReady)
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
    if (!gameReady) {
      if (useTiles)
        displayMenuSplash2();
      else
        displayOldSplash();
    }
    else
      drawMap();
  }
  if (flipBuffer)
    renderer.drawAndClearBuffer();
}

int indexHeight(const vector<View::ListElem>& options, int index) {
  CHECK(index < options.size() && index >= 0);
  int tmp = 0;
  for (int i : All(options))
    if (options[i].getMod() == View::NORMAL && tmp++ == index)
      return i;
  FAIL << "Bad index " << int(options.size()) << " " << index;
  return -1;
}

optional<int> reverseIndexHeight(const vector<View::ListElem>& options, int height) {
  if (height < 0 || height >= options.size() || options[height].getMod() != View::NORMAL)
    return none;
  int sub = 0;
  for (int i : Range(height))
    if (options[i].getMod() != View::NORMAL)
      ++sub;
  return height - sub;
}

optional<Vec2> WindowView::chooseDirection(const string& message) {
  RenderLock lock(renderMutex);
  TempClockPause pause(clock);
  gameInfo.messageBuffer = { PlayerMessage(message) };
  rebuildGui();
  SyncQueue<optional<Vec2>> returnQueue;
  addReturnDialog<optional<Vec2>>(returnQueue, [=] ()-> optional<Vec2> {
  refreshScreen();
  do {
    Event event;
    renderer.waitEvent(event);
    considerResizeEvent(event, getAllGuiElems());
    if (event.type == Event::MouseMoved || event.type == Event::MouseButtonPressed) {
      if (auto pos = mapGui->getHighlightedTile(renderer)) {
        refreshScreen(false);
        int numArrow = 0;
        Vec2 middle = mapLayout->getAllTiles(getMapGuiBounds(), Level::getMaxBounds(), mapGui->getScreenPos())
            .middle();
        if (pos == middle)
          continue;
        Vec2 dir = (*pos - middle).getBearing();
        switch (dir.getCardinalDir()) {
          case Dir::N: numArrow = 2; break;
          case Dir::S: numArrow = 6; break;
          case Dir::E: numArrow = 4; break;
          case Dir::W: numArrow = 0; break;
          case Dir::NE: numArrow = 3; break;
          case Dir::NW: numArrow = 1; break;
          case Dir::SE: numArrow = 5; break;
          case Dir::SW: numArrow = 7; break;
        }
        Vec2 wpos = mapLayout->projectOnScreen(getMapGuiBounds(), mapGui->getScreenPos(),
            (middle + dir).x, (middle + dir).y);
        if (currentTileLayout.sprites)
          renderer.drawTile(wpos.x, wpos.y, {Vec2(16, 8 + numArrow), 4});
        else {
          static sf::Uint32 arrows[] = { L'⇐', L'⇖', L'⇑', L'⇗', L'⇒', L'⇘', L'⇓', L'⇙'};
          renderer.drawText(Renderer::SYMBOL_FONT, 20, colors[ColorId::WHITE],
              wpos.x + mapLayout->squareWidth() / 2, wpos.y, arrows[numArrow], true);
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

bool WindowView::yesOrNoPrompt(const string& message) {
  return chooseFromListInternal("", {ListElem(message, TITLE), "Yes", "No"}, 0, MenuType::NORMAL_MENU, nullptr,
      none, none, {}) == 0;
}

optional<int> WindowView::getNumber(const string& title, int min, int max, int increments) {
  CHECK(min < max);
  vector<View::ListElem> options;
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
    MenuType type, int* scrollPos, optional<UserInputId> exitAction) {
  return chooseFromListInternal(title, options, index, type, scrollPos, exitAction, none, {});
}

Rectangle WindowView::getTextInputPosition() {
  Vec2 center = Vec2(renderer.getWidth(), renderer.getHeight()) / 2;
  return Rectangle(center - Vec2(300, 129), center + Vec2(300, 129));
}

static PGuiElem getTextContent(const string& title, const string& value, const string& hint) {
  vector<PGuiElem> lines = makeVec<PGuiElem>(
      GuiElem::variableLabel([&] { return title + ":  " + value + "_"; }));
  if (!hint.empty())
    lines.push_back(GuiElem::label(hint, GuiElem::inactiveText));
  return GuiElem::verticalList(std::move(lines), 40, 0);
}

optional<string> WindowView::getText(const string& title, const string& value, int maxLength, const string& hint) {
  RenderLock lock(renderMutex);
  TempClockPause pause(clock);
  SyncQueue<optional<string>> returnQueue;
  addReturnDialog<optional<string>>(returnQueue, [=] ()-> optional<string> {
  bool dismiss = false;
  string text = value;
  PGuiElem dismissBut = GuiElem::margins(GuiElem::stack(makeVec<PGuiElem>(
        GuiElem::button([&](){ dismiss = true; }),
        GuiElem::mouseHighlight2(GuiElem::mainMenuHighlight()),
        GuiElem::centerHoriz(
            GuiElem::label("Dismiss", colors[ColorId::WHITE]), renderer.getTextLength("Dismiss")))), 0, 5, 0, 0);
  PGuiElem stuff = GuiElem::margins(getTextContent(title, text, hint), 30, 50, 0, 0);
  stuff = GuiElem::margin(GuiElem::centerHoriz(std::move(dismissBut), renderer.getTextLength("Dismiss") + 100),
    std::move(stuff), 30, GuiElem::BOTTOM);
  stuff = GuiElem::window(std::move(stuff));
  while (1) {
    refreshScreen(false);
    stuff->setBounds(getTextInputPosition());
    stuff->render(renderer);
    renderer.drawAndClearBuffer();
    Event event;
    while (renderer.pollEvent(event)) {
      propagateEvent(event, {stuff.get()});
      if (dismiss)
        return none;
      if (considerResizeEvent(event, concat({stuff.get()}, getAllGuiElems())))
        continue;
      if (event.type == Event::TextEntered)
        if (isprint(event.text.unicode) && text.size() < maxLength)
          text += event.text.unicode;
      if (event.type == Event::KeyPressed)
        switch (event.key.code) {
          case Keyboard::BackSpace:
              if (!text.empty())
                text.pop_back();
              break;
          case Keyboard::Return: return text;
          case Keyboard::Escape: return none;
          default: break;
        }
    }
  }
  });
  lock.unlock();
  return returnQueue.pop();

}

int getScrollPos(int index, int count) {
  return max(0, min(count - 1, index - 3));
}

Rectangle WindowView::getMenuPosition(View::MenuType type) {
  int windowWidth = 800;
  int windowHeight = 400;
  int ySpacing;
  int yOffset = 0;
  switch (type) {
    case View::MAIN_MENU_NO_TILES:
      ySpacing = (renderer.getHeight() - windowHeight) / 2;
      break;
    case View::MAIN_MENU:
      windowWidth = 0.41 * renderer.getHeight();
      ySpacing = renderer.getHeight() / 3;
      break;
    case View::GAME_CHOICE_MENU:
      windowWidth = 0.41 * renderer.getHeight();
      ySpacing = renderer.getHeight() * 0.28;
      yOffset = renderer.getHeight() * 0.05;
      break;
    default: ySpacing = 100; break;
  }
  int xSpacing = (renderer.getWidth() - windowWidth) / 2;
  return Rectangle(xSpacing, ySpacing + yOffset, xSpacing + windowWidth, renderer.getHeight() - ySpacing + yOffset);
}

PGuiElem WindowView::drawGameChoices(optional<View::GameTypeChoice>& choice,optional<View::GameTypeChoice>& index) {
  return GuiElem::verticalAspect(
      GuiElem::marginFit(
      GuiElem::horizontalListFit(makeVec<PGuiElem>(
            GuiElem::stack(makeVec<PGuiElem>(
              GuiElem::button([&] { choice = View::KEEPER_CHOICE;}),
              GuiElem::mouseOverAction([&] { index = View::KEEPER_CHOICE;}),             
              GuiElem::sprite(GuiElem::KEEPER_GAME, GuiElem::Alignment::LEFT_STRETCHED),
              GuiElem::marginFit(GuiElem::empty(), GuiElem::mainMenuLabel("keeper          ", -0.08,
                  colors[ColorId::MAIN_MENU_OFF]), 0.94, GuiElem::TOP),
              GuiElem::mouseHighlightGameChoice(GuiElem::stack(
                GuiElem::sprite(GuiElem::KEEPER_HIGHLIGHT, GuiElem::Alignment::LEFT_STRETCHED),
              GuiElem::marginFit(GuiElem::empty(), GuiElem::mainMenuLabel("keeper          ", -0.08),
                  0.94, GuiElem::TOP)),
                View::KEEPER_CHOICE, index)
              )),
            GuiElem::stack(makeVec<PGuiElem>(
              GuiElem::button([&] { choice = View::ADVENTURER_CHOICE;}),
              GuiElem::sprite(GuiElem::ADVENTURER_GAME, GuiElem::Alignment::RIGHT_STRETCHED),
              GuiElem::marginFit(GuiElem::empty(), GuiElem::mainMenuLabel("          adventurer", -0.08,
                  colors[ColorId::MAIN_MENU_OFF]), 0.94, GuiElem::TOP),
              GuiElem::mouseHighlightGameChoice(GuiElem::stack(
                GuiElem::sprite(GuiElem::ADVENTURER_HIGHLIGHT, GuiElem::Alignment::RIGHT_STRETCHED),
                GuiElem::marginFit(GuiElem::empty(), GuiElem::mainMenuLabel("          adventurer", -0.08),
                  0.94, GuiElem::TOP)),
                View::ADVENTURER_CHOICE, index)
              ))), 0),
      GuiElem::margins(GuiElem::verticalListFit(makeVec<PGuiElem>(
          GuiElem::stack(
            GuiElem::button([&] { choice = View::LOAD_CHOICE;}),
            GuiElem::mainMenuLabelBg("Load game", 0.2),
            GuiElem::mouseHighlightGameChoice(GuiElem::mainMenuLabel("Load game", 0.2), View::LOAD_CHOICE, index)),
          GuiElem::stack(
            GuiElem::button([&] { choice = View::BACK_CHOICE;}),
            GuiElem::mainMenuLabelBg("Go back", 0.2),
            GuiElem::mouseHighlightGameChoice(GuiElem::mainMenuLabel("Go back", 0.2), View::BACK_CHOICE, index)
          )), 0), 0, 30, 0, 0),
      0.7, GuiElem::TOP), 1);
}

View::GameTypeChoice WindowView::chooseGameType() {
  if (!useTiles) {
    if (auto ind = chooseFromListInternal("", {
          View::ListElem("Choose your role:", View::TITLE),
          "Keeper",
          "Adventurer",
          View::ListElem("Or simply:", View::TITLE),
          "Load game",
          "Go back",
          },
        0, MenuType::MAIN_MENU, nullptr, none, none, {}))
      return (GameTypeChoice)(*ind);
    else
      return BACK_CHOICE;
  }
  RenderLock lock(renderMutex);
  uiLock = true;
  TempClockPause pause(clock);
  SyncQueue<GameTypeChoice> returnQueue;
  addReturnDialog<GameTypeChoice>(returnQueue, [=] ()-> GameTypeChoice {
  optional<View::GameTypeChoice> choice;
  optional<View::GameTypeChoice> index = KEEPER_CHOICE;
  PGuiElem stuff = drawGameChoices(choice, index);
  while (1) {
    refreshScreen(false);
    stuff->setBounds(getMenuPosition(View::GAME_CHOICE_MENU));
    stuff->render(renderer);
    renderer.drawAndClearBuffer();
    Event event;
    while (renderer.pollEvent(event)) {
      propagateEvent(event, {stuff.get()});
      if (choice)
        return *choice;
      if (considerResizeEvent(event, concat({stuff.get()}, getAllGuiElems())))
        continue;
      if (event.type == Event::KeyPressed)
        switch (event.key.code) {
          case Keyboard::Numpad4:
          case Keyboard::Left:
            if (index != KEEPER_CHOICE)
              index = KEEPER_CHOICE;
            else
              index = ADVENTURER_CHOICE;
            break;
          case Keyboard::Numpad6:
          case Keyboard::Right:
            if (index != ADVENTURER_CHOICE)
              index = ADVENTURER_CHOICE;
            else
              index = KEEPER_CHOICE;
            break;
          case Keyboard::Numpad8:
          case Keyboard::Up:
            if (!index)
              index = KEEPER_CHOICE;
            else
              switch (*index) {
                case LOAD_CHOICE: index = KEEPER_CHOICE; break;
                case BACK_CHOICE: index = LOAD_CHOICE; break;
                case KEEPER_CHOICE: 
                case ADVENTURER_CHOICE: index = BACK_CHOICE; break;
                default: break;
              }
            break;
          case Keyboard::Numpad2:
          case Keyboard::Down:
            if (!index)
              index = KEEPER_CHOICE;
            else
              switch (*index) {
                case LOAD_CHOICE: index = BACK_CHOICE; break;
                case BACK_CHOICE: index = KEEPER_CHOICE; break;
                case KEEPER_CHOICE: 
                case ADVENTURER_CHOICE: index = LOAD_CHOICE; break;
                default: break;
              }
            break;
          case Keyboard::Numpad5:
          case Keyboard::Return: if (index) return *index;
          case Keyboard::Escape: return BACK_CHOICE;
          default: break;
        }
    }
  }
  });
  lock.unlock();
  return returnQueue.pop();

}

optional<int> WindowView::chooseFromListInternal(const string& title, const vector<ListElem>& options, int index1,
    MenuType menuType, int* scrollPos1, optional<UserInputId> exitAction, optional<Event::KeyEvent> exitKey,
    vector<Event::KeyEvent> shortCuts) {
  if (!useTiles && menuType == MenuType::MAIN_MENU)
    menuType = MenuType::MAIN_MENU_NO_TILES;
  if (options.size() == 0)
    return none;
  RenderLock lock(renderMutex);
  uiLock = true;
  inputQueue.push(UserInputId::REFRESH);
  TempClockPause pause(clock);
  SyncQueue<optional<int>> returnQueue;
  addReturnDialog<optional<int>>(returnQueue, [=] ()-> optional<int> {
  int contentHeight;
  int choice = -1;
  int count = 0;
  int* scrollPos = scrollPos1;
  int index = index1;
  vector<int> indexes(options.size());
  vector<int> optionIndexes;
  int elemCount = 0;
  for (int i : All(options)) {
    if (options[i].getMod() == View::NORMAL) {
      indexes[count] = elemCount;
      optionIndexes.push_back(i);
      ++count;
    }
    if (options[i].getMod() != View::TITLE)
      ++elemCount;
  }
  if (optionIndexes.empty())
    optionIndexes.push_back(0);
  int localScrollPos = index >= 0 ? getScrollPos(optionIndexes[index], options.size()) : 0;
  if (scrollPos == nullptr)
    scrollPos = &localScrollPos;
  PGuiElem stuff = drawListGui(title, options, menuType, contentHeight, &index, &choice);
  PGuiElem dismissBut = GuiElem::margins(GuiElem::stack(makeVec<PGuiElem>(
        GuiElem::button([&](){ choice = -100; }),
        GuiElem::mouseHighlight(GuiElem::mainMenuHighlight(), count, &index),
        GuiElem::centerHoriz(
            GuiElem::label("Dismiss", colors[ColorId::WHITE]), renderer.getTextLength("Dismiss")))), 0, 5, 0, 0);
  if (menuType != MAIN_MENU) {
    stuff = GuiElem::scrollable(std::move(stuff), contentHeight, scrollPos);
    stuff = GuiElem::margins(std::move(stuff), 0, 15, 0, 0);
    stuff = GuiElem::margin(GuiElem::centerHoriz(std::move(dismissBut), renderer.getTextLength("Dismiss") + 100),
        std::move(stuff), 30, GuiElem::BOTTOM);
    stuff = GuiElem::window(std::move(stuff));
  }
  while (1) {
    refreshScreen(false);
    stuff->setBounds(getMenuPosition(menuType));
    stuff->render(renderer);
    renderer.drawAndClearBuffer();
    Event event;
    while (renderer.pollEvent(event)) {
      propagateEvent(event, {stuff.get()});
      if (choice > -1)
        return indexes[choice];
      if (choice == -100)
        return none;
      if (considerResizeEvent(event, concat({stuff.get()}, getAllGuiElems())))
        continue;
      if (event.type == Event::MouseWheelMoved)
          *scrollPos = min<int>(options.size() - 1, max(0, *scrollPos - event.mouseWheel.delta));
      if (event.type == Event::KeyPressed)
        switch (event.key.code) {
          case Keyboard::Numpad8:
          case Keyboard::Up:
            if (count > 0) {
              index = (index - 1 + count) % count;
              *scrollPos = getScrollPos(optionIndexes[index], options.size());
            } else
              *scrollPos = max(0, *scrollPos - 1);
            break;
          case Keyboard::Numpad2:
          case Keyboard::Down:
            if (count > 0) {
              index = (index + 1 + count) % count;
              *scrollPos = getScrollPos(optionIndexes[index], options.size());
            } else
              *scrollPos = min<int>(options.size() - 1, *scrollPos + 1);
            break;
          case Keyboard::Numpad5:
          case Keyboard::Return: if (count > 0 && index > -1) return indexes[index];
          case Keyboard::Escape: return none;
                                 //       case Keyboard::Space : refreshScreen(); return;
          default: break;
        }
    }
  }
  });
  lock.unlock();
  return returnQueue.pop();
}

static vector<string> breakText(const string& text) {
  if (text.empty())
    return {""};
  int maxWidth = 80;
  vector<string> rows;
  for (string line : split(text, {'\n'})) {
    rows.push_back("");
    for (string word : split(line, {' '}))
      if (rows.back().size() + word.size() + 1 <= maxWidth)
        rows.back().append((rows.back().size() > 0 ? " " : "") + word);
      else
        rows.push_back(word);
  }
  return rows;
}

void WindowView::presentText(const string& title, const string& text) {
  TempClockPause pause(clock);
  presentList(title, View::getListElem(breakText(text)), false);
}

void WindowView::presentList(const string& title, const vector<ListElem>& options, bool scrollDown, MenuType menu,
    optional<UserInputId> exitAction) {
  vector<ListElem> conv(options);
  for (ListElem& e : conv)
    if (e.getMod() == View::NORMAL)
      e.setMod(View::TEXT);
  int scrollPos = scrollDown ? options.size() - 1 : 0;
  chooseFromListInternal(title, conv, -1, menu, &scrollPos, exitAction, none, {});
}

const double menuLabelVPadding = 0.15;

static vector<PGuiElem> getMultiLine(const string& text, Color color, View::MenuType menuType) {
  vector<PGuiElem> ret;
  for (const string& s : breakText(text)) {
    if (menuType != View::MenuType::MAIN_MENU)
      ret.push_back(GuiElem::label(s, color));
    else
      ret.push_back(GuiElem::mainMenuLabelBg(s, menuLabelVPadding));

  }
  return ret;
}

static PGuiElem menuElemMargins(PGuiElem elem) {
  return GuiElem::margins(std::move(elem), 10, 3, 10, 0);
}

static PGuiElem getHighlight(View::MenuType type, const string& label) {
  switch (type) {
    case View::MAIN_MENU: return menuElemMargins(GuiElem::mainMenuLabel(label, menuLabelVPadding));
    default: return GuiElem::highlight(GuiElem::foreground1);
  }
}

PGuiElem WindowView::drawListGui(const string& title, const vector<ListElem>& options, MenuType menuType,
    int& height, int* highlight, int* choice) {
  vector<PGuiElem> lines;
  vector<int> heights;
  int lineHeight = 30;
  if (!title.empty()) {
    lines.push_back(GuiElem::label(capitalFirst(title), colors[ColorId::WHITE]));
    heights.push_back(lineHeight);
    lines.push_back(GuiElem::empty());
    heights.push_back(lineHeight);
  } else {
    lines.push_back(GuiElem::empty());
    heights.push_back(lineHeight / 2);
  }
  int numActive = 0;
  int secColumn = 300;
  for (auto& elem : options)
    secColumn = max(secColumn, renderer.getTextLength(elem.getText()) + 50);
  secColumn = min(secColumn, getMenuPosition(menuType).getKX() - 100);
  for (int i : All(options)) {
    Color color;
    switch (options[i].getMod()) {
      case View::TITLE: color = GuiElem::titleText; break;
      case View::INACTIVE: color = GuiElem::inactiveText; break;
      case View::TEXT:
      case View::NORMAL: color = GuiElem::text; break;
    }
    vector<PGuiElem> label1 = getMultiLine(options[i].getText(), color, menuType);
    heights.push_back(label1.size() * lineHeight);
    PGuiElem line;
    if (menuType != MAIN_MENU)
      line = GuiElem::verticalList(std::move(label1), lineHeight, 0);
    else
      line = std::move(getOnlyElement(label1));
    if (!options[i].getSecondColumn().empty())
      line = GuiElem::horizontalList(makeVec<PGuiElem>(std::move(line),
            GuiElem::label(options[i].getSecondColumn())), secColumn, 0);
    lines.push_back(menuElemMargins(std::move(line)));
    if (highlight && options[i].getMod() == View::NORMAL) {
      lines.back() = GuiElem::stack(makeVec<PGuiElem>(
            GuiElem::button([=]() { *choice = numActive; }),
            std::move(lines.back()),
            GuiElem::mouseHighlight(getHighlight(menuType, options[i].getText()), numActive, highlight)));
      ++numActive;
    }
  }
  height = accumulate(heights.begin(), heights.end(), 0);
  if (menuType != MAIN_MENU)
    return GuiElem::verticalList(std::move(lines), heights, 0);
  else
    return GuiElem::verticalListFit(std::move(lines), 0.0);
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
      renderer.getWidth() < Level::getSplashVisibleBounds().getW() * mapLayout->squareWidth())
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
bool WindowView::considerResizeEvent(sf::Event& event, vector<GuiElem*> gui) {
  if (event.type == Event::Resized) {
    resize(event.size.width, event.size.height, gui);
    return true;
  }
  return false;
}

Vec2 lastGoTo(-1, -1);

void WindowView::processEvents() {
  Event event;
  while (renderer.pollEvent(event)) {
    considerResizeEvent(event, getAllGuiElems());
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
        keyboardAction(event.key);
        renderer.flushEvents(Event::KeyPressed);
        break;
      case Event::MouseWheelMoved:
        zoom(event.mouseWheel.delta < 0);
        break;
      case Event::MouseButtonPressed :
        if (event.mouseButton.button == sf::Mouse::Right)
          guiBuilder.closeOverlayWindows();
        if (event.mouseButton.button == sf::Mouse::Middle)
          inputQueue.push(UserInput(UserInputId::DRAW_LEVEL_MAP));
        break;
      case Event::MouseButtonReleased :
        if (event.mouseButton.button == sf::Mouse::Left)
          inputQueue.push(UserInput(UserInputId::BUTTON_RELEASE, BuildingInfo(Vec2(0, 0),
              guiBuilder.getActiveBuilding())));
        break;
      default: break;
    }
  }
}

void WindowView::propagateEvent(const Event& event, vector<GuiElem*> guiElems) {
  CHECK(currentThreadId() == renderThreadId);
  if (gameReady)
    mapGui->setHint("");
  switch (event.type) {
    case Event::MouseButtonReleased : {
      Vec2 mousePos(event.mouseButton.x, event.mouseButton.y);
      for (GuiElem* elem : guiElems) {
        elem->onMouseRelease();
/*        if (mousePos.inRectangle(elem->getBounds()))
          break;*/
      }
    break; }
    case Event::MouseMoved: {
      Vec2 mousePos(event.mouseMove.x, event.mouseMove.y);
      for (GuiElem* elem : guiElems) {
        elem->onMouseMove(mousePos);
/*        if (mousePos.inRectangle(elem->getBounds()))
          break;*/
      }
      break; }
    case Event::MouseButtonPressed: {
      Vec2 clickPos(event.mouseButton.x, event.mouseButton.y);
      for (GuiElem* elem : guiElems) {
        if (event.mouseButton.button == sf::Mouse::Right)
          if (elem->onRightClick(clickPos))
            break;
        if (event.mouseButton.button == sf::Mouse::Left)
          if (elem->onLeftClick(clickPos))
            break;
      }
      }
      break;
    case Event::KeyPressed:
      for (GuiElem* elem : guiElems)
        elem->onKeyPressed2(event.key);
      break;
    case Event::TextEntered:
      if (event.text.unicode < 128) {
        char key = event.text.unicode;
        for (GuiElem* elem : guiElems)
          elem->onKeyPressed(key);
      }
      break;
    default: break;
  }
  if (gameReady)
    for (GuiElem* elem : guiElems)
      elem->onRefreshBounds();
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
#ifndef RELEASE
    case Keyboard::F8: renderer.startMonkey(); break;
    case Keyboard::Num1: GuiElem::changeBackground(1, 0, 0); break;
    case Keyboard::Num2: GuiElem::changeBackground(-1, 0, 0); break;
    case Keyboard::Num3: GuiElem::changeBackground(0, 1, 0); break;
    case Keyboard::Num4: GuiElem::changeBackground(0, -1, 0); break;
    case Keyboard::Num5: GuiElem::changeBackground(0, 0, 1); break;
    case Keyboard::Num6: GuiElem::changeBackground(0, 0, -1); break;
#endif
    case Keyboard::Z: switchZoom(); break;
    case Keyboard::F1:
      if (auto ev = getEventFromMenu()) {
        lockKeyboard = false;
        keyboardAction(*ev);
      } break;
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
      break;
    case Keyboard::Numpad9:
      inputQueue.push(UserInput(getDirActionId(key), Vec2(1, -1)));
      break;
    case Keyboard::Right: 
    case Keyboard::Numpad6:
      inputQueue.push(UserInput(getDirActionId(key), Vec2(1, 0)));
      break;
    case Keyboard::Numpad3:
      inputQueue.push(UserInput(getDirActionId(key), Vec2(1, 1)));
      break;
    case Keyboard::Down:
    case Keyboard::Numpad2:
      inputQueue.push(UserInput(getDirActionId(key), Vec2(0, 1))); break;
    case Keyboard::Numpad1:
      inputQueue.push(UserInput(getDirActionId(key), Vec2(-1, 1))); break;
    case Keyboard::Left:
    case Keyboard::Numpad4:
      inputQueue.push(UserInput(getDirActionId(key), Vec2(-1, 0))); break;
    case Keyboard::Numpad7:
      inputQueue.push(UserInput(getDirActionId(key), Vec2(-1, -1))); break;
    case Keyboard::Return:
    case Keyboard::Numpad5: inputQueue.push(UserInput(UserInputId::PICK_UP)); break;
    case Keyboard::I: inputQueue.push(UserInput(UserInputId::SHOW_INVENTORY)); break;
    case Keyboard::D: inputQueue.push(UserInput(key.shift ? UserInputId::EXT_DROP : UserInputId::DROP)); break;
    case Keyboard::A: inputQueue.push(UserInput(UserInputId::APPLY_ITEM)); break;
    case Keyboard::E: inputQueue.push(UserInput(UserInputId::EQUIPMENT)); break;
    case Keyboard::T: inputQueue.push(UserInput(UserInputId::THROW)); break;
    case Keyboard::M: inputQueue.push(UserInput(UserInputId::SHOW_HISTORY)); break;
    case Keyboard::H: inputQueue.push(UserInput(UserInputId::HIDE)); break;
    case Keyboard::P:
      if (key.shift)
        inputQueue.push(UserInput(UserInputId::EXT_PICK_UP));
      else
        inputQueue.push(UserInput(UserInputId::PAY_DEBT));
      break;
    case Keyboard::C: inputQueue.push(UserInput(key.shift ? UserInputId::CONSUME : UserInputId::CHAT)); break;
    case Keyboard::U: inputQueue.push(UserInput(UserInputId::UNPOSSESS)); break;
    case Keyboard::S: inputQueue.push(UserInput(UserInputId::CAST_SPELL)); break;
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
  return UserInput(UserInputId::IDLE);
}

vector<KeyInfo> keyInfo {
//  { "I", "Inventory", {Keyboard::I}},
//  { "E", "Manage equipment", {Keyboard::E}},
//  { "Enter", "Pick up items or interact with square", {Keyboard::Return}},
//  { "D", "Drop item", {Keyboard::D}},
  { "Shift + D", "Extended drop - choose the number of items", {Keyboard::D, false, false, true}},
  { "Shift + P", "Extended pick up - choose the number of items", {Keyboard::P, false, false, true}},
//  { "A", "Apply item", {Keyboard::A}},
//  { "T", "Throw item", {Keyboard::T}},
//  { "S", "Cast spell", {Keyboard::S}},
//  { "M", "Show message history", {Keyboard::M}},
//  { "C", "Chat with someone", {Keyboard::C}},
//  { "H", "Hide", {Keyboard::H}},
//  { "P", "Pay debt", {Keyboard::P}},
  //{ "U", "Unpossess", {Keyboard::U}},
//  { "Space", "Wait", {Keyboard::Space}},
  //{ "Z", "Zoom in/out", {Keyboard::Z}},
  { "F2", "Change settings", {Keyboard::F2}},
//  { "Escape", "Quit", {Keyboard::Escape}},
};

optional<Event::KeyEvent> WindowView::getEventFromMenu() {
  vector<View::ListElem> options {
      View::ListElem("Move around with the number pad.", View::TITLE),
      View::ListElem("Extended attack with ctrl + arrow.", View::TITLE),
      View::ListElem("Fast travel with ctrl + arrow.", View::TITLE),
      View::ListElem("Fire arrows with alt + arrow.", View::TITLE),
      View::ListElem("Choose action:", View::TITLE) };
  for (int i : All(keyInfo)) {
    Debug() << "Action " << keyInfo[i].action;
    options.push_back(keyInfo[i].action + "   [ " + keyInfo[i].keyDesc + " ]");
  }
  vector<Event::KeyEvent> shortCuts;
  for (KeyInfo key : keyInfo)
    shortCuts.push_back(key.event);
  auto index = chooseFromListInternal("", options, 0, NORMAL_MENU, nullptr, none,
      optional<Event::KeyEvent>(Event::KeyEvent{Keyboard::F1}), shortCuts);
  if (!index)
    return none;
  return keyInfo[*index].event;
}

double WindowView::getGameSpeed() {
  switch (guiBuilder.getGameSpeed()) {
    case GuiBuilder::GameSpeed::SLOW: return 0.015;
    case GuiBuilder::GameSpeed::NORMAL: return 0.025;
    case GuiBuilder::GameSpeed::FAST: return 0.04;
    case GuiBuilder::GameSpeed::VERY_FAST: return 0.06;
  }
}
