#include "stdafx.h"


using sf::Color;
using sf::String;
using sf::RenderWindow;
using sf::VideoMode;
using sf::Text;
using sf::Font;
using sf::Event;
using sf::RectangleShape;
using sf::Vector2f;
using sf::Vector2u;
using sf::Image;
using sf::Sprite;
using sf::Texture;
using sf::Keyboard;

using namespace std;

Color white(255, 255, 255);
Color yellow(250, 255, 0);
Color lightBrown(210, 150, 0);
Color orangeBrown(250, 150, 0);
Color brown(240, 130, 0);
Color darkBrown(100, 60, 0);
Color lightGray(150, 150, 150);
Color gray(100, 100, 100);
Color almostGray(102, 102, 102);
Color darkGray(50, 50, 50);
Color almostDarkGray(60, 60, 60);
Color black(0, 0, 0);
Color almostWhite(200, 200, 200);
Color green(0, 255, 0);
Color lightGreen(100, 255, 100);
Color darkGreen(0, 150, 0);
Color red(255, 0, 0);
Color lightRed(255, 100, 100);
Color pink(255, 20, 147);
Color orange(255, 165, 0);
Color blue(0, 0, 255);
Color darkBlue(50, 50, 200);
Color lightBlue(100, 100, 255);
Color purple(160, 32, 240);
Color violet(120, 0, 255);
Color translucentBlack(0, 0, 0);

View* View::createLoggingView(ofstream& of) {
  return new LoggingView<WindowView>(of);
}

View* View::createReplayView(ifstream& ifs) {
  return new ReplayView<WindowView>(ifs);
}

struct Tile {
  Color color;
  String text;
  bool symFont = false;
  int px = -1, py = -1;
  Tile(sf::Uint32 ch, Color col, bool sym = false) : color(col), text(ch), symFont(sym) {
  }
  Tile(int x, int y) : px(x), py(y) {}
};

Tile getSpecialCreature(const ViewObject& obj, bool humanoid) {
  RandomGen r;
  r.init(std::hash<string>()(obj.getBareDescription()));
  string let = humanoid ? "WETYUIPLKJHFAXBM" : "qwetyupkjfaxbnm";
  char c;
  if (contains(let, obj.getBareDescription()[0]))
    c = obj.getBareDescription()[0];
  else
  if (contains(let, tolower(obj.getBareDescription()[0])))
    c = tolower(obj.getBareDescription()[0]);
  else
    c = let[r.getRandom(let.size())];
  Color col(r.getRandom(80, 250), r.getRandom(80, 250), 0);
  return Tile(c, col);
}

