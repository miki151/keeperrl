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
#include "creature.h"
#include "level.h"
#include "options.h"
#include "location.h"
#include "window_renderer.h"
#include "tile.h"

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


using namespace colors;


View* View::createLoggingView(ofstream& of) {
  return new LoggingView<WindowView>(of);
}

View* View::createReplayView(ifstream& ifs) {
  return new ReplayView<WindowView>(ifs);
}

class Clock {
  public:
  
  int getMillis() {
    if (lastPause > -1)
      return lastPause - pausedTime;
    else
      return clock.getElapsedTime().asMilliseconds() - pausedTime;
  }

  void setMillis(int time) {
    if (lastPause > -1)
      pausedTime = lastPause - time;
    else
      pausedTime = clock.getElapsedTime().asMilliseconds() - time;
  }

  void pause() {
    if (lastPause < 0)
      lastPause = clock.getElapsedTime().asMilliseconds();
  }

  void cont() {
    if (lastPause > -1) {
      pausedTime += clock.getElapsedTime().asMilliseconds() - lastPause;
      lastPause = -1;
    }
  }

  bool isPaused() {
    return lastPause > -1;
  }

  private:
  int pausedTime = 0;
  int lastPause = -1;
  sf::Clock clock;
};

static Clock myClock;

class TempClockPause {
  public:
  TempClockPause() {
    if (!myClock.isPaused()) {
      myClock.pause();
      cont = true;
    }
  }

  ~TempClockPause() {
    if (cont)
      myClock.cont();
  }

  private:
  bool cont = false;
};

int rightBarWidthCollective = 330;
int rightBarWidthPlayer = 220;
int rightBarWidth = rightBarWidthCollective;
int bottomBarHeight = 75;

WindowRenderer renderer;

Rectangle WindowView::getMapViewBounds() const {
  return Rectangle(0, 0, renderer.getWidth() - rightBarWidth, renderer.getHeight() - bottomBarHeight);
}

WindowView::WindowView() : objects(Level::maxLevelBounds.getW(), Level::maxLevelBounds.getH()) {}

bool tilesOk = true;

void WindowView::initialize() {
  renderer.initialize(1024, 600, "KeeperRL");
  Image tileImage;
  tilesOk &= tileImage.loadFromFile("tiles_int.png");
  Image tileImage2;
  tilesOk &= tileImage2.loadFromFile("tiles2_int.png");
  Image tileImage3;
  tilesOk &= tileImage3.loadFromFile("tiles3_int.png");
  Image tileImage4;
  tilesOk &= tileImage4.loadFromFile("tiles4_int.png");
  Image tileImage5;
  tilesOk &= tileImage5.loadFromFile("tiles5_int.png");
  Image tileImage6;
  tilesOk &= tileImage6.loadFromFile("tiles6_int.png");
  Image tileImage7;
  tilesOk &= tileImage7.loadFromFile("tiles7_int.png");
  if (tilesOk) {
    Renderer::tiles.resize(7);
    Renderer::tiles[0].loadFromImage(tileImage);
    Renderer::tiles[1].loadFromImage(tileImage2);
    Renderer::tiles[2].loadFromImage(tileImage3);
    Renderer::tiles[3].loadFromImage(tileImage4);
    Renderer::tiles[4].loadFromImage(tileImage5);
    Renderer::tiles[5].loadFromImage(tileImage6);
    Renderer::tiles[6].loadFromImage(tileImage7);
  }
  asciiLayouts = {
    MapLayout(16, 20, allLayers),
    MapLayout(8, 10,
        {ViewLayer::FLOOR_BACKGROUND, ViewLayer::FLOOR, ViewLayer::LARGE_ITEM, ViewLayer::CREATURE}), false};
  spriteLayouts = {
    MapLayout(36, 36, allLayers),
    MapLayout(18, 18, allLayers), true};
  if (tilesOk)
    currentTileLayout = spriteLayouts;
  else
    currentTileLayout = asciiLayouts;
  mapGui = new MapGui(objects, [this](Vec2 pos) {
      chosenCreature = "";
      switch (gameInfo.infoType) {
        case GameInfo::InfoType::PLAYER: inputQueue.push(UserInput(UserInput::MOVE_TO, pos)); break;
        case GameInfo::InfoType::BAND:
          if (!contains({CollectiveOption::WORKSHOP, CollectiveOption::BUILDINGS, CollectiveOption::TECHNOLOGY},
              collectiveOption))
            inputQueue.push(UserInput(UserInput::POSSESS, pos));
          if (collectiveOption == CollectiveOption::WORKSHOP && activeWorkshop >= 0)
            inputQueue.push(UserInput(UserInput::WORKSHOP, pos, activeWorkshop));
          if (collectiveOption == CollectiveOption::TECHNOLOGY && activeLibrary >= 0)
            inputQueue.push(UserInput(UserInput::LIBRARY, pos, activeLibrary));
          if (collectiveOption == CollectiveOption::BUILDINGS) {
            if (Keyboard::isKeyPressed(Keyboard::LShift))
              inputQueue.push(UserInput(UserInput::RECT_SELECTION, pos, activeBuilding));
            else
              inputQueue.push(UserInput(UserInput::BUILD, pos, activeBuilding));
          }
          break;
      }});
  mapGui->setBounds(getMapViewBounds());
  MinimapGui::initialize();
  minimapGui = new MinimapGui([this]() { inputQueue.push(UserInput(UserInput::DRAW_LEVEL_MAP)); });
  Rectangle minimapBounds(20, 70, 100, 150);
  minimapGui->setBounds(minimapBounds);
  mapDecoration = GuiElem::border2(GuiElem::empty());
  mapDecoration->setBounds(getMapViewBounds().minusMargin(-6));
  minimapDecoration = GuiElem::border(GuiElem::empty());
  minimapDecoration->setBounds(minimapBounds.minusMargin(-6));
}

