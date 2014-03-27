#include "stdafx.h"

#include "window_view.h"
#include "logging_view.h"
#include "replay_view.h"
#include "creature.h"
#include "level.h"
#include "options.h"
#include "location.h"
#include "renderer.h"
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



Rectangle maxLevelBounds(600, 600);
Image mapBuffer;

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
    if (lastPause == -1)
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


int topBarHeight = 10;
int rightBarWidth = 300;
int rightBarText = rightBarWidth - 30;
int bottomBarHeight = 75;

Renderer renderer;

Rectangle WindowView::getMapViewBounds() const {
  return Rectangle(0, topBarHeight, renderer.getWidth() - rightBarWidth, renderer.getHeight() - bottomBarHeight);
}

Table<Optional<ViewIndex>> objects(maxLevelBounds.getW(), maxLevelBounds.getH());

bool tilesOk = true;

void WindowView::initialize() {
  renderer.initialize(1024, 600, 32, "KeeperRL");

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
  mapGui = new MapGui(getMapViewBounds(), objects);}

void WindowView::reset() {
  mapBuffer.create(maxLevelBounds.getW(), maxLevelBounds.getH());
  mapLayout = &currentTileLayout.normalLayout;
  center = {0, 0};
  gameReady = false;
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
        resize(event.size.width, event.size.height);
      }
    }
  }
}

bool keypressed() {
  Event event;
  while (renderer.pollEvent(event))
    if (event.type == Event::KeyPressed)
      return true;
  return false;
}

void WindowView::resize(int width, int height) {
  renderer.resize(width, height);
  mapGui->setBounds(getMapViewBounds());
}

Optional<Vec2> mousePos;

struct KeyInfo {
  string keyDesc;
  string action;
  Event::KeyEvent event;
};

Rectangle minimapBounds(20, 70, 100, 150);

vector<KeyInfo> bottomKeys;

static bool leftMouseButtonPressed = false;

WindowView::BlockingEvent WindowView::readkey() {
  Event event;
  while (1) {
    renderer.waitEvent(event);
    Debug() << "Event " << event.type;
    bool mouseEv = false;
    while (event.type == Event::MouseMoved && !Mouse::isButtonPressed(Mouse::Right)) {
      mouseEv = true;
      mousePos = Vec2(event.mouseMove.x, event.mouseMove.y);
      if (!renderer.pollEvent(event))
        return { BlockingEvent::MOUSE_MOVE };
    }
    if (event.type == sf::Event::MouseWheelMoved) {
      return { BlockingEvent::MOUSE_WHEEL, Nothing(), event.mouseWheel.delta };
    }
    if (mouseEv && event.type == Event::MouseMoved)
      return { BlockingEvent::MOUSE_MOVE };
    if (event.type == Event::KeyPressed) {
      Event::KeyEvent ret(event.key);
      mousePos = Nothing();
      while (renderer.pollEvent(event));
      return { BlockingEvent::KEY, ret };
    }
    bool scrolled = false;
    while (considerScrollEvent(event)) {
      mousePos = Nothing();
      if (!renderer.pollEvent(event))
        return { BlockingEvent::IDLE };
      scrolled = true;
    }
    if (scrolled)
      return { BlockingEvent::IDLE };
    if (event.type == Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Middle)
      return { BlockingEvent::WHEEL_BUTTON };
    if (event.type == Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
      Vec2 pos(event.mouseButton.x, event.mouseButton.y);
      if (pos.inRectangle(minimapBounds))
        return { BlockingEvent::MINIMAP };
      for (int i : All(optionButtons))
        if (pos.inRectangle(optionButtons[i])) {
          legendOption = LegendOption(i);
          return { BlockingEvent::IDLE };
        }
      for (int i : All(bottomKeyButtons))
        if (pos.inRectangle(bottomKeyButtons[i])) {
          return { BlockingEvent::KEY, bottomKeys[i].event };
        }
      return { BlockingEvent::MOUSE_LEFT };
    }
    if (event.type == Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
      leftMouseButtonPressed = false;
    }
    if (event.type == Event::Resized) {
      resize(event.size.width, event.size.height);
      return { BlockingEvent::IDLE };
    }
    if (event.type == Event::GainedFocus)
      return { BlockingEvent::IDLE };
  }
}

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

void WindowView::drawPlayerInfo() {
  GameInfo::PlayerInfo& info = gameInfo.playerInfo;
  string title = info.title;
  if (!info.adjectives.empty() || !info.playerName.empty())
    title = " " + title;
  for (int i : All(info.adjectives))
    title = string(i <info.adjectives.size() - 1 ? ", " : " ") + info.adjectives[i] + title;
  int line1 = renderer.getHeight() - bottomBarHeight + 10;
  int line2 = line1 + 28;
  renderer.drawFilledRectangle(0, renderer.getHeight() - bottomBarHeight, renderer.getWidth() - rightBarWidth, renderer.getHeight(),
      translucentBlack);
  string playerLine = capitalFirst(!info.playerName.empty() ? info.playerName + " the" + title : title) +
    "          T: " + convertToString<int>(info.time) + "            " + info.levelName;
  renderer.drawText(white, 10, line1, playerLine);
  int keySpacing = 50;
  int startX = 10;
  bottomKeyButtons.clear();
  bottomKeys =  {
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
    int endX = startX + renderer.getTextLength(text) + keySpacing;
    renderer.drawText(lightBlue, startX, line2, text);
    bottomKeyButtons.emplace_back(startX, line2, endX, line2 + 25);
    startX = endX;
  }
  sf::Uint32 optionSyms[] = {0x1f718, L'i'};
  optionButtons.clear();
  for (int i = 0; i < 2; ++i) {
    int w = 45;
    int line = topBarHeight;
    int h = 45;
    int leftPos = renderer.getWidth() - rightBarText + 15;
    renderer.drawText(i < 1 ? Renderer::SYMBOL_FONT : Renderer::TEXT_FONT, 35, i == int(legendOption) ? green : white,
        leftPos + i * w, line, optionSyms[i], true);
    optionButtons.emplace_back(leftPos + i * w - w / 2, line,
        leftPos + (i + 1) * w - w / 2, line + h);
  }
  switch (legendOption) {
    case LegendOption::STATS: drawPlayerStats(info); break;
    case LegendOption::OBJECTS: break;
  }
}