Tile getTile(const ViewObject& obj) {
  switch (obj.id()) {
    case ViewId::PLAYER: return Tile('@', white);
    case ViewId::UNKNOWN_MONSTER: return Tile('?', lightGreen);
    case ViewId::SPECIAL_BEAST: return getSpecialCreature(obj, false);
    case ViewId::SPECIAL_HUMANOID: return getSpecialCreature(obj, true);
    case ViewId::ELF: return Tile('@', green);
    case ViewId::ELF_CHILD: return Tile('@', lightGreen);
    case ViewId::ELF_LORD: return Tile('@', darkGreen);
    case ViewId::ELVEN_SHOPKEEPER: return Tile('@', lightBlue);
    case ViewId::IMP: return Tile('i', lightBrown);
    case ViewId::BILE_DEMON: return Tile('Q', orange);
    case ViewId::HELL_HOUND: return Tile('d', purple);
    case ViewId::CHICKEN: return Tile('c', yellow);
    case ViewId::DWARF: return Tile('h', blue);
    case ViewId::DWARF_BARON: return Tile('h', darkBlue);
    case ViewId::DWARVEN_SHOPKEEPER: return Tile('h', lightBlue);
    case ViewId::FLOOR: return Tile('.', white);
    case ViewId::BRIDGE: return Tile('_', brown);
    case ViewId::PATH: return Tile('.', lightGray);
    case ViewId::SAND: return Tile('.', yellow);
    case ViewId::GRASS: return Tile(0x1d0f0, green, true);
    case ViewId::WALL: return Tile('#', lightGray);
    case ViewId::MOUNTAIN: return Tile(0x25ee, darkGray, true);
    case ViewId::GOLD_ORE: return Tile(L'⁂', yellow, true);
    case ViewId::SNOW: return Tile(0x25ee, white, true);
    case ViewId::HILL: return Tile(0x1d022, darkGreen, true);
    case ViewId::WOOD_WALL: return Tile('#', darkBrown);
    case ViewId::BLACK_WALL: return Tile('#', lightGray);
    case ViewId::YELLOW_WALL: return Tile('#', yellow);
    case ViewId::SECRETPASS: return Tile('#', lightGray);
    case ViewId::DOWN_STAIRCASE: return Tile(0x2798, almostWhite, true);
    case ViewId::UP_STAIRCASE: return Tile(0x279a, almostWhite, true);
    case ViewId::GREAT_GOBLIN: return Tile('O', purple);
    case ViewId::GOBLIN: return Tile('o', darkBlue);
    case ViewId::BANDIT: return Tile('@', darkBlue);
    case ViewId::KNIGHT: return Tile('@', white);
    case ViewId::AVATAR: return Tile('@', blue);
    case ViewId::ARCHER: return Tile('@', brown);
    case ViewId::PESEANT: return Tile('@', green);
    case ViewId::CHILD: return Tile('@', lightGreen);
    case ViewId::ZOMBIE: return Tile('Z', green);
    case ViewId::VAMPIRE: return Tile('V', darkGray);
    case ViewId::MUMMY: return Tile('Z', yellow);
    case ViewId::MUMMY_LORD: return Tile('Z', orange);
    case ViewId::JACKAL: return Tile('d', lightBrown);
    case ViewId::DEER: return Tile('R', darkBrown);
    case ViewId::HORSE: return Tile('H', lightBrown);
    case ViewId::COW: return Tile('C', white);
    case ViewId::SHEEP: return Tile('s', white);
    case ViewId::PIG: return Tile('p', yellow);
    case ViewId::BOAR: return Tile('b', lightBrown);
    case ViewId::FOX: return Tile('d', orangeBrown);
    case ViewId::WOLF: return Tile('d', darkBlue);
    case ViewId::VODNIK: return Tile('f', green);
    case ViewId::KRAKEN: return Tile('S', darkGreen);
    case ViewId::DEATH: return Tile('D', darkGray);
    case ViewId::KRAKEN2: return Tile('S', green);
    case ViewId::NIGHTMARE: return Tile('n', purple);
    case ViewId::FIRE_SPHERE: return Tile('e', red);
    case ViewId::BEAR: return Tile('N', brown);
    case ViewId::BAT: return Tile('b', darkGray);
    case ViewId::GNOME: return Tile('g', green);
    case ViewId::LEPRECHAUN: return Tile('l', green);
    case ViewId::RAT: return Tile('r', brown);
    case ViewId::SNAKE: return Tile('s', yellow);
    case ViewId::VULTURE: return Tile('v', darkGray);
    case ViewId::BODY_PART: return Tile('%', red);
    case ViewId::BONE: return Tile('%', white);
    case ViewId::BUSH: return Tile('&', darkGreen);
    case ViewId::DECID_TREE: return Tile(0x1f70d, darkGreen, true);
    case ViewId::CANIF_TREE: return Tile(0x2663, darkGreen, true);
    case ViewId::WATER: return Tile('~', lightBlue);
    case ViewId::MAGMA: return Tile('~', red);
    case ViewId::ABYSS: return Tile('~', darkGray);
    case ViewId::DOOR: return Tile(0x1f062, brown, true);
    case ViewId::SWORD: return Tile(')', lightGray);
    case ViewId::ELVEN_SWORD: return Tile(')', gray);
    case ViewId::KNIFE: return Tile(')', white);
    case ViewId::WAR_HAMMER: return Tile(')', blue);
    case ViewId::BATTLE_AXE: return Tile(')', green);
    case ViewId::BOW: return Tile(')', brown);
    case ViewId::ARROW: return Tile('\\', brown);
    case ViewId::SCROLL: return Tile('?', white);
    case ViewId::STEEL_AMULET: return Tile('\"', yellow);
    case ViewId::COPPER_AMULET: return Tile('\"', yellow);
    case ViewId::CRYSTAL_AMULET: return Tile('\"', yellow);
    case ViewId::WOODEN_AMULET: return Tile('\"', yellow);
    case ViewId::AMBER_AMULET: return Tile('\"', yellow);
    case ViewId::BOOK: return Tile('+', yellow);
    case ViewId::FIRST_AID: return Tile('+', red);
    case ViewId::TRAP_ITEM: return Tile('+', yellow);
    case ViewId::EFFERVESCENT_POTION: return Tile('!', lightRed);
    case ViewId::MURKY_POTION: return Tile('!', blue);
    case ViewId::SWIRLY_POTION: return Tile('!', yellow);
    case ViewId::VIOLET_POTION: return Tile('!', violet);
    case ViewId::PUCE_POTION: return Tile('!', darkBrown);
    case ViewId::SMOKY_POTION: return Tile('!', lightGray);
    case ViewId::FIZZY_POTION: return Tile('!', lightBlue);
    case ViewId::MILKY_POTION: return Tile('!', white);
    case ViewId::SLIMY_MUSHROOM: return Tile(0x22c6, darkGray, true);
    case ViewId::PINK_MUSHROOM: return Tile(0x22c6, pink, true);
    case ViewId::DOTTED_MUSHROOM: return Tile(0x22c6, green, true);
    case ViewId::GLOWING_MUSHROOM: return Tile(0x22c6, lightBlue, true);
    case ViewId::GREEN_MUSHROOM: return Tile(0x22c6, green, true);
    case ViewId::BLACK_MUSHROOM: return Tile(0x22c6, darkGray, true);
    case ViewId::FOUNTAIN: return Tile('0', lightBlue);
    case ViewId::GOLD: return Tile('$', yellow);
    case ViewId::CHEST: return Tile('=', brown);
    case ViewId::COFFIN: return Tile(L'⚰', darkGray, true);
    case ViewId::BOULDER: return Tile(L'●', lightGray, true);
    case ViewId::UNARMED_BOULDER_TRAP: return Tile(L'○', lightGray, true);
    case ViewId::PORTAL: return Tile(0x1d6af, lightGreen, true);
    case ViewId::TRAP: return Tile(L'➹', yellow, true);
    case ViewId::GAS_TRAP: return Tile(L'☠', green, true);
    case ViewId::UNARMED_GAS_TRAP: return Tile(L'☠', lightGray, true);
    case ViewId::ROCK: return Tile('*', lightGray);
    case ViewId::BED: return Tile('=', white);
    case ViewId::DUNGEON_HEART: return Tile(L'♥', white, true);
    case ViewId::ALTAR: return Tile(L'Ω', white);
    case ViewId::TORTURE_TABLE: return Tile('=', gray);
    case ViewId::TRAINING_DUMMY: return Tile(L'‡', brown, true);
    case ViewId::WORKSHOP: return Tile('&', lightBlue);
    case ViewId::GRAVE: return Tile(0x2617, gray, true);
    case ViewId::BARS: return Tile(L'⧻', lightBlue);
    case ViewId::BORDER_GUARD: return Tile(' ', white);
    case ViewId::LEATHER_ARMOR: return Tile('[', brown);
    case ViewId::LEATHER_HELM: return Tile('[', brown);
    case ViewId::CHAIN_ARMOR: return Tile('[', lightGray);
    case ViewId::IRON_HELM: return Tile('[', lightGray);
    case ViewId::DESTROYED_FURNITURE: return Tile('*', brown);
    case ViewId::BURNT_FURNITURE: return Tile('*', darkGray);
    case ViewId::FALLEN_TREE: return Tile('*', green);
    case ViewId::BURNT_TREE: return Tile('*', darkGray);
    case ViewId::GUARD_POST: return Tile(L'⚐', yellow, true);
  }
  Debug(FATAL) << "unhandled view id " << (int)obj.id();
  return Tile(' ', white);
}

RenderWindow* display;
sf::View* sfView;

int screenWidth;
int screenHeight;

Font textFont;
int textSize = 20;
int smallTextSize = 12;

Font tileFont;
Font symbolFont;

Image mapBuffer;
Texture tiles;

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

int getTextWidth(const Font& font, int size, String s) {
  Text t(s, font, size);
  return t.getLocalBounds().width;
}

void drawText(const Font& font, int size, Color color, int x, int y, String s, bool center = false) {
  int ox = 0;
  int oy = 0;
  Text t(s, font, size);
  if (center) {
    sf::FloatRect bounds = t.getLocalBounds();
    ox -= bounds.left + bounds.width / 2;
    //oy -= bounds.top + bounds.height / 2;
  }
  t.setPosition(x + ox, y + oy);
  t.setColor(color);
  display->draw(t);
}