void WindowView::reset() {
  mapLayout = &currentTileLayout.normalLayout;
  center = {0, 0};
  gameReady = false;
  clearMessageBox();
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
 /* renderer.drawFilledRectangle(0, 0, 1000, 36, white);
  for (ViewId id : randomPermutation({ViewId::KEEPER, ViewId::IMP, ViewId::GOBLIN, ViewId::BILE_DEMON, ViewId::SPECIAL_HUMANOID, ViewId::STONE_GOLEM, ViewId::IRON_GOLEM, ViewId::LAVA_GOLEM, ViewId::SKELETON, ViewId::ZOMBIE, ViewId::VAMPIRE, ViewId::VAMPIRE_LORD, ViewId::RAVEN, ViewId::WOLF, ViewId::BEAR, ViewId::SPECIAL_BEAST})) {
  for (ViewId id : randomPermutation({ViewId::AVATAR, ViewId::KNIGHT, ViewId::ARCHER, ViewId::SHAMAN, ViewId::WARRIOR, ViewId::LIZARDMAN, ViewId::LIZARDLORD, ViewId::ELF_LORD, ViewId::ELF_ARCHER, ViewId::DWARF, ViewId::DWARF_BARON, ViewId::GOAT, ViewId::PIG, ViewId::COW, ViewId::CHILD, ViewId::WITCH})) {
    ViewObject obj(id, ViewLayer::CREATURE, "wpfok");
    Tile tile = Tile::getTile(obj, true);
    int sz = Renderer::tileSize[tile.getTexNum()];
    int of = (Renderer::nominalSize - sz) / 2;
    x += 24;
    Vec2 coord = tile.getSpriteCoord();
    renderer.drawSprite(x - sz / 2, y + of, coord.x * sz, coord.y * sz, sz, sz, Renderer::tiles[tile.getTexNum()], sz * 2 / 3, sz * 2 /3);
  }*/
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
 // drawText(white, renderer.getWidth() / 2, renderer.getHeight() - 60, "Loading...", true);
 // drawAndClearBuffer();
}

void WindowView::displaySplash(View::SplashType type, bool& ready) {
  string text;
  switch (type) {
    case View::CREATING: text = "Creating a new world, just for you..."; break;
    case View::LOADING: text = "Loading the game..."; break;
    case View::SAVING: text = "Saving the game..."; break;
  }
  Image splash;
  CHECK(splash.loadFromFile(splashPaths[Random.getRandom(1, splashPaths.size())]));
  while (!ready) {
    renderer.drawImage((renderer.getWidth() - splash.getSize().x) / 2, (renderer.getHeight() - splash.getSize().y) / 2, splash);
    renderer.drawText(white, renderer.getWidth() / 2, renderer.getHeight() - 60, text, true);
    renderer.drawAndClearBuffer();
    sf::sleep(sf::milliseconds(30));
    Event event;
    while (renderer.pollEvent(event)) {
      if (event.type == Event::Resized) {
        resize(event.size.width, event.size.height, {});
      }
    }
  }
}

void WindowView::resize(int width, int height, vector<GuiElem*> gui) {
  renderer.resize(width, height);
  refreshScreen(false);
  if (gameReady) {
    rebuildGui();
  }
}

struct KeyInfo {
  string keyDesc;
  string action;
  Event::KeyEvent event;
};

void WindowView::close() {
}

int fireVar = 50;

Color WindowView::getFireColor() {
  return Color(200 + Random.getRandom(-fireVar, fireVar), Random.getRandom(fireVar), Random.getRandom(fireVar), 150);
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

PGuiElem WindowView::drawBottomPlayerInfo(GameInfo::PlayerInfo& info) {
  string title = info.title;
  if (!info.adjectives.empty() || !info.playerName.empty())
    title = " " + title;
  for (int i : All(info.adjectives))
    title = string(i <info.adjectives.size() - 1 ? ", " : " ") + info.adjectives[i] + title;
  PGuiElem top = GuiElem::label(capitalFirst(!info.playerName.empty() ? info.playerName + " the" + title : title) +
    "          T: " + convertToString<int>(info.time) + "            " + info.levelName, white);
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
          GuiElem::label(text, lightBlue),
          GuiElem::button([this, key]() { keyboardAction(key);})));
    bottomWidths.push_back(renderer.getTextLength(text));
  }
  int keySpacing = 50;
  return GuiElem::verticalList(makeVec<PGuiElem>(std::move(top),
        GuiElem::horizontalList(std::move(bottomLine), bottomWidths, keySpacing)), 28, 0);
}