const int legendLineHeight = 30;
const int legendStartHeight = topBarHeight + 70;

static Color getBonusColor(int bonus) {
  switch (bonus) {
    case -1: return red;
    case 0: return white;
    case 1: return green;
  }
  FAIL << "Bad bonus number" << bonus;
  return white;
}

void WindowView::drawPlayerStats(GameInfo::PlayerInfo& info) {
  int lineStart = legendStartHeight;
  int lineX = renderer.getWidth() - rightBarText + 10;
  int line2X = renderer.getWidth() - rightBarText + 110;
  vector<string> lines {
      info.weaponName != "" ? "wielding " + info.weaponName : "barehanded",
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
  for (int i : All(lines)) {
    renderer.drawText(lines2[i].second, lineX, lineStart + legendLineHeight * i, lines[i]);
    renderer.drawText(lines2[i].second, line2X, lineStart + legendLineHeight * i, lines2[i].first);
  }
  int h = lineStart + legendLineHeight * lines.size();
  for (auto effect : info.effects) {
    renderer.drawText(effect.bad ? red : green, lineX, h += legendLineHeight, effect.name);
  }
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
    if (!creatureMap.count(elem->getName())) {
      creatureMap.insert(make_pair(elem->getName(), CreatureMapElem({elem->getViewObject(), 1, elem})));
    } else
      ++creatureMap[elem->getName()].count;
  }
  return creatureMap;
}

static void drawViewObject(const ViewObject& obj, int x, int y, bool sprite) {
    Tile tile = Tile::getTile(obj, sprite);
    if (tile.hasSpriteCoord()) {
      int sz = Renderer::tileSize[tile.getTexNum()];
      int of = (Renderer::nominalSize - sz) / 2;
      Vec2 coord = tile.getSpriteCoord();
      renderer.drawSprite(x - sz / 2, y + of, coord.x * sz, coord.y * sz, sz, sz, Renderer::tiles[tile.getTexNum()], sz * 2 / 3, sz * 2 /3);
    } else
      renderer.drawText(tile.symFont ? Renderer::SYMBOL_FONT : Renderer::TEXT_FONT, 20, Tile::getColor(obj), x, y, tile.text, true);
}

void WindowView::drawMinions(GameInfo::BandInfo& info) {
  map<string, CreatureMapElem> creatureMap = getCreatureMap(info.creatures);
  map<string, CreatureMapElem> enemyMap = getCreatureMap(info.enemies);
  renderer.drawText(white, renderer.getWidth() - rightBarText, legendStartHeight, info.monsterHeader);
  int cnt = 0;
  int lineStart = legendStartHeight + 35;
  int textX = renderer.getWidth() - rightBarText + 10;
  for (auto elem : creatureMap){
    int height = lineStart + cnt * legendLineHeight;
    drawViewObject(elem.second.object, textX, height, currentTileLayout.sprites);
    Color col = (elem.first == chosenCreature) ? green : white;
    renderer.drawText(col, textX + 20, height,
        convertToString(elem.second.count) + "   " + elem.first);
    creatureGroupButtons.emplace_back(textX, height, textX + 150, height + legendLineHeight);
    creatureNames.push_back(elem.first);
    if (elem.second.count == 1)
      uniqueCreatures.push_back(elem.second.any);
    else
      uniqueCreatures.push_back(nullptr);
    ++cnt;
  }
  
  if (info.gatheringTeam && !info.team.empty()) {
    renderer.drawText(white, textX, lineStart + (cnt + 1) * legendLineHeight, 
        getPlural("monster", "monsters", info.team.size()));
    ++cnt;
  }
  if (info.creatures.size() > 1 || info.gatheringTeam) {
    int height = lineStart + (cnt + 1) * legendLineHeight;
    renderer.drawText((info.gatheringTeam && info.team.empty()) ? green : white, textX, height, 
        info.team.empty() ? "[gather team]" : "[command team]");
    int butWidth = 150;
    teamButton = Rectangle(textX, height, textX + butWidth, height + legendLineHeight);
    if (info.gatheringTeam) {
      renderer.drawText(white, textX + butWidth, height, "[disband]");
      cancelTeamButton = Rectangle(textX + butWidth, height, textX + 230, height + legendLineHeight);
    }
    cnt += 2;
  }
  ++cnt;
  renderer.drawText(lightBlue, textX, lineStart + (cnt + 1) * legendLineHeight, "Click on minion to possess.");
  ++cnt;
  ++cnt;
  if (!enemyMap.empty()) {
    renderer.drawText(white, textX, lineStart + (cnt + 1) * legendLineHeight, "Enemies:");
    for (auto elem : enemyMap){
      int height = lineStart + (cnt + 2) * legendLineHeight + 10;
      drawViewObject(elem.second.object, textX, height, currentTileLayout.sprites);
      renderer.drawText(white, textX + 20, height, convertToString(elem.second.count) + "   " + elem.first);
      ++cnt;
    }
  }
  if (chosenCreature != "") {
    if (!creatureMap.count(chosenCreature)) {
      chosenCreature = "";
    } else {
      int width = 220;
      vector<const Creature*> chosen;
      for (const Creature* c : info.creatures)
        if (c->getName() == chosenCreature)
          chosen.push_back(c);
      int winX = renderer.getWidth() - rightBarWidth - width - 20;
      renderer.drawFilledRectangle(winX, lineStart - 15,
          winX + width + 20, legendStartHeight + (chosen.size() + 3) * legendLineHeight, black);
      renderer.drawText(lightBlue, winX + 10, lineStart, 
          info.gatheringTeam ? "Click to add to team:" : "Click to control:");
      int cnt = 1;
      for (const Creature* c : chosen) {
        int height = lineStart + cnt * legendLineHeight;
        drawViewObject(c->getViewObject(), winX + 35, height, currentTileLayout.sprites);
        renderer.drawText(contains(info.team, c) ? green : white, winX + 55, height,
            "level: " + convertToString(c->getExpLevel()) + "    " + info.tasks[c->getUniqueId()]);
        creatureButtons.emplace_back(winX + 20, height, winX + width + 20, height + legendLineHeight);
        chosenCreatures.push_back(c);
        ++cnt;
      }
    }
  }
}

