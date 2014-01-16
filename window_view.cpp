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

class Tile {
  public:
  Color color;
  String text;
  bool symFont = false;
  Optional<Tile> background;
  bool translucent = false;
  Tile(sf::Uint32 ch, Color col, bool sym = false) : color(col), text(ch), symFont(sym) {
  }
  Tile(int x, int y, int num = 0) : tileCoord(Vec2(x, y)), texNum(num) {}
  Tile(int x, int y, int num, Tile bg) : background(bg), tileCoord(Vec2(x, y)), texNum(num) {}

  Tile& addConnection(set<Dir> c, int x, int y) {
    connections.insert({c, Vec2(x, y)});
    return *this;
  }

  Tile& setTranslucent() {
    translucent = true;
    return *this;
  }

  bool hasSpriteCoord() {
    return tileCoord;
  }

  Vec2 getSpriteCoord() {
    return *tileCoord;
  }

  Vec2 getSpriteCoord(set<Dir> c) {
    if (connections.count(c))
      return connections.at(c);
    else return *tileCoord;
  }

  int getTexNum() {
    CHECK(tileCoord) << "Not a sprite tile";
    return texNum;
  }

  private:
  Optional<Vec2> tileCoord;
  int texNum = 0;
  unordered_map<set<Dir>, Vec2> connections;
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

Tile getSpecialCreatureSprite(const ViewObject& obj, bool humanoid) {
  RandomGen r;
  r.init(std::hash<string>()(obj.getBareDescription()));
  if (humanoid)
    return Tile(r.getRandom(7), 10);
  else
    return Tile(r.getRandom(7, 14), 10);
}

Tile getSprite(ViewId id);

Tile getRoadTile(int pathSet, ViewId background) {
  return Tile(0, pathSet, 5, getSprite(background))
    .addConnection({Dir::E, Dir::W}, 2, pathSet)
    .addConnection({Dir::W}, 3, pathSet)
    .addConnection({Dir::E}, 1, pathSet)
    .addConnection({Dir::S}, 4, pathSet)
    .addConnection({Dir::N, Dir::S}, 5, pathSet)
    .addConnection({Dir::N}, 6, pathSet)
    .addConnection({Dir::S, Dir::E}, 7, pathSet)
    .addConnection({Dir::S, Dir::W}, 8, pathSet)
    .addConnection({Dir::N, Dir::E}, 9, pathSet)
    .addConnection({Dir::N, Dir::W}, 10, pathSet)
    .addConnection({Dir::N, Dir::E, Dir::S, Dir::W}, 11, pathSet)
    .addConnection({Dir::E, Dir::S, Dir::W}, 12, pathSet)
    .addConnection({Dir::N, Dir::S, Dir::W}, 13, pathSet)
    .addConnection({Dir::N, Dir::E, Dir::S}, 14, pathSet)
    .addConnection({Dir::N, Dir::E, Dir::W}, 15, pathSet);
}

Tile getWallTile(int wallSet, Optional<ViewId> background = Nothing()) {
  return (background ? Tile(9, wallSet, 1, getSprite(*background)) : Tile(9, wallSet, 1))
    .addConnection({Dir::E, Dir::W}, 11, wallSet)
    .addConnection({Dir::W}, 12, wallSet)
    .addConnection({Dir::E}, 10, wallSet)
    .addConnection({Dir::S}, 13, wallSet)
    .addConnection({Dir::N, Dir::S}, 14, wallSet)
    .addConnection({Dir::N}, 15, wallSet)
    .addConnection({Dir::E, Dir::S}, 16, wallSet)
    .addConnection({Dir::S, Dir::W}, 17, wallSet)
    .addConnection({Dir::N, Dir::E}, 18, wallSet)
    .addConnection({Dir::N, Dir::W}, 19, wallSet)
    .addConnection({Dir::N, Dir::E, Dir::S, Dir::W}, 20, wallSet)
    .addConnection({Dir::E, Dir::S, Dir::W}, 21, wallSet)
    .addConnection({Dir::N, Dir::S, Dir::W}, 22, wallSet)
    .addConnection({Dir::N, Dir::E, Dir::S}, 23, wallSet)
    .addConnection({Dir::N, Dir::E, Dir::W}, 24, wallSet);
}

Tile getWaterTile(int leftX) {
  return Tile(leftX, 5, 4)
    .addConnection({Dir::N, Dir::E, Dir::S, Dir::W}, leftX - 1, 7)
    .addConnection({Dir::E, Dir::S, Dir::W}, leftX, 4)
    .addConnection({Dir::N, Dir::E, Dir::W}, leftX, 6)
    .addConnection({Dir::N, Dir::S, Dir::W}, leftX + 1, 5)
    .addConnection({Dir::N, Dir::E, Dir::S}, leftX - 1, 5)
    .addConnection({Dir::N, Dir::E}, leftX - 1, 6)
    .addConnection({Dir::E, Dir::S}, leftX - 1, 4)
    .addConnection({Dir::S, Dir::W}, leftX + 1, 4)
    .addConnection({Dir::N, Dir::W}, leftX + 1, 6)
    .addConnection({Dir::S}, leftX, 7)
    .addConnection({Dir::N}, leftX, 8)
    .addConnection({Dir::W}, leftX + 1, 7)
    .addConnection({Dir::E}, leftX + 1, 8)
    .addConnection({Dir::N, Dir::S}, leftX + 1, 12)
    .addConnection({Dir::E, Dir::W}, leftX, 11);
}

Tile getSprite(ViewId id) {
  switch (id) {
    case ViewId::PLAYER: return Tile(1, 0);
    case ViewId::UNKNOWN_MONSTER: return Tile('?', lightGreen);
    case ViewId::SPECIAL_BEAST: return Tile(7, 10);
    case ViewId::SPECIAL_HUMANOID: return Tile(6, 10);
    case ViewId::ELF: return Tile(12, 6);
    case ViewId::ELF_CHILD: return Tile(14, 6);
    case ViewId::ELF_LORD: return Tile(13, 6);
    case ViewId::ELVEN_SHOPKEEPER: return Tile(4, 2);
    case ViewId::IMP: return Tile(18, 0);
    case ViewId::BILE_DEMON: return Tile(8, 14);
    case ViewId::HELL_HOUND: return Tile(14, 12);
    case ViewId::CHICKEN: return Tile(18, 1);
    case ViewId::DWARF: return Tile(2, 6);
    case ViewId::DWARF_BARON: return Tile(3, 6);
    case ViewId::DWARVEN_SHOPKEEPER: return Tile(4, 2);
    case ViewId::BRIDGE: return Tile(24, 0, 4);
    case ViewId::HILL_ROAD: return getRoadTile(7, ViewId::HILL);
    case ViewId::GRASS_ROAD: return getRoadTile(7, ViewId::GRASS);
    case ViewId::PATH:
    case ViewId::FLOOR: return Tile(3, 14, 1);
    case ViewId::SAND: return Tile(7, 12, 2);
    case ViewId::MUD: return Tile(3, 12, 2);
    case ViewId::GRASS: return Tile(0, 13, 2);
    case ViewId::WALL: return getWallTile(2);
    case ViewId::MOUNTAIN: return Tile(17, 2, 2, getSprite(ViewId::HILL));
 /*                                 .addConnection({Dir::N, Dir::E, Dir::S, Dir::W, 
                                        Dir::NE, Dir::SE, Dir::SW, Dir::NW}, 19, 10)
                                  .addConnection({Dir::E, Dir::S, Dir::W, Dir::SE, Dir::SW}, 19, 9)
                                  .addConnection({Dir::N, Dir::E, Dir::W, Dir::NW, Dir::NE}, 19, 11)
                                  .addConnection({Dir::N, Dir::S, Dir::W, Dir::NW, Dir::SW}, 20, 10)
                                  .addConnection({Dir::N, Dir::E, Dir::S, Dir::NE, Dir::SE}, 18, 10)
                                  .addConnection({Dir::S, Dir::W, Dir::SW}, 20, 9)
                                  .addConnection({Dir::E, Dir::S, Dir::SE}, 18, 9)
                                  .addConnection({Dir::N, Dir::W, Dir::NW}, 20, 11)
                                  .addConnection({Dir::N, Dir::E, Dir::NE}, 18, 11);*/
    case ViewId::GOLD_ORE: return Tile(0, 16, 1);
    case ViewId::SNOW: return Tile(16, 2, 2, getSprite(ViewId::HILL));
    case ViewId::HILL: return Tile(3, 13, 2);
    case ViewId::WOOD_WALL: return getWallTile(4);
    case ViewId::BLACK_WALL: return getWallTile(2);
    case ViewId::YELLOW_WALL: return getWallTile(8);
    case ViewId::LOW_ROCK_WALL: return getWallTile(21, ViewId::PATH);
    case ViewId::HELL_WALL: return getWallTile(22, ViewId::PATH);
    case ViewId::CASTLE_WALL: return getWallTile(5);
    case ViewId::MUD_WALL: return getWallTile(13);
    case ViewId::SECRETPASS: return Tile(0, 15, 1);
    case ViewId::DUNGEON_ENTRANCE: return Tile(15, 2, 2, getSprite(ViewId::HILL));
    case ViewId::DUNGEON_ENTRANCE_MUD: return Tile(19, 2, 2, getSprite(ViewId::MUD));
    case ViewId::DOWN_STAIRCASE: return Tile(8, 0, 1);
    case ViewId::UP_STAIRCASE: return Tile(7, 0, 1, getSprite(ViewId::FLOOR));
    case ViewId::DOWN_STAIRCASE_CELLAR: return Tile(8, 21, 1);
    case ViewId::UP_STAIRCASE_CELLAR: return Tile(7, 21, 1, getSprite(ViewId::FLOOR));
    case ViewId::DOWN_STAIRCASE_HELL: return Tile(8, 1, 1);
    case ViewId::UP_STAIRCASE_HELL: return Tile(7, 22, 1, getSprite(ViewId::FLOOR));
    case ViewId::DOWN_STAIRCASE_PYR: return Tile(8, 8, 1);
    case ViewId::UP_STAIRCASE_PYR: return Tile(7, 8, 1, getSprite(ViewId::FLOOR));
    case ViewId::GREAT_GOBLIN: return Tile(6, 14);
    case ViewId::GOBLIN: return Tile(5, 14);
    case ViewId::BANDIT: return Tile(0, 2);
    case ViewId::GHOST: return Tile(6, 16);
    case ViewId::DEVIL: return Tile(17, 18);
    case ViewId::DARK_KNIGHT: return Tile(12, 14);
    case ViewId::DRAGON: return Tile(3, 18);
    case ViewId::CYCLOPS: return Tile(10, 14);
    case ViewId::KNIGHT: return Tile(0, 0);
    case ViewId::CASTLE_GUARD: return Tile(15, 2);
    case ViewId::AVATAR: return Tile(9, 0);
    case ViewId::ARCHER: return Tile(2, 0);
    case ViewId::PESEANT: return Tile(1, 2);
    case ViewId::CHILD: return Tile(2, 2);
    case ViewId::ZOMBIE: return Tile(0, 16);
    case ViewId::VAMPIRE: return Tile(12, 16);
    case ViewId::MUMMY: return Tile(7, 16);
    case ViewId::MUMMY_LORD: return Tile(8, 16);
    case ViewId::JACKAL: return Tile(12, 12);
    case ViewId::DEER: return Tile(18, 4);
    case ViewId::HORSE: return Tile(18, 2);
    case ViewId::COW: return Tile(18, 3);
    case ViewId::SHEEP: return Tile('s', white);
    case ViewId::PIG: return Tile(18, 5);
    case ViewId::BOAR: return Tile(18, 6);
    case ViewId::FOX: return Tile(13, 12);
    case ViewId::WOLF: return Tile(14, 12);
    case ViewId::VODNIK: return Tile('f', green);
    case ViewId::KRAKEN: return Tile(7, 19);
    case ViewId::DEATH: return Tile(9, 16);
    case ViewId::KRAKEN2: return Tile(7, 19);
    case ViewId::NIGHTMARE: return Tile(9, 16);
    case ViewId::FIRE_SPHERE: return Tile(16, 20);
    case ViewId::BEAR: return Tile(12, 18);
    case ViewId::BAT: return Tile(2, 12);
    case ViewId::GNOME: return Tile(13, 8);
    case ViewId::LEPRECHAUN: return Tile(16, 8);
    case ViewId::RAT: return Tile(7, 12);
    case ViewId::SPIDER: return Tile(6, 12);
    case ViewId::FLY: return Tile(10, 12);
    case ViewId::SCORPION: return Tile(11, 18);
    case ViewId::SNAKE: return Tile(9, 12);
    case ViewId::VULTURE: return Tile(17, 12);
    case ViewId::BODY_PART: return Tile(9, 4, 3);
    case ViewId::BONE: return Tile(3, 0, 2);
    case ViewId::BUSH: return Tile(15, 3, 2, getSprite(ViewId::GRASS));
    case ViewId::MOUNTAIN_BUSH: return Tile(15, 3, 2, getSprite(ViewId::HILL));
    case ViewId::DECID_TREE: return Tile(21, 3, 2, getSprite(ViewId::HILL));
    case ViewId::CANIF_TREE: return Tile(20, 3, 2, getSprite(ViewId::GRASS));
    case ViewId::WATER: return getWaterTile(5);
    case ViewId::MAGMA: return getWaterTile(11);
    case ViewId::ABYSS: return Tile('~', darkGray);
    case ViewId::DOOR: return Tile(4, 2, 2, getSprite(ViewId::FLOOR));
    case ViewId::SWORD: return Tile(12, 9, 3);
    case ViewId::SPECIAL_SWORD: return Tile(13, 9, 3);
    case ViewId::ELVEN_SWORD: return Tile(14, 9, 3);
    case ViewId::KNIFE: return Tile(20, 9, 3);
    case ViewId::WAR_HAMMER: return Tile(10, 7, 3);
    case ViewId::SPECIAL_WAR_HAMMER: return Tile(11, 7, 3);
    case ViewId::BATTLE_AXE: return Tile(13, 7, 3);
    case ViewId::SPECIAL_BATTLE_AXE: return Tile(21, 7, 3);
    case ViewId::BOW: return Tile(14, 8, 3);
    case ViewId::ARROW: return Tile(5, 8, 3);
    case ViewId::SCROLL: return Tile(3, 6, 3);
    case ViewId::STEEL_AMULET: return Tile(1, 1, 3);
    case ViewId::COPPER_AMULET: return Tile(2, 1, 3);
    case ViewId::CRYSTAL_AMULET: return Tile(4, 1, 3);
    case ViewId::WOODEN_AMULET: return Tile(0, 1, 3);
    case ViewId::AMBER_AMULET: return Tile(3, 1, 3);
    case ViewId::BOOK: return Tile(0, 3, 3);
    case ViewId::FIRST_AID: return Tile(12, 2, 3);
    case ViewId::TRAP_ITEM: return Tile(12, 4, 3);
    case ViewId::EFFERVESCENT_POTION: return Tile(6, 0, 3);
    case ViewId::MURKY_POTION: return Tile(10, 0, 3);
    case ViewId::SWIRLY_POTION: return Tile(9, 0, 3);
    case ViewId::VIOLET_POTION: return Tile(7, 0, 3);
    case ViewId::PUCE_POTION: return Tile(8, 0, 3);
    case ViewId::SMOKY_POTION: return Tile(11, 0, 3);
    case ViewId::FIZZY_POTION: return Tile(9, 0, 3);
    case ViewId::MILKY_POTION: return Tile(11, 0, 3);
    case ViewId::PINK_MUSHROOM:
    case ViewId::DOTTED_MUSHROOM:
    case ViewId::GLOWING_MUSHROOM:
    case ViewId::GREEN_MUSHROOM:
    case ViewId::BLACK_MUSHROOM:
    case ViewId::SLIMY_MUSHROOM: return Tile(5, 4, 3);
    case ViewId::FOUNTAIN: return Tile(0, 7, 2, getSprite(ViewId::FLOOR));
    case ViewId::GOLD: return Tile(8, 3, 3);
    case ViewId::CHEST: return Tile(3, 3, 2, getSprite(ViewId::FLOOR));
    case ViewId::OPENED_CHEST: return Tile(6, 3, 2, getSprite(ViewId::FLOOR));
    case ViewId::COFFIN: return Tile(7, 3, 2, getSprite(ViewId::FLOOR));
    case ViewId::OPENED_COFFIN: return Tile(8, 3, 2, getSprite(ViewId::FLOOR));
    case ViewId::BOULDER: return Tile(18, 7);
    case ViewId::UNARMED_BOULDER_TRAP: return Tile(18, 7).setTranslucent();
    case ViewId::PORTAL: return Tile(1, 6, 2);
    case ViewId::TRAP: return Tile(L'➹', yellow, true);
    case ViewId::GAS_TRAP: return Tile(L'☠', green, true);
    case ViewId::UNARMED_GAS_TRAP: return Tile(L'☠', lightGray, true);
    case ViewId::ROCK: return Tile(6, 1, 3);
    case ViewId::BED: return Tile(5, 4, 2, getSprite(ViewId::FLOOR));
    case ViewId::DUNGEON_HEART: return Tile(6, 10, 2);
    case ViewId::ALTAR: return Tile(2, 7, 2, getSprite(ViewId::FLOOR));
    case ViewId::TORTURE_TABLE: return Tile(1, 5, 2, getSprite(ViewId::FLOOR));
    case ViewId::TRAINING_DUMMY: return Tile(0, 5, 2, getSprite(ViewId::FLOOR));
    case ViewId::WORKSHOP: return Tile(9, 4, 2, getSprite(ViewId::FLOOR));
    case ViewId::GRAVE: return Tile(0, 0, 2, getSprite(ViewId::FLOOR));
    case ViewId::BARS: return Tile(L'⧻', lightBlue);
    case ViewId::BORDER_GUARD: return Tile(' ', white);
    case ViewId::LEATHER_ARMOR: return Tile(0, 12, 3);
    case ViewId::LEATHER_HELM: return Tile(10, 12, 3);
    case ViewId::TELEPATHY_HELM: return Tile(17, 12, 3);
    case ViewId::CHAIN_ARMOR: return Tile(1, 12, 3);
    case ViewId::IRON_HELM: return Tile(14, 12, 3);
    case ViewId::LEATHER_BOOTS: return Tile(0, 13, 3);
    case ViewId::IRON_BOOTS: return Tile(6, 13, 3);
    case ViewId::SPEED_BOOTS: return Tile(3, 13, 3);
    case ViewId::DESTROYED_FURNITURE: return Tile('*', brown);
    case ViewId::BURNT_FURNITURE: return Tile('*', darkGray);
    case ViewId::FALLEN_TREE: return Tile('*', green);
    case ViewId::BURNT_TREE: return Tile('*', darkGray);
    case ViewId::GUARD_POST: return Tile(L'⚐', yellow, true);
  }
  Debug(FATAL) << "unhandled view id " << (int)id;
  return Tile(' ', white);
}

Tile getSpriteTile(const ViewObject& obj) {
  if (obj.id() == ViewId::SPECIAL_BEAST)
    return getSpecialCreatureSprite(obj, false);
  if (obj.id() == ViewId::SPECIAL_HUMANOID)
    return getSpecialCreatureSprite(obj, true);
  return getSprite(obj.id());
}

Tile getAsciiTile(const ViewObject& obj) {
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
    case ViewId::BILE_DEMON: return Tile('O', green);
    case ViewId::HELL_HOUND: return Tile('d', purple);
    case ViewId::CHICKEN: return Tile('c', yellow);
    case ViewId::DWARF: return Tile('h', blue);
    case ViewId::DWARF_BARON: return Tile('h', darkBlue);
    case ViewId::DWARVEN_SHOPKEEPER: return Tile('h', lightBlue);
    case ViewId::FLOOR: return Tile('.', white);
    case ViewId::BRIDGE: return Tile('_', brown);
    case ViewId::GRASS_ROAD: return Tile('.', lightGray);
    case ViewId::HILL_ROAD: return Tile('.', lightGray);
    case ViewId::PATH: return Tile('.', lightGray);
    case ViewId::SAND: return Tile('.', yellow);
    case ViewId::MUD: return Tile(0x1d0f0, brown, true);
    case ViewId::GRASS: return Tile(0x1d0f0, green, true);
    case ViewId::CASTLE_WALL: return Tile('#', lightGray);
    case ViewId::MUD_WALL: return Tile('#', lightBrown);
    case ViewId::WALL: return Tile('#', lightGray);
    case ViewId::MOUNTAIN: return Tile(0x25ee, darkGray, true);
    case ViewId::GOLD_ORE: return Tile(L'⁂', yellow, true);
    case ViewId::SNOW: return Tile(0x25ee, white, true);
    case ViewId::HILL: return Tile(0x1d022, darkGreen, true);
    case ViewId::WOOD_WALL: return Tile('#', darkBrown);
    case ViewId::BLACK_WALL: return Tile('#', lightGray);
    case ViewId::YELLOW_WALL: return Tile('#', yellow);
    case ViewId::LOW_ROCK_WALL: return Tile('#', darkGray);
    case ViewId::HELL_WALL: return Tile('#', red);
    case ViewId::SECRETPASS: return Tile('#', lightGray);
    case ViewId::DUNGEON_ENTRANCE:
    case ViewId::DUNGEON_ENTRANCE_MUD: Tile(0x2798, brown, true);
    case ViewId::DOWN_STAIRCASE_CELLAR:
    case ViewId::DOWN_STAIRCASE: return Tile(0x2798, almostWhite, true);
    case ViewId::UP_STAIRCASE_CELLAR:
    case ViewId::UP_STAIRCASE: return Tile(0x279a, almostWhite, true);
    case ViewId::DOWN_STAIRCASE_HELL: return Tile(0x2798, red, true);
    case ViewId::UP_STAIRCASE_HELL: return Tile(0x279a, red, true);
    case ViewId::DOWN_STAIRCASE_PYR: return Tile(0x2798, yellow, true);
    case ViewId::UP_STAIRCASE_PYR: return Tile(0x279a, yellow, true);
    case ViewId::GREAT_GOBLIN: return Tile('O', purple);
    case ViewId::GOBLIN: return Tile('o', darkBlue);
    case ViewId::BANDIT: return Tile('@', darkBlue);
    case ViewId::DARK_KNIGHT: return Tile('@', purple);
    case ViewId::DRAGON: return Tile('D', green);
    case ViewId::CYCLOPS: return Tile('C', green);
    case ViewId::GHOST: return Tile('&', white);
    case ViewId::DEVIL: return Tile('&', purple);
    case ViewId::CASTLE_GUARD: return Tile('@', lightGray);
    case ViewId::KNIGHT: return Tile('@', lightGray);
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
    case ViewId::SPIDER: return Tile('s', brown);
    case ViewId::FLY: return Tile('b', gray);
    case ViewId::SCORPION: return Tile('s', lightGray);
    case ViewId::SNAKE: return Tile('S', yellow);
    case ViewId::VULTURE: return Tile('v', darkGray);
    case ViewId::BODY_PART: return Tile('%', red);
    case ViewId::BONE: return Tile('%', white);
    case ViewId::MOUNTAIN_BUSH:
    case ViewId::BUSH: return Tile('&', darkGreen);
    case ViewId::DECID_TREE: return Tile(0x1f70d, darkGreen, true);
    case ViewId::CANIF_TREE: return Tile(0x2663, darkGreen, true);
    case ViewId::WATER: return Tile('~', lightBlue);
    case ViewId::MAGMA: return Tile('~', red);
    case ViewId::ABYSS: return Tile('~', darkGray);
    case ViewId::DOOR: return Tile('|', brown);
    case ViewId::SWORD: return Tile(')', lightGray);
    case ViewId::SPECIAL_SWORD: return Tile(')', yellow);
    case ViewId::ELVEN_SWORD: return Tile(')', gray);
    case ViewId::KNIFE: return Tile(')', white);
    case ViewId::WAR_HAMMER: return Tile(')', blue);
    case ViewId::SPECIAL_WAR_HAMMER: return Tile(')', lightBlue);
    case ViewId::BATTLE_AXE: return Tile(')', green);
    case ViewId::SPECIAL_BATTLE_AXE: return Tile(')', lightGreen);
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
    case ViewId::OPENED_CHEST:
    case ViewId::CHEST: return Tile('=', brown);
    case ViewId::OPENED_COFFIN:
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
    case ViewId::TELEPATHY_HELM: return Tile('[', lightGreen);
    case ViewId::CHAIN_ARMOR: return Tile('[', lightGray);
    case ViewId::IRON_HELM: return Tile('[', lightGray);
    case ViewId::LEATHER_BOOTS: return Tile('[', brown);
    case ViewId::IRON_BOOTS: return Tile('[', lightGray);
    case ViewId::SPEED_BOOTS: return Tile('[', lightBlue);
    case ViewId::DESTROYED_FURNITURE: return Tile('*', brown);
    case ViewId::BURNT_FURNITURE: return Tile('*', darkGray);
    case ViewId::FALLEN_TREE: return Tile('*', green);
    case ViewId::BURNT_TREE: return Tile('*', darkGray);
    case ViewId::GUARD_POST: return Tile(L'⚐', yellow, true);
  }
  Debug(FATAL) << "unhandled view id " << (int)obj.id();
  return Tile(' ', white);
}