PGuiElem WindowView::drawRightPlayerInfo(GameInfo::PlayerInfo& info) {
  return drawPlayerStats(info);
/*  vector<PGuiElem> buttons = makeVec<PGuiElem>(
    GuiElem::label(0x1f718, legendOption == LegendOption::STATS ? green : white, 35, Renderer::SYMBOL_FONT),
    GuiElem::label(L'i', legendOption == LegendOption::OBJECTS ? green : white, 35, Renderer::TEXT_FONT));
  for (int i : All(buttons))
    buttons[i] = GuiElem::stack(std::move(buttons[i]),
        GuiElem::button([this, i]() { legendOption = LegendOption(i); }));
  PGuiElem main;
  switch (legendOption) {
    case LegendOption::OBJECTS:
    case LegendOption::STATS: main = drawPlayerStats(info); break;
  }
  return GuiElem::margin(GuiElem::horizontalList(std::move(buttons), 40, 7), std::move(main), 80, GuiElem::TOP);*/
}

const int legendLineHeight = 30;
const int legendStartHeight = 70;

static Color getBonusColor(int bonus) {
  switch (bonus) {
    case -1: return red;
    case 0: return white;
    case 1: return green;
  }
  FAIL << "Bad bonus number" << bonus;
  return white;
}

PGuiElem WindowView::drawPlayerStats(GameInfo::PlayerInfo& info) {
  vector<string> lines {
      "",
      "Attack: ",
      "Defense: ",
      "Strength: ",
      "Dexterity: ",
      "Speed: ",
      "Gold:",
  };
  vector<pair<string, Color>> lines2 {
    {"", white},
    {"", white},
    {convertToString(info.attack), getBonusColor(info.attBonus)},
    {convertToString(info.defense), getBonusColor(info.defBonus)},
    {convertToString(info.strength), getBonusColor(info.strBonus)},
    {convertToString(info.dexterity), getBonusColor(info.dexBonus)},
    {convertToString(info.speed), getBonusColor(info.speedBonus)},
    {"$" + convertToString(info.numGold), white},
  };
  if (info.weaponName != "") {
    lines = concat({"wielding:", info.weaponName}, lines);
    lines2 = concat({{"", white}}, lines2);
  } else lines = concat({"barehanded"}, lines);
  vector<PGuiElem> elems;
  for (int i : All(lines)) {
    elems.push_back(GuiElem::horizontalList(makeVec<PGuiElem>(
        GuiElem::label(lines[i], white),
        GuiElem::label(lines2[i].first, lines2[i].second)), 100, 1));
  }
  elems.push_back(GuiElem::empty());
  for (auto effect : info.effects)
    elems.push_back(GuiElem::label(effect.name, effect.bad ? red : green));
  return GuiElem::verticalList(std::move(elems), legendLineHeight, 0);
}

string getPlural(const string& a, const string&b, int num) {
  if (num == 1)
    return "1 " + a;
  else
    return convertToString(num) + " " + b;
}

struct CreatureMapElem {
  ViewObject object;
  int count;
  const Creature* any;
};