void WindowView::drawVillages(GameInfo::VillageInfo& info) {
  int textX = renderer.getWidth() - rightBarText;
  int height = legendStartHeight;
  for (auto elem : info.villages) {
    renderer.drawText(white, textX, height, capitalFirst(elem.name));
    height += legendLineHeight;
    renderer.drawText(white, textX + 40, height, "tribe: " + elem.tribeName);
    height += legendLineHeight;
    if (!elem.state.empty()) {
      renderer.drawText(elem.state == "conquered" ? green : red, textX + 40, height, elem.state);
      height += legendLineHeight;
    }
  }
}

void WindowView::drawButtons(vector<GameInfo::BandInfo::Button> buttons, int active,
    vector<Rectangle>& viewButtons, vector<string>& inactiveReasons) {
  viewButtons.clear();
  inactiveReasons.clear();
  int textX = renderer.getWidth() - rightBarText;
  for (int i : All(buttons)) {
    int height = legendStartHeight + i * legendLineHeight;
    drawViewObject(buttons[i].object, textX, height, currentTileLayout.sprites);
    Color color = white;
    if (i == active)
      color = green;
    else if (!buttons[i].inactiveReason.empty())
      color = lightGray;
    inactiveReasons.push_back(buttons[i].inactiveReason);
    string text = buttons[i].name + " " + buttons[i].count;
    renderer.drawText(color, textX + 30, height, text);
    if (buttons[i].cost) {
      string costText = convertToString(buttons[i].cost->second);
      int posX = renderer.getWidth() - renderer.getTextLength(costText) - 10;
      drawViewObject(buttons[i].cost->first, renderer.getWidth() - 45, height, currentTileLayout.sprites);
      renderer.drawText(color, posX, height, costText);
    }
    viewButtons.emplace_back(textX, height,textX + 150, height + legendLineHeight);
    if (!buttons[i].help.empty() && mousePos && mousePos->inRectangle(viewButtons.back()))
      mapGui->drawHint(renderer, white, buttons[i].help);
  }
}
  
void WindowView::drawBuildings(GameInfo::BandInfo& info) {
  drawButtons(info.buildings, activeBuilding, buildingButtons, inactiveBuildingReasons);
}

void WindowView::drawTechnology(GameInfo::BandInfo& info) {
  int textX = renderer.getWidth() - rightBarText;
  for (int i : All(info.techButtons)) {
    int height = legendStartHeight + i * legendLineHeight;
    drawViewObject(ViewObject(info.techButtons[i].viewId, ViewLayer::CREATURE, ""),
        textX, height, currentTileLayout.sprites);
    renderer.drawText(white, textX + 20, height, info.techButtons[i].name);
    techButtons.emplace_back(textX, height, textX + 150, height + legendLineHeight);
  }
}

void WindowView::drawWorkshop(GameInfo::BandInfo& info) {
  drawButtons(info.workshop, activeWorkshop, workshopButtons, inactiveWorkshopReasons);
}

void WindowView::drawKeeperHelp() {
  vector<string> helpText { "use mouse to", "dig and build", "", "scroll with arrows", "or right mouse button", "", "click on minion", "to possess",
    "", "your enemies ", "are in the west", "", "[space]  pause", "[z]  zoom", "", "follow the red hints :-)"};
  int cnt = 0;
  for (string line : helpText) {
    int height = legendStartHeight + cnt * legendLineHeight;
    renderer.drawText(lightBlue, renderer.getWidth() - rightBarText, height, line);
    cnt ++;
  }
}