void drawText(Color color, int x, int y, string s, bool center = false, int size = textSize) {
  std::basic_string<sf::Uint32> utf32;
  sf::Utf8::toUtf32(s.begin(), s.end(), std::back_inserter(utf32));
  drawText(textFont, size, color, x, y, utf32, center);
}

void drawText(Color color, int x, int y, const char* c, bool center = false, int size = textSize) {
  drawText(textFont, size, color, x, y, String(c), center);
}

void drawImage(int px, int py, const Image& image) {
  Texture t;
  t.loadFromImage(image);
  Sprite s(t);
  s.setPosition(px, py);
  display->draw(s);
}

void drawSprite(int x, int y, int px, int py, int w, int h, const Texture& t) {
  Sprite s(t, sf::IntRect(px, py, w, h));
  s.setPosition(x, y);
  display->draw(s);
}

void WindowView::initialize() {
  display = new RenderWindow(VideoMode(1024, 600, 32), "KeeperRL");
  sfView = new sf::View(display->getDefaultView());
  screenHeight = display->getSize().y;
  screenWidth = display->getSize().x;

  textFont.loadFromFile("coolvetica rg.ttf");
  tileFont.loadFromFile("coolvetica rg.ttf");
  symbolFont.loadFromFile("Symbola.ttf");

  normalLayout = MapLayout::gridLayout(screenWidth, screenHeight, 16, 20, 0, 30, 255, 0, 1, allLayers);
  tppLayout = MapLayout::tppLayout(screenWidth, screenHeight, 16, 20, 0, 30, 220, 85);
  unzoomLayout = MapLayout::gridLayout(screenWidth, screenHeight, 8, 10, 0, 30, 220, 85, 1,
      {ViewLayer::FLOOR, ViewLayer::CREATURE});
  worldLayout = MapLayout::worldLayout(screenWidth, screenHeight, 0, 80, 220, 75);
  allLayouts.push_back(normalLayout);
  allLayouts.push_back(tppLayout);
  allLayouts.push_back(unzoomLayout);
  allLayouts.push_back(worldLayout);
  mapBuffer.create(600, 600);
  mapLayout = normalLayout;
   //Image tileImage;
  //CHECK(tileImage.loadFromFile("tile_test.png"));
  //tiles.loadFromImage(tileImage);
}

void WindowView::displaySplash(bool& ready) {
  Image splash;
  CHECK(splash.loadFromFile("splash.png"));
  drawImage((screenWidth - splash.getSize().x) / 2, (screenHeight - splash.getSize().y) / 2, splash);
  drawText(white, screenWidth / 2, screenHeight - 60, "Loading...", true);
  drawAndClearBuffer();
  while (!ready) {
    sf::sleep(sf::milliseconds(30));
    Event event;
    while (display->pollEvent(event)) {
      if (event.type == Event::Resized) {
        resize(event.size.width, event.size.height);
      }
      if (event.type == Event::GainedFocus || event.type == Event::Resized) {
        drawImage((screenWidth - splash.getSize().x) / 2, (screenHeight - splash.getSize().y) / 2, splash);
        drawText(white, screenWidth / 2, screenHeight - 60, "Loading...", true);
        drawAndClearBuffer();
      }
    }
  }
}

bool keypressed() {
  Event event;
  while (display->pollEvent(event))
    if (event.type == Event::KeyPressed)
      return true;
  return false;
}

void WindowView::resize(int width, int height) {
  screenWidth = width;
  screenHeight = height;
  for (MapLayout* layout : allLayouts)
    layout->updateScreenSize(screenWidth, screenHeight);
  display->setView(*(sfView = new sf::View(sf::FloatRect(0, 0, screenWidth, screenHeight))));
}

Optional<Event::KeyEvent> WindowView::readkey() {
  Event event;
  while (1) {
    bool wasPaused = false;
    if (!myClock.isPaused()) {
      myClock.pause();
      wasPaused = true;
    }
    display->waitEvent(event);
    if (considerScrollEvent(event))
      return Nothing();
    if (wasPaused)
      myClock.cont();
    Debug() << "Event " << event.type;
    if (event.type == Event::KeyPressed) {
      Event::KeyEvent ret(event.key);
      while (display->pollEvent(event));
      return ret;
    }
    if (event.type == Event::Resized) {
      resize(event.size.width, event.size.height);
      return Nothing();
    }
    if (event.type == Event::GainedFocus)
      return Nothing();
  }
}

void WindowView::close() {
}

void drawFilledRectangle(const Rectangle& t, Color color, Optional<Color> outline = Nothing()) {
  RectangleShape r(Vector2f(t.getW(), t.getH()));
  r.setPosition(t.getPX(), t.getPY());
  r.setFillColor(color);
  if (outline) {
    r.setOutlineThickness(2);
    r.setOutlineColor(*outline);
  }
  display->draw(r);
}

void drawFilledRectangle(int px, int py, int kx, int ky, Color color, Optional<Color> outline = Nothing()) {
  drawFilledRectangle(Rectangle(px, py, kx, ky), color, outline);
}

static Color getColor(const ViewObject& object) {
  if (object.isInvisible())
    return darkGray;
  if (object.isHidden())
    return lightGray;
 /* if (object.isBurning())
    return red;*/
  double bleeding = object.getBleeding();
  if (bleeding > 0)
    bleeding = 0.5 + bleeding / 2;
  bleeding = min(1., bleeding);
  Color color = getTile(object).color;
  return Color(
      (1 - bleeding) * color.r + bleeding * 255,
      (1 - bleeding) * color.g,
      (1 - bleeding) * color.b);
}

static Color getMemoryColor(const ViewObject& object) {
  Color color = getTile(object).color;
  float cf = 3.5;
  float r = color.r, g = color.g, b = color.b;
  return Color(
        (cf * r + g + b) / (2 + cf),
        (r + g * cf + b) / (2 + cf),
        (r + g + b * cf) / (2 + cf));
}

int fireVar = 50;

Color getFireColor() {
  return Color(200 + Random.getRandom(-fireVar, fireVar), Random.getRandom(fireVar), Random.getRandom(fireVar), 150);
}

void printStanding(int x, int y, double standing, const string& tribeName) {
  standing = min(1., max(-1., standing));
  Color color = standing < 0
      ? Color(255, 255 * (1. + standing), 255 *(1. + standing))
      : Color(255 * (1. - standing), 255, 255 * (1. - standing));
  drawText(color, x, y, (standing >= 0 ? "friend of " : "enemy of ") + tribeName);
}

Color getSpeedColor(int value) {
  if (value > 100)
    return Color(max(0, 255 - (value - 100) * 2), 255, max(0, 255 - (value - 100) * 2));
  else
    return Color(255, max(0, 255 + (value - 100) * 4), max(0, 255 + (value - 100) * 4));
}