static map<string, CreatureMapElem> getCreatureMap(vector<const Creature*> creatures) {
  map<string, CreatureMapElem> creatureMap;
  for (int i : All(creatures)) {
    auto elem = creatures[i];
    if (!creatureMap.count(elem->getSpeciesName())) {
      creatureMap.insert(make_pair(elem->getSpeciesName(), CreatureMapElem({elem->getViewObject(), 1, elem})));
    } else
      ++creatureMap[elem->getSpeciesName()].count;
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
  list.push_back(GuiElem::label(info.monsterHeader, white));
  for (auto elem : creatureMap){
    vector<PGuiElem> line;
    vector<int> widths;
    line.push_back(GuiElem::viewObject(elem.second.object));
    widths.push_back(40);
    Color col = (elem.first == chosenCreature 
        || (elem.second.count == 1 && contains(info.team, elem.second.any))) ? green : white;
    line.push_back(GuiElem::label(convertToString(elem.second.count) + "   " + elem.first, col));
    widths.push_back(200);
    function<void()> action;
    if (elem.second.count == 1)
      action = [this, elem]() {
        inputQueue.push(UserInput(UserInput::CREATURE_BUTTON, elem.second.any->getUniqueId()));
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
    list.push_back(GuiElem::label("Team: " + getPlural("monster", "monsters", info.team.size()), white));
    if (!info.gatheringTeam) {
      vector<PGuiElem> line;
      vector<int> widths;
      line.push_back(GuiElem::stack(GuiElem::button(getButtonCallback(UserInput::GATHER_TEAM)),
          GuiElem::label("[command]", white)));
      widths.push_back(10 + renderer.getTextLength("[command]"));
      line.push_back(GuiElem::stack(GuiElem::button(getButtonCallback(UserInput::EDIT_TEAM)),
          GuiElem::label("[edit]", white)));
      widths.push_back(10 + renderer.getTextLength("[edit]"));
      line.push_back(GuiElem::stack(GuiElem::button(getButtonCallback(UserInput::CANCEL_TEAM)),
          GuiElem::label("[cancel]", white)));
      widths.push_back(100);
      list.push_back(GuiElem::horizontalList(std::move(line), widths, 0));
    }
    
  }
  if ((info.creatures.size() > 1 && info.team.empty()) || info.gatheringTeam) {
    vector<PGuiElem> line;
    line.push_back(GuiElem::stack(GuiElem::button(getButtonCallback(UserInput::GATHER_TEAM)),
        GuiElem::label(info.team.empty() ? "[form team]" : "[done]",
            (info.gatheringTeam && info.team.empty()) ? green : white)));
    if (info.gatheringTeam) {
      line.push_back(GuiElem::stack(GuiElem::button(getButtonCallback(UserInput::CANCEL_TEAM)),
          GuiElem::label("[cancel]", white)));
    }
    list.push_back(GuiElem::horizontalList(std::move(line), 150, 0));
  }
  list.push_back(GuiElem::label("Click on minion to possess.", lightBlue));
  list.push_back(GuiElem::empty());
  if (!enemyMap.empty()) {
    list.push_back(GuiElem::label("Enemies:", white));
    for (auto elem : enemyMap){
      vector<PGuiElem> line;
      line.push_back(GuiElem::viewObject(elem.second.object));
      line.push_back(GuiElem::label(convertToString(elem.second.count) + "   " + elem.first, white));
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
    vector<const Creature*> chosen;
    for (const Creature* c : info.creatures)
      if (c->getSpeciesName() == chosenCreature)
        chosen.push_back(c);
    lines.push_back(GuiElem::label(info.gatheringTeam ? "Click to add to team:" : "Click to control:", lightBlue));
    for (const Creature* c : chosen) {
      string text = "L: " + convertToString(c->getExpLevel()) + "    " + info.tasks[c->getUniqueId()];
      if (c->getSpeciesName() != c->getName())
        text = c->getName() + " " + text;
      vector<PGuiElem> line;
      line.push_back(GuiElem::viewObject(c->getViewObject()));
      line.push_back(GuiElem::label(text, (info.gatheringTeam && contains(info.team, c)) ? green : white));
      lines.push_back(GuiElem::stack(GuiElem::button(
              getButtonCallback(UserInput(UserInput::CREATURE_BUTTON, c->getUniqueId()))),
            GuiElem::horizontalList(std::move(line), 40, 0)));
    }
    return GuiElem::verticalList(std::move(lines), legendLineHeight, 0);
  } else
    return PGuiElem();
}


PGuiElem WindowView::drawVillages(GameInfo::VillageInfo& info) {
  vector<PGuiElem> lines;
  for (auto elem : info.villages) {
    lines.push_back(GuiElem::label(capitalFirst(elem.name), white));
    lines.push_back(GuiElem::margins(GuiElem::label("tribe: " + elem.tribeName, white), 40, 0, 0, 0));
    if (!elem.state.empty())
      lines.push_back(GuiElem::margins(GuiElem::label(elem.state, elem.state == "conquered" ? green : red),
            40, 0, 0, 0));
  }
  return GuiElem::verticalList(std::move(lines), legendLineHeight, 0);
}

vector<PGuiElem> WindowView::drawButtons(vector<GameInfo::BandInfo::Button> buttons, int& active,
    CollectiveOption option) {
  vector<PGuiElem> elems;
  for (int i : All(buttons)) {
    vector<PGuiElem> line;
    line.push_back(GuiElem::viewObject(buttons[i].object));
    vector<int> widths { 35 };
    Color color = white;
    if (i == active)
      color = green;
    else if (!buttons[i].inactiveReason.empty())
      color = lightGray;
    line.push_back(GuiElem::label(buttons[i].name + " " + buttons[i].count, color, buttons[i].hotkey));
    widths.push_back(100);
    if (buttons[i].cost) {
      string costText = convertToString(buttons[i].cost->second);
      line.push_back(GuiElem::label(costText, color));
      widths.push_back(renderer.getTextLength(costText) + 8);
      line.push_back(GuiElem::viewObject(buttons[i].cost->first));
      widths.push_back(25);
    }
    function<void()> buttonFun;
    if (buttons[i].inactiveReason == "" || buttons[i].inactiveReason == "inactive")
      buttonFun = [this, &active, i, option] () {
        active = i;
        collectiveOption = option;
      };
    else {
      string s = buttons[i].inactiveReason;
      buttonFun = [this, s] () {
        presentText("", s);
      };
    }
    elems.push_back(GuiElem::stack(
          GuiElem::button(buttonFun, buttons[i].hotkey),
          GuiElem::horizontalList(std::move(line), widths, 0, buttons[i].cost ? 2 : 0)));
  }
  return elems;
}
  
PGuiElem WindowView::drawBuildings(GameInfo::BandInfo& info) {
  return GuiElem::verticalList(drawButtons(info.buildings, activeBuilding, CollectiveOption::BUILDINGS),
      legendLineHeight, 0);
}

PGuiElem WindowView::drawWorkshop(GameInfo::BandInfo& info) {
  return GuiElem::verticalList(drawButtons(info.workshop, activeWorkshop, CollectiveOption::WORKSHOP),
      legendLineHeight, 0);
}

PGuiElem WindowView::drawTechnology(GameInfo::BandInfo& info) {
  vector<PGuiElem> lines = drawButtons(info.libraryButtons, activeLibrary, CollectiveOption::TECHNOLOGY);
  for (int i : All(info.techButtons)) {
    vector<PGuiElem> line;
    line.push_back(GuiElem::viewObject(ViewObject(info.techButtons[i].viewId, ViewLayer::CREATURE, "")));
    line.push_back(GuiElem::label(info.techButtons[i].name, white, info.techButtons[i].hotkey));
    lines.push_back(GuiElem::stack(
          GuiElem::button(getButtonCallback(UserInput(UserInput::TECHNOLOGY, i)), info.techButtons[i].hotkey),
          GuiElem::horizontalList(std::move(line), 35, 0)));
  }
  return GuiElem::verticalList(std::move(lines), legendLineHeight, 0);
}

PGuiElem WindowView::drawKeeperHelp() {
  vector<string> helpText { "use mouse to dig and build", "shift selects rectangles", "", "scroll with arrows", "or right mouse button", "", "click on minion", "to possess",
    "", "your enemies ", "are in the west", "", "[space]  pause", "[z]  zoom", "", "follow the red hints :-)"};
  vector<PGuiElem> lines;
  for (string line : helpText)
    lines.push_back(GuiElem::label(line, lightBlue));
  return GuiElem::verticalList(std::move(lines), legendLineHeight, 0);
}

PGuiElem WindowView::drawBottomBandInfo(GameInfo::BandInfo& info) {
  vector<PGuiElem> topLine;
  vector<int> topWidths;
  int resourceSpacing = 95;
  for (int i : All(info.numGold)) {
    vector<PGuiElem> res;
    res.push_back(GuiElem::viewObject(info.numGold[i].viewObject));
    res.push_back(GuiElem::label(convertToString<int>(info.numGold[i].count), white));
    topWidths.push_back(resourceSpacing);
    topLine.push_back(GuiElem::stack(GuiElem::mouseOverAction(getHintCallback(info.numGold[i].name)),
            GuiElem::horizontalList(std::move(res), 30, 0)));
  }
  topLine.push_back(GuiElem::label("T:" + convertToString<int>(info.time), white));
  topWidths.push_back(50);
  vector<PGuiElem> bottomLine;
  if (myClock.isPaused())
    bottomLine.push_back(GuiElem::stack(GuiElem::button([&]() { myClock.cont(); }),
        GuiElem::label("PAUSED", red)));
  else
    bottomLine.push_back(GuiElem::stack(GuiElem::button([&]() { myClock.pause(); }),
        GuiElem::label("PAUSE", lightBlue)));
  bottomLine.push_back(GuiElem::stack(GuiElem::button([&]() { unzoom(); }),
        GuiElem::label("ZOOM", lightBlue)));
  bottomLine.push_back(GuiElem::label(info.warning, red));
  return GuiElem::verticalList(makeVec<PGuiElem>(GuiElem::horizontalList(std::move(topLine), topWidths, 0),
        GuiElem::horizontalList(std::move(bottomLine), 85, 0)), 28, 0);
}

PGuiElem WindowView::drawRightBandInfo(GameInfo::BandInfo& info, GameInfo::VillageInfo& villageInfo) {
  vector<PGuiElem> buttons = makeVec<PGuiElem>(
    GuiElem::icon(GuiElem::BUILDING),
    GuiElem::icon(GuiElem::MINION),
    GuiElem::icon(GuiElem::LIBRARY),
    GuiElem::icon(GuiElem::WORKSHOP),
    GuiElem::icon(GuiElem::DIPLOMACY),
    GuiElem::icon(GuiElem::HELP));
  for (int i : All(buttons)) {
    if (int(collectiveOption) == i)
      buttons[i] = GuiElem::border2(std::move(buttons[i]));
    else
      buttons[i] = GuiElem::margins(std::move(buttons[i]), 6, 6, 6, 6);
    buttons[i] = GuiElem::stack(std::move(buttons[i]),
        GuiElem::button([this, i]() { collectiveOption = CollectiveOption(i); }));
  }
  if (collectiveOption != CollectiveOption::MINIONS)
    chosenCreature = "";
  PGuiElem main;
  vector<PGuiElem> invisible;
  vector<pair<CollectiveOption, PGuiElem>> elems = makeVec<pair<CollectiveOption, PGuiElem>>(
    make_pair(CollectiveOption::MINIONS, drawMinions(info)),
    make_pair(CollectiveOption::BUILDINGS, drawBuildings(info)),
    make_pair(CollectiveOption::KEY_MAPPING, drawKeeperHelp()),
    make_pair(CollectiveOption::TECHNOLOGY, drawTechnology(info)),
    make_pair(CollectiveOption::WORKSHOP, drawWorkshop(info)),
    make_pair(CollectiveOption::VILLAGES, drawVillages(villageInfo)));
  for (auto& elem : elems)
    if (elem.first == collectiveOption)
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
        bottom = drawBottomPlayerInfo(gameInfo.playerInfo);
        rightBarWidth = rightBarWidthPlayer;
        break;
    case GameInfo::InfoType::BAND:
        right = drawRightBandInfo(gameInfo.bandInfo, gameInfo.villageInfo);
        overMap = drawMinionWindow(gameInfo.bandInfo);
        bottom = drawBottomBandInfo(gameInfo.bandInfo);
        rightBarWidth = rightBarWidthCollective;
        break;
  }
  mapDecoration->setBounds(getMapViewBounds().minusMargin(-6));
  mapGui->setBounds(getMapViewBounds());
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
  vector<GuiElem*> ret = extractRefs(tempGuiElems);
  if (gameReady)
    ret = concat(concat({mapGui}, ret), {mapDecoration.get(), minimapDecoration.get(), minimapGui});
  return ret;
}

vector<GuiElem*> WindowView::getClickableGuiElems() {
  vector<GuiElem*> ret = extractRefs(tempGuiElems);
  ret.push_back(minimapGui);
  ret.push_back(mapGui);
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
        transparency(black, 100));
  for (int i : All(currentMessage))
    renderer.drawText(oldMessage ? gray : white, 10, 5 + lineHeight * i, currentMessage[i]);
}

void WindowView::resetCenter() {
  center = {0, 0};
}

void WindowView::updateView(const CreatureView* collective) {
  refreshViewInt(collective, false);
}

void WindowView::refreshView(const CreatureView* collective) {
  refreshViewInt(collective);
}

function<void()> WindowView::getHintCallback(const string& s) {
  return [this, s]() { hint = s; };
}

function<void()> WindowView::getButtonCallback(UserInput input) {
  return [this, input]() { inputQueue.push(input); };
}

void WindowView::drawLevelMap(const CreatureView* creature) {
  TempClockPause pause;
  minimapGui->presentMap(creature, getMapViewBounds(), renderer,
      [this](double x, double y) { center = {x, y};});
}

void WindowView::updateMinimap(const CreatureView* creature) {
  const Level* level = creature->getLevel();
  Vec2 rad(40, 40);
  Rectangle bounds(mapLayout->getPlayerPos() - rad, mapLayout->getPlayerPos() + rad);
  if (level->getBounds().intersects(bounds))
    minimapGui->update(level, bounds, creature);
}

void WindowView::refreshViewInt(const CreatureView* collective, bool flipBuffer) {
  updateMinimap(collective);
  gameReady = true;
  switchTiles();
  const Level* level = collective->getLevel();
  collective->refreshGameInfo(gameInfo);
  for (Vec2 pos : mapLayout->getAllTiles(getMapViewBounds(), Level::maxLevelBounds))
    objects[pos] = Nothing();
  if ((center.x == 0 && center.y == 0) || collective->staticPosition())
    center = {double(collective->getPosition().x), double(collective->getPosition().y)};
  Vec2 movePos = Vec2((center.x - mouseOffset.x) * mapLayout->squareWidth(),
      (center.y - mouseOffset.y) * mapLayout->squareHeight());
  movePos.x = max(movePos.x, 0);
  movePos.x = min(movePos.x, int(collective->getLevel()->getBounds().getKX() * mapLayout->squareWidth()));
  movePos.y = max(movePos.y, 0);
  movePos.y = min(movePos.y, int(collective->getLevel()->getBounds().getKY() * mapLayout->squareHeight()));
  mapLayout->updatePlayerPos(movePos);
  const MapMemory* memory = &collective->getMemory(); 
  for (Vec2 pos : mapLayout->getAllTiles(getMapViewBounds(), Level::maxLevelBounds)) 
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
      objects[pos] = index;
    }
  mapGui->setLayout(mapLayout);
  mapGui->setSpriteMode(currentTileLayout.sprites);
  mapGui->updateObjects(memory);
  mapGui->setLevelBounds(level->getBounds());
  rebuildGui();
  refreshScreen(flipBuffer);
}