void WindowView::drawBandInfo() {
  GameInfo::BandInfo& info = gameInfo.bandInfo;
  int lineHeight = 28;
  int line0 = renderer.getHeight() - 90;
  int line1 = line0 + lineHeight;
  int line2 = line1 + lineHeight;
  renderer.drawFilledRectangle(0, line1 - 10, renderer.getWidth() - rightBarWidth, renderer.getHeight(), translucentBlack);
  string playerLine = "T:" + convertToString<int>(info.time);
  renderer.drawText(white, renderer.getWidth() - rightBarWidth - 60, line1, playerLine);
  renderer.drawText(red, 120, line2, info.warning);
  if (myClock.isPaused())
    renderer.drawText(red, 10, line2, "PAUSED");
  else
    renderer.drawText(lightBlue, 10, line2, "PAUSE");
  pauseButton = Rectangle(10, line2, 80, line2 + lineHeight);
  string resources;
  int resourceSpacing = 95;
  int resourceX = 45;
  for (int i : All(info.numGold)) {
    renderer.drawText(white, resourceX + resourceSpacing * i, line1, convertToString<int>(info.numGold[i].count));
    drawViewObject(info.numGold[i].viewObject, resourceX - 12 + resourceSpacing * i, line1,currentTileLayout.sprites);
    if (mousePos && mousePos->inRectangle(Rectangle(resourceX + resourceSpacing * i - 20, line1,
            resourceX + resourceSpacing * (i + 1) - 20, line1 + 30)))
      mapGui->drawHint(renderer, white, info.numGold[i].name);
  }
  sf::Uint32 optionSyms[] = {L'⌂', 0x1f718, 0x1f4d6, 0x2692, 0x2694, L'?'};
  optionButtons.clear();
  for (int i = 0; i < 6; ++i) {
    int w = 50;
    int line = topBarHeight;
    int h = 45;
    int leftPos = renderer.getWidth() - rightBarText - 5;
    renderer.drawText(i < 5 ? Renderer::SYMBOL_FONT : Renderer::TEXT_FONT, 35, i == int(collectiveOption) ? green : white,
        leftPos + i * w, line, optionSyms[i], true);
    optionButtons.emplace_back(leftPos + i * w - w / 2, line,
        leftPos + (i + 1) * w - w / 2, line + h);
  }
  techButtons.clear();
  int cnt = 0;
  creatureGroupButtons.clear();
  creatureButtons.clear();
  creatureNames.clear();
  teamButton = Nothing();
  cancelTeamButton = Nothing();
  chosenCreatures.clear();
  uniqueCreatures.clear();
  if (collectiveOption != CollectiveOption::MINIONS)
    chosenCreature = "";
  switch (collectiveOption) {
    case CollectiveOption::MINIONS: drawMinions(info); break;
    case CollectiveOption::BUILDINGS: drawBuildings(info); break;
    case CollectiveOption::KEY_MAPPING: drawKeeperHelp(); break;
    case CollectiveOption::TECHNOLOGY: drawTechnology(info); break;
    case CollectiveOption::WORKSHOP: drawWorkshop(info); break;
    case CollectiveOption::VILLAGES: drawVillages(gameInfo.villageInfo); break;
  }
}

