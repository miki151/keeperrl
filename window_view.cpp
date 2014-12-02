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
#include "window_renderer.h"
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

View* View::createDefaultView() {
  return new WindowView();
}

View* View::createLoggingView(OutputArchive& of) {
  return new LoggingView<WindowView>(of);
}

View* View::createReplayView(InputArchive& ifs) {
  return new ReplayView<WindowView>(ifs);
}

class TempClockPause {
  public:
  TempClockPause() {
    if (!Clock::get().isPaused()) {
      Clock::get().pause();
      cont = true;
    }
  }

  ~TempClockPause() {
    if (cont)
      Clock::get().cont();
  }

  private:
  bool cont = false;
};

int rightBarWidthCollective = 330;
int rightBarWidthPlayer = 290;
int bottomBarHeightCollective = 62;
int bottomBarHeightPlayer = 62;

WindowRenderer renderer;

Rectangle WindowView::getMapGuiBounds() const {
  switch (gameInfo.infoType) {
    case GameInfo::InfoType::PLAYER:
      return Rectangle(0, 0, renderer.getWidth() - rightBarWidthPlayer,
          renderer.getHeight() - bottomBarHeightPlayer);
    case GameInfo::InfoType::BAND:
      return Rectangle(0, 0, renderer.getWidth() - rightBarWidthCollective,
          renderer.getHeight() - bottomBarHeightCollective);
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

WindowView::WindowView() : objects(Level::getMaxBounds()),
    guiBuilder(renderer,
        [this](UserInput input) { inputQueue.push(input);},
        [this](const string& s) { mapGui->setHint(s);},
        [this](sf::Event::KeyEvent ev) { keyboardAction(ev);}) {}

static bool tilesOk = true;

bool WindowView::areTilesOk() {
  return tilesOk;
}

void WindowView::initialize() {
  renderer.initialize(1024, 600, "KeeperRL");
  Clock::set(new Clock());
  renderThreadId = currentThreadId();
  Renderer::setNominalSize(Vec2(36, 36));
  if (!ifstream("tiles_int.png")) {
    tilesOk = false;
  } else {
    Renderer::loadTilesFromFile("tiles_int.png", Vec2(36, 36));
    Renderer::loadTilesFromFile("tiles2_int.png", Vec2(36, 36));
    Renderer::loadTilesFromFile("tiles3_int.png", Vec2(36, 36));
    Renderer::loadTilesFromFile("tiles4_int.png", Vec2(24, 24));
    Renderer::loadTilesFromFile("tiles5_int.png", Vec2(36, 36));
    Renderer::loadTilesFromFile("tiles6_int.png", Vec2(36, 36));
    Renderer::loadTilesFromFile("tiles7_int.png", Vec2(36, 36));
    Renderer::loadTilesFromDir("shroom24", Vec2(24, 24));
    Renderer::loadTilesFromDir("shroom36", Vec2(36, 36));
    Renderer::loadTilesFromDir("shroom46", Vec2(46, 46));
  }
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
  Tile::loadUnicode();
  if (tilesOk) {
    currentTileLayout = spriteLayouts;
    Tile::loadTiles();
  } else
    currentTileLayout = asciiLayouts;
  mapGui = new MapGui(objects,
      [this](Vec2 pos) { mapLeftClickFun(pos); },
      [this](Vec2 pos) { mapRightClickFun(pos); },
      [this] { refreshInput = true;} );
  MinimapGui::initialize();
  minimapGui = new MinimapGui([this]() { inputQueue.push(UserInput(UserInputId::DRAW_LEVEL_MAP)); });
  minimapDecoration = GuiElem::border2(GuiElem::rectangle(colors[ColorId::BLACK]));
  resetMapBounds();
  guiBuilder.setTilesOk(tilesOk);
  if (tilesOk) {
    CHECK(menuCore.loadFromFile("ui/menu_core.png"));
    CHECK(menuMouth.loadFromFile("ui/menu_mouth.png"));
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
  mapGui->clearCenter();
  guiBuilder.reset();
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
      (mouthPos2 + menuMouth.getSize().y) * scale, sf::Color(30, 38, 24));
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
    int t0 = Clock::get().getRealMillis();
    int mouthMillis = 400;
    while (!splashDone) {
      if (tilesOk)
        drawMenuBackground(meter.getProgress(), min(1.0, double(Clock::get().getRealMillis() - t0) / mouthMillis));
      else
        renderer.drawImage((renderer.getWidth() - loadingSplash.getSize().x) / 2,
            (renderer.getHeight() - loadingSplash.getSize().y) / 2, loadingSplash);
      renderer.drawText(colors[ColorId::WHITE], renderer.getWidth() / 2, renderer.getHeight() * 0.6, text, true);
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

void printStanding(int x, int y, double standing, const string& tribeName) {
  standing = min(1., max(-1., standing));
  Color color = standing < 0
      ? Color(255, 255 * (1. + standing), 255 *(1. + standing))
      : Color(255 * (1. - standing), 255, 255 * (1. - standing));
  renderer.drawText(color, x, y, (standing >= 0 ? "friend of " : "enemy of ") + tribeName);
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
  switch (gameInfo.infoType) {
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
  tempGuiElems.clear();
  tempGuiElems.push_back(GuiElem::mainDecoration(rightBarWidth, bottomBarHeight));
  tempGuiElems.back()->setBounds(Rectangle(renderer.getWidth(), renderer.getHeight()));
  tempGuiElems.push_back(GuiElem::margins(std::move(right), 20, 20, 10, 0));
  tempGuiElems.back()->setBounds(Rectangle(
        renderer.getWidth() - rightBarWidth, 0, renderer.getWidth(), renderer.getHeight()));
  tempGuiElems.push_back(GuiElem::margins(std::move(bottom), 80, 10, 80, 0));
  tempGuiElems.back()->setBounds(Rectangle(
      0, renderer.getHeight() - bottomBarHeight,
      renderer.getWidth() - rightBarWidth, renderer.getHeight()));
  int bottomOffset = 15;
  int leftMargin = 10;
  int topMargin = 10;
  int rightMargin = 20;
  int bottomMargin = 10;
  int sideOffset = 10;
  int rightWindowHeight = 80;
  guiBuilder.drawMessages(overlays, gameInfo.messageBuffer,
      renderer.getWidth() - rightBarWidth - leftMargin - rightMargin);
  for (auto& overlay : overlays) {
    Vec2 pos;
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
    }
    tempGuiElems.push_back(GuiElem::translucentBackground(
        GuiElem::margins(std::move(overlay.elem), leftMargin, topMargin, rightMargin, bottomMargin)));
    tempGuiElems.back()->setBounds(Rectangle(pos, pos + Vec2(width, height)));
  }
}

vector<GuiElem*> WindowView::getAllGuiElems() {
  CHECK(currentThreadId() == renderThreadId);
  vector<GuiElem*> ret = extractRefs(tempGuiElems);
  if (gameReady)
    ret = concat(concat({mapGui}, ret), {minimapDecoration.get(), minimapGui});
  return ret;
}

vector<GuiElem*> WindowView::getClickableGuiElems() {
  CHECK(currentThreadId() == renderThreadId);
  vector<GuiElem*> ret = extractRefs(tempGuiElems);
  ret = getSuffix(ret, ret.size() - 1);
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
  TempClockPause pause;
  addVoidDialog([=] {
    minimapGui->presentMap(creature, getMapGuiBounds(), renderer,
        [this](double x, double y) { mapGui->setCenter(x, y);}); });
}

void WindowView::updateMinimap(const CreatureView* creature) {
  const Level* level = creature->getLevel();
  Vec2 rad(40, 40);
  Rectangle bounds(mapLayout->getPlayerPos() - rad, mapLayout->getPlayerPos() + rad);
  if (level->getBounds().intersects(bounds))
    minimapGui->update(level, bounds, creature);
}

void WindowView::updateView(const CreatureView* collective) {
  RenderLock lock(renderMutex);
  updateMinimap(collective);
  gameReady = true;
  switchTiles();
  const Level* level = collective->getLevel();
  collective->refreshGameInfo(gameInfo);
  for (Vec2 pos : mapLayout->getAllTiles(getMapGuiBounds(), Level::getMaxBounds()))
    objects[pos] = Nothing();
  if (!mapGui->isCentered())
    mapGui->setCenter(*collective->getPosition(true));
  else if (auto pos = collective->getPosition(false))
    mapGui->setCenter(*pos);
  const MapMemory* memory = &collective->getMemory(); 
  mapGui->updateLayout(mapLayout, collective->getLevel()->getBounds());
  for (Vec2 pos : mapLayout->getAllTiles(getMapGuiBounds(), Level::getMaxBounds())) 
    if (level->inBounds(pos)) {
      ViewIndex index;
      collective->getViewIndex(pos, index);
      index.setHighlight(HighlightType::NIGHT, 1.0 - collective->getLevel()->getLight(pos));
      objects[pos] = index;
    }
  mapGui->setSpriteMode(currentTileLayout.sprites);
  mapGui->updateObjects(memory);
  mapGui->setLevelBounds(level->getBounds());
  rebuildGui();
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
  RenderLock lock(renderMutex);
  CHECK(currentThreadId() == renderThreadId);
  if (gameReady)
    processEvents();
  if (renderDialog) {
    renderDialog();
  } else {
    if (gameReady)
      refreshScreen(true);
  }
}

void WindowView::drawMap() {
  for (GuiElem* gui : getAllGuiElems())
    gui->render(renderer);
  guiBuilder.addFpsCounterTick();
}

void WindowView::refreshScreen(bool flipBuffer) {
  if (!gameReady) {
    if (tilesOk)
      displayMenuSplash2();
    else
      displayOldSplash();
  }
  else
    drawMap();
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

Optional<int> reverseIndexHeight(const vector<View::ListElem>& options, int height) {
  if (height < 0 || height >= options.size() || options[height].getMod() != View::NORMAL)
    return Nothing();
  int sub = 0;
  for (int i : Range(height))
    if (options[i].getMod() != View::NORMAL)
      ++sub;
  return height - sub;
}

Optional<Vec2> WindowView::chooseDirection(const string& message) {
  RenderLock lock(renderMutex);
  TempClockPause pause;
  SyncQueue<Optional<Vec2>> returnQueue;
  addReturnDialog<Optional<Vec2>>(returnQueue, [=] ()-> Optional<Vec2> {
  gameInfo.messageBuffer = { PlayerMessage(message) };
  refreshScreen();
  do {
    Event event;
    renderer.waitEvent(event);
    considerResizeEvent(event, getAllGuiElems());
    if (event.type == Event::MouseMoved || event.type == Event::MouseButtonPressed) {
      if (auto pos = mapGui->getHighlightedTile(renderer)) {
        refreshScreen(false);
        int numArrow = 0;
        Vec2 middle = mapLayout->getAllTiles(getMapGuiBounds(), Level::getMaxBounds()).middle();
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
        Vec2 wpos = mapLayout->projectOnScreen(getMapGuiBounds(), middle + dir);
        if (currentTileLayout.sprites)
          renderer.drawSprite(wpos.x, wpos.y, 16 * 36, (8 + numArrow) * 36, 36, 36, Renderer::tiles[4]);
        else {
          static sf::Uint32 arrows[] = { L'⇐', L'⇖', L'⇑', L'⇗', L'⇒', L'⇘', L'⇓', L'⇙'};
          renderer.drawText(Renderer::SYMBOL_FONT, 20, colors[ColorId::WHITE], wpos.x + mapLayout->squareWidth() / 2, wpos.y,
              arrows[numArrow], true);
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
        case Keyboard::Escape: refreshScreen(); return Nothing();
        default: break;
      }
  } while (1);
  });
  lock.unlock();
  return returnQueue.pop();
}

bool WindowView::yesOrNoPrompt(const string& message) {
  return chooseFromList("", {ListElem(message, TITLE), "Yes", "No"}) == 0;
}

Optional<int> WindowView::getNumber(const string& title, int min, int max, int increments) {
  CHECK(min < max);
  vector<View::ListElem> options;
  vector<int> numbers;
  for (int i = min; i <= max; i += increments) {
    options.push_back(toString(i));
    numbers.push_back(i);
  }
  Optional<int> res = WindowView::chooseFromList(title, options);
  if (!res)
    return Nothing();
  else
    return numbers[*res];
}

Optional<int> WindowView::chooseFromList(const string& title, const vector<ListElem>& options, int index,
    MenuType type, int* scrollPos, Optional<UserInputId> exitAction) {
  return chooseFromListInternal(title, options, index, type, scrollPos, exitAction, Nothing(), {});
}


int getScrollPos(int index, int count) {
  return max(0, min(count - 1, index - 3));
}

Rectangle WindowView::getMenuPosition(View::MenuType type) {
  int windowWidth = 800;
  int windowHeight = 400;
  int ySpacing;
  switch (type) {
    case View::MAIN_MENU_NO_TILES:
      ySpacing = (renderer.getHeight() - windowHeight) / 2;
      break;
    case View::MAIN_MENU:
      windowWidth = renderer.getWidth() / 5;
      ySpacing = renderer.getHeight() / 3;
      break;
    default: ySpacing = 100; break;
  }
  int xSpacing = (renderer.getWidth() - windowWidth) / 2;
  return Rectangle(xSpacing, ySpacing, xSpacing + windowWidth, renderer.getHeight() - ySpacing);
}

Optional<int> WindowView::chooseFromListInternal(const string& title, const vector<ListElem>& options, int index1,
    MenuType menuType, int* scrollPos1, Optional<UserInputId> exitAction, Optional<Event::KeyEvent> exitKey,
    vector<Event::KeyEvent> shortCuts) {
  if (!tilesOk && menuType == MenuType::MAIN_MENU)
    menuType = MenuType::MAIN_MENU_NO_TILES;
  if (options.size() == 0)
    return Nothing();
  RenderLock lock(renderMutex);
  TempClockPause pause;
  SyncQueue<Optional<int>> returnQueue;
  addReturnDialog<Optional<int>>(returnQueue, [=] ()-> Optional<int> {
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
        GuiElem::mouseHighlight(GuiElem::highlight(GuiElem::foreground1), count, &index),
        GuiElem::centerHoriz(
            GuiElem::label("Dismiss", colors[ColorId::WHITE]), renderer.getTextLength("Dismiss")))), 0, 5, 0, 0);
  if (menuType != MAIN_MENU) {
    stuff = GuiElem::scrollable(std::move(stuff), contentHeight, scrollPos);
    stuff = GuiElem::margin(GuiElem::centerHoriz(std::move(dismissBut), 200),
        std::move(stuff), 30, GuiElem::BOTTOM);
    stuff = GuiElem::window(std::move(stuff));
  }
  while (1) {
/*    for (GuiElem* gui : getAllGuiElems())
      gui->render(renderer);*/
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
        return Nothing();
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
          case Keyboard::Escape: return Nothing();
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
  TempClockPause pause;
  presentList(title, View::getListElem(breakText(text)), false);
}

void WindowView::presentList(const string& title, const vector<ListElem>& options, bool scrollDown, MenuType menu,
    Optional<UserInputId> exitAction) {
  vector<ListElem> conv;
  for (ListElem e : options) {
    View::ElemMod mod = e.getMod();
    if (mod == View::NORMAL)
      mod = View::TEXT;
    conv.emplace_back(e.getText(), mod, e.getAction());
  }
  int scrollPos = scrollDown ? options.size() - 1 : 0;
  chooseFromListInternal(title, conv, -1, menu, &scrollPos, exitAction, Nothing(), {});
}

static vector<PGuiElem> getMultiLine(const string& text, Color color, View::MenuType menuType) {
  vector<PGuiElem> ret;
  for (const string& s : breakText(text)) {
    if (menuType != View::MenuType::MAIN_MENU)
      ret.push_back(GuiElem::label(s, color));
    else
      ret.push_back(GuiElem::centerHoriz(GuiElem::mainMenuLabel(s, color), renderer.getTextLength(s)));

  }
  return ret;
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
    lines.push_back(GuiElem::margins(std::move(line), 10, 3, 0, 0));
    if (highlight && options[i].getMod() == View::NORMAL) {
      lines.back() = GuiElem::stack(makeVec<PGuiElem>(
            GuiElem::button([=]() { *choice = numActive; }),
            GuiElem::mouseHighlight(GuiElem::highlight(GuiElem::foreground1), numActive, highlight),
            std::move(lines.back())));
      ++numActive;
    }
  }
  height = accumulate(heights.begin(), heights.end(), 0);
  if (menuType != MAIN_MENU)
    return GuiElem::verticalList(std::move(lines), heights, 0);
  else
    return GuiElem::verticalListFit(std::move(lines), 0.5);
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
  if (Options::getValue(OptionId::ASCII) || !tilesOk)
    currentTileLayout = asciiLayouts;
  else
    currentTileLayout = spriteLayouts;
  if (normal)
    mapLayout = &currentTileLayout.normalLayout;
  else
    mapLayout = &currentTileLayout.unzoomLayout;
}

bool WindowView::travelInterrupt() {
  return lockKeyboard;
}

int WindowView::getTimeMilli() {
  return Clock::get().getMillis();
}

int WindowView::getTimeMilliAbsolute() {
  return Clock::get().getRealMillis();
}

void WindowView::setTimeMilli(int time) {
  Clock::get().setMillis(time);
}

void WindowView::stopClock() {
  Clock::get().pause();
}

void WindowView::continueClock() {
  Clock::get().cont();
}

bool WindowView::isClockStopped() {
  return Clock::get().isPaused();
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
    propagateEvent(event, getClickableGuiElems());
    switch (event.type) {
      case Event::TextEntered:
        break;
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
          elem->onRightClick(clickPos);
        if (event.mouseButton.button == sf::Mouse::Left)
          elem->onLeftClick(clickPos);
        if (clickPos.inRectangle(elem->getBounds()))
          break;
      }
      }
      break;
    case Event::KeyPressed:
      for (GuiElem* elem : guiElems)
        elem->onKeyPressed(event.key);
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
    case Keyboard::F2: Options::handle(this, OptionSet::GENERAL); refreshScreen(); break;
    case Keyboard::Space:
      if (!Clock::get().isPaused())
        Clock::get().pause();
      else
        Clock::get().cont();
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

Optional<Event::KeyEvent> WindowView::getEventFromMenu() {
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
  auto index = chooseFromListInternal("", options, 0, NORMAL_MENU, nullptr, Nothing(),
      Optional<Event::KeyEvent>({Keyboard::F1}), shortCuts);
  if (!index)
    return Nothing();
  return keyInfo[*index].event;
}