void WindowView::animateObject(vector<Vec2> trajectory, ViewObject object) {
  for (Vec2 pos : trajectory) {
    if (!objects[pos])
      continue;
    ViewIndex& index = *objects[pos];
    Optional<ViewObject> prev;
    if (index.hasObject(object.layer()))
      prev = index.getObject(object.layer());
    index.removeObject(object.layer());
    index.insert(object);
    refreshScreen();
    // sf::sleep(sf::milliseconds(30));
    index.removeObject(object.layer());
    if (prev)
      index.insert(*prev);
  }
  if (objects[trajectory.back()]) {
    ViewIndex& index = *objects[trajectory.back()];
    if (index.hasObject(object.layer()))
      index.removeObject(object.layer());
    index.insert(object);
  }
}

void WindowView::animation(Vec2 pos, AnimationId id) {
  CHECK(id == AnimationId::EXPLOSION);
  Vec2 wpos = mapLayout->projectOnScreen(getMapViewBounds(), pos);
  refreshScreen(false);
  renderer.drawSprite(wpos.x, wpos.y, 510, 628, 36, 36, Renderer::tiles[6]);
  renderer.drawAndClearBuffer();
  sf::sleep(sf::milliseconds(50));
  refreshScreen(false);
  renderer.drawSprite(wpos.x - 17, wpos.y - 17, 683, 611, 70, 70, Renderer::tiles[6]);
  renderer.drawAndClearBuffer();
  sf::sleep(sf::milliseconds(50));
  refreshScreen(false);
  renderer.drawSprite(wpos.x - 29, wpos.y - 29, 577, 598, 94, 94, Renderer::tiles[6]);
  renderer.drawAndClearBuffer();
  sf::sleep(sf::milliseconds(50));
  refreshScreen(true);
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
  map<string, ViewObject> objIndex;
  for (Vec2 wpos : mapLayout->getAllTiles(getMapViewBounds(), objects.getBounds())) 
    if (objects[wpos]) {
      const ViewIndex& index = *objects[wpos];
      if (auto topObject = index.getTopObject(mapLayout->getLayers()))
        objIndex.insert(std::make_pair(topObject->getDescription(), *topObject));
    }
/*  int rightPos = renderer.getWidth() -rightBarText;
  if (gameInfo.infoType == GameInfo::InfoType::PLAYER) {
    int cnt = 0;
    if (legendOption == LegendOption::OBJECTS) {
      for (auto elem : objIndex) {
        drawViewObject(elem.second, rightPos, legendStartHeight + cnt * 25, currentTileLayout.sprites);
        renderer.drawText(white, rightPos + 30, legendStartHeight + cnt * 25, elem.first);
        ++cnt;
      }
    }
  }*/
  for (GuiElem* gui : getAllGuiElems())
    gui->render(renderer);
  refreshText();
  fpsCounter.addTick();
  renderer.drawText(white, renderer.getWidth() - 70, renderer.getHeight() - 30, "FPS " + convertToString(fpsCounter.getFps()));
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
        Vec2 middle = mapLayout->getAllTiles(getMapViewBounds(), Level::maxLevelBounds).middle();
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
        Vec2 wpos = mapLayout->projectOnScreen(getMapViewBounds(), middle + dir);
        renderer.drawSprite(wpos.x, wpos.y, 16 * 36, (8 + numArrow) * 36, 36, 36, Renderer::tiles[4]);
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
}

