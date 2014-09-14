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

View* View::createLoggingView(ofstream& of) {
  return new LoggingView<WindowView>(of);
}

View* View::createReplayView(ifstream& ifs) {
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
int rightBarWidthPlayer = 220;
int rightBarWidth = rightBarWidthCollective;
int bottomBarHeight = 75;

WindowRenderer renderer;

Rectangle WindowView::getMapGuiBounds() const {
  return Rectangle(0, 0, renderer.getWidth() - rightBarWidth, renderer.getHeight() - bottomBarHeight);
}

Rectangle WindowView::getMinimapBounds() const {
  int x = 20;
  int y = 70;
  return Rectangle(x, y, x + renderer.getWidth() / 12, y + renderer.getWidth() / 12);
}

void WindowView::resetMapBounds() {
  mapGui->setBounds(getMapGuiBounds());
  minimapGui->setBounds(getMinimapBounds());
  mapDecoration->setBounds(getMapGuiBounds().minusMargin(-6));
  minimapDecoration->setBounds(getMinimapBounds().minusMargin(-6));
}

WindowView::WindowView() : objects(Level::getMaxBounds()) {}

static bool tilesOk = true;

bool WindowView::areTilesOk() {
  return tilesOk;
}

void WindowView::initialize() {
  renderer.initialize(1024, 600, "KeeperRL");
  Clock::set(new Clock());
  renderThreadId = std::this_thread::get_id();
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
  mapGui = new MapGui(objects, [this](Vec2 pos) {
      chosenCreature = "";
      switch (gameInfo.infoType) {
        case GameInfo::InfoType::PLAYER: inputQueue.push(UserInput(UserInput::MOVE_TO, pos)); break;
        case GameInfo::InfoType::BAND:
          if (!contains({CollectiveTab::WORKSHOP, CollectiveTab::BUILDINGS, CollectiveTab::TECHNOLOGY},
              collectiveTab))
            inputQueue.push(UserInput(UserInput::POSSESS, pos));
          if (collectiveTab == CollectiveTab::WORKSHOP && activeWorkshop >= 0)
            inputQueue.push(UserInput(UserInput::WORKSHOP, pos, {activeWorkshop}));
          if (collectiveTab == CollectiveTab::TECHNOLOGY && activeLibrary >= 0)
            inputQueue.push(UserInput(UserInput::LIBRARY, pos, {activeLibrary}));
          if (collectiveTab == CollectiveTab::BUILDINGS) {
            if (Keyboard::isKeyPressed(Keyboard::LShift))
              inputQueue.push(UserInput(UserInput::RECT_SELECTION, pos));
            else
              inputQueue.push(UserInput(UserInput::BUILD, pos, {activeBuilding}));
          }
          break;
      }});
  MinimapGui::initialize();
  minimapGui = new MinimapGui([this]() { inputQueue.push(UserInput(UserInput::DRAW_LEVEL_MAP)); });
  mapDecoration = GuiElem::border2(GuiElem::empty());
  minimapDecoration = GuiElem::border(GuiElem::empty());
  resetMapBounds();
}

void WindowView::reset() {
  RenderLock lock(renderMutex);
  mapLayout = &currentTileLayout.normalLayout;
  center = {0, 0};
  gameReady = false;
  clearMessageBox();
  mapGui->clearOptions();
  activeBuilding = 0;
  activeWorkshop = -1;
  activeLibrary = -1;
}

static vector<Vec2> splashPositions;
static vector<string> splashPaths { "splash2e.png", "splash2a.png", "splash2b.png", "splash2c.png", "splash2d.png" };

void displayMenuSplash2() {
  Image splash;
  CHECK(splash.loadFromFile("splash2f.png"));
  int bottomMargin = 90;
  renderer.drawImage(renderer.getWidth() / 2 - 415, renderer.getHeight() - bottomMargin, splash);
  CHECK(splash.loadFromFile("splash2e.png"));
  renderer.drawImage((renderer.getWidth() - splash.getSize().x) / 2, 90 - splash.getSize().y, splash);
  int x = 0;
  int y =0;
}

void displayMenuSplash() {
  Image splash;
  CHECK(splash.loadFromFile("splash2f.png"));
  int bottomMargin = 100;
  renderer.drawImage(100, renderer.getHeight() - bottomMargin, splash);
  vector<Rectangle> drawn;
  if (splashPositions.empty())
    random_shuffle(++splashPaths.begin(), splashPaths.end(), [](int a) { return Random.getRandom(a);});
  for (int path : All(splashPaths)) {
    CHECK(splash.loadFromFile(splashPaths[path]));
    int cnt = 100;
    while (1) {
      int px, py;
      if (!splashPositions.empty()) {
        px = splashPositions[path].x;
        py = splashPositions[path].y;
      } else {
        px = Random.getRandom(renderer.getWidth() - splash.getSize().x);
        py = Random.getRandom(renderer.getHeight() - bottomMargin - splash.getSize().y);
        splashPositions.push_back({px, py});
      }
      Rectangle pos(px, py, px + splash.getSize().x, py + splash.getSize().y);
      bool bad = false;
      for (Rectangle& rec : drawn)
        if (pos.intersects(rec)) {
          bad = true;
          break;
        }
      if (bad) {
        if (--cnt > 0)
          continue;
        else
          break;
      }
      drawn.push_back(pos);
      renderer.drawImage(pos.getPX(), pos.getPY(), splash);
      break;
    }
  }
 // drawText(colors[ColorId::WHITE], renderer.getWidth() / 2, renderer.getHeight() - 60, "Loading...", true);
 // drawAndClearBuffer();
}

void WindowView::displaySplash(View::SplashType type, atomic<bool>& ready) {
  RenderLock lock(renderMutex);
  string text;
  switch (type) {
    case View::CREATING: text = "Creating a new world, just for you..."; break;
    case View::LOADING: text = "Loading the game..."; break;
    case View::SAVING: text = "Saving the game..."; break;
  }
  Image splash;
  CHECK(splash.loadFromFile(splashPaths[Random.getRandom(1, splashPaths.size())]));
  renderDialog = [=, &ready] {
    while (!ready) {
      renderer.drawImage((renderer.getWidth() - splash.getSize().x) / 2, (renderer.getHeight() - splash.getSize().y) / 2, splash);
      renderer.drawText(colors[ColorId::WHITE], renderer.getWidth() / 2, renderer.getHeight() - 60, text, true);
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

PGuiElem WindowView::getSunlightInfoGui(GameInfo::SunlightInfo& sunlightInfo) {
  vector<PGuiElem> line;
  Color color = sunlightInfo.description == "day" ? colors[ColorId::WHITE] : colors[ColorId::LIGHT_BLUE];
  line.push_back(GuiElem::label(sunlightInfo.description, color));
  line.push_back(GuiElem::label("[" + convertToString(sunlightInfo.timeRemaining) + "]", color));
  return GuiElem::stack(
    mapGui->getHintCallback(sunlightInfo.description == "day"
      ? "Time remaining till nightfall." : "Time remaining till day."),
    GuiElem::horizontalList(std::move(line), renderer.getTextLength(sunlightInfo.description) + 5, 0));
}

PGuiElem WindowView::getTurnInfoGui(int turn) {
  return GuiElem::stack(mapGui->getHintCallback("Current turn"),
      GuiElem::label("T: " + convertToString(turn), colors[ColorId::WHITE]));
}

PGuiElem WindowView::drawBottomPlayerInfo(GameInfo::PlayerInfo& info, GameInfo::SunlightInfo& sunlightInfo) {
  string title = info.title;
  if (!info.adjectives.empty() || !info.playerName.empty())
    title = " " + title;
  for (int i : All(info.adjectives))
    title = string(i <info.adjectives.size() - 1 ? ", " : " ") + info.adjectives[i] + title;
  vector<PGuiElem> topLine;
  vector<int> topWidths;
  string nameLine = capitalFirst(!info.playerName.empty() ? info.playerName + " the" + title : title);
  topLine.push_back(GuiElem::label(nameLine, colors[ColorId::WHITE]));
  topWidths.push_back(renderer.getTextLength(nameLine) + 50);
  topLine.push_back(GuiElem::label(info.levelName, colors[ColorId::WHITE]));
  topWidths.push_back(200);
  topLine.push_back(getTurnInfoGui(info.time));;
  topWidths.push_back(100);
  topLine.push_back(getSunlightInfoGui(sunlightInfo));
  topWidths.push_back(100);
  vector<PGuiElem> bottomLine;
  vector<int> bottomWidths;
  vector<KeyInfo> bottomKeys =  {
      { "Z", "Zoom", {Keyboard::Z}},
      { "I", "Inventory", {Keyboard::I}},
      { "E", "Equipment", {Keyboard::E}},
      { "F1", "More commands", {Keyboard::F1}},
  };
  if (info.possessed)
    bottomKeys = concat({{ "U", "Leave minion", {Keyboard::U}}}, bottomKeys);
  if (info.spellcaster)
    bottomKeys = concat({{ "S", "Cast spell", {Keyboard::S}}}, bottomKeys);
  for (int i : All(bottomKeys)) {
    string text = "[" + bottomKeys[i].keyDesc + "] " + bottomKeys[i].action;
    Event::KeyEvent key = bottomKeys[i].event;
    bottomLine.push_back(GuiElem::stack(
          GuiElem::label(text, colors[ColorId::LIGHT_BLUE]),
          GuiElem::button([this, key]() { keyboardAction(key);})));
    bottomWidths.push_back(renderer.getTextLength(text));
  }
  int keySpacing = 50;
  return GuiElem::verticalList(makeVec<PGuiElem>(GuiElem::horizontalList(std::move(topLine), topWidths, 0, 2),
        GuiElem::horizontalList(std::move(bottomLine), bottomWidths, keySpacing)), 28, 0);
}

PGuiElem WindowView::drawRightPlayerInfo(GameInfo::PlayerInfo& info) {
  return drawPlayerStats(info);
}

const int legendLineHeight = 30;
const int legendStartHeight = 70;

static Color getBonusColor(int bonus) {
  if (bonus < 0)
    return colors[ColorId::RED];
  if (bonus > 0)
    return colors[ColorId::GREEN];
  return colors[ColorId::WHITE];
}

PGuiElem WindowView::drawPlayerStats(GameInfo::PlayerInfo& info) {
  vector<PGuiElem> elems;
  if (info.weaponName != "") {
    elems.push_back(GuiElem::label("Wielding:", colors[ColorId::WHITE]));
    elems.push_back(GuiElem::label(info.weaponName, colors[ColorId::WHITE]));
  } else
    elems.push_back(GuiElem::label("barehanded", colors[ColorId::RED]));
  elems.push_back(GuiElem::empty());
  for (auto& elem : info.attributes) {
    elems.push_back(GuiElem::stack(mapGui->getHintCallback(elem.help),
        GuiElem::horizontalList(makeVec<PGuiElem>(
          GuiElem::label(capitalFirst(elem.name + ":"), colors[ColorId::WHITE]),
          GuiElem::label(convertToString(elem.value), getBonusColor(elem.bonus))), 100, 1)));
  }
  elems.push_back(GuiElem::empty());
  elems.push_back(GuiElem::label("Skills:", colors[ColorId::WHITE]));
  for (auto& elem : info.skills) {
    elems.push_back(GuiElem::stack(mapGui->getHintCallback(elem.help),
          GuiElem::label(capitalFirst(elem.name), colors[ColorId::WHITE])));
  }
  elems.push_back(GuiElem::empty());
  for (auto effect : info.effects)
    elems.push_back(GuiElem::label(effect.name, effect.bad ? colors[ColorId::RED] : colors[ColorId::GREEN]));
  return GuiElem::verticalList(std::move(elems), legendLineHeight, 0);
}

typedef GameInfo::CreatureInfo CreatureInfo;

struct CreatureMapElem {
  ViewObject object;
  int count;
  CreatureInfo any;
};

static map<string, CreatureMapElem> getCreatureMap(vector<CreatureInfo>& creatures) {
  map<string, CreatureMapElem> creatureMap;
  for (int i : All(creatures)) {
    auto elem = creatures[i];
    if (!creatureMap.count(elem.speciesName)) {
      creatureMap.insert(make_pair(elem.speciesName, CreatureMapElem({elem.viewObject, 1, elem})));
    } else
      ++creatureMap.at(elem.speciesName).count;
  }
  return creatureMap;
}

/*static void drawViewObject(const ViewObject& obj, int x, int y, bool sprite) {
    Tile tile = Tile::getTile(obj, sprite);
    if (tile.hasSpriteCoord()) {
      int sz = Renderer::tileSize[tile.getTexNum()];
      int of = (Renderer::nominalSize - sz) / 2;
      Vec2 coord = tile.getSpriteCoord();
      renderer.drawSprite(x - sz / 2, y + of, coord.x * sz, coord.y * sz, sz, sz, Renderer::tiles[tile.getTexNum()], sz * 2 / 3, sz * 2 /3);
    } else
      renderer.drawText(tile.symFont ? Renderer::SYMBOL_FONT : Renderer::TEXT_FONT, 20, Tile::getColor(obj), x, y, tile.text, true);
}*/

PGuiElem WindowView::drawMinions(GameInfo::BandInfo& info) {
  map<string, CreatureMapElem> creatureMap = getCreatureMap(info.creatures);
  map<string, CreatureMapElem> enemyMap = getCreatureMap(info.enemies);
  vector<PGuiElem> list;
  list.push_back(GuiElem::label(info.monsterHeader, colors[ColorId::WHITE]));
  for (auto elem : creatureMap){
    vector<PGuiElem> line;
    vector<int> widths;
    line.push_back(GuiElem::viewObject(elem.second.object, tilesOk));
    widths.push_back(40);
    Color col = (elem.first == chosenCreature 
        || (elem.second.count == 1 && contains(info.team, elem.second.any.uniqueId))) ? colors[ColorId::GREEN] : colors[ColorId::WHITE];
    line.push_back(GuiElem::label(convertToString(elem.second.count) + "   " + elem.first, col));
    widths.push_back(200);
    function<void()> action;
    if (elem.second.count == 1)
      action = [this, elem]() {
        inputQueue.push(UserInput(UserInput::CREATURE_BUTTON, elem.second.any.uniqueId));
      };
    else if (chosenCreature == elem.first)
      action = [this] () {
        chosenCreature = "";
      };
    else
      action = [this, elem] () {
        chosenCreature = elem.first;
      };
    list.push_back(GuiElem::margins(GuiElem::stack(GuiElem::button(action), GuiElem::horizontalList(std::move(line), widths, 0)), 20, 0, 0, 0));
  }
  if (!info.team.empty()) {
    list.push_back(GuiElem::label("Team: " + getPlural("monster", "monsters", info.team.size()), colors[ColorId::WHITE]));
    if (!info.gatheringTeam) {
      vector<PGuiElem> line;
      vector<int> widths;
      line.push_back(GuiElem::stack(GuiElem::button(getButtonCallback(UserInput::GATHER_TEAM)),
          GuiElem::label("[command]", colors[ColorId::WHITE])));
      widths.push_back(10 + renderer.getTextLength("[command]"));
      line.push_back(GuiElem::stack(GuiElem::button(getButtonCallback(UserInput::EDIT_TEAM)),
          GuiElem::label("[edit]", colors[ColorId::WHITE])));
      widths.push_back(10 + renderer.getTextLength("[edit]"));
      line.push_back(GuiElem::stack(GuiElem::button(getButtonCallback(UserInput::CANCEL_TEAM)),
          GuiElem::label("[cancel]", colors[ColorId::WHITE])));
      widths.push_back(100);
      list.push_back(GuiElem::horizontalList(std::move(line), widths, 0));
    }
    
  }
  if ((info.creatures.size() > 1 && info.team.empty()) || info.gatheringTeam) {
    vector<PGuiElem> line;
    line.push_back(GuiElem::stack(GuiElem::button(getButtonCallback(UserInput::GATHER_TEAM)),
        GuiElem::label(info.team.empty() ? "[form team]" : "[done]",
            (info.gatheringTeam && info.team.empty()) ? colors[ColorId::GREEN] : colors[ColorId::WHITE])));
    if (info.gatheringTeam) {
      line.push_back(GuiElem::stack(GuiElem::button(getButtonCallback(UserInput::CANCEL_TEAM)),
          GuiElem::label("[cancel]", colors[ColorId::WHITE])));
    }
    list.push_back(GuiElem::horizontalList(std::move(line), 150, 0));
  }
  list.push_back(GuiElem::label("Click on minion to possess.", colors[ColorId::LIGHT_BLUE]));
  list.push_back(GuiElem::empty());
  vector<PGuiElem> res;
  res.push_back(GuiElem::label("Next payout [" + convertToString(info.payoutTimeRemaining) + "]:",
        colors[ColorId::WHITE]));
  res.push_back(GuiElem::viewObject(info.numResource[0].viewObject, tilesOk));
  res.push_back(GuiElem::label(convertToString<int>(info.nextPayout), colors[ColorId::WHITE]));
  list.push_back(GuiElem::horizontalList(std::move(res), {170, 30, 1}, 0));
  list.push_back(GuiElem::empty());
  if (!enemyMap.empty()) {
    list.push_back(GuiElem::label("Enemies:", colors[ColorId::WHITE]));
    for (auto elem : enemyMap){
      vector<PGuiElem> line;
      line.push_back(GuiElem::viewObject(elem.second.object, tilesOk));
      line.push_back(GuiElem::label(convertToString(elem.second.count) + "   " + elem.first, colors[ColorId::WHITE]));
      list.push_back(GuiElem::horizontalList(std::move(line), 20, 0));
    }
  }
  if (!creatureMap.count(chosenCreature)) {
    chosenCreature = "";
  }
  return GuiElem::verticalList(std::move(list), legendLineHeight, 0);
}

PGuiElem WindowView::drawMinionWindow(GameInfo::BandInfo& info) {
  if (chosenCreature != "") {
    vector<PGuiElem> lines;
    vector<CreatureInfo> chosen;
    for (auto& c : info.creatures)
      if (c.speciesName == chosenCreature)
        chosen.push_back(c);
    lines.push_back(GuiElem::label(info.gatheringTeam ? "Click to add to team:" : "Click to control:", colors[ColorId::LIGHT_BLUE]));
    for (auto& c : chosen) {
      string text = "L: " + convertToString(c.expLevel) + "    " + info.tasks[c.uniqueId];
      if (c.speciesName != c.name)
        text = c.name + " " + text;
      vector<PGuiElem> line;
      line.push_back(GuiElem::viewObject(c.viewObject, tilesOk));
      line.push_back(GuiElem::label(text, (info.gatheringTeam && contains(info.team, c.uniqueId)) ? colors[ColorId::GREEN] : colors[ColorId::WHITE]));
      lines.push_back(GuiElem::stack(GuiElem::button(
              getButtonCallback(UserInput(UserInput::CREATURE_BUTTON, c.uniqueId))),
            GuiElem::horizontalList(std::move(line), 40, 0)));
    }
    return GuiElem::verticalList(std::move(lines), legendLineHeight, 0);
  } else
    return PGuiElem();
}


PGuiElem WindowView::drawVillages(GameInfo::VillageInfo& info) {
  vector<PGuiElem> lines;
  for (auto elem : info.villages) {
    lines.push_back(GuiElem::label(capitalFirst(elem.name), colors[ColorId::WHITE]));
    lines.push_back(GuiElem::margins(GuiElem::label("tribe: " + elem.tribeName, colors[ColorId::WHITE]), 40, 0, 0, 0));
    if (!elem.state.empty())
      lines.push_back(GuiElem::margins(GuiElem::label(elem.state, elem.state == "conquered" ? colors[ColorId::GREEN] : colors[ColorId::RED]),
            40, 0, 0, 0));
  }
  return GuiElem::verticalList(std::move(lines), legendLineHeight, 0);
}

void WindowView::setOptionsMenu(const string& title, const vector<GameInfo::BandInfo::Button>& buttons, int offset,
    int& active, CollectiveTab tab) {
  vector<PGuiElem> elems;
  for (int i : All(buttons)) {
    elems.push_back(getButtonLine(buttons[i], [=] {}, offset + i, active, tab));
  }
  mapGui->setOptions(title, std::move(elems));
}

void WindowView::setCollectiveTab(CollectiveTab t) {
  if (collectiveTab != t) {
    collectiveTab = t;
    mapGui->clearOptions();
  }
}

PGuiElem WindowView::getButtonLine(GameInfo::BandInfo::Button button, function<void()> fun, int num, int& active,
    CollectiveTab tab) {
  vector<PGuiElem> line;
  line.push_back(GuiElem::viewObject(button.object, tilesOk));
  vector<int> widths { 35 };
  if (!button.inactiveReason.empty())
    line.push_back(GuiElem::label(button.name + " " + button.count, colors[ColorId::GRAY], button.hotkey));
  else
    line.push_back(GuiElem::conditional(
          GuiElem::label(button.name + " " + button.count, colors[ColorId::GREEN], button.hotkey),
          GuiElem::label(button.name + " " + button.count, colors[ColorId::WHITE], button.hotkey),
          [=, &active] (GuiElem*) { return active == num; }));
  widths.push_back(100);
  if (button.cost) {
    string costText = convertToString(button.cost->second);
    line.push_back(GuiElem::label(costText, colors[ColorId::WHITE]));
    widths.push_back(renderer.getTextLength(costText) + 8);
    line.push_back(GuiElem::viewObject(button.cost->first, tilesOk));
    widths.push_back(25);
  }
  function<void()> buttonFun;
  if (button.inactiveReason == "" || button.inactiveReason == "inactive")
    buttonFun = [this, &active, num, tab, fun] {
      active = num;
      setCollectiveTab(tab);
      fun();
    };
  else {
    string s = button.inactiveReason;
    buttonFun = [this, s] {
      presentText("", s);
    };
  }
  return GuiElem::stack(
      mapGui->getHintCallback(capitalFirst(button.help)),
      GuiElem::button(buttonFun, button.hotkey),
      GuiElem::horizontalList(std::move(line), widths, 0, button.cost ? 2 : 0));
}

vector<PGuiElem> WindowView::drawButtons(vector<GameInfo::BandInfo::Button> buttons, int& active,
    CollectiveTab tab) {
  vector<PGuiElem> elems;
  vector<PGuiElem> invisible;
  vector<GameInfo::BandInfo::Button> groupedButtons;
  for (int i = 0; i <= buttons.size(); ++i) {
    if (!groupedButtons.empty() && (i == buttons.size() || buttons[i].groupName != groupedButtons[0].groupName)) {
      int firstItem = i - groupedButtons.size();
      int lastItem = i - 1;
      GameInfo::BandInfo::Button button1 = groupedButtons.front();
      function<void()> buttonFun = [=, &active] {
        setOptionsMenu(button1.groupName, groupedButtons, firstItem, active, tab);
        active = firstItem;
      };
      vector<PGuiElem> line;
      line.push_back(GuiElem::viewObject(button1.object, tilesOk));
      line.push_back(GuiElem::label(groupedButtons.front().groupName,
            active <= lastItem && active >= firstItem ? colors[ColorId::GREEN] : colors[ColorId::WHITE]));
      elems.push_back(GuiElem::stack(
            GuiElem::button(buttonFun),
            GuiElem::horizontalList(std::move(line), 35, 0)));
      for (int groupedInd : All(groupedButtons))
        invisible.push_back(getButtonLine(groupedButtons[groupedInd], [=, &active] {
            setOptionsMenu(button1.groupName, groupedButtons, firstItem, active, tab);},
            firstItem + groupedInd, active, tab));
      groupedButtons.clear();
    }
    if (i == buttons.size())
      break;
    if (!buttons[i].groupName.empty())
      groupedButtons.push_back(buttons[i]);
    else
      elems.push_back(getButtonLine(buttons[i], [=] { mapGui->clearOptions(); }, i, active, tab));
  }
  elems.push_back(GuiElem::invisible(GuiElem::stack(std::move(invisible))));
  return elems;
}
  
PGuiElem WindowView::drawBuildings(GameInfo::BandInfo& info) {
  return GuiElem::verticalList(drawButtons(info.buildings, activeBuilding, CollectiveTab::BUILDINGS),
      legendLineHeight, 0);
}

static PGuiElem getStandingGui(double standing) {
  sf::Color color = standing >= 0 ? sf::Color((1. - standing) * 255, 255, (1. - standing) * 255)
      : sf::Color(255, (1. + standing) * 255, (1. + standing) * 255);
  if (standing < -0.33)
    return GuiElem::label("bad", color);
  if (standing < 0.33)
    return GuiElem::label("neutral", color);
  else
    return GuiElem::label("good", color);
}

PGuiElem WindowView::drawDeities(GameInfo::BandInfo& info) {
  vector<PGuiElem> lines;
  for (int i : All(info.deities)) {
    lines.push_back(GuiElem::stack(
          GuiElem::button(getButtonCallback(UserInput(UserInput::DEITIES, i))),
          GuiElem::label(capitalFirst(info.deities[i].name), colors[ColorId::WHITE])));
    lines.push_back(GuiElem::margins(GuiElem::horizontalList(makeVec<PGuiElem>(
              GuiElem::label("standing: ", colors[ColorId::WHITE]),
              getStandingGui(info.deities[i].standing)), 85, 0), 40, 0, 0, 0));
  }
  return GuiElem::verticalList(std::move(lines), legendLineHeight, 0);
}

PGuiElem WindowView::drawWorkshop(GameInfo::BandInfo& info) {
  return GuiElem::verticalList(drawButtons(info.workshop, activeWorkshop, CollectiveTab::WORKSHOP),
      legendLineHeight, 0);
}

PGuiElem WindowView::drawTechnology(GameInfo::BandInfo& info) {
  vector<PGuiElem> lines = drawButtons(info.libraryButtons, activeLibrary, CollectiveTab::TECHNOLOGY);
  for (int i : All(info.techButtons)) {
    vector<PGuiElem> line;
    line.push_back(GuiElem::viewObject(ViewObject(info.techButtons[i].viewId, ViewLayer::CREATURE, ""), tilesOk));
    line.push_back(GuiElem::label(info.techButtons[i].name, colors[ColorId::WHITE], info.techButtons[i].hotkey));
    lines.push_back(GuiElem::stack(
          GuiElem::button(getButtonCallback(UserInput(UserInput::TECHNOLOGY, i)), info.techButtons[i].hotkey),
          GuiElem::horizontalList(std::move(line), 35, 0)));
  }
  return GuiElem::verticalList(std::move(lines), legendLineHeight, 0);
}

PGuiElem WindowView::drawKeeperHelp() {
  vector<string> helpText { "use mouse to dig and build", "shift selects rectangles", "", "scroll with arrows", "or right mouse button", "shift with arros scrolls faster", "", "click on minion", "to possess",
   "", "[space] pause", "[z] or mouse wheel: zoom", "press mouse wheel: level map", "", "follow the red hints :-)"};
  vector<PGuiElem> lines;
  for (string line : helpText)
    lines.push_back(GuiElem::label(line, colors[ColorId::LIGHT_BLUE]));
  return GuiElem::verticalList(std::move(lines), legendLineHeight, 0);
}

PGuiElem WindowView::drawBottomBandInfo(GameInfo::BandInfo& info, GameInfo::SunlightInfo& sunlightInfo) {
  vector<PGuiElem> topLine;
  vector<int> topWidths;
  int resourceSpacing = 95;
  for (int i : All(info.numResource)) {
    vector<PGuiElem> res;
    res.push_back(GuiElem::viewObject(info.numResource[i].viewObject, tilesOk));
    res.push_back(GuiElem::label(convertToString<int>(info.numResource[i].count), colors[ColorId::WHITE]));
    topWidths.push_back(resourceSpacing);
    topLine.push_back(GuiElem::stack(mapGui->getHintCallback(info.numResource[i].name),
            GuiElem::horizontalList(std::move(res), 30, 0)));
  }
  topLine.push_back(getTurnInfoGui(info.time));
  topWidths.push_back(100);
  topLine.push_back(getSunlightInfoGui(sunlightInfo));
  topWidths.push_back(100);
  vector<PGuiElem> bottomLine;
  if (Clock::get().isPaused())
    bottomLine.push_back(GuiElem::stack(GuiElem::button([&]() { Clock::get().cont(); }),
        GuiElem::label("PAUSED", colors[ColorId::RED])));
  else
    bottomLine.push_back(GuiElem::stack(GuiElem::button([&]() { Clock::get().pause(); }),
        GuiElem::label("PAUSE", colors[ColorId::LIGHT_BLUE])));
  bottomLine.push_back(GuiElem::stack(GuiElem::button([&]() { switchZoom(); }),
        GuiElem::label("ZOOM", colors[ColorId::LIGHT_BLUE])));
  bottomLine.push_back(GuiElem::label(info.warning, colors[ColorId::RED]));
  return GuiElem::verticalList(makeVec<PGuiElem>(GuiElem::horizontalList(std::move(topLine), topWidths, 0, 2),
        GuiElem::horizontalList(std::move(bottomLine), 85, 0)), 28, 0);
}

PGuiElem WindowView::drawRightBandInfo(GameInfo::BandInfo& info, GameInfo::VillageInfo& villageInfo) {
  vector<PGuiElem> buttons = makeVec<PGuiElem>(
    GuiElem::icon(GuiElem::BUILDING),
    GuiElem::icon(GuiElem::MINION),
    GuiElem::icon(GuiElem::LIBRARY),
    GuiElem::icon(GuiElem::DEITIES),
    GuiElem::icon(GuiElem::DIPLOMACY),
    GuiElem::icon(GuiElem::HELP));
  for (int i : All(buttons)) {
    if (int(collectiveTab) == i)
      buttons[i] = GuiElem::border2(std::move(buttons[i]));
    else
      buttons[i] = GuiElem::margins(std::move(buttons[i]), 6, 6, 6, 6);
    buttons[i] = GuiElem::stack(std::move(buttons[i]),
        GuiElem::button([this, i]() { setCollectiveTab(CollectiveTab(i)); }));
  }
  if (collectiveTab != CollectiveTab::MINIONS)
    chosenCreature = "";
  PGuiElem main;
  vector<PGuiElem> invisible;
  vector<pair<CollectiveTab, PGuiElem>> elems = makeVec<pair<CollectiveTab, PGuiElem>>(
    make_pair(CollectiveTab::MINIONS, drawMinions(info)),
    make_pair(CollectiveTab::BUILDINGS, drawBuildings(info)),
    make_pair(CollectiveTab::KEY_MAPPING, drawKeeperHelp()),
    make_pair(CollectiveTab::TECHNOLOGY, drawTechnology(info)),
    make_pair(CollectiveTab::WORKSHOP, drawDeities(info)),
    make_pair(CollectiveTab::VILLAGES, drawVillages(villageInfo)));
  for (auto& elem : elems)
    if (elem.first == collectiveTab)
      main = std::move(elem.second);
    else
      invisible.push_back(GuiElem::invisible(std::move(elem.second)));
  main = GuiElem::border2(GuiElem::margins(std::move(main), 15, 15, 15, 5));
  PGuiElem butGui = GuiElem::margins(GuiElem::horizontalList(std::move(buttons), 50, 0), 0, 0, 0, 5);
  return GuiElem::stack(GuiElem::stack(std::move(invisible)),
      GuiElem::margin(std::move(butGui), std::move(main), 55, GuiElem::TOP));
}

const int minionWindowRightMargin = 20;
const int minionWindowWidth = 340;
const int minionWindowHeight = 600;

void WindowView::rebuildGui() {
  PGuiElem bottom, right, overMap;
  switch (gameInfo.infoType) {
    case GameInfo::InfoType::PLAYER:
        right = drawRightPlayerInfo(gameInfo.playerInfo);
        bottom = drawBottomPlayerInfo(gameInfo.playerInfo, gameInfo.sunlightInfo);
        rightBarWidth = rightBarWidthPlayer;
        break;
    case GameInfo::InfoType::BAND:
        right = drawRightBandInfo(gameInfo.bandInfo, gameInfo.villageInfo);
        overMap = drawMinionWindow(gameInfo.bandInfo);
        bottom = drawBottomBandInfo(gameInfo.bandInfo, gameInfo.sunlightInfo);
        rightBarWidth = rightBarWidthCollective;
        break;
  }
  resetMapBounds();
  CHECK(std::this_thread::get_id() != renderThreadId);
  tempGuiElems.clear();
  tempGuiElems.push_back(GuiElem::stack(GuiElem::background(GuiElem::background2), 
        GuiElem::margins(std::move(right), 20, 20, 10, 20)));
  tempGuiElems.back()->setBounds(Rectangle(
        renderer.getWidth() - rightBarWidth, 0, renderer.getWidth(), renderer.getHeight()));
  tempGuiElems.push_back(GuiElem::stack(GuiElem::background(GuiElem::background2),
        GuiElem::margins(std::move(bottom), 10, 10, 0, 0)));
  tempGuiElems.back()->setBounds(Rectangle(
        0, renderer.getHeight() - bottomBarHeight, renderer.getWidth() - rightBarWidth, renderer.getHeight()));
  if (overMap) {
    tempGuiElems.push_back(GuiElem::window(GuiElem::stack(GuiElem::background(GuiElem::background2), 
          GuiElem::margins(std::move(overMap), 20, 20, 20, 20))));
    tempGuiElems.back()->setBounds(Rectangle(
          renderer.getWidth() - rightBarWidth - minionWindowWidth - minionWindowRightMargin, 100,
          renderer.getWidth() - rightBarWidth - minionWindowRightMargin, 100 + minionWindowHeight));
  }
}

vector<GuiElem*> WindowView::getAllGuiElems() {
  CHECK(std::this_thread::get_id() == renderThreadId);
  vector<GuiElem*> ret = extractRefs(tempGuiElems);
  if (gameReady)
    ret = concat(concat({mapGui}, ret), {mapDecoration.get(), minimapDecoration.get(), minimapGui});
  return ret;
}

vector<GuiElem*> WindowView::getClickableGuiElems() {
  CHECK(std::this_thread::get_id() == renderThreadId);
  vector<GuiElem*> ret = extractRefs(tempGuiElems);
  if (gameReady) {
    ret.push_back(minimapGui);
    ret.push_back(mapGui);
  }
  return ret;
}

void WindowView::refreshText() {
  int lineHeight = 20;
  int numMsg = 0;
  for (int i : All(currentMessage))
    if (!currentMessage[i].empty())
      numMsg = i + 1;
  if (numMsg > 0)
    renderer.drawFilledRectangle(0, 0, renderer.getWidth() - rightBarWidth, lineHeight * numMsg + 15,
        transparency(colors[ColorId::BLACK], 100));
  for (int i : All(currentMessage))
    renderer.drawText(oldMessage ? colors[ColorId::GRAY] : colors[ColorId::WHITE],
        10, 5 + lineHeight * i, currentMessage[i]);
}

void WindowView::resetCenter() {
  center = {0, 0};
}

function<void()> WindowView::getButtonCallback(UserInput input) {
  return [this, input]() { inputQueue.push(input); };
}

void WindowView::addVoidDialog(function<void()> fun) {
  RenderLock lock(renderMutex);
  Semaphore sem;
  renderDialog = [this, &sem, fun] {
    fun();
    sem.v();
    renderDialog = nullptr;
  };
  if (std::this_thread::get_id() == renderThreadId)
    renderDialog();
  lock.unlock();
  sem.p();
}

void WindowView::drawLevelMap(const CreatureView* creature) {
  TempClockPause pause;
  addVoidDialog([=] {
    minimapGui->presentMap(creature, getMapGuiBounds(), renderer,
        [this](double x, double y) { center = {x, y};}); });
}

void WindowView::updateMinimap(const CreatureView* creature) {
  const Level* level = creature->getViewLevel();
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
  const Level* level = collective->getViewLevel();
  collective->refreshGameInfo(gameInfo);
  for (Vec2 pos : mapLayout->getAllTiles(getMapGuiBounds(), Level::getMaxBounds()))
    objects[pos] = Nothing();
  if ((center.x == 0 && center.y == 0) || collective->staticPosition())
    center = {double(collective->getPosition().x), double(collective->getPosition().y)};
  Vec2 movePos = Vec2((center.x - mouseOffset.x) * mapLayout->squareWidth(),
      (center.y - mouseOffset.y) * mapLayout->squareHeight());
  movePos.x = max(movePos.x, 0);
  movePos.x = min(movePos.x, int(collective->getViewLevel()->getBounds().getKX() * mapLayout->squareWidth()));
  movePos.y = max(movePos.y, 0);
  movePos.y = min(movePos.y, int(collective->getViewLevel()->getBounds().getKY() * mapLayout->squareHeight()));
  mapLayout->updatePlayerPos(movePos);
  const MapMemory* memory = &collective->getMemory(); 
  for (Vec2 pos : mapLayout->getAllTiles(getMapGuiBounds(), Level::getMaxBounds())) 
    if (level->inBounds(pos)) {
      ViewIndex index = collective->getViewIndex(pos);
      if (!index.hasObject(ViewLayer::FLOOR) && !index.hasObject(ViewLayer::FLOOR_BACKGROUND) &&
          !index.isEmpty() && memory->hasViewIndex(pos)) {
        // special case when monster or item is visible but floor is only in memory
        if (memory->getViewIndex(pos).hasObject(ViewLayer::FLOOR))
          index.insert(memory->getViewIndex(pos).getObject(ViewLayer::FLOOR));
        if (memory->getViewIndex(pos).hasObject(ViewLayer::FLOOR_BACKGROUND))
          index.insert(memory->getViewIndex(pos).getObject(ViewLayer::FLOOR_BACKGROUND));
      }
      if (index.isEmpty() && memory->hasViewIndex(pos))
        index = memory->getViewIndex(pos);
      index.setHighlight(HighlightType::NIGHT, 1.0 - collective->getViewLevel()->getLight(pos));
      objects[pos] = index;
    }
  mapGui->setLayout(mapLayout);
  mapGui->setSpriteMode(currentTileLayout.sprites);
  mapGui->updateObjects(memory);
  mapGui->setLevelBounds(level->getBounds());
  rebuildGui();
}

void WindowView::refreshView() {
  RenderLock lock(renderMutex);
  CHECK(std::this_thread::get_id() == renderThreadId);
  processEvents();
  if (renderDialog) {
    renderDialog();
  } else
    refreshScreen(true);
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

class FpsCounter {
  public:

  int getSec() {
    return clock.getElapsedTime().asSeconds();
  }
  void addTick() {
    if (getSec() == curSec)
      ++curFps;
    else {
      lastFps = curFps;
      curSec = getSec();
      curFps = 0;
    }
  }

  int getFps() {
    return lastFps;
  }

  int lastFps = 0;
  int curSec = -1;
  int curFps = 0;
  sf::Clock clock;
} fpsCounter;


void WindowView::drawMap() {
  for (GuiElem* gui : getAllGuiElems())
    gui->render(renderer);
  refreshText();
  fpsCounter.addTick();
  renderer.drawText(colors[ColorId::WHITE], renderer.getWidth() - 70, renderer.getHeight() - 30, "FPS " + convertToString(fpsCounter.getFps()));
}

void WindowView::refreshScreen(bool flipBuffer) {
  if (!gameReady)
    displayMenuSplash2();
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
  showMessage(message);
  refreshScreen();
  do {
    showMessage(message);
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
        case Keyboard::Numpad8: showMessage(""); refreshScreen(); return Vec2(0, -1);
        case Keyboard::Numpad9: showMessage(""); refreshScreen(); return Vec2(1, -1);
        case Keyboard::Right:
        case Keyboard::Numpad6: showMessage(""); refreshScreen(); return Vec2(1, 0);
        case Keyboard::Numpad3: showMessage(""); refreshScreen(); return Vec2(1, 1);
        case Keyboard::Down:
        case Keyboard::Numpad2: showMessage(""); refreshScreen(); return Vec2(0, 1);
        case Keyboard::Numpad1: showMessage(""); refreshScreen(); return Vec2(-1, 1);
        case Keyboard::Left:
        case Keyboard::Numpad4: showMessage(""); refreshScreen(); return Vec2(-1, 0);
        case Keyboard::Numpad7: showMessage(""); refreshScreen(); return Vec2(-1, -1);
        case Keyboard::Escape: showMessage(""); refreshScreen(); return Nothing();
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
    options.push_back(convertToString(i));
    numbers.push_back(i);
  }
  Optional<int> res = WindowView::chooseFromList(title, options);
  if (!res)
    return Nothing();
  else
    return numbers[*res];
}

Optional<int> WindowView::chooseFromList(const string& title, const vector<ListElem>& options, int index,
    MenuType type, int* scrollPos, Optional<UserInput::Type> exitAction) {
  return chooseFromListInternal(title, options, index, type, scrollPos, exitAction, Nothing(), {});
}


int getScrollPos(int index, int count) {
  return max(0, min(count - 1, index - 3));
}

Rectangle WindowView::getMenuPosition(View::MenuType type) {
  int windowWidth = 800;
  int ySpacing = 100;
  int xSpacing = (renderer.getWidth() - windowWidth) / 2;
  return Rectangle(xSpacing, ySpacing, xSpacing + windowWidth, renderer.getHeight() - ySpacing);
/*  switch (type) {
    case View::MAIN_MENU:
    case View::NORMAL_MENU:
      return Rectangle(xSpacing, ySpacing, xSpacing + windowWidth, renderer.getHeight() - ySpacing);
    case View::MINION_MENU:
      return Rectangle(renderer.getWidth() - rightBarWidth - 2 * minionWindowRightMargin - minionWindowWidth - 600,
          100, renderer.getWidth() - rightBarWidth - 2 * minionWindowRightMargin - minionWindowWidth,
        100 + minionWindowHeight);
  }
  FAIL << "ewf";
  return Rectangle(1, 1);*/
}

Optional<int> WindowView::chooseFromListInternal(const string& title, const vector<ListElem>& options, int index1,
    MenuType menuType, int* scrollPos1, Optional<UserInput::Type> exitAction, Optional<Event::KeyEvent> exitKey,
    vector<Event::KeyEvent> shortCuts) {
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
  PGuiElem list = drawListGui(title, options, contentHeight, &index, &choice);
  PGuiElem dismissBut = GuiElem::stack(makeVec<PGuiElem>(
        GuiElem::button([&](){ choice = -100; }),
        GuiElem::mouseHighlight(GuiElem::highlight(GuiElem::foreground1), count, &index),
        GuiElem::centerHoriz(GuiElem::label("Dismiss", colors[ColorId::WHITE]), renderer.getTextLength("Dismiss"))));
  PGuiElem stuff = GuiElem::scrollable(std::move(list), contentHeight, scrollPos);
  if (menuType != MAIN_MENU)
    stuff = GuiElem::margin(GuiElem::centerHoriz(std::move(dismissBut), 200), std::move(stuff), 30, GuiElem::BOTTOM);
  PGuiElem window = GuiElem::window(std::move(stuff));
  while (1) {
/*    for (GuiElem* gui : getAllGuiElems())
      gui->render(renderer);*/
    refreshScreen(false);
    window->setBounds(getMenuPosition(menuType));
    window->render(renderer);
    renderer.drawAndClearBuffer();
    Event event;
    while (renderer.pollEvent(event)) {
      propagateEvent(event, {window.get()});
      if (choice > -1)
        return indexes[choice];
      if (choice == -100)
        return Nothing();
      if (considerResizeEvent(event, concat({window.get()}, getAllGuiElems())))
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

void WindowView::presentList(const string& title, const vector<ListElem>& options, bool scrollDown,
    Optional<UserInput::Type> exitAction) {
  vector<ListElem> conv;
  for (ListElem e : options) {
    View::ElemMod mod = e.getMod();
    if (mod == View::NORMAL)
      mod = View::TEXT;
    conv.emplace_back(e.getText(), mod, e.getAction());
  }
  int scrollPos = scrollDown ? 1 : 0;
  chooseFromListInternal(title, conv, -1, NORMAL_MENU, &scrollPos, exitAction, Nothing(), {});
}

static vector<PGuiElem> getMultiLine(const string& text, Color color) {
  vector<PGuiElem> ret;
  for (const string& s : breakText(text))
    ret.push_back(GuiElem::label(s, color));
  return ret;
}

PGuiElem WindowView::drawListGui(const string& title, const vector<ListElem>& options, int& height,
    int* highlight, int* choice) {
  vector<PGuiElem> lines;
  int lineHeight = 30;
  if (!title.empty())
    lines.push_back(GuiElem::label(capitalFirst(title), colors[ColorId::WHITE]));
  lines.push_back(GuiElem::empty());
  vector<int> heights(lines.size(), lineHeight);
  int numActive = 0;
  for (int i : All(options)) {
    Color color;
    switch (options[i].getMod()) {
      case View::TITLE: color = GuiElem::titleText; break;
      case View::INACTIVE: color = GuiElem::inactiveText; break;
      case View::TEXT:
      case View::NORMAL: color = GuiElem::text; break;
    }
    vector<PGuiElem> label1 = getMultiLine(options[i].getText(), color);
    heights.push_back(label1.size() * lineHeight);
    lines.push_back(GuiElem::margins(GuiElem::verticalList(std::move(label1), lineHeight, 0), 10, 3, 0, 0));
    if (highlight && options[i].getMod() == View::NORMAL) {
      lines.back() = GuiElem::stack(makeVec<PGuiElem>(
            GuiElem::button([=]() { *choice = numActive; }),
            GuiElem::margins(
              GuiElem::mouseHighlight(GuiElem::highlight(GuiElem::foreground1), numActive, highlight), 0, 0, 0, 0),
            std::move(lines.back())));
      ++numActive;
    }
  }
  height = accumulate(heights.begin(), heights.end(), 0);
  return GuiElem::verticalList(std::move(lines), heights, 0);
}

void WindowView::clearMessageBox() {
  messageInd = 0;
  oldMessage = false;
  for (int i : All(currentMessage))
    currentMessage[i].clear();
}


void WindowView::showMessage(const string& message) {
  messageInd = 0;
  oldMessage = false;
  for (int i : All(currentMessage))
    currentMessage[i].clear();
  currentMessage[0] = message;
}

void WindowView::addImportantMessage(const string& message) {
  presentText("", message);
}

void WindowView::clearMessages() { 
  showMessage("");
}

void WindowView::retireMessages() {
  string lastMsg = currentMessage[messageInd];
  showMessage(lastMsg);
  oldMessage = true;
}

void WindowView::addMessage(const string& message) {
  if (oldMessage)
    showMessage("");
  oldMessage = false;
  if (currentMessage[messageInd].size() + message.size() + 1 > maxMsgLength &&
      messageInd == currentMessage.size() - 1) {
    currentMessage.pop_front();
    currentMessage.push_back("");
  }
  if (currentMessage[messageInd].size() + message.size() + 1 > maxMsgLength)
    ++messageInd;
  currentMessage[messageInd] += (currentMessage[messageInd].size() > 0 ? " " : "") + message;
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

bool WindowView::considerScrollEvent(sf::Event& event) {
  static bool lastPressed = false;
  if (lastPressed && !Mouse::isButtonPressed(Mouse::Right)) {
    center = {center.x - mouseOffset.x, center.y - mouseOffset.y};
    mouseOffset = {0.0, 0.0};
    lastPressed = false;  
    refreshInput = true;
    return true;
  }
  if (!lastPressed && Mouse::isButtonPressed(Mouse::Right)) {
    lastMousePos = renderer.getMousePos();
    lastPressed = true;
    chosenCreature = "";
    return true;
  }
  if (event.type == Event::MouseMoved && lastPressed) {
    mouseOffset = {double(event.mouseMove.x - lastMousePos.x) / mapLayout->squareWidth(),
      double(event.mouseMove.y - lastMousePos.y) / mapLayout->squareHeight() };
    refreshInput = true;
    return true;
  }
  return false;
}

Vec2 lastGoTo(-1, -1);

void WindowView::processEvents() {
  Event event;
  while (renderer.pollEvent(event)) {
    considerScrollEvent(event);
    considerResizeEvent(event, getAllGuiElems());
    propagateEvent(event, getClickableGuiElems());
    switch (event.type) {
      case Event::TextEntered:
        /*if (event.text.unicode < 128) {
          char key = event.text.unicode;
          for (int i : All(techHotkeys))
            if (key == techHotkeys[i])
              return UserInput(UserInput::TECHNOLOGY, i);
        }*/
        break;
      case Event::KeyPressed:
        keyboardAction(event.key);
        renderer.flushEvents(Event::KeyPressed);
        break;
      case Event::MouseWheelMoved:
          zoom(event.mouseWheel.delta < 0);
          break;
      case Event::MouseButtonPressed :
          if (event.mouseButton.button == sf::Mouse::Middle)
            inputQueue.push(UserInput(UserInput::DRAW_LEVEL_MAP));
          break;
      case Event::MouseButtonReleased :
          if (event.mouseButton.button == sf::Mouse::Left)
            inputQueue.push(UserInput(UserInput::BUTTON_RELEASE, Vec2(0, 0), {activeBuilding}));
          break;
      default: break;
    }
  }
}

void WindowView::propagateEvent(const Event& event, vector<GuiElem*> guiElems) {
  CHECK(std::this_thread::get_id() == renderThreadId);
  if (gameReady)
    mapGui->resetHint();
  switch (event.type) {
    case Event::MouseButtonReleased :
      for (GuiElem* elem : guiElems)
        elem->onMouseRelease();
      break;
    case Event::MouseMoved:
      for (GuiElem* elem : guiElems)
        elem->onMouseMove(Vec2(event.mouseMove.x, event.mouseMove.y));
      break;
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

UserInput::Type getDirActionId(const Event::KeyEvent& key) {
  if (key.control)
    return UserInput::TRAVEL;
  if (key.alt)
    return UserInput::FIRE;
  else
    return UserInput::MOVE;
}

const double shiftScroll = 10;
const double normalScroll = 2.5;

void WindowView::keyboardAction(Event::KeyEvent key) {
  if (lockKeyboard)
    return;
  lockKeyboard = true;
  switch (key.code) {
#ifndef RELEASE
    case Keyboard::F8: renderer.startMonkey(); break;
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
      inputQueue.push(UserInput(UserInput::WAIT));
      break;
    case Keyboard::Escape:
      if (!renderer.isMonkey())
        inputQueue.push(UserInput(UserInput::EXIT));
      break;
    case Keyboard::Up:
    case Keyboard::Numpad8:
      center.y -= key.shift ? shiftScroll : normalScroll;
      inputQueue.push(UserInput(getDirActionId(key), Vec2(0, -1)));
      break;
    case Keyboard::Numpad9:
      center.y -= key.shift ? shiftScroll : normalScroll;
      center.x += key.shift ? shiftScroll : normalScroll;
      inputQueue.push(UserInput(getDirActionId(key), Vec2(1, -1)));
      break;
    case Keyboard::Right: 
    case Keyboard::Numpad6:
      center.x += key.shift ? shiftScroll : normalScroll;
      inputQueue.push(UserInput(getDirActionId(key), Vec2(1, 0)));
      break;
    case Keyboard::Numpad3:
      center.x += key.shift ? shiftScroll : normalScroll;
      center.y += key.shift ? shiftScroll : normalScroll;
      inputQueue.push(UserInput(getDirActionId(key), Vec2(1, 1)));
      break;
    case Keyboard::Down:
    case Keyboard::Numpad2:
      center.y += key.shift ? shiftScroll : normalScroll;
      inputQueue.push(UserInput(getDirActionId(key), Vec2(0, 1))); break;
    case Keyboard::Numpad1:
      center.x -= key.shift ? shiftScroll : normalScroll;
      center.y += key.shift ? shiftScroll : normalScroll;
      inputQueue.push(UserInput(getDirActionId(key), Vec2(-1, 1))); break;
    case Keyboard::Left:
    case Keyboard::Numpad4:
      center.x -= key.shift ? shiftScroll : normalScroll;
      inputQueue.push(UserInput(getDirActionId(key), Vec2(-1, 0))); break;
    case Keyboard::Numpad7:
      center.x -= key.shift ? shiftScroll : normalScroll;
      center.y -= key.shift ? shiftScroll : normalScroll;     
      inputQueue.push(UserInput(getDirActionId(key), Vec2(-1, -1))); break;
    case Keyboard::Return:
    case Keyboard::Numpad5: inputQueue.push(UserInput(UserInput::PICK_UP)); break;
    case Keyboard::I: inputQueue.push(UserInput(UserInput::SHOW_INVENTORY)); break;
    case Keyboard::D: inputQueue.push(UserInput(key.shift ? UserInput::EXT_DROP : UserInput::DROP)); break;
    case Keyboard::A: inputQueue.push(UserInput(UserInput::APPLY_ITEM)); break;
    case Keyboard::E: inputQueue.push(UserInput(UserInput::EQUIPMENT)); break;
    case Keyboard::T: inputQueue.push(UserInput(UserInput::THROW)); break;
    case Keyboard::M: inputQueue.push(UserInput(UserInput::SHOW_HISTORY)); break;
    case Keyboard::H: inputQueue.push(UserInput(UserInput::HIDE)); break;
    case Keyboard::P:
      if (key.shift)
        inputQueue.push(UserInput(UserInput::EXT_PICK_UP));
      else
        inputQueue.push(UserInput(UserInput::PAY_DEBT));
      break;
    case Keyboard::C: inputQueue.push(UserInput(key.shift ? UserInput::CONSUME : UserInput::CHAT)); break;
    case Keyboard::U: inputQueue.push(UserInput(UserInput::UNPOSSESS)); break;
    case Keyboard::S: inputQueue.push(UserInput(UserInput::CAST_SPELL)); break;
    default: break;
  }
}

UserInput WindowView::getAction() {
  lockKeyboard = false;
  if (refreshInput) {
    refreshInput = false;
    return UserInput::REFRESH;
  }
  if (auto input = inputQueue.popAsync())
    return *input;
  else
    return UserInput(UserInput::IDLE);
  return UserInput(UserInput::IDLE);
}

vector<KeyInfo> keyInfo {
//  { "I", "Inventory", {Keyboard::I}},
//  { "E", "Manage equipment", {Keyboard::E}},
  { "Enter", "Pick up items or interact with square", {Keyboard::Return}},
  { "D", "Drop item", {Keyboard::D}},
  { "Shift + D", "Extended drop - choose the number of items", {Keyboard::D, false, false, true}},
  { "Shift + P", "Extended pick up - choose the number of items", {Keyboard::P, false, false, true}},
  { "A", "Apply item", {Keyboard::A}},
  { "T", "Throw item", {Keyboard::T}},
//  { "S", "Cast spell", {Keyboard::S}},
  { "M", "Show message history", {Keyboard::M}},
  { "C", "Chat with someone", {Keyboard::C}},
  { "H", "Hide", {Keyboard::H}},
  { "P", "Pay debt", {Keyboard::P}},
  //{ "U", "Unpossess", {Keyboard::U}},
  { "Space", "Wait", {Keyboard::Space}},
  //{ "Z", "Zoom in/out", {Keyboard::Z}},
  { "Shift + Z", "World map", {Keyboard::Z, false, false, true}},
  { "F2", "Change settings", {Keyboard::F2}},
  { "Escape", "Quit", {Keyboard::Escape}},
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