static Tile getTile(const ViewObject& obj, bool sprite) {
  if (sprite)
    return getSpriteTile(obj);
  else
    return getAsciiTile(obj);
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

int maxTilesX = 600;
int maxTilesY = 600;
Image mapBuffer;
vector<Texture> tiles;
vector<int> tileSize { 36, 36, 36, 24, 36, 36 };
int nominalSize = 36;

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

static int getTextLength(string s, const Font& font = textFont, int size = textSize) {
  Text t(s, font, size);
  return t.getLocalBounds().width;
}

static void drawText(const Font& font, int size, Color color, int x, int y, String s, bool center = false) {
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

static void drawText(Color color, int x, int y, string s, bool center = false, int size = textSize) {
  std::basic_string<sf::Uint32> utf32;
  sf::Utf8::toUtf32(s.begin(), s.end(), std::back_inserter(utf32));
  drawText(textFont, size, color, x, y, utf32, center);
}

static void drawText(Color color, int x, int y, const char* c, bool center = false, int size = textSize) {
  drawText(textFont, size, color, x, y, String(c), center);
}

static void drawImage(int px, int py, const Image& image) {
  Texture t;
  t.loadFromImage(image);
  Sprite s(t);
  s.setPosition(px, py);
  display->draw(s);
}

static void drawSprite(int x, int y, int px, int py, int w, int h, const Texture& t, int dw = -1, int dh = -1,
    Optional<Color> color = Nothing()) {
  Sprite s(t, sf::IntRect(px, py, w, h));
  s.setPosition(x, y);
  if (color)
    s.setColor(*color);
  if (dw != -1)
    s.setScale(double(dw) / w, double(dh) / h);
  display->draw(s);
}

int topBarHeight = 50;
int rightBarWidth = 250;
int bottomBarHeight = 75;

void WindowView::initialize() {
  display = new RenderWindow(VideoMode(1024, 600, 32), "KeeperRL");
  sfView = new sf::View(display->getDefaultView());
  screenHeight = display->getSize().y;
  screenWidth = display->getSize().x;

  textFont.loadFromFile("coolvetica rg.ttf");
  tileFont.loadFromFile("coolvetica rg.ttf");
  symbolFont.loadFromFile("Symbola.ttf");

  asciiLayouts = {
      MapLayout::gridLayout(screenWidth, screenHeight, 16, 20, -20, topBarHeight - 20, rightBarWidth - 20,
          bottomBarHeight - 20, 1, allLayers),
      MapLayout::gridLayout(screenWidth, screenHeight, 8, 10, 0, 30, 220, 85, 1,
      {ViewLayer::FLOOR, ViewLayer::LARGE_ITEM, ViewLayer::CREATURE}), false};
  spriteLayouts = {
      MapLayout::gridLayout(screenWidth, screenHeight, 36, 36, -20, 0, 225, 0, 1, allLayers),
      MapLayout::gridLayout(screenWidth, screenHeight, 18, 18, 0, 30, 220, 85, 1,
      {ViewLayer::FLOOR, ViewLayer::LARGE_ITEM, ViewLayer::CREATURE}), true};
  currentTileLayout = spriteLayouts;

  mapLayout = currentTileLayout.normalLayout;

  worldLayout = MapLayout::worldLayout(screenWidth, screenHeight, 0, 80, 220, 75);
  allLayouts.push_back(asciiLayouts.normalLayout);
  allLayouts.push_back(asciiLayouts.unzoomLayout);
  allLayouts.push_back(spriteLayouts.normalLayout);
  allLayouts.push_back(spriteLayouts.unzoomLayout);
  allLayouts.push_back(worldLayout);
  mapBuffer.create(maxTilesX, maxTilesY);
  Image tileImage;
  CHECK(tileImage.loadFromFile("tiles_int.png"));
  Image tileImage2;
  CHECK(tileImage2.loadFromFile("tiles2_int.png"));
  Image tileImage3;
  CHECK(tileImage3.loadFromFile("tiles3_int.png"));
  Image tileImage4;
  CHECK(tileImage4.loadFromFile("tiles4_int.png"));
  Image tileImage5;
  CHECK(tileImage5.loadFromFile("tiles5_int.png"));
  Image tileImage6;
  CHECK(tileImage6.loadFromFile("tiles6_int.png"));
  Image tileImage7;
  CHECK(tileImage7.loadFromFile("tiles7_int.png"));
  tiles.resize(7);
  tiles[0].loadFromImage(tileImage);
  tiles[1].loadFromImage(tileImage2);
  tiles[2].loadFromImage(tileImage3);
  tiles[3].loadFromImage(tileImage4);
  tiles[4].loadFromImage(tileImage5);
  tiles[5].loadFromImage(tileImage6);
  tiles[6].loadFromImage(tileImage7);
  //for (Texture& tex : tiles)
  //  tex.setSmooth(true);
}

void WindowView::displaySplash(bool& ready) {
  Image splash;
  CHECK(splash.loadFromFile("splash2f.png"));
  int bottomMargin = 100;
  drawImage(50, screenHeight - bottomMargin, splash);
  vector<Rectangle> drawn;
  vector<string> paths { "splash2e.png", "splash2a.png", "splash2b.png", "splash2c.png", "splash2d.png" };
  random_shuffle(++paths.begin(), paths.end(), [](int a) { return Random.getRandom(a);});
  for (auto path : paths) {
    CHECK(splash.loadFromFile(path));
    int cnt = 100;
    while (1) {
      int px = Random.getRandom(screenWidth - splash.getSize().x);
      int py = Random.getRandom(screenHeight - bottomMargin - splash.getSize().y);
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
      drawImage(pos.getPX(), pos.getPY(), splash);
      break;
    }
  }
 // drawText(white, screenWidth / 2, screenHeight - 60, "Loading...", true);
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

Optional<Vec2> mousePos;

Optional<Vec2> WindowView::getHighlightedTile() {
  if (!mousePos)
    return Nothing();
  return mapLayout->projectOnMap(*mousePos);
}

static bool leftMouseButtonPressed = false;

WindowView::BlockingEvent WindowView::readkey() {
  Event event;
  while (1) {
    bool wasPaused = false;
    if (!myClock.isPaused()) {
      myClock.pause();
      wasPaused = true;
    }
    display->waitEvent(event);
    if (wasPaused)
      myClock.cont();
    bool scrolled = false;
    while (considerScrollEvent(event)) {
      mousePos = Nothing();
      if (!display->pollEvent(event))
        return { BlockingEvent::IDLE };
      scrolled = true;
    }
    if (scrolled)
      return { BlockingEvent::IDLE };
    Debug() << "Event " << event.type;
    if (event.type == Event::KeyPressed) {
      Event::KeyEvent ret(event.key);
      mousePos = Nothing();
      while (display->pollEvent(event));
      return { BlockingEvent::KEY, ret };
    }
    bool mouseEv = false;
    while (event.type == Event::MouseMoved) {
      mouseEv = true;
      mousePos = Vec2(event.mouseMove.x, event.mouseMove.y);
      if (!display->pollEvent(event))
        return { BlockingEvent::MOUSE_MOVE };
    }
    if (mouseEv && event.type == Event::MouseMoved)
      return { BlockingEvent::MOUSE_MOVE };
    if (event.type == Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
      return { BlockingEvent::MOUSE_LEFT };
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

void drawFilledRectangle(const Rectangle& t, Color color, Optional<Color> outline = Nothing()) {
  RectangleShape r(Vector2f(t.getW(), t.getH()));
  r.setPosition(t.getPX(), t.getPY());
  r.setFillColor(color);
  if (outline) {
    r.setOutlineThickness(-2);
    r.setOutlineColor(*outline);
  }
  display->draw(r);
}

void drawFilledRectangle(int px, int py, int kx, int ky, Color color, Optional<Color> outline = Nothing()) {
  drawFilledRectangle(Rectangle(px, py, kx, ky), color, outline);
}

static Color getBleedingColor(const ViewObject& object) {
  double bleeding = object.getBleeding();
 /* if (object.isPoisoned())
    return Color(0, 255, 0);*/
  if (bleeding > 0)
    bleeding = 0.3 + bleeding * 0.7;
  return Color(255, max(0., (1 - bleeding) * 255), max(0., (1 - bleeding) * 255));
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
  Color color = getAsciiTile(object).color;
  return Color(
      (1 - bleeding) * color.r + bleeding * 255,
      (1 - bleeding) * color.g,
      (1 - bleeding) * color.b);
}

/*static Color getMemoryColor(const ViewObject& object) {
  Color color = getTile(object).color;
  float cf = 3.5;
  float r = color.r, g = color.g, b = color.b;
  return Color(
        (cf * r + g + b) / (2 + cf),
        (r + g * cf + b) / (2 + cf),
        (r + g + b * cf) / (2 + cf));
}*/

static Color transparency(const Color& color, int trans) {
  return Color(color.r, color.g, color.b, trans);
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

void WindowView::drawPlayerInfo() {
  GameInfo::PlayerInfo& info = gameInfo.playerInfo;
  string title = info.title;
  if (!info.adjectives.empty() || !info.playerName.empty())
    title = " " + title;
  for (int i : All(info.adjectives))
    title = string(i <info.adjectives.size() - 1 ? ", " : " ") + info.adjectives[i] + title;
 // int line0 = screenHeight - 115;
  int line1 = screenHeight - bottomBarHeight + 10;
  int line2 = line1 + 25;
  drawFilledRectangle(0, screenHeight - bottomBarHeight, screenWidth - rightBarWidth, screenHeight, translucentBlack);
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

static void drawViewObject(const ViewObject& obj, int x, int y, bool sprite) {
    Tile tile = getTile(obj, sprite);
    if (tile.hasSpriteCoord()) {
      int sz = tileSize[tile.getTexNum()];
      int of = (nominalSize - sz) / 2;
      Vec2 coord = tile.getSpriteCoord();
      drawSprite(x - sz / 2, y + of, coord.x * sz, coord.y * sz, sz, sz, tiles[tile.getTexNum()], sz * 2 / 3, sz * 2 /3);
    } else
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
    drawViewObject(elem.second.first, screenWidth - rightBarWidth + 10, height, currentTileLayout.sprites);
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
      drawViewObject(elem.second.first, screenWidth - rightBarWidth + 10, height, currentTileLayout.sprites);
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
          screenWidth - rightBarWidth - 10, legendStartHeight + 35 + (chosen.size() + 3) * legendLineHeight, black);
      drawText(lightBlue, screenWidth - rightBarWidth - width - 10, lineStart, 
          info.gatheringTeam ? "Click to add to team:" : "Click to possess:");
      int cnt = 1;
      for (const Creature* c : chosen) {
        int height = lineStart + cnt * legendLineHeight;
        drawViewObject(c->getViewObject(), screenWidth - rightBarWidth - width, height, currentTileLayout.sprites);
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
    drawViewObject(get<1>(info.buttons[i]), screenWidth - rightBarWidth, height, currentTileLayout.sprites);
    drawText(i == info.activeButton ? green : (get<2>(info.buttons[i]) ? white : lightGray),
        screenWidth - rightBarWidth + 30, height, text);
    roomButtons.emplace_back(screenWidth - rightBarWidth, height,
        screenWidth - rightBarWidth + 150, height + legendLineHeight);
  }
}
  
void WindowView::drawKeeperHelp() {
  vector<string> helpText { "use mouse to", "dig and build", "", "click on minion list", "to possess",
    "", "heroes come from", "the stairs", "", "[space]  pause", "[z]  zoom"};
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
  drawFilledRectangle(0, line1 - 10, screenWidth - rightBarWidth - 30, screenHeight, translucentBlack);
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

Color getHighlightColor(ViewIndex::HighlightInfo info) {
  switch (info.type) {
    case HighlightType::BUILD: return transparency(yellow, 170);
    case HighlightType::FOG: return transparency(white, 170 * info.amount);
    case HighlightType::POISON_GAS: return Color(0, min(255., info.amount * 500), 0, info.amount * 140);
    case HighlightType::MEMORY: return transparency(black, 80);
  }
  Debug(FATAL) << "pokpok";
  return black;
}

Table<Optional<ViewIndex>> objects(maxTilesX, maxTilesY);
const MapMemory* lastMemory = nullptr;
set<Vec2> shadowed;

enum class ConnectionId {
  ROAD,
  WALL,
  WATER,
};

Optional<ConnectionId> getConnectionId(ViewId id) {
  switch (id) {
    case ViewId::GRASS_ROAD:
    case ViewId::HILL_ROAD: return ConnectionId::ROAD;
    case ViewId::BLACK_WALL:
    case ViewId::YELLOW_WALL:
    case ViewId::HELL_WALL:
    case ViewId::LOW_ROCK_WALL:
    case ViewId::WOOD_WALL:
    case ViewId::CASTLE_WALL:
    case ViewId::MUD_WALL:
    case ViewId::WALL: return ConnectionId::WALL;
    case ViewId::MAGMA:
    case ViewId::WATER: return ConnectionId::WATER;
    default: return Nothing();
  }
}

vector<Vec2> getConnectionDirs(ViewId id) {
  return Vec2::directions4();
}

map<Vec2, ConnectionId> floorIds;

bool tileConnects(ViewId id, Vec2 pos) {
  return floorIds.count(pos) && getConnectionId(id) == floorIds.at(pos);
}



Optional<ViewObject> WindowView::drawObjectAbs(int x, int y, const ViewIndex& index, int sizeX, int sizeY,
    Vec2 tilePos) {
  vector<ViewObject> objects;
  if (currentTileLayout.sprites) {
    for (ViewLayer layer : mapLayout->getLayers())
      if (index.hasObject(layer))
        objects.push_back(index.getObject(layer));
  } else
    if (auto object = index.getTopObject(mapLayout->getLayers()))
      objects.push_back(*object);
  for (ViewObject& object : objects) {
    if (object.isPlayer()) {
      drawFilledRectangle(x, y, x + sizeX, y + sizeY, Color::Transparent, lightGray);
    }
    Tile tile = getTile(object, currentTileLayout.sprites);
    Optional<Color> color;
    color = getBleedingColor(object);
    if (object.isInvisible() || tile.translucent) {
      if (color)
        color = transparency(*color, 70);
      else
        color = Color(255, 255, 255, 70);
    }
    if (tile.hasSpriteCoord()) {
      int moveY = 0;
      int off = (nominalSize -  tileSize[tile.getTexNum()]) / 2;
      int sz = tileSize[tile.getTexNum()];
      int width = mapLayout->squareWidth(Vec2(x, y)) - 2 * off;
      int height = mapLayout->squareHeight(Vec2(x, y)) - 2 * off;
      set<Dir> dirs;
      for (Vec2 dir : getConnectionDirs(object.id()))
        if (tileConnects(object.id(), tilePos + dir))
          dirs.insert(dir.getCardinalDir());
      Vec2 coord = tile.getSpriteCoord(dirs);

      if (object.layer() == ViewLayer::CREATURE) {
        drawSprite(x, y, 2 * nominalSize, 22 * nominalSize, nominalSize, nominalSize, tiles[0], width, height);
        moveY = -4 - object.getSizeIncrease() / 2;
      }
      if (tile.background) {
        Vec2 bgCoord = tile.background->getSpriteCoord();
        drawSprite(x + off, y + moveY + off, bgCoord.x * sz,
            bgCoord.y * sz, sz, sz, tiles[tile.background->getTexNum()], width, height, color);
        drawSprite(x, y, nominalSize, 22 * nominalSize, nominalSize, nominalSize, tiles[0], width, height);
        if (object.layer() == ViewLayer::FLOOR && shadowed.count(tilePos))
          drawSprite(x, y, 1 * nominalSize, 21 * nominalSize, nominalSize, nominalSize, tiles[5], width, height);
      }
      drawSprite(x + off, y + moveY + off, coord.x * sz,
          coord.y * sz, sz, sz, tiles[tile.getTexNum()], width, height, color);
      if (object.layer() == ViewLayer::FLOOR && shadowed.count(tilePos) && !tile.background)
        drawSprite(x, y, 1 * nominalSize, 21 * nominalSize, nominalSize, nominalSize, tiles[5], width, height);
      if (object.getBurning() > 0) {
        drawSprite(x, y, Random.getRandom(10, 12) * nominalSize, 0 * nominalSize,
            nominalSize, nominalSize, tiles[2], width, height);
      }
    } else {
      drawText(tile.symFont ? symbolFont : tileFont, sizeY + object.getSizeIncrease(), getColor(object),
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
  if (getHighlightedTile() == tilePos) {
    drawFilledRectangle(x, y, x + sizeX, y + sizeY, Color::Transparent, lightGray);
  }
  if (auto highlight = index.getHighlight())
    drawFilledRectangle(x, y, x + sizeX, y + sizeY, getHighlightColor(*highlight));
  if (!objects.empty())
    return objects.back();
  else
    return Nothing();
}


void WindowView::resetCenter() {
  center = Vec2(0, 0);
}

void WindowView::refreshView(const CreatureView* collective) {
  const Level* level = collective->getLevel();
  collective->refreshGameInfo(gameInfo);
  for (Vec2 pos : mapLayout->getAllTiles(Rectangle(maxTilesX, maxTilesY)))
    objects[pos] = Nothing();
  if (center == Vec2(0, 0) || collective->staticPosition())
    center = collective->getPosition();
  Vec2 movePos = (center - mouseOffset).mult(Vec2(mapLayout->squareWidth(Vec2(0, 0)),
        mapLayout->squareHeight(Vec2(0, 0))));
  movePos.x = max(movePos.x, 0);
  movePos.x = min(movePos.x, int(collective->getLevel()->getBounds().getKX() * mapLayout->squareWidth(Vec2(0, 0))));
  movePos.y = max(movePos.y, 0);
  movePos.y = min(movePos.y, int(collective->getLevel()->getBounds().getKY() * mapLayout->squareHeight(Vec2(0, 0))));
  mapLayout->updatePlayerPos(movePos);
  shadowed.clear();
  floorIds.clear();
  lastMemory = &collective->getMemory(level);
  for (Vec2 pos : mapLayout->getAllTiles(Rectangle(maxTilesX, maxTilesY))) 
    if (level->inBounds(pos)) {
      ViewIndex index = collective->getViewIndex(pos);
      if (!index.hasObject(ViewLayer::FLOOR) && !index.isEmpty() && lastMemory->hasViewIndex(pos)
          && lastMemory->getViewIndex(pos).hasObject(ViewLayer::FLOOR)) {
        // special case when monster is visible but floor is only in memory
        index.insert(lastMemory->getViewIndex(pos).getObject(ViewLayer::FLOOR));
      }
      if (index.isEmpty() && lastMemory->hasViewIndex(pos))
        index = lastMemory->getViewIndex(pos);
      objects[pos] = index;
      if (index.hasObject(ViewLayer::FLOOR)) {
        ViewObject object = index.getObject(ViewLayer::FLOOR);
        if (object.castsShadow()) {
          shadowed.erase(pos);
          shadowed.insert(pos + Vec2(0, 1));
        }
        if (auto id = getConnectionId(object.id()))
          floorIds.insert(make_pair(pos, *id));
      }
    }
  //  for (const Creature* c : player->getVisibleCreatures())
  //    if (!projectOnScreen(c->getPosition()).inRectangle(mapWindowMargin))
//      objects.insert(std::make_pair(c->getPosition(), c->getViewObject()));
  refreshScreen();
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
  Vec2 wpos = mapLayout->projectOnScreen(pos);
  refreshScreen(false);
  drawSprite(wpos.x, wpos.y, 510, 628, 36, 36, tiles[6]);
  drawAndClearBuffer();
  sf::sleep(sf::milliseconds(50));
  refreshScreen(false);
  drawSprite(wpos.x - 17, wpos.y - 17, 683, 611, 70, 70, tiles[6]);
  drawAndClearBuffer();
  sf::sleep(sf::milliseconds(50));
  refreshScreen(false);
  drawSprite(wpos.x - 29, wpos.y - 29, 577, 598, 94, 94, tiles[6]);
  drawAndClearBuffer();
  sf::sleep(sf::milliseconds(50));
  refreshScreen(true);
}

void WindowView::drawMap() {
  if (!lastMemory)
    return;

  Rectangle mapWindow = mapLayout->getBounds();
  drawFilledRectangle(mapWindow, black);
  bool pixelView = false;
  map<string, ViewObject> objIndex;
  Optional<ViewObject> highlighted;
  for (Vec2 wpos : mapLayout->getAllTiles(Rectangle(maxTilesX, maxTilesY))) {
    if (!objects[wpos])
      continue;
    int sizeX = mapLayout->squareWidth(wpos);
    int sizeY = mapLayout->squareHeight(wpos);
    Vec2 pos = mapLayout->projectOnScreen(wpos);
    const ViewIndex& index = *objects[wpos];
    Optional<ViewObject> topObject;
    if (sizeX > 1) {
      topObject = drawObjectAbs(pos.x, pos.y, index, sizeX, sizeY, wpos);
    } else {
      topObject = index.getTopObject(mapLayout->getLayers());
      if (topObject)
        mapBuffer.setPixel(pos.x, pos.y, getColor(*topObject));
      pixelView = true;
    }
    if (topObject) {
      objIndex.insert(std::make_pair(topObject->getDescription(), *topObject));
      if (getHighlightedTile() == wpos)
        highlighted = *topObject;
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
  drawFilledRectangle(rightPos - 30, 0, screenWidth, screenHeight, translucentBlack);
  if (gameInfo.infoType == GameInfo::InfoType::PLAYER || collectiveOption == CollectiveOption::LEGEND) {
    int cnt = 0;
    if (legendOption == LegendOption::OBJECTS) {
      for (auto elem : objIndex) {
        drawViewObject(elem.second, rightPos, legendStartHeight + cnt * 25, currentTileLayout.sprites);
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
  if (getHighlightedTile() && highlighted) {
    string text = highlighted->getDescription(true);
    int height = 30;
    int width = getTextLength(text) + 30;
    Vec2 pos(screenWidth - rightBarWidth - width, screenHeight - bottomBarHeight - height); //mapLayout->projectOnScreen(*getHighlightedTile()) + Vec2(30, 30);
    drawFilledRectangle(pos.x, pos.y, pos.x + width, pos.y + height, transparency(black, 190));
    Color col = white;
    if (highlighted->isHostile())
      col = red;
    else if (highlighted->isFriendly())
      col = green;
    drawText(col, pos.x + 10, pos.y + 1, text);
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

Optional<int> reverseIndexHeight(const vector<string>& options, int height) {
  if (height < 0 || height >= options.size() || View::hasTitlePrefix(options[height]) )
    return Nothing();
  int sub = 0;
  for (int i : Range(height))
    if (View::hasTitlePrefix(options[i]))
      ++sub;
  return height - sub;
}

Optional<Vec2> WindowView::chooseDirection(const string& message) {
  showMessage(message);
  refreshScreen();
  do {
    BlockingEvent event = readkey();
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

Optional<int> getIndex(const vector<string>& options, bool hasTitle, Vec2 mousePos);

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
    BlockingEvent event = readkey();
    if (event.type == BlockingEvent::KEY)
      switch (event.key->code) {
        case Keyboard::PageDown: index += 10; if (index >= count) index = count - 1; break;
        case Keyboard::PageUp: index -= 10; if (index < 0) index = 0; break;
        case Keyboard::Numpad8:
        case Keyboard::Up: if (index > 0) --index; break;
        case Keyboard::Numpad2:
        case Keyboard::Down: if (index < count - 1) ++index; break;
        case Keyboard::Numpad5:
        case Keyboard::Return : clearMessageBox(); return index;
        case Keyboard::Escape : clearMessageBox(); return Nothing();
        default: break;
      }
    else if (event.type == BlockingEvent::MOUSE_MOVE && mousePos) {
      if (Optional<int> mouseIndex = getIndex(window, !title.empty(), *mousePos)) {
        index = *mouseIndex + itemsCutoff;
        Debug() << "Index " << index;
      }
    } else if (event.type == BlockingEvent::MOUSE_LEFT) {
      clearMessageBox();
      if (mousePos && getIndex(window, !title.empty(), *mousePos))
        return index;
      else
        return Nothing();
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
    BlockingEvent event = readkey();
    if (event.type == BlockingEvent::KEY)
      switch (event.key->code) {
        case Keyboard::Numpad8:
        case Keyboard::Up: if (index > 0) --index; break;
        case Keyboard::Numpad2:
        case Keyboard::Down: if (index < options.size() - numLines) ++index; break;
        case Keyboard::Return : refreshScreen(); return;
        case Keyboard::Escape : refreshScreen(); return;
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
  return (screenHeight - ySpacing - ySpacing - 2 * yMargin - itemSpacing - itemYMargin) / itemSpacing;
}

Optional<int> getIndex(const vector<string>& options, bool hasTitle, Vec2 mousePos) {
  int xSpacing = (screenWidth - windowWidth) / 2;
  Rectangle window(xSpacing, ySpacing, xSpacing + windowWidth, screenHeight - ySpacing);
  if (!mousePos.inRectangle(window))
    return Nothing();
  return reverseIndexHeight(options,
      (mousePos.y - ySpacing - yMargin + (hasTitle ? 0 : itemSpacing) - itemYMargin) / itemSpacing - 1);
}

void WindowView::drawList(const string& title, const vector<string>& options, int hightlight) {
  int xMargin = 10;
  int itemXMargin = 30;
  int border = 2;
  int h = -1;
  int xSpacing = (screenWidth - windowWidth) / 2;
  int topMargin = yMargin;
  refreshScreen(false);
  if (hightlight > -1) 
    h = indexHeight(options, hightlight);
  Rectangle window(xSpacing, ySpacing, xSpacing + windowWidth, screenHeight - ySpacing);
  drawFilledRectangle(window, translucentBlack, white);
  int mouseHeight = -1;
 /* if (mousePos && mousePos->inRectangle(window)) {
    h = -1;
    mouseHeight = mousePos->y;
  }*/
  if (!title.empty())
    drawText(white, xSpacing + xMargin, ySpacing + yMargin, title);
  else
    topMargin -= itemSpacing;
  for (int i : All(options)) {  
    int beginH = ySpacing + topMargin + (i + 1) * itemSpacing + itemYMargin;
    if (!hasTitlePrefix(options[i])) {
      if (hightlight > -1 && (h == i || (mouseHeight >= beginH && mouseHeight < beginH + itemSpacing))) 
        drawFilledRectangle(xSpacing + xMargin, beginH, 
            windowWidth + xSpacing - xMargin, beginH + itemSpacing - 1, darkGreen);
      drawText(white, xSpacing + xMargin + itemXMargin, beginH, options[i]);
    }
    else
      drawText(yellow, xSpacing + xMargin, beginH, removeTitlePrefix(options[i]));

  }
  drawAndClearBuffer();
}

void WindowView::drawAndClearBuffer() {
  display->display();
  display->clear(black);
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
  presentText("Important!", message);
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
/*  if (oldMessage)
    showMessage("");*/
  if (currentMessage[messageInd].size() + message.size() + 1 > maxMsgLength &&
      messageInd == currentMessage.size() - 1) {
    currentMessage.pop_front();
    currentMessage.push_back("");
  }
 /*   currentMessage[messageInd] += " (more)";
    refreshScreen();
    while (1) {
      BlockingEvent event = readkey();
      if (event.type == BlockingEvent::KEY &&
          (event.key->code == Keyboard::Space || event.key->code == Keyboard::Return))
        break;
    }
    showMessage(message);
  } else {*/
    if (currentMessage[messageInd].size() + message.size() + 1 > maxMsgLength)
      ++messageInd;
    currentMessage[messageInd] += (currentMessage[messageInd].size() > 0 ? " " : "") + message;
    refreshScreen();
//  }
}

void WindowView::unzoom(bool pixel, bool tpp) {
  if (pixel && mapLayout != worldLayout)
    mapLayout = worldLayout;
 /* else if (tpp && mapLayout != tppLayout)
    mapLayout = tppLayout;*/
  else if (mapLayout != currentTileLayout.normalLayout)
    mapLayout = currentTileLayout.normalLayout;
  else
    mapLayout = currentTileLayout.unzoomLayout;
  refreshScreen();
}

void WindowView::switchTiles() {
  if (!currentTileLayout.sprites)
    currentTileLayout = spriteLayouts;
  else
    currentTileLayout = asciiLayouts;
  mapLayout = currentTileLayout.normalLayout;
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
        mouseOffset = (Vec2(event.mouseMove.x, event.mouseMove.y) - lastMousePos).div(
            Vec2(mapLayout->squareWidth(Vec2(0, 0)), mapLayout->squareHeight(Vec2(0, 0))));
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
            return CollectiveAction(CollectiveAction::IDLE);
          case Keyboard::F2:
            switchTiles();
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
            leftMouseButtonPressed = false;
            return CollectiveAction(CollectiveAction::BUTTON_RELEASE);
          }
          break;
      case Event::MouseMoved:
          mousePos = Vec2(event.mouseMove.x, event.mouseMove.y);
          if (leftMouseButtonPressed) {
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
            leftMouseButtonPressed = true;
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

struct KeyInfo {
  string keyDesc;
  string action;
  Event::KeyEvent event;
};

vector<KeyInfo> keyInfo {
  { "I", "Inventory", {Keyboard::I}},
  { "E", "Manage equipment", {Keyboard::E}},
  { "Enter", "Pick up items or interact with square", {Keyboard::Return}},
  { "D", "Drop item", {Keyboard::D}},
  { "A", "Apply item", {Keyboard::A}},
  { "T", "Throw item", {Keyboard::T}},
  { "M", "Show message history", {Keyboard::M}},
  { "C", "Chat with someone", {Keyboard::C}},
  { "P", "Pay debt", {Keyboard::P}},
  { "U", "Unpossess", {Keyboard::U}},
  { "Space", "Wait", {Keyboard::Space}},
  { "Z", "Zoom in/out", {Keyboard::Z}},
  { "Shift + Z", "World map", {Keyboard::Z, false, false, true}},
  { "F2", "Switch between graphics and ascii", {Keyboard::F2}},
  { "Escape", "Quit", {Keyboard::Escape}},
};

Optional<Event::KeyEvent> WindowView::getEventFromMenu() {
  vector<string> options {
      View::getTitlePrefix("Move around with the number pad."),
      View::getTitlePrefix("Fast travel with ctrl + arrow."),
      View::getTitlePrefix("Fire arrows with alt + arrow."),
      View::getTitlePrefix("Choose action:") };
  for (int i : All(keyInfo)) {
    Debug() << "Action " << keyInfo[i].action;
    options.push_back(keyInfo[i].action + "   [ " + keyInfo[i].keyDesc + " ]");
  }
  auto index = chooseFromList("", options);
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
    if (event.type == BlockingEvent::MOUSE_LEFT) {
      if (auto pos = getHighlightedTile()) {
 //       mousePos = Nothing();
        return Action(ActionId::MOVE_TO, *pos);
      } else
        return Action(ActionId::IDLE);
    }
    auto key = event.key;
    if (key->code == Keyboard::F1)
      key = getEventFromMenu();
    if (!key)
      return Action(ActionId::IDLE);
    if (mapLayout == worldLayout)
      unzoom(false, false);
    auto optionalAction = mapLayout->overrideAction(*key);
    if (optionalAction)
      return *optionalAction;
    switch (key->code) {
      case Keyboard::Escape : if (yesOrNoPrompt("Are you sure you want to quit?")) exit(0); break;
      case Keyboard::Z: unzoom(key->shift, key->control); return Action(ActionId::IDLE);
      case Keyboard::F1: legendOption = (LegendOption)(1 - (int)legendOption); return Action(ActionId::IDLE);
      case Keyboard::F2: switchTiles(); return Action(ActionId::IDLE);
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