int topBarHeight = 50;
int rightBarWidth = 230;

void WindowView::drawPlayerInfo() {
  GameInfo::PlayerInfo& info = gameInfo.playerInfo;
  string title = info.title;
  if (!info.adjectives.empty() || !info.playerName.empty())
    title = " " + title;
  for (int i : All(info.adjectives))
    title = string(i <info.adjectives.size() - 1 ? ", " : " ") + info.adjectives[i] + title;
 // int line0 = screenHeight - 115;
  int line0 = screenHeight - 90;
  int line1 = screenHeight - 65;
  int line2 = screenHeight - 40;
  drawFilledRectangle(0, line1 - 10, screenWidth - rightBarWidth, screenHeight, translucentBlack);
  string wielding = info.weaponName != "" ? "wielding " + info.weaponName : "barehanded";  
  string playerLine = capitalFirst(!info.playerName.empty() ? info.playerName + " the" + title : title);
  int attrSpace = 8;
  string speed =
    "Speed: " + convertToString(info.speed) + string(attrSpace, ' ');
  string attr =
    "Attack: " + convertToString(info.attack) + string(attrSpace, ' ') +
    "Defense: " + convertToString(info.defense) + string(attrSpace, ' ') +
    "Strength: " + convertToString(info.strength) + string(attrSpace, ' ') +
    "Dexterity: " + convertToString(info.dexterity) + string(attrSpace, ' ');
 //   (info.bleeding ? "  bleeding" : "");
  string money = "$: " + convertToString(info.numGold);
  string turn = " T:" + convertToString<int>(info.time);
  drawText(white, 10, line1, playerLine);
  drawText(getSpeedColor(info.speed), 10, line2, speed);
  drawText(white, 135, line2, attr + "    " + turn);
  drawText(white, 350, line1, info.levelName + "  " + money);
  drawText(white, 600, line1, wielding);
 // printStanding(screenWidth - rightBarWidth, line0, info.elfStanding, "elves");
 // printStanding(screenWidth - rightBarWidth, line1, info.dwarfStanding, "dwarves");
 // printStanding(screenWidth - rightBarWidth, line2, info.goblinStanding, "goblins");
}

string getPlural(const string& a, const string&b, int num) {
  if (num == 1)
    return "1 " + a;
  else
    return convertToString(num) + " " + b;
}

const int legendLineHeight = 30;
const int legendStartHeight = topBarHeight + 80;

static map<string, pair<ViewObject, int>> getCreatureMap(vector<const Creature*> creatures) {
  map<string, pair<ViewObject, int>> creatureMap;
  for (int i : All(creatures)) {
    auto elem = creatures[i];
    if (!creatureMap.count(elem->getName())) {
      creatureMap.insert(make_pair(elem->getName(), make_pair(elem->getViewObject(), 1)));
    } else
      ++creatureMap[elem->getName()].second;
  }
  return creatureMap;
}

static void drawViewObject(const ViewObject& obj, int x, int y) {
    Tile tile = getTile(obj);
    drawText(tile.symFont ? symbolFont : textFont, 20, getColor(obj), x, y, tile.text, true);
}

void WindowView::drawMinions(GameInfo::BandInfo& info) {
  map<string, pair<ViewObject, int>> creatureMap = getCreatureMap(info.creatures);
  map<string, pair<ViewObject, int>> enemyMap = getCreatureMap(info.enemies);
  drawText(white, screenWidth - rightBarWidth, legendStartHeight, info.monsterHeader);
  int cnt = 0;
  int lineStart = legendStartHeight + 35;
  for (auto elem : creatureMap){
    int height = lineStart + cnt * legendLineHeight;
    drawViewObject(elem.second.first, screenWidth - rightBarWidth + 10, height);
    Color col = (elem.first == chosenCreature) ? green : white;
    drawText(col, screenWidth - rightBarWidth + 30, height,
        convertToString(elem.second.second) + "   " + elem.first);
    creatureGroupButtons.emplace_back(
        screenWidth - rightBarWidth, height, screenWidth - rightBarWidth + 150, height + legendLineHeight);
    creatureNames.push_back(elem.first);
    ++cnt;
  }
  
  if (info.gatheringTeam && !info.team.empty()) {
    drawText(white, screenWidth - rightBarWidth, lineStart + (cnt + 1) * legendLineHeight, 
        getPlural("monster", "monsters", info.team.size()));
    ++cnt;
  }
  if (info.creatures.size() > 1 || info.gatheringTeam) {
    int height = lineStart + (cnt + 1) * legendLineHeight;
    drawText((info.gatheringTeam && info.team.empty()) ? green : white, screenWidth - rightBarWidth, height, 
        info.team.empty() ? "[gather team]" : "[command team]");
    int butWidth = 150;
    teamButton = Rectangle(screenWidth - rightBarWidth, height,
        screenWidth - rightBarWidth + butWidth, height + legendLineHeight);
    if (info.gatheringTeam) {
      drawText(white, screenWidth - rightBarWidth + butWidth, height, "[cancel]");
      cancelTeamButton = Rectangle(screenWidth - rightBarWidth + butWidth, height,
          screenWidth - rightBarWidth + 230, height + legendLineHeight);
    }
    cnt += 2;
  }
  ++cnt;
  if (!enemyMap.empty()) {
    drawText(white, screenWidth - rightBarWidth, lineStart + (cnt + 1) * legendLineHeight, "Enemies:");
    for (auto elem : enemyMap){
      int height = lineStart + (cnt + 2) * legendLineHeight + 10;
      drawViewObject(elem.second.first, screenWidth - rightBarWidth + 10, height);
      drawText(white, screenWidth - rightBarWidth + 30, height,
          convertToString(elem.second.second) + "   " + elem.first);
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
      drawFilledRectangle(screenWidth - rightBarWidth - width - 20, lineStart,
          screenWidth - rightBarWidth, legendStartHeight + 35 + (chosen.size() + 3) * legendLineHeight, black);
      drawText(lightBlue, screenWidth - rightBarWidth - width - 10, lineStart, 
          info.gatheringTeam ? "Click to add to team:" : "Click to possess:");
      int cnt = 1;
      for (const Creature* c : chosen) {
        int height = lineStart + cnt * legendLineHeight;
        drawViewObject(c->getViewObject(), screenWidth - rightBarWidth - width, height);
        drawText(contains(info.team, c) ? green : white, screenWidth - rightBarWidth - width + 30, height,
            "level: " + convertToString(c->getExpLevel()) + "    " + info.tasks[c]);
        creatureButtons.emplace_back(screenWidth - rightBarWidth - width, height,
            screenWidth - rightBarWidth, height + legendLineHeight);
        chosenCreatures.push_back(c);
        ++cnt;
      }
      int height = lineStart + cnt * legendLineHeight + 10;
      drawText(white, screenWidth - rightBarWidth - width, height, "[show description]");
      descriptionButton = Rectangle(screenWidth - rightBarWidth - width, height,
          screenWidth - rightBarWidth, height + legendLineHeight);
    }
  }
}