bool WindowView::yesOrNoPrompt(const string& message) {
  TempClockPause pause;
  OnExit onExit([&]() { renderer.flushAllEvents(); });
  do {
    showMessage(message + " (y/n)");
    refreshScreen();
    Event event;
    renderer.waitEvent(event);
    if (event.type == Event::KeyPressed)
      switch (event.key.code) {
        case Keyboard::Y: showMessage(""); refreshScreen(); return true;
        case Keyboard::Escape:
 //       case Keyboard::Space:
        case Keyboard::N: showMessage(""); refreshScreen(); return false;
        default: break;
      }
  } while (1);
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
    MenuType type, double* scrollPos, Optional<UserInput::Type> exitAction) {
  return chooseFromList(title, options, index, type, scrollPos, exitAction, Nothing(), {});
}


double getScrollPos(int index, int count) {
  return max(0., min(1., double(max(0, index - 2)) / max(1, (count - 4))));
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

Optional<int> WindowView::chooseFromList(const string& title, const vector<ListElem>& options, int index,
    MenuType menuType, double* scrollPos, Optional<UserInput::Type> exitAction, Optional<Event::KeyEvent> exitKey,
    vector<Event::KeyEvent> shortCuts) {
  if (options.size() == 0)
    return Nothing();
  TempClockPause pause;
  int contentHeight;
  int choice = -1;
  int count = 0;
  vector<int> indexes(options.size());
  int elemCount = 0;
  for (int i : All(options)) {
    if (options[i].getMod() == View::NORMAL) {
      indexes[count] = elemCount;
      ++count;
    }
    if (options[i].getMod() != View::TITLE)
      ++elemCount;
  }
  double localScrollPos = index >= 0 ? getScrollPos(indexes[index], count) : 0;
  if (scrollPos == nullptr)
    scrollPos = &localScrollPos;
  PGuiElem list = drawListGui(title, options, contentHeight, &index, &choice);
  PGuiElem dismissBut = GuiElem::stack(makeVec<PGuiElem>(
        GuiElem::button([&](){ choice = -100; }),
        GuiElem::mouseHighlight(GuiElem::highlight(GuiElem::foreground1), count, &index),
        GuiElem::centerHoriz(GuiElem::label("Dismiss", white), renderer.getTextLength("Dismiss"))));
  double scrollJump = 60. / contentHeight;
  PGuiElem stuff = GuiElem::scrollable(std::move(list), contentHeight, scrollPos, scrollJump);
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
          *scrollPos = min(1., max(0., *scrollPos - double(event.mouseWheel.delta) * scrollJump));
      if (event.type == Event::KeyPressed)
        switch (event.key.code) {
          case Keyboard::Numpad8:
          case Keyboard::Up:
            if (count > 0) {
              index = (index - 1 + count) % count;
              *scrollPos = getScrollPos(indexes[index], count);
            } else
              *scrollPos = max(0., *scrollPos - scrollJump);
            break;
          case Keyboard::Numpad2:
          case Keyboard::Down:
            if (count > 0) {
              index = (index + 1 + count) % count;
              *scrollPos = getScrollPos(indexes[index], count);
            } else
              *scrollPos = min(1., *scrollPos + scrollJump);
            break;
          case Keyboard::Numpad5:
          case Keyboard::Return: if (count > 0 && index > -1) return indexes[index];
          case Keyboard::Escape: return Nothing();
                                 //       case Keyboard::Space : refreshScreen(); return;
          default: break;
        }
    }
  }
 // refreshScreen();
}

