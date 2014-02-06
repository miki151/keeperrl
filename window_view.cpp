#include "stdafx.h"

#include "window_view.h"
#include "logging_view.h"
#include "replay_view.h"
#include "creature.h"
#include "level.h"
#include "options.h"

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
  double translucent = 0;
  bool stickingOut = false;
  Tile(sf::Uint32 ch, Color col, bool sym = false) : color(col), text(ch), symFont(sym) {
  }
  Tile(int x, int y, int num = 0, bool _stickingOut = false) : stickingOut(_stickingOut),tileCoord(Vec2(x, y)), 
      texNum(num) {}

  Tile& addConnection(set<Dir> c, int x, int y) {
    connections.insert({c, Vec2(x, y)});
    return *this;
  }

  Tile& setTranslucent(double v) {
    translucent = v;
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
  string let = humanoid ? "WETUIPLKJHFAXBM" : "qwetyupkfaxbnm";
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
    return Tile(r.getRandom(7, 10), 10);
}

Tile getSprite(ViewId id);

Tile getRoadTile(int pathSet) {
  return Tile(0, pathSet, 5)
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

Tile getWallTile(int wallSet) {
  return Tile(9, wallSet, 1, true)
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
    case ViewId::KEEPER: return Tile(3, 0);
    case ViewId::UNKNOWN_MONSTER: return Tile('?', lightGreen);
    case ViewId::SPECIAL_BEAST: return Tile(7, 10);
    case ViewId::SPECIAL_HUMANOID: return Tile(6, 10);
    case ViewId::ELF: return Tile(12, 6);
    case ViewId::ELF_CHILD: return Tile(14, 6);
    case ViewId::ELF_LORD: return Tile(13, 6);
    case ViewId::ELVEN_SHOPKEEPER: return Tile(4, 2);
    case ViewId::IMP: return Tile(18, 19);
    case ViewId::BILE_DEMON: return Tile(8, 14);
    case ViewId::CHICKEN: return Tile(18, 1);
    case ViewId::DWARF: return Tile(2, 6);
    case ViewId::DWARF_BARON: return Tile(3, 6);
    case ViewId::DWARVEN_SHOPKEEPER: return Tile(4, 2);
    case ViewId::BRIDGE: return Tile(24, 0, 4);
    case ViewId::ROAD: return getRoadTile(7);
    case ViewId::PATH:
    case ViewId::FLOOR: return Tile(3, 14, 1);
    case ViewId::SAND: return Tile(7, 12, 2);
    case ViewId::MUD: return Tile(3, 12, 2);
    case ViewId::GRASS: return Tile(0, 13, 2);
    case ViewId::CROPS: return Tile(9, 12, 2);
    case ViewId::WALL: return getWallTile(2);
    case ViewId::MOUNTAIN: return Tile(17, 2, 2, true);
    case ViewId::MOUNTAIN2: return getWallTile(21);
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
    case ViewId::IRON_ORE: return Tile(0, 17, 1);
    case ViewId::SNOW: return Tile(16, 2, 2, true);
    case ViewId::HILL: return Tile(3, 13, 2);
    case ViewId::WOOD_WALL: return getWallTile(4);
    case ViewId::BLACK_WALL: return getWallTile(2);
    case ViewId::YELLOW_WALL: return getWallTile(8);
    case ViewId::LOW_ROCK_WALL: return getWallTile(21);
    case ViewId::HELL_WALL: return getWallTile(22);
    case ViewId::CASTLE_WALL: return getWallTile(5);
    case ViewId::MUD_WALL: return getWallTile(13);
    case ViewId::SECRETPASS: return Tile(0, 15, 1);
    case ViewId::DUNGEON_ENTRANCE: return Tile(15, 2, 2, true);
    case ViewId::DUNGEON_ENTRANCE_MUD: return Tile(19, 2, 2, true);
    case ViewId::DOWN_STAIRCASE: return Tile(8, 0, 1, true);
    case ViewId::UP_STAIRCASE: return Tile(7, 0, 1, true);
    case ViewId::DOWN_STAIRCASE_CELLAR: return Tile(8, 21, 1, true);
    case ViewId::UP_STAIRCASE_CELLAR: return Tile(7, 21, 1, true);
    case ViewId::DOWN_STAIRCASE_HELL: return Tile(8, 1, 1, true);
    case ViewId::UP_STAIRCASE_HELL: return Tile(7, 22, 1, true);
    case ViewId::DOWN_STAIRCASE_PYR: return Tile(8, 8, 1, true);
    case ViewId::UP_STAIRCASE_PYR: return Tile(7, 8, 1, true);
    case ViewId::GREAT_GOBLIN: return Tile(6, 14);
    case ViewId::GOBLIN: return Tile(5, 14);
    case ViewId::BANDIT: return Tile(0, 2);
    case ViewId::GHOST: return Tile(6, 16).setTranslucent(0.5);
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
    case ViewId::CLAY_GOLEM: return Tile(12, 11);
    case ViewId::STONE_GOLEM: return Tile(10, 10);
    case ViewId::IRON_GOLEM: return Tile(12, 10);
    case ViewId::LAVA_GOLEM: return Tile(13, 10);
    case ViewId::ZOMBIE: return Tile(0, 16);
    case ViewId::SKELETON: return Tile(2, 16);
    case ViewId::VAMPIRE: return Tile(12, 16);
    case ViewId::VAMPIRE_LORD: return Tile(13, 16);
    case ViewId::MUMMY: return Tile(7, 16);
    case ViewId::MUMMY_LORD: return Tile(8, 16);
    case ViewId::ACID_MOUND: return Tile(1, 12);
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
    case ViewId::BEAR: return Tile(8, 18);
    case ViewId::BAT: return Tile(2, 12);
    case ViewId::GNOME: return Tile(13, 8);
    case ViewId::LEPRECHAUN: return Tile(16, 8);
    case ViewId::RAT: return Tile(7, 12);
    case ViewId::SPIDER: return Tile(6, 12);
    case ViewId::FLY: return Tile(10, 12);
    case ViewId::SCORPION: return Tile(11, 18);
    case ViewId::SNAKE: return Tile(9, 12);
    case ViewId::VULTURE: return Tile(17, 12);
    case ViewId::RAVEN: return Tile(17, 12);
    case ViewId::BODY_PART: return Tile(9, 4, 3);
    case ViewId::BONE: return Tile(3, 0, 2);
    case ViewId::BUSH: return Tile(17, 0, 2, true);
    case ViewId::DECID_TREE: return Tile(21, 3, 2, true);
    case ViewId::CANIF_TREE: return Tile(20, 3, 2, true);
    case ViewId::TREE_TRUNK: return Tile(26, 3, 2, true);
    case ViewId::WATER: return getWaterTile(5);
    case ViewId::MAGMA: return getWaterTile(11);
    case ViewId::ABYSS: return Tile('~', darkGray);
    case ViewId::DOOR: return Tile(4, 2, 2, true);
    case ViewId::PLANNED_DOOR: return Tile(4, 2, 2, true).setTranslucent(0.5);
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
    case ViewId::FOUNTAIN: return Tile(0, 7, 2, true);
    case ViewId::GOLD: return Tile(8, 3, 3, true);
    case ViewId::CHEST: return Tile(3, 3, 2, true);
    case ViewId::OPENED_CHEST: return Tile(6, 3, 2, true);
    case ViewId::COFFIN: return Tile(7, 3, 2, true);
    case ViewId::OPENED_COFFIN: return Tile(8, 3, 2, true);
    case ViewId::BOULDER: return Tile(18, 7);
    case ViewId::UNARMED_BOULDER_TRAP: return Tile(18, 7).setTranslucent(0.6);
    case ViewId::PORTAL: return Tile(1, 6, 2);
    case ViewId::TRAP: return Tile(L'➹', yellow, true);
    case ViewId::GAS_TRAP: return Tile(L'☠', green, true);
    case ViewId::UNARMED_GAS_TRAP: return Tile(L'☠', lightGray, true);
    case ViewId::ROCK: return Tile(6, 1, 3);
    case ViewId::IRON_ROCK: return Tile(10, 1, 3);
    case ViewId::WOOD_PLANK: return Tile(7, 10, 2);
    case ViewId::STOCKPILE: return Tile(4, 1, 1);
    case ViewId::BED: return Tile(5, 4, 2, true);
    case ViewId::THRONE: return Tile(7, 4, 2, true);
    case ViewId::DUNGEON_HEART: return Tile(6, 10, 2);
    case ViewId::ALTAR: return Tile(2, 7, 2, true);
    case ViewId::TORTURE_TABLE: return Tile(1, 5, 2, true);
    case ViewId::TRAINING_DUMMY: return Tile(0, 5, 2, true);
    case ViewId::LIBRARY: return Tile(2, 4, 2, true);
    case ViewId::LABORATORY: return Tile(2, 5, 2, true);
    case ViewId::ANIMAL_TRAP: return Tile(3, 8, 2, true);
    case ViewId::WORKSHOP: return Tile(9, 4, 2, true);
    case ViewId::GRAVE: return Tile(0, 0, 2, true);
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
    case ViewId::BURNT_TREE:
    case ViewId::FALLEN_TREE: return Tile(26, 3, 2, true);
    case ViewId::GUARD_POST: return Tile(L'⚐', yellow, true);
    case ViewId::DESTROY_BUTTON: return Tile('X', red);
    case ViewId::MANA: return Tile(5, 10, 2);
  }
  FAIL << "unhandled view id " << (int)id;
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
    case ViewId::KEEPER: return Tile('@', purple);
    case ViewId::UNKNOWN_MONSTER: return Tile('?', lightGreen);
    case ViewId::SPECIAL_BEAST: return getSpecialCreature(obj, false);
    case ViewId::SPECIAL_HUMANOID: return getSpecialCreature(obj, true);
    case ViewId::ELF: return Tile('@', green);
    case ViewId::ELF_CHILD: return Tile('@', lightGreen);
    case ViewId::ELF_LORD: return Tile('@', darkGreen);
    case ViewId::ELVEN_SHOPKEEPER: return Tile('@', lightBlue);
    case ViewId::IMP: return Tile('i', lightBrown);
    case ViewId::BILE_DEMON: return Tile('O', green);
    case ViewId::CHICKEN: return Tile('c', yellow);
    case ViewId::DWARF: return Tile('h', blue);
    case ViewId::DWARF_BARON: return Tile('h', darkBlue);
    case ViewId::DWARVEN_SHOPKEEPER: return Tile('h', lightBlue);
    case ViewId::FLOOR: return Tile('.', white);
    case ViewId::BRIDGE: return Tile('_', brown);
    case ViewId::ROAD: return Tile('.', lightGray);
    case ViewId::PATH: return Tile('.', lightGray);
    case ViewId::SAND: return Tile('.', yellow);
    case ViewId::MUD: return Tile(0x1d0f0, brown, true);
    case ViewId::GRASS: return Tile(0x1d0f0, green, true);
    case ViewId::CROPS: return Tile(0x1d0f0, yellow, true);
    case ViewId::CASTLE_WALL: return Tile('#', lightGray);
    case ViewId::MUD_WALL: return Tile('#', lightBrown);
    case ViewId::WALL: return Tile('#', lightGray);
    case ViewId::MOUNTAIN: return Tile(0x25ee, darkGray, true);
    case ViewId::MOUNTAIN2: return Tile('#', darkGray);
    case ViewId::GOLD_ORE: return Tile(L'⁂', yellow, true);
    case ViewId::IRON_ORE: return Tile(L'⁂', lightGray, true);
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
    case ViewId::CLAY_GOLEM: return Tile('Y', yellow);
    case ViewId::STONE_GOLEM: return Tile('Y', lightGray);
    case ViewId::IRON_GOLEM: return Tile('Y', orange);
    case ViewId::LAVA_GOLEM: return Tile('Y', purple);
    case ViewId::ZOMBIE: return Tile('Z', green);
    case ViewId::SKELETON: return Tile('Z', white);
    case ViewId::VAMPIRE: return Tile('V', darkGray);
    case ViewId::VAMPIRE_LORD: return Tile('V', purple);
    case ViewId::MUMMY: return Tile('Z', yellow);
    case ViewId::MUMMY_LORD: return Tile('Z', orange);
    case ViewId::ACID_MOUND: return Tile('j', green);
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
    case ViewId::RAVEN: return Tile('v', darkGray);
    case ViewId::BODY_PART: return Tile('%', red);
    case ViewId::BONE: return Tile('%', white);
    case ViewId::BUSH: return Tile('&', darkGreen);
    case ViewId::DECID_TREE: return Tile(0x1f70d, darkGreen, true);
    case ViewId::CANIF_TREE: return Tile(0x2663, darkGreen, true);
    case ViewId::TREE_TRUNK: return Tile('.', brown);
    case ViewId::WATER: return Tile('~', lightBlue);
    case ViewId::MAGMA: return Tile('~', red);
    case ViewId::ABYSS: return Tile('~', darkGray);
    case ViewId::DOOR: return Tile('|', brown);
    case ViewId::PLANNED_DOOR: return Tile('|', darkBrown);
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
    case ViewId::IRON_ROCK: return Tile('*', orange);
    case ViewId::WOOD_PLANK: return Tile('\\', brown);
    case ViewId::STOCKPILE: return Tile('.', yellow);
    case ViewId::BED: return Tile('=', white);
    case ViewId::DUNGEON_HEART: return Tile(L'♥', white, true);
    case ViewId::THRONE: return Tile(L'Ω', purple);
    case ViewId::ALTAR: return Tile(L'Ω', white);
    case ViewId::TORTURE_TABLE: return Tile('=', gray);
    case ViewId::TRAINING_DUMMY: return Tile(L'‡', brown, true);
    case ViewId::LIBRARY: return Tile(L'▤', purple, true);
    case ViewId::LABORATORY: return Tile(L'ω', purple, true);
    case ViewId::ANIMAL_TRAP: return Tile(L'▥', lightGray, true);
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
    case ViewId::DESTROY_BUTTON: return Tile('X', red);
    case ViewId::MANA: return Tile(5, 10, 2);
  }
  FAIL << "unhandled view id " << (int)obj.id();
  return Tile(' ', white);
}