void WindowView::drawBuildings(GameInfo::BandInfo& info) {
  for (int i : All(info.buttons)) {
    string text = get<0>(info.buttons[i]);
    int height = legendStartHeight + i * legendLineHeight;
    drawViewObject(get<1>(info.buttons[i]), screenWidth - rightBarWidth, height);
    drawText(i == info.activeButton ? green : (get<2>(info.buttons[i]) ? white : lightGray),
        screenWidth - rightBarWidth + 30, height, text);
    roomButtons.emplace_back(screenWidth - rightBarWidth, height,
        screenWidth - rightBarWidth + 150, height + legendLineHeight);
  }
}
  
void WindowView::drawKeeperHelp() {
  vector<string> helpText { "use mouse to", "dig and build", "", "click on minion list", "to possess",
    "", "heroes come from", "the up staircase", "", "[space]  pause", "[z]  zoom"};
  int cnt = 0;
  for (string line : helpText) {
    int height = legendStartHeight + cnt * legendLineHeight;
    drawText(lightBlue, screenWidth - rightBarWidth, height, line);
    cnt ++;
  }
}

void WindowView::drawBandInfo() {
  GameInfo::BandInfo& info = gameInfo.bandInfo;
  int line0 = screenHeight - 90;
  int line1 = screenHeight - 65;
  int line2 = screenHeight - 40;
  drawFilledRectangle(0, line1 - 10, screenWidth - rightBarWidth, screenHeight, translucentBlack);
  string playerLine = info.name;
  drawText(white, 10, line1, playerLine);
  if (!myClock.isPaused())
    drawText(red, 10, line2, info.warning);
  else
    drawText(red, 10, line2, "PAUSED");

  drawText(white, 600, line2, "$ " + convertToString(info.numGold) + "    T:" + convertToString<int>(info.time));
  sf::Uint32 optionSyms[] = {L'⌂', 0x1f718, L'i', L'?'};
  optionButtons.clear();
  for (int i = 0; i < 4; ++i) {
    int w = 45;
    int line = topBarHeight - 20;
    int h = 45;
    int leftPos = screenWidth - rightBarWidth + 15;
    drawText(i < 2 ? symbolFont : textFont, 35, i == int(collectiveOption) ? green : white,
        leftPos + i * w, line, optionSyms[i], true);
    optionButtons.emplace_back(leftPos + i * w - w / 2, line,
        leftPos + (i + 1) * w - w / 2, line + h);
  }
  roomButtons.clear();
  int cnt = 0;
  creatureGroupButtons.clear();
  creatureButtons.clear();
  creatureNames.clear();
  teamButton = Nothing();
  cancelTeamButton = Nothing();
  chosenCreatures.clear();
  descriptionButton = Nothing();
  if (collectiveOption != CollectiveOption::MINIONS)
    chosenCreature = "";
  switch (collectiveOption) {
    case CollectiveOption::MINIONS: drawMinions(info); break;
    case CollectiveOption::BUILDINGS: drawBuildings(info); break;
    case CollectiveOption::KEY_MAPPING: drawKeeperHelp(); break;
    case CollectiveOption::LEGEND: break;
  }
}

void WindowView::refreshText() {
  int lineHeight = 25;
  int numMsg = 0;
  for (int i : All(currentMessage))
    if (!currentMessage[i].empty())
      numMsg = i + 1;
  drawFilledRectangle(0, 0, screenWidth, lineHeight * (numMsg + 1), translucentBlack);
  for (int i : All(currentMessage))
    drawText(oldMessage ? gray : white, 10, 10 + lineHeight * i, currentMessage[i]);
  switch (gameInfo.infoType) {
    case GameInfo::InfoType::PLAYER:
        drawPlayerInfo();
        break;
    case GameInfo::InfoType::BAND: drawBandInfo(); break;
  }
}

static vector<pair<string, string>> keyMapping {
  {"u", "leave minion"},
  {"i", "inventory"},
  {"e", "equipment"},
  {"pad 5 or enter", ""},
  {"", "pick up or"},
  {"", "interact"},
  {"", "with square"},
  {"d", "drop"},
  {"space", "wait"},
  {"a", "apply item"},
  {"t", "throw"},
  {"m", "history"},
  {"h", "hide"},
  {"c", "chat"},
  {"p", "pay"},
  {"ctrl + arrow", "travel"},
  {"alt + arrow", "fire"},
  {"z", "zoom"},
  {"shift + z", "world map"},
  {"F1", "legend"},
};

/*Vec2 WindowView::projectOnBorders(Rectangle area, Vec2 pos) {
  Vec2 center = Vec2((area.getPX() + area.getKX()) / 2, (area.getPY() + area.getKY()) / 2);
  Vec2 d = pos - center;
  if (d.x == 0) {
    return Vec2(center.x, d.y > 0 ? area.getKY() - 1 : area.getPY());
  }
  int cy = d.y * area.getW() / 2 / abs(d.x);
  if (center.y + cy >= area.getPY() && center.y + cy < area.getKY())
    return Vec2(d.x > 0 ? area.getKX() - 1 : area.getPX(), center.y + cy);
  int cx = d.x * area.getH() / 2 / abs(d.y);
  CHECK(center.x + cx >= area.getPX() && center.x + cx < area.getKX());
  return Vec2(center.x + cx, d.y > 0 ? area.getKY() - 1: area.getPY());
}*/

void WindowView::drawObjectAbs(int x, int y, const ViewObject& object, Color color, int sizeX, int sizeY) {
  Tile tile = getTile(object);
  if (tile.px >= 0) {
    drawSprite(x, y, tile.px * 32, tile.py * 32, 32, 32, tiles);
  }
  else {
    if (object.isPlayer()) {
      drawFilledRectangle(x, y, x + sizeX, y + sizeY, gray);
      int w = 1;
      drawFilledRectangle(x + w, y + w, x + sizeX - w, y + sizeY - w, black);
    }
    drawText(tile.symFont ? symbolFont : tileFont, sizeY + object.getSizeIncrease(), color,
        x + sizeX / 2, y - 3 - object.getSizeIncrease(), tile.text, true);
    if (object.getBurning() > 0) {
      drawText(symbolFont, sizeY, getFireColor(),
          x + sizeX / 2, y + sizeY - 3, L'ѡ', true);
      if (object.getBurning() > 0.5)
        drawText(symbolFont, sizeY, getFireColor(),
            x + sizeX / 2, y + sizeY - 3, L'Ѡ', true);
    }
  }
}