void WindowView::presentText(const string& title, const string& text) {
  TempClockPause pause;
  int maxWidth = 80;
  vector<string> rows;
  for (string line : split(text, '\n')) {
    rows.push_back("");
    for (string word : split(line, ' '))
      if (rows.back().size() + word.size() + 1 <= maxWidth)
        rows.back().append((rows.back().size() > 0 ? " " : "") + word);
      else
        rows.push_back(word);
  }
  presentList(title, View::getListElem(rows), false);
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
  double scrollPos = scrollDown ? 1 : 0;
  chooseFromList(title, conv, -1, NORMAL_MENU, &scrollPos, exitAction);
}

PGuiElem WindowView::drawListGui(const string& title, const vector<ListElem>& options, int& height, int* highlight,
    int* choice) {
  vector<PGuiElem> lines;
  if (!title.empty())
    lines.push_back(GuiElem::label(title, white));
  lines.push_back(GuiElem::empty());
  int numActive = 0;
  for (int i : All(options)) {
    Color color;
    switch (options[i].getMod()) {
      case View::TITLE: color = GuiElem::titleText; break;
      case View::INACTIVE: color = GuiElem::inactiveText; break;
      case View::TEXT:
      case View::NORMAL: color = GuiElem::text; break;
    }
    PGuiElem label1 = GuiElem::label(options[i].getText(), color);
    lines.push_back(GuiElem::margins(std::move(label1), 10, 3, 0, 0));
    if (highlight && options[i].getMod() == View::NORMAL) {
      lines.back() = GuiElem::stack(makeVec<PGuiElem>(
            GuiElem::button([=]() { *choice = numActive; }),
            GuiElem::margins(
              GuiElem::mouseHighlight(GuiElem::highlight(GuiElem::foreground1), numActive, highlight), 0, 0, 0, 0),
            std::move(lines.back())));
      ++numActive;
    }
  }
  int lineHeight = 30;
  height = lineHeight * lines.size();
  return GuiElem::verticalList(std::move(lines), 30, 0);
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
    refreshScreen();
}