static Tile getTile(const ViewObject& obj, bool sprite) {
  if (sprite)
    return getSpriteTile(obj);
  else
    return getAsciiTile(obj);
}

RenderWindow* display = nullptr;
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
  bool cont;
};

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

int topBarHeight = 10;
int rightBarWidth = 300;
int rightBarText = rightBarWidth - 30;
int bottomBarHeight = 75;

Rectangle WindowView::getMapViewBounds() const {
  return Rectangle(0, topBarHeight, screenWidth - rightBarWidth, screenHeight - bottomBarHeight);
}

const MapMemory* lastMemory = nullptr;

void WindowView::initialize() {
  if (!display) {
    display = new RenderWindow(VideoMode(1024, 600, 32), "KeeperRL");
    sfView = new sf::View(display->getDefaultView());
    screenHeight = display->getSize().y;
    screenWidth = display->getSize().x;

    CHECK(textFont.loadFromFile("Lato-Bol.ttf"));
    CHECK(tileFont.loadFromFile("coolvetica rg.ttf"));
    CHECK(symbolFont.loadFromFile("Symbola.ttf"));

    asciiLayouts = {
      MapLayout::gridLayout(screenWidth, screenHeight, 16, 20, 0, topBarHeight, rightBarWidth, bottomBarHeight,
          allLayers),
      MapLayout::gridLayout(screenWidth, screenHeight, 8, 10, 0, topBarHeight, rightBarWidth, bottomBarHeight,
          {ViewLayer::FLOOR_BACKGROUND, ViewLayer::FLOOR, ViewLayer::LARGE_ITEM, ViewLayer::CREATURE}), false};
    spriteLayouts = {
      MapLayout::gridLayout(screenWidth, screenHeight, 36, 36, 0, topBarHeight, rightBarWidth,
          bottomBarHeight, allLayers),
      MapLayout::gridLayout(screenWidth, screenHeight, 18, 18, 0, topBarHeight, rightBarWidth,
          bottomBarHeight, allLayers), true};
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
  } else {
    lastMemory = nullptr;
  }
}