unordered_map<Vec2, ViewObject> objects;
const MapMemory* lastMemory = nullptr;
unordered_map<Vec2, Color> marked;

Color getHighlightColor(ViewIndex::HighlightInfo info) {
  switch (info.type) {
    case HighlightType::BUILD: return yellow;
    case HighlightType::POISON_GAS: return Color(0, info.amount * 255, 0);
  }
  Debug(FATAL) << "pokpok";
  return black;
}

void WindowView::resetCenter() {
  center = Vec2(0, 0);
}

void WindowView::refreshView(const CreatureView* collective) {
  const Level* level = collective->getLevel();
  collective->refreshGameInfo(gameInfo);
  objects.clear();
  marked.clear();
  if (center == Vec2(0, 0) || collective->staticPosition())
    center = collective->getPosition().mult(Vec2(mapLayout->squareWidth(Vec2(0, 0)),
          mapLayout->squareHeight(Vec2(0, 0))));
  Vec2 movePos = center - mouseOffset;
  movePos.x = max(movePos.x, 0);
  movePos.x = min(movePos.x, int(collective->getLevel()->getBounds().getKX() * mapLayout->squareWidth(Vec2(0, 0))));
  movePos.y = max(movePos.y, 0);
  movePos.y = min(movePos.y, int(collective->getLevel()->getBounds().getKY() * mapLayout->squareHeight(Vec2(0, 0))));
  mapLayout->updatePlayerPos(movePos);
  for (Vec2 pos : mapLayout->getAllTiles()) 
    if (level->inBounds(pos)) {
      ViewIndex index = collective->getViewIndex(pos);
      if (index.getHighlight())
        marked[pos] = getHighlightColor(*index.getHighlight());
      Optional<ViewObject> obj = index.getTopObject(mapLayout->getLayers());
      if (obj)
        objects.insert(std::make_pair(pos, *obj));
    }
  //  for (const Creature* c : player->getVisibleCreatures())
//    if (!projectOnScreen(c->getPosition()).inRectangle(mapWindowMargin))
//      objects.insert(std::make_pair(c->getPosition(), c->getViewObject()));
  lastMemory = &collective->getMemory(level);
  refreshScreen();
}

void WindowView::animateObject(vector<Vec2> trajectory, ViewObject object) {
  for (Vec2 pos : trajectory) {
    Optional<ViewObject> prev;
    if (objects.count(pos))
      prev = objects.at(pos);
    objects.erase(pos);
    objects.insert({pos, object});
    refreshScreen();
    // sf::sleep(sf::milliseconds(30));
    objects.erase(pos);
    if (prev)
      objects.insert({pos, *prev});
  }
  objects.erase(trajectory.back());
  objects.insert({trajectory.back(), object});
}

void WindowView::drawMap() {
  if (!lastMemory)
    return;

  Rectangle mapWindow = mapLayout->getBounds();
  drawFilledRectangle(mapWindow, black);
  bool pixelView = false;
  for (Vec2 wpos : mapLayout->getAllTiles()) {
    int sizeX = mapLayout->squareWidth(wpos);
    int sizeY = mapLayout->squareHeight(wpos);
    Vec2 pos = mapLayout->projectOnScreen(wpos, 0);
    if (marked.count(wpos))
      drawFilledRectangle(pos.x, pos.y, pos.x + sizeX, pos.y + sizeY, marked.at(wpos));
    if (lastMemory->hasViewIndex(wpos) && !objects.count(wpos)) {
      Optional<ViewObject> object = lastMemory->getViewIndex(wpos).getTopObject(mapLayout->getLayers());
      if (object) {
        if (sizeX > 1)
          drawObjectAbs(pos.x, pos.y, *object, getMemoryColor(*object), sizeX, sizeY);
        else {
          mapBuffer.setPixel(pos.x, pos.y, getColor(*object));
          pixelView = true;
        }
      }
    }
  }
  map<string, ViewObject> objIndex;
  for (auto elem : objects) {
    objIndex.insert(std::make_pair(elem.second.getDescription(), elem.second));
    Vec2 wpos = elem.first;
    int sizeX = mapLayout->squareWidth(wpos);
    int sizeY = mapLayout->squareHeight(wpos);
    Vec2 pos = mapLayout->projectOnScreen(wpos, elem.second.getHeight());
    if (sizeX > 1)
      drawObjectAbs(pos.x, pos.y,
          elem.second, getColor(elem.second), sizeX, sizeY);
    else {
      mapBuffer.setPixel(pos.x, pos.y, getColor(elem.second));
      pixelView = true;
    }
  //  }
  /* else  if (elem.second.layer() == ViewLayer::CREATURE && mapView->font != nullptr) {
      Vec2 proj = projectOnBorders(mapWindow, pos);
      drawObjectAbs(
          mapView->mapX + proj.x * mapView->squareWidth,
          mapView->mapY + proj.y * mapView->squareHeight,
          elem.second, getColor(elem.second), &smallFont, &smallSymbolFont);
    }*/
  }
  if (pixelView)
    drawImage(mapLayout->getBounds().getPX(), mapLayout->getBounds().getPY(), mapBuffer);
  int rightPos = screenWidth - rightBarWidth;
  drawFilledRectangle(rightPos - 10, topBarHeight - 10, screenWidth, screenHeight, translucentBlack);
  if (gameInfo.infoType == GameInfo::InfoType::PLAYER || collectiveOption == CollectiveOption::LEGEND) {
    int cnt = 0;
    if (legendOption == LegendOption::OBJECTS) {
      for (auto elem : objIndex) {
        drawViewObject(elem.second, rightPos, legendStartHeight + cnt * 25);
        drawText(white, rightPos + 30, legendStartHeight + cnt * 25, elem.first);
        ++cnt;
      }
      string f1text;
      if (gameInfo.infoType == GameInfo::InfoType::PLAYER)
        f1text = "F1 for help";
      drawText(lightBlue, rightPos, legendStartHeight + cnt * 25 + 25, f1text);
    } else {
      int i = 0;
      for (auto elem : keyMapping) {
        if (elem.first.size() > 0)
          drawText(lightBlue, rightPos - 5, legendStartHeight + i * 21, "[" + elem.first + "]");
        drawText(lightBlue, rightPos + 110, legendStartHeight + i * 21, elem.second);
        ++i;
      }
    }
  }
  refreshText();

//}
}