void WindowView::unzoom() {
  if (mapLayout != &currentTileLayout.normalLayout)
    mapLayout = &currentTileLayout.normalLayout;
  else
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
  Event event;
  return renderer.pollEvent(event, Event::MouseButtonPressed) || renderer.pollEvent(event, Event::KeyPressed);
}

int WindowView::getTimeMilli() {
  return myClock.getMillis();
}

void WindowView::setTimeMilli(int time) {
  myClock.setMillis(time);
}

void WindowView::stopClock() {
  myClock.pause();
}

void WindowView::continueClock() {
  myClock.cont();
}

bool WindowView::isClockStopped() {
  return myClock.isPaused();
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
    return true;
  }
  return false;
}

Vec2 lastGoTo(-1, -1);

void WindowView::propagateEvent(const Event& event, vector<GuiElem*> guiElems) {
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

void WindowView::keyboardAction(Event::KeyEvent key) {
  switch (key.code) {
#ifndef RELEASE
    case Keyboard::F8: renderer.startMonkey(); break;
#endif
    case Keyboard::Z: unzoom(); break;
    case Keyboard::F1:
      if (auto ev = getEventFromMenu())
        keyboardAction(*ev);
      break;
    case Keyboard::F2: Options::handle(this, OptionSet::GENERAL); refreshScreen(); break;
    case Keyboard::Space:
      if (!myClock.isPaused())
        myClock.pause();
      else
        myClock.cont();
      inputQueue.push(UserInput(UserInput::WAIT));
      break;
    case Keyboard::Escape:
      if (!renderer.isMonkey())
        inputQueue.push(UserInput(UserInput::EXIT));
      break;
    case Keyboard::Up:
    case Keyboard::Numpad8: center.y -= 2.5; inputQueue.push(UserInput(getDirActionId(key), Vec2(0, -1))); break;
    case Keyboard::Numpad9: inputQueue.push(UserInput(getDirActionId(key), Vec2(1, -1))); break;
    case Keyboard::Right: 
    case Keyboard::Numpad6: center.x += 2.5; inputQueue.push(UserInput(getDirActionId(key), Vec2(1, 0))); break;
    case Keyboard::Numpad3: inputQueue.push(UserInput(getDirActionId(key), Vec2(1, 1))); break;
    case Keyboard::Down:
    case Keyboard::Numpad2: center.y += 2.5; inputQueue.push(UserInput(getDirActionId(key), Vec2(0, 1))); break;
    case Keyboard::Numpad1: inputQueue.push(UserInput(getDirActionId(key), Vec2(-1, 1))); break;
    case Keyboard::Left:
    case Keyboard::Numpad4: center.x -= 2.5; inputQueue.push(UserInput(getDirActionId(key), Vec2(-1, 0))); break;
    case Keyboard::Numpad7: inputQueue.push(UserInput(getDirActionId(key), Vec2(-1, -1))); break;
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
    case Keyboard::C: inputQueue.push(UserInput(UserInput::CHAT)); break;
    case Keyboard::U: inputQueue.push(UserInput(UserInput::UNPOSSESS)); break;
    case Keyboard::S: inputQueue.push(UserInput(UserInput::CAST_SPELL)); break;
    default: break;
  }
}

UserInput WindowView::getAction() {
  Event event;
  while (renderer.pollEvent(event)) {
    considerScrollEvent(event);
    considerResizeEvent(event, getAllGuiElems());
    propagateEvent(event, getClickableGuiElems());
    UserInput input;
    if (inputQueue.pop(input))
      return input;
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
      case Event::MouseButtonReleased :
          if (event.mouseButton.button == sf::Mouse::Left) {
            return UserInput(UserInput::BUTTON_RELEASE, activeBuilding);
          }
          break;
      default: break;
    }
  }
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
  auto index = chooseFromList("", options, 0, NORMAL_MENU, nullptr, Nothing(),
      Optional<Event::KeyEvent>({Keyboard::F1}), shortCuts);
  if (!index)
    return Nothing();
  return keyInfo[*index].event;
}