void WindowView::refreshText() {
  int lineHeight = 25;
  int numMsg = 0;
  for (int i : All(currentMessage))
    if (!currentMessage[i].empty())
      numMsg = i + 1;
  renderer.drawFilledRectangle(0, 0, renderer.getWidth(), max(topBarHeight, lineHeight * (numMsg + 1)), translucentBlack);
  for (int i : All(currentMessage))
    renderer.drawText(oldMessage ? gray : white, 10, 10 + lineHeight * i, currentMessage[i]);
  switch (gameInfo.infoType) {
    case GameInfo::InfoType::PLAYER:
        drawPlayerInfo();
        break;
    case GameInfo::InfoType::BAND: drawBandInfo(); break;
  }
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

const CreatureView* lastCreatureView = nullptr;

void WindowView::refreshViewInt(const CreatureView* collective, bool flipBuffer) {
  lastCreatureView = collective;
  gameReady = true;
  switchTiles();
  const Level* level = collective->getLevel();
  collective->refreshGameInfo(gameInfo);
  for (Vec2 pos : mapLayout->getAllTiles(getMapViewBounds(), maxLevelBounds))
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
  const MapMemory* memory = &collective->getMemory(level); 
  for (Vec2 pos : mapLayout->getAllTiles(getMapViewBounds(), maxLevelBounds)) 
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

static void putMapPixel(Vec2 pos, Color col) {
  if (pos.inRectangle(maxLevelBounds))
    mapBuffer.setPixel(pos.x, pos.y, col);
}

void WindowView::drawLevelMapPart(const Level* level, Rectangle levelPart, Rectangle bounds,
    const CreatureView* creature, bool printLocations) {
  vector<Vec2> roads;
  double scale = min(double(bounds.getW()) / levelPart.getW(),
      double(bounds.getH()) / levelPart.getH());
  for (Vec2 v : Rectangle(bounds.getW() / scale, bounds.getH() / scale)) {
    putMapPixel(v, black);
  }
  for (Vec2 v : levelPart) {
    if (!v.inRectangle(level->getBounds()) || (!creature->getMemory(level).hasViewIndex(v) && !creature->canSee(v)))
      putMapPixel(v - levelPart.getTopLeft(), black);
    else {
      putMapPixel(v - levelPart.getTopLeft(), Tile::getColor(level->getSquare(v)->getViewObject()));
      if (level->getSquare(v)->getName() == "road")
        roads.push_back(v - levelPart.getTopLeft());
    }
  }
  renderer.drawImage(bounds.getPX(), bounds.getPY(), bounds.getKX(), bounds.getKY(), mapBuffer, scale);
  for (Vec2 v : roads) {
    Vec2 rrad(1, 1);
    Vec2 pos = bounds.getTopLeft() + v * scale;
    renderer.drawFilledRectangle(Rectangle(pos - rrad, pos + rrad), brown);
  }
  Vec2 playerPos = bounds.getTopLeft() + (creature->getPosition() - levelPart.getTopLeft()) * scale;
  Vec2 rad(3, 3);
  if (playerPos.inRectangle(bounds.minusMargin(rad.x)))
    renderer.drawFilledRectangle(Rectangle(playerPos - rad, playerPos + rad), red);
  if (printLocations)
    for (const Location* loc : level->getAllLocations())
      if (loc->hasName())
        for (Vec2 v : loc->getBounds())
          if (creature->getMemory(level).hasViewIndex(v) || creature->canSee(v)) {
            string text = loc->getName();
            Vec2 pos = bounds.getTopLeft() + loc->getBounds().getBottomRight() * scale;
            renderer.drawFilledRectangle(pos.x, pos.y, pos.x + renderer.getTextLength(text) + 10, pos.y + 25,
                transparency(black, 130));
            renderer.drawText(white, pos.x + 5, pos.y, text);
            break;
          }
}

void WindowView::drawLevelMap(const CreatureView* creature) {
  const Level* level = creature->getLevel();
  TempClockPause pause;
  Rectangle bounds = getMapViewBounds();
  double scale = min(double(bounds.getW()) / level->getBounds().getW(),
      double(bounds.getH()) / level->getBounds().getH());
  while (1) {
    drawLevelMapPart(level, level->getBounds(), bounds, creature);
    renderer.drawAndClearBuffer();
    BlockingEvent ev = readkey();
    if (ev.type == BlockingEvent::KEY)
      return;
    if (ev.type == BlockingEvent::MOUSE_LEFT) {
      Vec2 pos = renderer.getMousePos() - bounds.getTopLeft();
      center = {double(pos.x) / scale, double(pos.y) / scale};
      return;
    }
  }
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
  mapGui->render(renderer);
  if (lastCreatureView) {
    const Level* level = lastCreatureView->getLevel();
    Vec2 rad(40, 40);
    Rectangle bounds(mapLayout->getPlayerPos() - rad, mapLayout->getPlayerPos() + rad);
    renderer.drawFilledRectangle(minimapBounds.minusMargin(-2), Color::Transparent, gray);
    if (level->getBounds().intersects(bounds))
      drawLevelMapPart(level, bounds, minimapBounds, lastCreatureView, false);
  }
  map<string, ViewObject> objIndex;
  for (Vec2 wpos : mapLayout->getAllTiles(getMapViewBounds(), objects.getBounds())) 
    if (objects[wpos]) {
      const ViewIndex& index = *objects[wpos];
      if (auto topObject = index.getTopObject(mapLayout->getLayers()))
        objIndex.insert(std::make_pair(topObject->getDescription(), *topObject));
    }
  int rightPos = renderer.getWidth() -rightBarText;
  renderer.drawFilledRectangle(renderer.getWidth() - rightBarWidth, 0, renderer.getWidth(), renderer.getHeight(), translucentBlack);
  if (gameInfo.infoType == GameInfo::InfoType::PLAYER) {
    int cnt = 0;
    if (legendOption == LegendOption::OBJECTS) {
      for (auto elem : objIndex) {
        drawViewObject(elem.second, rightPos, legendStartHeight + cnt * 25, currentTileLayout.sprites);
        renderer.drawText(white, rightPos + 30, legendStartHeight + cnt * 25, elem.first);
        ++cnt;
      }
    }
  }
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
    BlockingEvent event = readkey();
    if (event.type == BlockingEvent::MOUSE_MOVE || event.type == BlockingEvent::MOUSE_LEFT) {
      if (auto pos = mapGui->getHighlightedTile(renderer)) {
        refreshScreen(false);
        int numArrow = 0;
        Vec2 middle = mapLayout->getAllTiles(getMapViewBounds(), maxLevelBounds).middle();
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
        if (event.type == BlockingEvent::MOUSE_LEFT)
          return dir;
      }
    } else
    refreshScreen();
    if (event.type == BlockingEvent::KEY)
      switch (event.key->code) {
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
  showMessage(message + " (y/n)");
  refreshScreen();
  do {
    BlockingEvent event = readkey();
    if (event.type == BlockingEvent::KEY)
      switch (event.key->code) {
        case Keyboard::Y: showMessage(""); refreshScreen(); return true;
        case Keyboard::Escape:
        case Keyboard::Space:
        case Keyboard::N: showMessage(""); refreshScreen(); return false;
        default: break;
      }
  } while (1);
}

Optional<int> WindowView::getNumber(const string& title, int max, int increments) {
  vector<View::ListElem> options;
  vector<int> numbers;
  for (int i = 0; i <= max; i += increments) {
    options.push_back(convertToString(i));
    numbers.push_back(i);
  }
  Optional<int> res = WindowView::chooseFromList(title, options);
  if (!res)
    return Nothing();
  else
    return numbers[*res];
}

int getMaxLines();

Optional<int> getIndex(const vector<View::ListElem>& options, bool hasTitle, Vec2 mousePos);

Optional<int> WindowView::chooseFromList(const string& title, const vector<ListElem>& options, int index,
    Optional<ActionId> exitAction) {
  return chooseFromList(title, options, index, exitAction, Nothing(), {});
}

Optional<int> WindowView::chooseFromList(const string& title, const vector<ListElem>& options, int index,
    Optional<ActionId> exitAction, Optional<Event::KeyEvent> exitKey, vector<Event::KeyEvent> shortCuts) {
  TempClockPause pause;
  if (options.size() == 0)
    return Nothing();
  vector<int> indexes(options.size());
  int numLines = min((int) options.size(), getMaxLines());
  int count = 0;
  int elemCount = 0;
  for (int i : All(options)) {
    if (options[i].getMod() == View::NORMAL) {
      indexes[count] = elemCount;
      ++count;
    }
    if (options[i].getMod() != View::TITLE)
      ++elemCount;
  }
  if (count == 0) {
    presentList(title, options, false);
    return Nothing();
  }
  index = min(index, count - 1);
  int setMousePos = -1;
  int cutoff = -1;
  while (1) {
    numLines = min((int) options.size(), getMaxLines());
    int height = indexHeight(options, index);
    int newCutoff = min(max(0, height - numLines / 2), (int) options.size() - numLines);
    if (newCutoff != cutoff) {
      cutoff = newCutoff;
    } else
      setMousePos = -1;
    int itemsCutoff = 0;
    for (int i : Range(cutoff))
      if (options[i].getMod() == View::NORMAL)
        ++itemsCutoff;
    vector<ListElem> window = getPrefix(options, cutoff, numLines);
    drawList(title, window, index - itemsCutoff, setMousePos);
    setMousePos = -1;
    BlockingEvent event = readkey();
    if (event.type == BlockingEvent::KEY) {
      if (exitAction && getSimpleActionId(*event.key) == *exitAction)
        return Nothing();
      switch (event.key->code) {
        case Keyboard::PageDown: index += 10; if (index >= count) index = count - 1; break;
        case Keyboard::PageUp: index -= 10; if (index < 0) index = 0; break;
        case Keyboard::Numpad8:
        case Keyboard::Up: index = (index - 1 + count) % count; height = indexHeight(options, index); break;
        case Keyboard::Numpad2:
        case Keyboard::Down: index = (index + 1 + count) % count; height = indexHeight(options, index); break;
        case Keyboard::Numpad5:
        case Keyboard::Return : clearMessageBox(); return indexes[index];
        case Keyboard::Escape : clearMessageBox(); return Nothing();
        default: break;
      }
      if (exitKey && exitKey->code == event.key->code && exitKey->shift == event.key->shift)
        return Nothing();
      for (int i : All(shortCuts))
        if (shortCuts[i].code == event.key->code && shortCuts[i].shift == event.key->shift)
          return i;
    }
    else if (event.type == BlockingEvent::MOUSE_MOVE && mousePos) {
      if (Optional<int> mouseIndex = getIndex(window, !title.empty(), *mousePos)) {
        int newIndex = *mouseIndex + itemsCutoff;
        if (newIndex != index) {
          setMousePos = (newIndex > index ? 0 : 1);
          index = newIndex;
        }
        Debug() << "Index " << index;
      }
    }
    else if (event.type == BlockingEvent::MOUSE_LEFT) {
      clearMessageBox();
      if (mousePos && getIndex(window, !title.empty(), *mousePos))
        return indexes[index];
      else
        return Nothing();
    }
  }
  FAIL << "Very bad";
  return 123;
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
    Optional<ActionId> exitAction) {
  TempClockPause pause;
  int numLines = min((int) options.size(), getMaxLines());
  if (numLines == 0)
    return;
  int index = scrollDown ? options.size() - numLines : 0;
  while (1) {
    numLines = min((int) options.size(), getMaxLines());
    vector<ListElem> window = getPrefix(options, index, numLines);
    drawList(title, window, -1);
    BlockingEvent event = readkey();
    if (event.type == BlockingEvent::KEY)
      switch (event.key->code) {
        case Keyboard::Numpad8:
        case Keyboard::Up: if (index > 0) --index; break;
        case Keyboard::Numpad2:
        case Keyboard::Down: if (index < options.size() - numLines) ++index; break;
        case Keyboard::Return : refreshScreen(); return;
        case Keyboard::Escape : refreshScreen(); return;
 //       case Keyboard::Space : refreshScreen(); return;
        default: break;
      }
    else
      if (event.type == BlockingEvent::MOUSE_LEFT) {
        refreshScreen();
        return;
      }
  }
  refreshScreen();
}

static int yMargin = 10;
static int itemYMargin = 20;
static int itemSpacing = 25;
static int ySpacing = 100;
int windowWidth = 800;

int getMaxLines() {
  return (renderer.getHeight() - ySpacing - ySpacing - 2 * yMargin - itemSpacing - itemYMargin) / itemSpacing;
}

Optional<int> getIndex(const vector<View::ListElem>& options, bool hasTitle, Vec2 mousePos) {
  int xSpacing = (renderer.getWidth() - windowWidth) / 2;
  Rectangle window(xSpacing, ySpacing, xSpacing + windowWidth, renderer.getHeight() - ySpacing);
  if (!mousePos.inRectangle(window))
    return Nothing();
  return reverseIndexHeight(options,
      (mousePos.y - ySpacing - yMargin + (hasTitle ? 0 : itemSpacing) - itemYMargin) / itemSpacing - 1);
}

void WindowView::drawList(const string& title, const vector<ListElem>& options, int hightlight, int setMousePos) {
  int xMargin = 10;
  int itemXMargin = 30;
  int border = 2;
  int h = -1;
  int xSpacing = (renderer.getWidth() - windowWidth) / 2;
  int topMargin = yMargin;
  refreshScreen(false);
  if (hightlight > -1) 
    h = indexHeight(options, hightlight);
  Rectangle window(xSpacing, ySpacing, xSpacing + windowWidth, renderer.getHeight() - ySpacing);
  renderer.drawFilledRectangle(window, translucentBlack, white);
  if (!title.empty())
    renderer.drawText(white, xSpacing + xMargin, ySpacing + yMargin, title);
  else
    topMargin -= itemSpacing;
  for (int i : All(options)) {  
    int beginH = ySpacing + topMargin + (i + 1) * itemSpacing + itemYMargin;
    if (options[i].getMod() == View::NORMAL) {
      if (hightlight > -1 && h == i) {
        renderer.drawFilledRectangle(xSpacing + xMargin, beginH, 
            windowWidth + xSpacing - xMargin, beginH + itemSpacing - 1, darkGreen);
        if (setMousePos > -1)
          renderer.setMousePos(Vec2(renderer.getMousePos().x, beginH + 1 + (itemSpacing - 3) * setMousePos));
      }
      renderer.drawText(white, xSpacing + xMargin + itemXMargin, beginH, options[i].getText());
    } else if (options[i].getMod() == View::TITLE)
      renderer.drawText(yellow, xSpacing + xMargin, beginH, options[i].getText());
    else if (options[i].getMod() == View::INACTIVE)
      renderer.drawText(gray, xSpacing + xMargin + itemXMargin, beginH, options[i].getText());
  }
  renderer.drawAndClearBuffer();
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
  return keypressed();
}

ActionId getDirActionId(const Event::KeyEvent& key) {
  if (key.control)
    return ActionId::TRAVEL;
  if (key.alt)
    return ActionId::FIRE;
  else
    return ActionId::MOVE;
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
CollectiveAction WindowView::getClick(double time) {
  Event event;
  while (renderer.pollEvent(event)) {
    considerScrollEvent(event);
    switch (event.type) {
      case Event::KeyPressed:
        switch (event.key.code) {
          case Keyboard::Up: center.y -= 2.5; break;
          case Keyboard::Down: center.y += 2.5; break;
          case Keyboard::Left: center.x -= 2.5; break;
          case Keyboard::Right: center.x += 2.5; break;
          case Keyboard::Z:
            unzoom();
            return CollectiveAction(CollectiveAction::IDLE);
          case Keyboard::F2: Options::handle(this, OptionSet::GENERAL); refreshScreen(); break;
          case Keyboard::Space:
            if (!myClock.isPaused())
              myClock.pause();
            else
              myClock.cont();
            return CollectiveAction(CollectiveAction::IDLE);
          case Keyboard::Escape:
            return CollectiveAction(CollectiveAction::EXIT);
          default:
            break;
        }
        break;
      case Event::MouseButtonReleased :
          if (event.mouseButton.button == sf::Mouse::Left) {
            leftMouseButtonPressed = false;
            if (collectiveOption == CollectiveOption::BUILDINGS)
              return CollectiveAction(CollectiveAction::BUTTON_RELEASE, activeBuilding);
          }
          break;
      case Event::MouseMoved:
          mousePos = Vec2(event.mouseMove.x, event.mouseMove.y);
          if (leftMouseButtonPressed) {
            Vec2 goTo = mapLayout->projectOnMap(getMapViewBounds(), *mousePos);
            if (goTo != lastGoTo && mousePos->inRectangle(getMapViewBounds())) {
              if (collectiveOption == CollectiveOption::BUILDINGS)
                return CollectiveAction(Keyboard::isKeyPressed(Keyboard::LShift) ? CollectiveAction::RECT_SELECTION : 
                    CollectiveAction::BUILD, goTo, activeBuilding);
              lastGoTo = goTo;
            } else
              continue;
          }
          break;
      case Event::MouseButtonPressed: {
          Vec2 clickPos(event.mouseButton.x, event.mouseButton.y);
          if (event.mouseButton.button == sf::Mouse::Right)
            chosenCreature = "";
          if (event.mouseButton.button == sf::Mouse::Left) {
            if (clickPos.inRectangle(minimapBounds))
              return CollectiveAction(CollectiveAction::DRAW_LEVEL_MAP);
            for (int i : All(techButtons))
              if (clickPos.inRectangle(techButtons[i]))
                return CollectiveAction(CollectiveAction::TECHNOLOGY, i);
            if (teamButton && clickPos.inRectangle(*teamButton)) {
              chosenCreature = "";
              return CollectiveAction(CollectiveAction::GATHER_TEAM);
            }
            if (cancelTeamButton && clickPos.inRectangle(*cancelTeamButton)) {
              chosenCreature = "";
              return CollectiveAction(CollectiveAction::CANCEL_TEAM);
            }
            if (pauseButton && clickPos.inRectangle(*pauseButton)) {
              if (!myClock.isPaused())
                myClock.pause();
              else
                myClock.cont();
            }
            for (int i : All(optionButtons))
              if (clickPos.inRectangle(optionButtons[i]))
                  collectiveOption = (CollectiveOption) i;
            if (collectiveOption == CollectiveOption::BUILDINGS)
              for (int i : All(buildingButtons))
                if (clickPos.inRectangle(buildingButtons[i])) {
                  chosenCreature = "";
                  if (inactiveBuildingReasons[i] == "" || inactiveBuildingReasons[i] == "inactive")
                    activeBuilding = i;
                  else
                    presentText("", inactiveBuildingReasons[i]);
                }
            if (collectiveOption == CollectiveOption::WORKSHOP)
            for (int i : All(workshopButtons))
              if (clickPos.inRectangle(workshopButtons[i])) {
                chosenCreature = "";
                if (inactiveWorkshopReasons[i] == "" || inactiveWorkshopReasons[i] == "inactive")
                  activeWorkshop = i;
                else
                  presentText("", inactiveWorkshopReasons[i]);
              }
            for (int i : All(creatureGroupButtons))
              if (clickPos.inRectangle(creatureGroupButtons[i])) {
                if (uniqueCreatures[i] != nullptr)
                  return CollectiveAction(CollectiveAction::CREATURE_BUTTON, uniqueCreatures[i]->getUniqueId());
                if (chosenCreature == creatureNames[i])
                  chosenCreature = "";
                else
                  chosenCreature = creatureNames[i];
                return CollectiveAction(CollectiveAction::IDLE);
              }
            for (int i : All(creatureButtons))
              if (clickPos.inRectangle(creatureButtons[i])) {
                return CollectiveAction(CollectiveAction::CREATURE_BUTTON, chosenCreatures[i]->getUniqueId());
              }
            leftMouseButtonPressed = true;
            chosenCreature = "";
            if (clickPos.inRectangle(getMapViewBounds())) {
              Vec2 pos = mapLayout->projectOnMap(getMapViewBounds(), clickPos);
              if (!contains({CollectiveOption::WORKSHOP, CollectiveOption::BUILDINGS}, collectiveOption))
                return CollectiveAction(CollectiveAction::POSSESS, pos);
              if (collectiveOption == CollectiveOption::WORKSHOP && activeWorkshop >= 0)
                return CollectiveAction(CollectiveAction::WORKSHOP, pos, activeWorkshop);
              if (collectiveOption == CollectiveOption::BUILDINGS) {
                if (Keyboard::isKeyPressed(Keyboard::LShift))
                  return CollectiveAction(CollectiveAction::RECT_SELECTION, pos, activeBuilding);
                else
                  return CollectiveAction(CollectiveAction::BUILD, pos, activeBuilding);
              }
            }
          }
          }
          break;
      case Event::Resized:
          resize(event.size.width, event.size.height);
          return CollectiveAction(CollectiveAction::IDLE);
      default: break;
    }
  }
  return CollectiveAction(CollectiveAction::IDLE);
}

vector<KeyInfo> keyInfo {
//  { "I", "Inventory", {Keyboard::I}},
//  { "E", "Manage equipment", {Keyboard::E}},
  { "Enter", "Pick up items or interact with square", {Keyboard::Return}},
  { "D", "Drop item", {Keyboard::D}},
  { "Shift + D", "Extended drop - choose the number of items", {Keyboard::D}},
  { "A", "Apply item", {Keyboard::A}},
  { "T", "Throw item", {Keyboard::T}},
//  { "S", "Cast spell", {Keyboard::S}},
  { "M", "Show message history", {Keyboard::M}},
  { "C", "Chat with someone", {Keyboard::C}},
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
  auto index = chooseFromList("", options, 0, Nothing(), Optional<Event::KeyEvent>({Keyboard::F1}), shortCuts);
  if (!index)
    return Nothing();
  return keyInfo[*index].event;
}

Action WindowView::getAction() {
  while (1) {
    BlockingEvent event = readkey();
    retireMessages();
    if (event.type == BlockingEvent::IDLE || event.type == BlockingEvent::MOUSE_MOVE)
      return Action(ActionId::IDLE);
    if (event.type == BlockingEvent::MINIMAP)
      return Action(ActionId::DRAW_LEVEL_MAP);
    if (event.type == BlockingEvent::MOUSE_LEFT) {
      if (auto pos = mapGui->getHighlightedTile(renderer))
        return Action(ActionId::MOVE_TO, *pos);
      else
        return Action(ActionId::IDLE);
    }
    auto key = event.key;
    if (key->code == Keyboard::F1)
      key = getEventFromMenu();
    if (!key)
      return Action(ActionId::IDLE);
    if (auto actionId = getSimpleActionId(*key))
      return Action(*actionId);

    switch (key->code) {
      case Keyboard::Escape: return Action(ActionId::EXIT);
      case Keyboard::Z: unzoom(); return Action(ActionId::IDLE);
      case Keyboard::F1: legendOption = (LegendOption)(1 - (int)legendOption); return Action(ActionId::IDLE);
      case Keyboard::F2: Options::handle(this, OptionSet::GENERAL); return Action(ActionId::IDLE);
      case Keyboard::Up:
      case Keyboard::Numpad8: return Action(getDirActionId(*key), Vec2(0, -1));
      case Keyboard::Numpad9: return Action(getDirActionId(*key), Vec2(1, -1));
      case Keyboard::Right: 
      case Keyboard::Numpad6: return Action(getDirActionId(*key), Vec2(1, 0));
      case Keyboard::Numpad3: return Action(getDirActionId(*key), Vec2(1, 1));
      case Keyboard::Down:
      case Keyboard::Numpad2: return Action(getDirActionId(*key), Vec2(0, 1));
      case Keyboard::Numpad1: return Action(getDirActionId(*key), Vec2(-1, 1));
      case Keyboard::Left:
      case Keyboard::Numpad4: return Action(getDirActionId(*key), Vec2(-1, 0));
      case Keyboard::Numpad7: return Action(getDirActionId(*key), Vec2(-1, -1));
      default: break;
    }
  }
}

Optional<ActionId> WindowView::getSimpleActionId(sf::Event::KeyEvent key) {
  switch (key.code) {
    case Keyboard::Return:
    case Keyboard::Numpad5: if (key.shift) return ActionId::EXT_PICK_UP; 
                              else return ActionId::PICK_UP;
    case Keyboard::I: return ActionId::SHOW_INVENTORY;
    case Keyboard::D: return key.shift ? ActionId::EXT_DROP : ActionId::DROP;
    case Keyboard::Space:
    case Keyboard::Period: return ActionId::WAIT;
    case Keyboard::A: return ActionId::APPLY_ITEM;
    case Keyboard::E: return ActionId::EQUIPMENT;
    case Keyboard::T: return ActionId::THROW;
    case Keyboard::M: return ActionId::SHOW_HISTORY;
    case Keyboard::H: return ActionId::HIDE;
    case Keyboard::P: return ActionId::PAY_DEBT;
    case Keyboard::C: return ActionId::CHAT;
    case Keyboard::U: return ActionId::UNPOSSESS;
    case Keyboard::S: return ActionId::CAST_SPELL;
    default: break;
  }
  return Nothing();
}