static vector<Vec2> splashPositions;
static vector<string> splashPaths { "splash2e.png", "splash2a.png", "splash2b.png", "splash2c.png", "splash2d.png" };

void displayMenuSplash2() {
  Image splash;
  CHECK(splash.loadFromFile("splash2f.png"));
  int bottomMargin = 90;
  drawImage(screenWidth / 2 - 415, screenHeight - bottomMargin, splash);
  CHECK(splash.loadFromFile("splash2e.png"));
  drawImage((screenWidth - splash.getSize().x) / 2, 90 - splash.getSize().y, splash);
}

void displayMenuSplash() {
  Image splash;
  CHECK(splash.loadFromFile("splash2f.png"));
  int bottomMargin = 100;
  drawImage(100, screenHeight - bottomMargin, splash);
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
        px = Random.getRandom(screenWidth - splash.getSize().x);
        py = Random.getRandom(screenHeight - bottomMargin - splash.getSize().y);
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
      drawImage(pos.getPX(), pos.getPY(), splash);
      break;
    }
  }
 // drawText(white, screenWidth / 2, screenHeight - 60, "Loading...", true);
 // drawAndClearBuffer();
}

void WindowView::displaySplash(bool& ready) {
  Image splash;
  CHECK(splash.loadFromFile(splashPaths[Random.getRandom(1, splashPaths.size())]));
  while (!ready) {
    drawImage((screenWidth - splash.getSize().x) / 2, (screenHeight - splash.getSize().y) / 2, splash);
    drawText(white, screenWidth / 2, screenHeight - 60, "Creating a new world, just for you...", true);
    drawAndClearBuffer();
    sf::sleep(sf::milliseconds(30));
    Event event;
    while (display->pollEvent(event)) {
      if (event.type == Event::Resized) {
        resize(event.size.width, event.size.height);
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

struct KeyInfo {
  string keyDesc;
  string action;
  Event::KeyEvent event;
};

vector<KeyInfo> bottomKeys;

static bool leftMouseButtonPressed = false;
static bool rightMouseButtonPressed = false;

WindowView::BlockingEvent WindowView::readkey() {
  Event event;
  while (1) {
    display->waitEvent(event);
    Debug() << "Event " << event.type;
    bool mouseEv = false;
    while (event.type == Event::MouseMoved && !rightMouseButtonPressed) {
      mouseEv = true;
      mousePos = Vec2(event.mouseMove.x, event.mouseMove.y);
      if (!display->pollEvent(event))
        return { BlockingEvent::MOUSE_MOVE };
    }
    if (mouseEv && event.type == Event::MouseMoved)
      return { BlockingEvent::MOUSE_MOVE };
    if (event.type == Event::KeyPressed) {
      Event::KeyEvent ret(event.key);
      mousePos = Nothing();
      while (display->pollEvent(event));
      return { BlockingEvent::KEY, ret };
    }
    bool scrolled = false;
    while (considerScrollEvent(event)) {
      mousePos = Nothing();
      if (!display->pollEvent(event))
        return { BlockingEvent::IDLE };
      scrolled = true;
    }
    if (scrolled)
      return { BlockingEvent::IDLE };
    if (event.type == Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
      for (int i : All(optionButtons))
        if (Vec2(event.mouseButton.x, event.mouseButton.y).inRectangle(optionButtons[i])) {
          legendOption = LegendOption(i);
          return { BlockingEvent::IDLE };
        }
      for (int i : All(bottomKeyButtons))
        if (Vec2(event.mouseButton.x, event.mouseButton.y).inRectangle(bottomKeyButtons[i])) {
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
  int line1 = screenHeight - bottomBarHeight + 10;
  int line2 = line1 + 28;
  drawFilledRectangle(0, screenHeight - bottomBarHeight, screenWidth - rightBarWidth, screenHeight, translucentBlack);
  string playerLine = capitalFirst(!info.playerName.empty() ? info.playerName + " the" + title : title) +
    "          T: " + convertToString<int>(info.time) + "            " + info.levelName;
  drawText(white, 10, line1, playerLine);
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
    int endX = startX + getTextLength(text) + keySpacing;
    drawText(lightBlue, startX, line2, text);
    bottomKeyButtons.emplace_back(startX, line2, endX, line2 + 25);
    startX = endX;
  }
  sf::Uint32 optionSyms[] = {0x1f718, L'i'};
  optionButtons.clear();
  for (int i = 0; i < 2; ++i) {
    int w = 45;
    int line = topBarHeight;
    int h = 45;
    int leftPos = screenWidth - rightBarText + 15;
    drawText(i < 1 ? symbolFont : textFont, 35, i == int(legendOption) ? green : white,
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

void WindowView::drawPlayerStats(GameInfo::PlayerInfo& info) {
  int lineStart = legendStartHeight;
  int lineX = screenWidth - rightBarText + 10;
  int line2X = screenWidth - rightBarText + 110;
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
  vector<string> lines2 {
    "",
    "",
    convertToString(info.attack),
    convertToString(info.defense),
    convertToString(info.strength),
    convertToString(info.dexterity),
    convertToString(info.speed),
    "$" + convertToString(info.numGold),
  };
  for (int i : All(lines)) {
    drawText(white, lineX, lineStart + legendLineHeight * i, lines[i]);
    drawText(white, line2X, lineStart + legendLineHeight * i, lines2[i]);
  }
}

string getPlural(const string& a, const string&b, int num) {
  if (num == 1)
    return "1 " + a;
  else
    return convertToString(num) + " " + b;
}

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
  drawText(white, screenWidth - rightBarText, legendStartHeight, info.monsterHeader);
  int cnt = 0;
  int lineStart = legendStartHeight + 35;
  int textX = screenWidth - rightBarText + 10;
  for (auto elem : creatureMap){
    int height = lineStart + cnt * legendLineHeight;
    drawViewObject(elem.second.first, textX, height, currentTileLayout.sprites);
    Color col = (elem.first == chosenCreature) ? green : white;
    drawText(col, textX + 20, height,
        convertToString(elem.second.second) + "   " + elem.first);
    creatureGroupButtons.emplace_back(textX, height, textX + 150, height + legendLineHeight);
    creatureNames.push_back(elem.first);
    ++cnt;
  }
  
  if (info.gatheringTeam && !info.team.empty()) {
    drawText(white, textX, lineStart + (cnt + 1) * legendLineHeight, 
        getPlural("monster", "monsters", info.team.size()));
    ++cnt;
  }
  if (info.creatures.size() > 1 || info.gatheringTeam) {
    int height = lineStart + (cnt + 1) * legendLineHeight;
    drawText((info.gatheringTeam && info.team.empty()) ? green : white, textX, height, 
        info.team.empty() ? "[gather team]" : "[command team]");
    int butWidth = 150;
    teamButton = Rectangle(textX, height, textX + butWidth, height + legendLineHeight);
    if (info.gatheringTeam) {
      drawText(white, textX + butWidth, height, "[cancel]");
      cancelTeamButton = Rectangle(textX + butWidth, height, textX + 230, height + legendLineHeight);
    }
    cnt += 2;
  }
  ++cnt;
  if (!enemyMap.empty()) {
    drawText(white, textX, lineStart + (cnt + 1) * legendLineHeight, "Enemies:");
    for (auto elem : enemyMap){
      int height = lineStart + (cnt + 2) * legendLineHeight + 10;
      drawViewObject(elem.second.first, textX, height, currentTileLayout.sprites);
      drawText(white, textX + 20, height, convertToString(elem.second.second) + "   " + elem.first);
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
      int winX = screenWidth - rightBarWidth - width - 20;
      drawFilledRectangle(winX, lineStart,
          winX + width + 20, legendStartHeight + 35 + (chosen.size() + 3) * legendLineHeight, black);
      drawText(lightBlue, winX + 10, lineStart, 
          info.gatheringTeam ? "Click to add to team:" : "Click to possess:");
      int cnt = 1;
      for (const Creature* c : chosen) {
        int height = lineStart + cnt * legendLineHeight;
        drawViewObject(c->getViewObject(), winX + 20, height, currentTileLayout.sprites);
        drawText(contains(info.team, c) ? green : white, textX - width + 30, height,
            "level: " + convertToString(c->getExpLevel()) + "    " + info.tasks[c]);
        creatureButtons.emplace_back(winX + 20, height, winX + width + 20, height + legendLineHeight);
        chosenCreatures.push_back(c);
        ++cnt;
      }
      int height = lineStart + cnt * legendLineHeight + 10;
      drawText(white, winX + 20, height, "[show description]");
      descriptionButton = Rectangle(winX, height, winX + width + 20, height + legendLineHeight);
    }
  }
}

void WindowView::drawBuildings(GameInfo::BandInfo& info) {
  int textX = screenWidth - rightBarText;
  for (int i : All(info.buttons)) {
    int height = legendStartHeight + i * legendLineHeight;
    drawViewObject(info.buttons[i].object, textX, height, currentTileLayout.sprites);
    Color color = white;
    if (i == info.activeButton)
      color = green;
    else if (!info.buttons[i].active)
      color = lightGray;
    string text = info.buttons[i].name + " " + info.buttons[i].count;
    drawText(color, textX + 30, height, text);
 //   int posX = screenWidth - rightBarWidth + 60 + getTextLength(text);
    if (info.buttons[i].cost) {
      string costText = convertToString(info.buttons[i].cost->second);
      int posX = screenWidth - getTextLength(costText) - 10;
      drawViewObject(info.buttons[i].cost->first, screenWidth - 40, height, true);
      drawText(color, posX, height, costText);
    }
    roomButtons.emplace_back(textX, height,textX + 150, height + legendLineHeight);
  }
}
  
void WindowView::drawTechnology(GameInfo::BandInfo& info) {
  int textX = screenWidth - rightBarText;
  for (int i : All(info.techButtons)) {
    int height = legendStartHeight + i * legendLineHeight;
    if (info.techButtons[i].viewObject)
      drawViewObject(*info.techButtons[i].viewObject, textX, height, true);
    drawText(white, textX + 20, height, info.techButtons[i].name);
    techButtons.emplace_back(textX, height, textX + 150, height + legendLineHeight);
  }
}

void WindowView::drawKeeperHelp() {
  vector<string> helpText { "use mouse to", "dig and build", "", "click on minion", "to possess",
    "", "your enemies ", "are in the west", "[space]  pause", "[z]  zoom"};
  int cnt = 0;
  for (string line : helpText) {
    int height = legendStartHeight + cnt * legendLineHeight;
    drawText(lightBlue, screenWidth - rightBarText, height, line);
    cnt ++;
  }
}

void WindowView::drawBandInfo() {
  GameInfo::BandInfo& info = gameInfo.bandInfo;
  int lineHeight = 28;
  int line0 = screenHeight - 90;
  int line1 = line0 + lineHeight;
  int line2 = line1 + lineHeight;
  drawFilledRectangle(0, line1 - 10, screenWidth - rightBarWidth, screenHeight, translucentBlack);
  string playerLine = info.name + "   T:" + convertToString<int>(info.time);
  drawText(white, 10, line1, playerLine);
  drawText(red, 120, line2, info.warning);
  if (myClock.isPaused())
    drawText(red, 10, line2, "PAUSED");
  else
    drawText(lightBlue, 10, line2, "PAUSE");
  pauseButton = Rectangle(10, line2, 80, line2 + lineHeight);
  string resources;
  int resourceSpacing = 100;
  int resourceX = 300;
  for (int i : All(info.numGold)) {
    drawText(white, resourceX + resourceSpacing * i, line1, convertToString<int>(info.numGold[i].count));
    drawViewObject(info.numGold[i].viewObject, 288 + resourceSpacing * i, line1, true);
  }
  int marketX = resourceX + resourceSpacing * info.numGold.size();
 /* drawText(white, marketX, line1, "black market");
  marketButton = Rectangle(marketX, line1, marketX + getTextLength("market"), line1 + legendLineHeight);*/
  sf::Uint32 optionSyms[] = {L'⌂', 0x1f718, 0x1f728, L'?'};
  optionButtons.clear();
  for (int i = 0; i < 4; ++i) {
    int w = 45;
    int line = topBarHeight;
    int h = 45;
    int leftPos = screenWidth - rightBarText + 15;
    drawText(i < 3 ? symbolFont : textFont, 35, i == int(collectiveOption) ? green : white,
        leftPos + i * w, line, optionSyms[i], true);
    optionButtons.emplace_back(leftPos + i * w - w / 2, line,
        leftPos + (i + 1) * w - w / 2, line + h);
  }
  roomButtons.clear();
  techButtons.clear();
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
    case CollectiveOption::TECHNOLOGY: drawTechnology(info); break;
  }
}

void WindowView::refreshText() {
  int lineHeight = 25;
  int numMsg = 0;
  for (int i : All(currentMessage))
    if (!currentMessage[i].empty())
      numMsg = i + 1;
  drawFilledRectangle(0, 0, screenWidth, max(topBarHeight, lineHeight * (numMsg + 1)), translucentBlack);
  for (int i : All(currentMessage))
    drawText(oldMessage ? gray : white, 10, 10 + lineHeight * i, currentMessage[i]);
  switch (gameInfo.infoType) {
    case GameInfo::InfoType::PLAYER:
        drawPlayerInfo();
        break;
    case GameInfo::InfoType::BAND: drawBandInfo(); break;
  }
}

Vec2 WindowView::projectOnBorders(Rectangle area, Vec2 pos) {
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
}

Color getHighlightColor(ViewIndex::HighlightInfo info) {
  switch (info.type) {
    case HighlightType::BUILD: return transparency(yellow, 170);
    case HighlightType::FOG: return transparency(white, 120 * info.amount);
    case HighlightType::POISON_GAS: return Color(0, min(255., info.amount * 500), 0, info.amount * 140);
    case HighlightType::MEMORY: return transparency(black, 80);
  }
  FAIL << "pokpok";
  return black;
}

Table<Optional<ViewIndex>> objects(maxTilesX, maxTilesY);
map<Vec2, ViewObject> borderCreatures;
set<Vec2> shadowed;

enum class ConnectionId {
  ROAD,
  WALL,
  WATER,
  MOUNTAIN2,
};

Optional<ConnectionId> getConnectionId(ViewId id) {
  switch (id) {
    case ViewId::ROAD: return ConnectionId::ROAD;
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
    case ViewId::MOUNTAIN2: return ConnectionId::MOUNTAIN2;
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
    Color color = getBleedingColor(object);
    if (object.isInvisible())
      color = transparency(color, 70);
    else
    if (tile.translucent > 0)
      color = transparency(color, 255 * (1 - tile.translucent));
    else if (object.isIllusion())
      color = transparency(color, 150);

    if (object.getWaterDepth() > 0) {
      int val = max(0.0, 255.0 - min(2.0, object.getWaterDepth()) * 60);
      color = Color(val, val, val);
    }
    if (tile.hasSpriteCoord()) {
      int moveY = 0;
      int off = (nominalSize -  tileSize[tile.getTexNum()]) / 2;
      int sz = tileSize[tile.getTexNum()];
      int width = sizeX - 2 * off;
      int height = sizeY - 2 * off;
      set<Dir> dirs;
      for (Vec2 dir : getConnectionDirs(object.id()))
        if (tileConnects(object.id(), tilePos + dir))
          dirs.insert(dir.getCardinalDir());
      Vec2 coord = tile.getSpriteCoord(dirs);

      if (object.layer() == ViewLayer::CREATURE) {
        drawSprite(x, y, 2 * nominalSize, 22 * nominalSize, nominalSize, nominalSize, tiles[0], width, height);
        moveY = -4 - object.getSizeIncrease() / 2;
      }
      drawSprite(x + off, y + moveY + off, coord.x * sz,
          coord.y * sz, sz, sz, tiles[tile.getTexNum()], width, height, color);
      if (contains({ViewLayer::FLOOR, ViewLayer::FLOOR_BACKGROUND}, object.layer()) && 
          shadowed.count(tilePos) && !tile.stickingOut)
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
            x + sizeX / 2, y - 3, L'ѡ', true);
        if (object.getBurning() > 0.5)
          drawText(symbolFont, sizeY, getFireColor(),
              x + sizeX / 2, y - 3, L'Ѡ', true);
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
  center = {0, 0};
}

void WindowView::updateView(const CreatureView* collective) {
  refreshViewInt(collective, false);
}

void WindowView::refreshView(const CreatureView* collective) {
  refreshViewInt(collective);
}

void WindowView::refreshViewInt(const CreatureView* collective, bool flipBuffer) {
  switchTiles();
  const Level* level = collective->getLevel();
  collective->refreshGameInfo(gameInfo);
  for (Vec2 pos : mapLayout->getAllTiles(Rectangle(maxTilesX, maxTilesY)))
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
  shadowed.clear();
  floorIds.clear();
  lastMemory = &collective->getMemory(level);
  for (Vec2 pos : mapLayout->getAllTiles(Rectangle(maxTilesX, maxTilesY))) 
    if (level->inBounds(pos)) {
      ViewIndex index = collective->getViewIndex(pos);
      if (!index.hasObject(ViewLayer::FLOOR) && !index.hasObject(ViewLayer::FLOOR_BACKGROUND) &&
          !index.isEmpty() && lastMemory->hasViewIndex(pos)) {
        // special case when monster or item is visible but floor is only in memory
        if (lastMemory->getViewIndex(pos).hasObject(ViewLayer::FLOOR))
          index.insert(lastMemory->getViewIndex(pos).getObject(ViewLayer::FLOOR));
        if (lastMemory->getViewIndex(pos).hasObject(ViewLayer::FLOOR_BACKGROUND))
          index.insert(lastMemory->getViewIndex(pos).getObject(ViewLayer::FLOOR_BACKGROUND));
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
  borderCreatures.clear();
 /* for (const Creature* c : collective->getVisibleCreatures())
    if (!c->getPosition().inRectangle(mapLayout->getAllTiles(Rectangle(maxTilesX, maxTilesY))))
      borderCreatures.insert(std::make_pair(c->getPosition(), c->getViewObject()));*/
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
  if (!lastMemory) {
    displayMenuSplash2();
    return;
  }

  int sizeX = mapLayout->squareWidth();
  int sizeY = mapLayout->squareHeight();
 Rectangle mapWindow = mapLayout->getBounds();
  drawFilledRectangle(mapWindow, black);
  bool pixelView = false;
  map<string, ViewObject> objIndex;
  Optional<ViewObject> highlighted;
  for (Vec2 wpos : mapLayout->getAllTiles(Rectangle(maxTilesX, maxTilesY))) {
    if (!objects[wpos])
      continue;
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
  }
  for (auto elem : borderCreatures) {
    Vec2 pos = mapLayout->projectOnScreen(elem.first);
    Vec2 proj = projectOnBorders(mapLayout->getBounds().minusMargin(10), pos);
    ViewIndex index;
    index.insert(elem.second);
      drawObjectAbs(
          proj.x,
          proj.y,
          index, sizeX / 2, sizeY / 2, elem.first);
    }
  if (pixelView)
    drawImage(mapLayout->getBounds().getPX(), mapLayout->getBounds().getPY(), mapBuffer);
  int rightPos = screenWidth -rightBarText;
  drawFilledRectangle(screenWidth - rightBarWidth, 0, screenWidth, screenHeight, translucentBlack);
  if (gameInfo.infoType == GameInfo::InfoType::PLAYER) {
    int cnt = 0;
    if (legendOption == LegendOption::OBJECTS) {
      for (auto elem : objIndex) {
        drawViewObject(elem.second, rightPos, legendStartHeight + cnt * 25, currentTileLayout.sprites);
        drawText(white, rightPos + 30, legendStartHeight + cnt * 25, elem.first);
        ++cnt;
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

int indexHeight(const vector<View::ListElem>& options, int index) {
  CHECK(index < options.size() && index >= 0);
  int tmp = 0;
  for (int i : All(options))
    if (!options[i].getMod() && tmp++ == index)
      return i;
  FAIL << "Bad index " << int(options.size()) << " " << index;
  return -1;
}

Optional<int> reverseIndexHeight(const vector<View::ListElem>& options, int height) {
  if (height < 0 || height >= options.size() || options[height].getMod())
    return Nothing();
  int sub = 0;
  for (int i : Range(height))
    if (options[i].getMod())
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

Optional<int> WindowView::getNumber(const string& title, int max) {
  vector<View::ListElem> options;
  for (int i : Range(1, max + 1))
    options.push_back(convertToString(i));
  Optional<int> res = WindowView::chooseFromList(title, options);
  if (!res)
    return Nothing();
  else
    return 1 + *res;
}

int getMaxLines();

Optional<int> getIndex(const vector<View::ListElem>& options, bool hasTitle, Vec2 mousePos);

Optional<int> WindowView::chooseFromList(const string& title, const vector<ListElem>& options, int index,
    Optional<ActionId> exitAction) {
  TempClockPause pause;
  if (options.size() == 0)
    return Nothing();
  int numLines = min((int) options.size(), getMaxLines());
  int count = 0;
  for (ListElem elem : options)
    if (!elem.getMod())
      ++count;
  if (count == 0) {
    presentList(title, options, false);
    return Nothing();
  }
  index = min(index, count - 1);
  while (1) {
    numLines = min((int) options.size(), getMaxLines());
    int height = indexHeight(options, index);
    int cutoff = min(max(0, height - numLines / 2), (int) options.size() - numLines);
    int itemsCutoff = 0;
    for (int i : Range(cutoff))
      if (!options[i].getMod())
        ++itemsCutoff;
    vector<ListElem> window = getPrefix(options, cutoff , numLines);
    drawList(title, window, index - itemsCutoff);
    BlockingEvent event = readkey();
    if (event.type == BlockingEvent::KEY)
      switch (event.key->code) {
        case Keyboard::PageDown: index += 10; if (index >= count) index = count - 1; break;
        case Keyboard::PageUp: index -= 10; if (index < 0) index = 0; break;
        case Keyboard::Numpad8:
        case Keyboard::Up: index = (index - 1 + count) % count;  break;
        case Keyboard::Numpad2:
        case Keyboard::Down: index = (index + 1 + count) % count; break;
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
  return (screenHeight - ySpacing - ySpacing - 2 * yMargin - itemSpacing - itemYMargin) / itemSpacing;
}

Optional<int> getIndex(const vector<View::ListElem>& options, bool hasTitle, Vec2 mousePos) {
  int xSpacing = (screenWidth - windowWidth) / 2;
  Rectangle window(xSpacing, ySpacing, xSpacing + windowWidth, screenHeight - ySpacing);
  if (!mousePos.inRectangle(window))
    return Nothing();
  return reverseIndexHeight(options,
      (mousePos.y - ySpacing - yMargin + (hasTitle ? 0 : itemSpacing) - itemYMargin) / itemSpacing - 1);
}

void WindowView::drawList(const string& title, const vector<ListElem>& options, int hightlight) {
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
    if (!options[i].getMod()) {
      if (hightlight > -1 && (h == i || (mouseHeight >= beginH && mouseHeight < beginH + itemSpacing))) 
        drawFilledRectangle(xSpacing + xMargin, beginH, 
            windowWidth + xSpacing - xMargin, beginH + itemSpacing - 1, darkGreen);
      drawText(white, xSpacing + xMargin + itemXMargin, beginH, options[i].getText());
    } else if (options[i].getMod() == View::TITLE)
      drawText(yellow, xSpacing + xMargin, beginH, options[i].getText());
    else if (options[i].getMod() == View::INACTIVE)
      drawText(gray, xSpacing + xMargin + itemXMargin, beginH, options[i].getText());
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
}

void WindowView::switchTiles() {
  bool normal = (mapLayout == currentTileLayout.normalLayout);
  if (Options::getValue(OptionId::ASCII))
    currentTileLayout = asciiLayouts;
  else
    currentTileLayout = spriteLayouts;
  if (normal)
    mapLayout = currentTileLayout.normalLayout;
  else
    mapLayout = currentTileLayout.unzoomLayout;
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
  switch (event.type) {
    case Event::MouseButtonReleased :
      if (event.mouseButton.button == sf::Mouse::Right) {
        center = {center.x - mouseOffset.x, center.y - mouseOffset.y};
        mouseOffset = {0.0, 0.0};
        rightMouseButtonPressed = false;
        return true;
      }
      break;
    case Event::MouseMoved:
      if (rightMouseButtonPressed) {
        mouseOffset = {double(event.mouseMove.x - lastMousePos.x) / mapLayout->squareWidth(),
                       double(event.mouseMove.y - lastMousePos.y) / mapLayout->squareHeight() };
        return true;
      }
      break;
    case Event::MouseButtonPressed:
      if (event.mouseButton.button == sf::Mouse::Right) {
        rightMouseButtonPressed = true;
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
            unzoom(event.key.shift, event.key.control);
            return CollectiveAction(CollectiveAction::IDLE);
          case Keyboard::F2: Options::handle(this); refreshScreen(); break;
          case Keyboard::Space:
            if (!myClock.isPaused())
              myClock.pause();
            else
              myClock.cont();
            return CollectiveAction(CollectiveAction::IDLE);
          case Keyboard::Escape:
            if (yesOrNoPrompt("Are you sure you want to abandon your game?"))
              throw GameOverException();
            break;
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
            Vec2 goTo = mapLayout->projectOnMap(*mousePos);
            if (goTo != lastGoTo && mousePos->inRectangle(getMapViewBounds())) {
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
 /*           if (marketButton && clickPos.inRectangle(*marketButton))
              return CollectiveAction(CollectiveAction::MARKET);*/
            for (int i : All(techButtons))
              if (clickPos.inRectangle(techButtons[i]))
                return CollectiveAction(CollectiveAction::TECHNOLOGY, i);
            if (teamButton && clickPos.inRectangle(*teamButton))
              return CollectiveAction(CollectiveAction::GATHER_TEAM);
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
            if (clickPos.inRectangle(getMapViewBounds())) {
              t = CollectiveAction::GO_TO;
              return CollectiveAction(t, mapLayout->projectOnMap(clickPos));
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
  { "F2", "Change options", {Keyboard::F2}},
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
      case Keyboard::Escape : if (yesOrNoPrompt("Are you sure you want to abandon your game?"))
                                throw GameOverException();
                              break;
      case Keyboard::Z: unzoom(key->shift, key->control); return Action(ActionId::IDLE);
      case Keyboard::F1: legendOption = (LegendOption)(1 - (int)legendOption); return Action(ActionId::IDLE);
      case Keyboard::F2: Options::handle(this); return Action(ActionId::IDLE);
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
      case Keyboard::S: return Action(ActionId::CAST_SPELL);
      default: break;
    }
  }
}