void WindowView::refreshScreen(bool flipBuffer) {
  drawMap();
  if (flipBuffer)
    drawAndClearBuffer();
}

int indexHeight(const vector<string>& options, int index) {
  CHECK(index < options.size() && index >= 0);
  int tmp = 0;
  for (int i : All(options))
    if (!View::hasTitlePrefix(options[i]) && tmp++ == index)
      return i;
  Debug(FATAL) << "Bad index " << options << " " << index;
  return -1;
}

Optional<Vec2> WindowView::chooseDirection(const string& message) {
  showMessage(message);
  refreshScreen();
  do {
    auto key = readkey();
    if (key)
      switch (key->code) {
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

bool WindowView::yesOrNoPrompt(const string& message) {
  showMessage(message + " (y/n)");
  refreshScreen();
  do {
    auto key = readkey();
    if (key)
      switch (key->code) {
        case Keyboard::Y: showMessage(""); refreshScreen(); return true;
        case Keyboard::Escape:
        case Keyboard::Space:
        case Keyboard::N: showMessage(""); refreshScreen(); return false;
        default: break;
      }
  } while (1);
}

Optional<int> WindowView::getNumber(const string& title, int max) {
  vector<string> options;
  for (int i : Range(1, max + 1))
    options.push_back(convertToString(i));
  Optional<int> res = WindowView::chooseFromList(title, options);
  if (!res)
    return Nothing();
  else
    return 1 + *res;
}

int getMaxLines();

Optional<int> WindowView::chooseFromList(const string& title, const vector<string>& options, int index) {
  if (options.size() == 0)
    return Nothing();
  int numLines = min((int) options.size(), getMaxLines());
  int count = 0;
  for (string s : options)
    if (!hasTitlePrefix(s))
      ++count;
  while (1) {
    numLines = min((int) options.size(), getMaxLines());
    int height = indexHeight(options, index);
    int cutoff = min(max(0, height - numLines / 2), (int) options.size() - numLines);
    int itemsCutoff = 0;
    for (int i : Range(cutoff))
      if (!hasTitlePrefix(options[i]))
        ++itemsCutoff;
    vector<string> window = getPrefix(options, cutoff , numLines);
    drawList(title, window, index - itemsCutoff);
    auto key = readkey();
    if (key)
      switch (key->code) {
        case Keyboard::PageDown: index += 10; if (index >= count) index = count - 1; break;
        case Keyboard::PageUp: index -= 10; if (index < 0) index = 0; break;
        case Keyboard::Numpad8:
        case Keyboard::Up: if (index > 0) --index; break;
        case Keyboard::Numpad2:
        case Keyboard::Down: if (index < count - 1) ++index; break;
        case Keyboard::Numpad5:
        case Keyboard::Return : clearMessageBox(); /*showMessage("");*/ return index;
        case Keyboard::Escape : clearMessageBox();/* showMessage("");*/ return Nothing();
        default: break;
      }
  }
  Debug(FATAL) << "Very bad";
  return 123;
}

void WindowView::presentText(const string& title, const string& text) {
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
  presentList(title, rows, false);
}

void WindowView::presentList(const string& title, const vector<string>& options, bool scrollDown) {
  int numLines = min((int) options.size(), getMaxLines());
  if (numLines == 0)
    return;
  int index = scrollDown ? options.size() - numLines : 0;
  while (1) {
    numLines = min((int) options.size(), getMaxLines());
    vector<string> window = getPrefix(options, index, numLines);
    drawList(title, window, -1);
    auto key = readkey();
    if (key)
      switch (key->code) {
        case Keyboard::Numpad8:
        case Keyboard::Up: if (index > 0) --index; break;
        case Keyboard::Numpad2:
        case Keyboard::Down: if (index < options.size() - numLines) ++index; break;
        case Keyboard::Return : refreshScreen(); return;
        case Keyboard::Escape : refreshScreen(); return;
        default: break;
      }
  }
  refreshScreen();
}

static int yMargin = 10;
static int itemYMargin = 20;
static int itemSpacing = 25;
static int ySpacing = 100;

int getMaxLines() {
  return (screenHeight - ySpacing - ySpacing - 2 * yMargin - itemSpacing - itemYMargin) / itemSpacing;
}

void WindowView::drawList(const string& title, const vector<string>& options, int hightlight) {
  int xMargin = 10;
  int itemXMargin = 30;
  int border = 2;
  int h = 0;
  int width = 800;
  int xSpacing = (screenWidth - width) / 2;
  refreshScreen(false);
  if (hightlight > -1) 
    h = indexHeight(options, hightlight);
  //drawFilledRectangle(xSpacing, ySpacing, screenWidth - xSpacing, screenHeight - ySpacing, white);
  drawFilledRectangle(xSpacing, ySpacing, xSpacing + width, screenHeight - ySpacing, translucentBlack, white);
  drawText(white, xSpacing + xMargin, ySpacing + yMargin, title);
  if (hightlight > -1) 
    drawFilledRectangle(xSpacing + xMargin, ySpacing + yMargin + (h + 1) * itemSpacing + itemYMargin, 
        width + xSpacing - xMargin, ySpacing + yMargin + (h + 2) * itemSpacing + itemYMargin -1, darkGreen);
  for (int i : All(options)) {  
    if (!hasTitlePrefix(options[i]))
      drawText(white, xSpacing + xMargin + itemXMargin, ySpacing + yMargin + (i + 1) * itemSpacing + itemYMargin, options[i]);
    else
      drawText(yellow, xSpacing + xMargin, ySpacing + yMargin + (i + 1) * itemSpacing + itemYMargin, removeTitlePrefix(options[i]));

  }
  drawAndClearBuffer();
}

void WindowView::drawAndClearBuffer() {
  display->display();
  display->clear(black);
}



void WindowView::addImportantMessage(const string& message) {
  presentText("Important!", message);
}

void WindowView::clearMessages() { 
  showMessage("");
}

void WindowView::addMessage(const string& message) {
  if (oldMessage)
    showMessage("");
  if (currentMessage[messageInd].size() + message.size() + 1 > maxMsgLength &&
      messageInd == currentMessage.size() - 1) {
    currentMessage[messageInd] += " (more)";
    refreshScreen();
    while (1) {
      auto key = readkey();
      if (key && (key->code == Keyboard::Space || key->code == Keyboard::Return))
        break;
    }
    showMessage(message);
  } else {
    if (currentMessage[messageInd].size() + message.size() + 1 > maxMsgLength)
      ++messageInd;
    currentMessage[messageInd] += (currentMessage[messageInd].size() > 0 ? " " : "") + message;
    refreshScreen();
  }
}

void WindowView::unzoom(bool pixel, bool tpp) {
  if (pixel && mapLayout != worldLayout)
    mapLayout = worldLayout;
 /* else if (tpp && mapLayout != tppLayout)
    mapLayout = tppLayout;*/
  else if (mapLayout != normalLayout)
    mapLayout = normalLayout;
  else
    mapLayout = unzoomLayout;
  refreshScreen();
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

bool pressed = false;
bool rightPressed = false;

bool WindowView::considerScrollEvent(sf::Event& event) {
  switch (event.type) {
    case Event::MouseButtonReleased :
      if (event.mouseButton.button == sf::Mouse::Right) {
        center -= mouseOffset;
        mouseOffset = Vec2(0, 0);
        rightPressed = false;
        return true;
      }
      break;
    case Event::MouseMoved:
      if (rightPressed) {
        mouseOffset = Vec2(event.mouseMove.x, event.mouseMove.y) - lastMousePos;
        return true;
      }
      break;
    case Event::MouseButtonPressed:
      if (event.mouseButton.button == sf::Mouse::Right) {
        rightPressed = true;
        lastMousePos = Vec2(event.mouseButton.x, event.mouseButton.y);
        return true;
      }
      break;
    default:
      break;
  }
  return false;
}

Vec2 lastGoTo(-1, -1);
CollectiveAction WindowView::getClick() {
  Event event;
  while (display->pollEvent(event)) {
    considerScrollEvent(event);
    switch (event.type) {
      case Event::KeyPressed:
        switch (event.key.code) {
          case Keyboard::Z:
            unzoom(false, false);
            center = Vec2(0, 0);
            return CollectiveAction(CollectiveAction::IDLE);
          case Keyboard::Space:
            if (!myClock.isPaused())
              myClock.pause();
            else
              myClock.cont();
            return CollectiveAction(CollectiveAction::IDLE);
          case Keyboard::Escape:
            if (yesOrNoPrompt("Are you sure you want to quit?"))
              exit(0);
            break;
 /*         case Keyboard::F1:
            collectiveOption = CollectiveOption((1 + int(collectiveOption)) % 4);
            break;*/
          default:
            break;
        }
        break;
      case Event::MouseButtonReleased :
          if (event.mouseButton.button == sf::Mouse::Left) {
            pressed = false;
            return CollectiveAction(CollectiveAction::BUTTON_RELEASE);
          }
          break;
      case Event::MouseMoved:
          if (pressed) {
            Vec2 goTo = mapLayout->projectOnMap(Vec2(event.mouseMove.x, event.mouseMove.y));
            if (goTo != lastGoTo ) {
              return CollectiveAction(CollectiveAction::GO_TO, goTo);
              lastGoTo = goTo;
            } else
              continue;
          }
          break;
      case Event::MouseButtonPressed: {
          CollectiveAction::Type t;
          Vec2 clickPos(event.mouseButton.x, event.mouseButton.y);
          if (event.mouseButton.button == sf::Mouse::Right)
            chosenCreature = "";
          if (event.mouseButton.button == sf::Mouse::Left) {
            if (teamButton && clickPos.inRectangle(*teamButton))
              return CollectiveAction(CollectiveAction::GATHER_TEAM);
            if (cancelTeamButton && clickPos.inRectangle(*cancelTeamButton)) {
              chosenCreature = "";
              return CollectiveAction(CollectiveAction::CANCEL_TEAM);
            }
            for (int i : All(optionButtons))
              if (clickPos.inRectangle(optionButtons[i]))
                  collectiveOption = (CollectiveOption) i;
            for (int i : All(roomButtons))
              if (clickPos.inRectangle(roomButtons[i])) {
                chosenCreature = "";
                return CollectiveAction(CollectiveAction::ROOM_BUTTON, i);
              }
            for (int i : All(creatureGroupButtons))
              if (clickPos.inRectangle(creatureGroupButtons[i])) {
                if (chosenCreature == creatureNames[i])
                  chosenCreature = "";
                else
                  chosenCreature = creatureNames[i];
                return CollectiveAction(CollectiveAction::IDLE);
              }
            for (int i : All(creatureButtons))
              if (clickPos.inRectangle(creatureButtons[i])) {
                return CollectiveAction(CollectiveAction::CREATURE_BUTTON, chosenCreatures[i]);
              }
            if (descriptionButton && clickPos.inRectangle(*descriptionButton)) {
              return CollectiveAction(CollectiveAction::CREATURE_DESCRIPTION, chosenCreatures[0]);
            }
            pressed = true;
            chosenCreature = "";
            t = CollectiveAction::GO_TO;
            return CollectiveAction(t, mapLayout->projectOnMap(clickPos));
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

Action WindowView::getAction() {
  while (1) {
    auto key = readkey();
    oldMessage = true;
    if (!key)
      return Action(ActionId::IDLE);
    auto optionalAction = mapLayout->overrideAction(*key);
    if (optionalAction)
      return *optionalAction;
    switch (key->code) {
      case Keyboard::Escape : if (yesOrNoPrompt("Are you sure you want to quit?")) exit(0); break;
      case Keyboard::Z: unzoom(key->shift, key->control); return Action(ActionId::IDLE);
      case Keyboard::F1: legendOption = (LegendOption)(1 - (int)legendOption); return Action(ActionId::IDLE);
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
      case Keyboard::Return:
      case Keyboard::Numpad5: if (key->shift) return Action(ActionId::EXT_PICK_UP); 
                                else return Action(ActionId::PICK_UP);
      case Keyboard::I: return Action(ActionId::SHOW_INVENTORY);
      case Keyboard::D: return Action(key->shift ? ActionId::EXT_DROP : ActionId::DROP);
      case Keyboard::Space:
      case Keyboard::Period: return Action(ActionId::WAIT);
      case Keyboard::A: return Action(ActionId::APPLY_ITEM);
      case Keyboard::E: return Action(ActionId::EQUIPMENT);
      case Keyboard::T: return Action(ActionId::THROW);
      case Keyboard::M: return Action(ActionId::SHOW_HISTORY);
      case Keyboard::H: return Action(ActionId::HIDE);
      case Keyboard::P: return Action(ActionId::PAY_DEBT);
      case Keyboard::C: return Action(ActionId::CHAT);
      case Keyboard::U: return Action(ActionId::UNPOSSESS);
      default: break;
    }
  }
}

