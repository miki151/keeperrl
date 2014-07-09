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
#include "tile.h"
#include "view_id.h"

using namespace colors;

const set<Dir> Tile::allDirs {Dir::N, Dir::E, Dir::S, Dir::W, Dir::NE, Dir::SE, Dir::SW, Dir::NW};

Tile::Tile(sf::Uint32 ch, Color col, bool sym) : color(col), text(ch), symFont(sym) {
}

Tile::Tile(int x, int y, int num, bool _noShadow)
    : noShadow(_noShadow),tileCoord(Vec2(x, y)), texNum(num) {}

Tile::Tile(const string& s, bool noShadow) : Tile(Renderer::getTileCoords(s), noShadow) {
}

Tile Tile::empty() {
  return Tile(-1, -1, -1);
}

Tile::Tile(Renderer::TileCoords coords, bool noShadow)
    : Tile(coords.pos.x, coords.pos.y, coords.texNum, noShadow) {}

Tile& Tile::addConnection(set<Dir> c, int x, int y) {
  connections.insert({c, Vec2(x, y)});
  return *this;
}

Tile& Tile::addConnection(set<Dir> c, const string& name) {
  Renderer::TileCoords coords = Renderer::getTileCoords(name);
  texNum = coords.texNum;
  return addConnection(c, coords.pos.x, coords.pos.y);
}

Tile& Tile::addBackground(int x, int y) {
  backgroundCoord = Vec2(x, y);
  return *this;
}

Tile& Tile::addBackground(const string& name) {
  Renderer::TileCoords coords = Renderer::getTileCoords(name);
  texNum = coords.texNum;
  return addBackground(coords.pos.x, coords.pos.y);
}

Tile& Tile::setTranslucent(double v) {
  translucent = v;
  return *this;
}

bool Tile::hasSpriteCoord() {
  return tileCoord;
}

Vec2 Tile::getSpriteCoord() {
  return *tileCoord;
}

Optional<Vec2> Tile::getBackgroundCoord() {
  return backgroundCoord;
}

Vec2 Tile::getSpriteCoord(const set<Dir>& c) {
  if (connections.count(c))
    return connections.at(c);
  else return *tileCoord;
}

int Tile::getTexNum() {
  CHECK(tileCoord) << "Not a sprite tile";
  return texNum;
}

Tile getSpecialCreature(const ViewObject& obj, bool humanoid) {
  RandomGen r;
  r.init(hash<string>()(obj.getBareDescription()));
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
  r.init(hash<string>()(obj.getBareDescription()));
  if (humanoid)
    return Tile(r.getRandom(7), 10);
  else
    return Tile(r.getRandom(7, 10), 10);
}

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

static Tile getSprite(ViewId id) {
  static map<ViewId, Tile> tiles {
    { ViewId::EMPTY, Tile(' ', black)},
    { ViewId::PLAYER, Tile(1, 0)},
    { ViewId::KEEPER, Tile(3, 0)},
    { ViewId::UNKNOWN_MONSTER, Tile('?', lightGreen)},
    { ViewId::ELF, Tile(10, 6)},
    { ViewId::ELF_ARCHER, Tile(12, 6)},
    { ViewId::ELF_CHILD, Tile(14, 6)},
    { ViewId::ELF_LORD, Tile(13, 6)},
    { ViewId::ELVEN_SHOPKEEPER, Tile(4, 2)},
    { ViewId::LIZARDMAN, Tile(8, 8)},
    { ViewId::LIZARDLORD, Tile(11, 8)},
    { ViewId::IMP, Tile(18, 19)},
    { ViewId::PRISONER, Tile("prisoner")},
    { ViewId::BILE_DEMON, Tile(8, 14)},
    { ViewId::CHICKEN, Tile("chicken")},
    { ViewId::DWARF, Tile(2, 6)},
    { ViewId::DWARF_BARON, Tile(3, 6)},
    { ViewId::DWARVEN_SHOPKEEPER, Tile(4, 2)},
    { ViewId::BRIDGE, Tile(24, 0, 4)},
    { ViewId::ROAD, getRoadTile(7)},
    { ViewId::PATH, Tile(3, 14, 1)},
    { ViewId::FLOOR, Tile(3, 14, 1)},
    { ViewId::SAND, Tile(7, 12, 2)},
    { ViewId::MUD, Tile(3, 12, 2)},
    { ViewId::GRASS, Tile(0, 13, 2)},
    { ViewId::CROPS, Tile(9, 12, 2)},
    { ViewId::WALL, getWallTile(2)},
    { ViewId::MOUNTAIN, Tile(17, 2, 2, true)},
    { ViewId::MOUNTAIN2, getWallTile(19)},
    { ViewId::GOLD_ORE, Tile("gold")},
    { ViewId::IRON_ORE, Tile("iron2")},
    { ViewId::STONE, Tile("stone")},
    { ViewId::SNOW, Tile(16, 2, 2, true)},
    { ViewId::HILL, Tile(3, 13, 2)},
    { ViewId::WOOD_WALL, getWallTile(4)},
    { ViewId::BLACK_WALL, getWallTile(2)},
    { ViewId::YELLOW_WALL, getWallTile(8)},
    { ViewId::LOW_ROCK_WALL, getWallTile(21)},
    { ViewId::HELL_WALL, getWallTile(22)},
    { ViewId::CASTLE_WALL, getWallTile(5)},
    { ViewId::MUD_WALL, getWallTile(13)},
    { ViewId::SECRETPASS, Tile(0, 15, 1)},
    { ViewId::DUNGEON_ENTRANCE, Tile(15, 2, 2, true)},
    { ViewId::DUNGEON_ENTRANCE_MUD, Tile(19, 2, 2, true)},
    { ViewId::DOWN_STAIRCASE, Tile(8, 0, 1, true)},
    { ViewId::UP_STAIRCASE, Tile(7, 0, 1, true)},
    { ViewId::DOWN_STAIRCASE_CELLAR, Tile(8, 21, 1, true)},
    { ViewId::UP_STAIRCASE_CELLAR, Tile(7, 21, 1, true)},
    { ViewId::DOWN_STAIRCASE_HELL, Tile(8, 1, 1, true)},
    { ViewId::UP_STAIRCASE_HELL, Tile(7, 22, 1, true)},
    { ViewId::DOWN_STAIRCASE_PYR, Tile(8, 8, 1, true)},
    { ViewId::UP_STAIRCASE_PYR, Tile(7, 8, 1, true)},
    { ViewId::WELL, Tile(5, 8, 2, true)},
    { ViewId::STATUE1, Tile(6, 5, 2, true)},
    { ViewId::STATUE2, Tile(7, 5, 2, true)},
    { ViewId::GREAT_GOBLIN, Tile(6, 14)},
    { ViewId::GOBLIN, Tile("orc")},
    { ViewId::BANDIT, Tile(0, 2)},
    { ViewId::GHOST, Tile(6, 16).setTranslucent(0.5)},
    { ViewId::SPIRIT, Tile(17, 14)},
    { ViewId::DEVIL, Tile(17, 18)},
    { ViewId::DARK_KNIGHT, Tile(12, 14)},
    { ViewId::GREEN_DRAGON, Tile(3, 18)},
    { ViewId::RED_DRAGON, Tile(0, 18)},
    { ViewId::CYCLOPS, Tile(10, 14)},
    { ViewId::WITCH, Tile(15, 16)},
    { ViewId::KNIGHT, Tile(0, 0)},
    { ViewId::WARRIOR, Tile(6, 0)},
    { ViewId::SHAMAN, Tile(5, 0)},
    { ViewId::CASTLE_GUARD, Tile(15, 2)},
    { ViewId::AVATAR, Tile(9, 0)},
    { ViewId::ARCHER, Tile(2, 0)},
    { ViewId::PESEANT, Tile(1, 2)},
    { ViewId::CHILD, Tile(2, 2)},
    { ViewId::CLAY_GOLEM, Tile(12, 11)},
    { ViewId::STONE_GOLEM, Tile(10, 10)},
    { ViewId::IRON_GOLEM, Tile(12, 10)},
    { ViewId::LAVA_GOLEM, Tile(13, 10)},
    { ViewId::ZOMBIE, Tile(0, 16)},
    { ViewId::SKELETON, Tile(2, 16)},
    { ViewId::VAMPIRE, Tile("vampire3")},
    { ViewId::VAMPIRE_LORD, Tile(13, 16)},
    { ViewId::MUMMY, Tile(7, 16)},
    { ViewId::MUMMY_LORD, Tile(8, 16)},
    { ViewId::ACID_MOUND, Tile(1, 12)},
    { ViewId::JACKAL, Tile(12, 12)},
    { ViewId::DEER, Tile("deer")},
    { ViewId::HORSE, Tile("horse")},
    { ViewId::COW, Tile("cow")},
    { ViewId::SHEEP, Tile('s', white)},
    { ViewId::PIG, Tile("pig")},
    { ViewId::GOAT, Tile("goat")},
    { ViewId::BOAR, Tile(18, 6)},
    { ViewId::FOX, Tile(13, 12)},
    { ViewId::WOLF, Tile(14, 12)},
    { ViewId::DOG, Tile("dog")},
    { ViewId::VODNIK, Tile('f', green)},
    { ViewId::KRAKEN, Tile(7, 19)},
    { ViewId::DEATH, Tile(9, 16)},
    { ViewId::KRAKEN2, Tile(7, 19)},
    { ViewId::NIGHTMARE, Tile(9, 16)},
    { ViewId::FIRE_SPHERE, Tile(16, 20)},
    { ViewId::BEAR, Tile(8, 18)},
    { ViewId::BAT, Tile(2, 12)},
    { ViewId::GNOME, Tile("goblin")},
    { ViewId::LEPRECHAUN, Tile(16, 8)},
    { ViewId::RAT, Tile("rat")},
    { ViewId::SPIDER, Tile(6, 12)},
    { ViewId::FLY, Tile(10, 12)},
    { ViewId::SCORPION, Tile(11, 18)},
    { ViewId::SNAKE, Tile(9, 12)},
    { ViewId::VULTURE, Tile(17, 12)},
    { ViewId::RAVEN, Tile(17, 12)},
    { ViewId::BODY_PART, Tile(9, 4, 3)},
    { ViewId::BONE, Tile(3, 0, 2)},
    { ViewId::BUSH, Tile(17, 0, 2, true)},
    { ViewId::DECID_TREE, Tile(21, 3, 2, true)},
    { ViewId::CANIF_TREE, Tile(20, 3, 2, true)},
    { ViewId::TREE_TRUNK, Tile(26, 3, 2, true)},
    { ViewId::BURNT_TREE, Tile(25, 3, 2, true)},
    { ViewId::WATER, getWaterTile(5)},
    { ViewId::MAGMA, getWaterTile(11)},
    { ViewId::ABYSS, Tile('~', darkGray)},
    { ViewId::DOOR, Tile(4, 2, 2, true)},
    { ViewId::BARRICADE, Tile(13, 10, 2, true)},
    { ViewId::DIG_ICON, Tile(8, 10, 2)},
    { ViewId::SWORD, Tile(12, 9, 3)},
    { ViewId::SPEAR, Tile(5, 8, 3)},
    { ViewId::SPECIAL_SWORD, Tile(13, 9, 3)},
    { ViewId::ELVEN_SWORD, Tile(14, 9, 3)},
    { ViewId::KNIFE, Tile(20, 9, 3)},
    { ViewId::WAR_HAMMER, Tile(10, 7, 3)},
    { ViewId::SPECIAL_WAR_HAMMER, Tile(11, 7, 3)},
    { ViewId::BATTLE_AXE, Tile(13, 7, 3)},
    { ViewId::SPECIAL_BATTLE_AXE, Tile(21, 7, 3)},
    { ViewId::BOW, Tile(14, 8, 3)},
    { ViewId::ARROW, Tile(5, 8, 3)},
    { ViewId::SCROLL, Tile(3, 6, 3)},
    { ViewId::STEEL_AMULET, Tile(1, 1, 3)},
    { ViewId::COPPER_AMULET, Tile(2, 1, 3)},
    { ViewId::CRYSTAL_AMULET, Tile(4, 1, 3)},
    { ViewId::WOODEN_AMULET, Tile(0, 1, 3)},
    { ViewId::AMBER_AMULET, Tile(3, 1, 3)},
    { ViewId::FIRE_RESIST_RING, Tile(11, 3, 3)},
    { ViewId::POISON_RESIST_RING, Tile(16, 3, 3)},
    { ViewId::BOOK, Tile(0, 3, 3)},
    { ViewId::FIRST_AID, Tile(12, 2, 3)},
    { ViewId::TRAP_ITEM, Tile(12, 4, 3)},
    { ViewId::EFFERVESCENT_POTION, Tile(6, 0, 3)},
    { ViewId::MURKY_POTION, Tile(10, 0, 3)},
    { ViewId::SWIRLY_POTION, Tile(9, 0, 3)},
    { ViewId::VIOLET_POTION, Tile(7, 0, 3)},
    { ViewId::PUCE_POTION, Tile(8, 0, 3)},
    { ViewId::SMOKY_POTION, Tile(11, 0, 3)},
    { ViewId::FIZZY_POTION, Tile(9, 0, 3)},
    { ViewId::MILKY_POTION, Tile(11, 0, 3)},
    { ViewId::MUSHROOM, Tile(5, 4, 3)},
    { ViewId::FOUNTAIN, Tile(0, 7, 2, true)},
    { ViewId::GOLD, Tile(8, 3, 3, true)},
    { ViewId::TREASURE_CHEST, Tile("treasurydeco", true).addBackground("treasury")},
    { ViewId::CHEST, Tile(3, 3, 2, true)},
    { ViewId::OPENED_CHEST, Tile(6, 3, 2, true)},
    { ViewId::COFFIN, Tile(7, 3, 2, true)},
    { ViewId::OPENED_COFFIN, Tile(8, 3, 2, true)},
    { ViewId::BOULDER, Tile("boulder")},
    { ViewId::PORTAL, Tile("surprise")},
    { ViewId::TRAP, Tile(L'➹', yellow, true)},
    { ViewId::GAS_TRAP, Tile(2, 6, 3)},
    { ViewId::ALARM_TRAP, Tile(16, 5, 3)},
    { ViewId::WEB_TRAP, Tile(4, 1, 2)},
    { ViewId::SURPRISE_TRAP, Tile(9, 10, 2)},
    { ViewId::TERROR_TRAP, Tile(1, 6, 3)},
    { ViewId::ROCK, Tile("stonepile")},
    { ViewId::IRON_ROCK, Tile("ironpile2")},
    { ViewId::WOOD_PLANK, Tile("wood2")},
    { ViewId::STOCKPILE1, Tile("storage1")},
    { ViewId::STOCKPILE2, Tile("storage2")},
    { ViewId::STOCKPILE3, Tile("storage3")},
    { ViewId::PRISON, Tile(6, 2, 1)},
    { ViewId::BED, Tile("sleepdeco", true)},
    { ViewId::KEEPER_BED, Tile("sleepdeco", true).addBackground("sleep")},
    { ViewId::DORM, Tile("sleep")},
    { ViewId::TORCH, Tile(13, 1, 2, true).setTranslucent(0.35)},
    { ViewId::DUNGEON_HEART, Tile(6, 10, 2)},
    { ViewId::ALTAR, Tile(2, 7, 2, true)},
    { ViewId::CREATURE_ALTAR, Tile(3, 7, 2, true)},
    { ViewId::TORTURE_TABLE, Tile::empty().addConnection(Tile::allDirs, "torturedeco").addBackground("torture")},
    { ViewId::IMPALED_HEAD, Tile(10, 10, 2, true)},
    { ViewId::TRAINING_ROOM, Tile::empty().addConnection(Tile::allDirs, "traindeco").addBackground("train")},
    { ViewId::LIBRARY, Tile::empty().addConnection(Tile::allDirs, "libdeco").addBackground("lib")},
    { ViewId::LABORATORY, Tile::empty().addConnection(Tile::allDirs, "labdeco").addBackground("lab")},
    { ViewId::BEAST_LAIR, Tile("lair")},
    { ViewId::BEAST_CAGE, Tile("lairdeco", true).addBackground("lair")},
    { ViewId::WORKSHOP, Tile::empty().addConnection(Tile::allDirs, "workshopdeco").addBackground("workshop")},
    { ViewId::CEMETERY, Tile("graveyard")},
    { ViewId::GRAVE, Tile("gravedeco", true).addBackground("graveyard")},
    { ViewId::BARS, Tile(L'⧻', lightBlue)},
    { ViewId::BORDER_GUARD, Tile(' ', white)},
    { ViewId::ROBE, Tile(7, 11, 3)},
    { ViewId::LEATHER_GLOVES, Tile(15, 11, 3)},
    { ViewId::DEXTERITY_GLOVES, Tile(19, 11, 3)},
    { ViewId::STRENGTH_GLOVES, Tile(20, 11, 3)},
    { ViewId::LEATHER_ARMOR, Tile(0, 12, 3)},
    { ViewId::LEATHER_HELM, Tile(10, 12, 3)},
    { ViewId::TELEPATHY_HELM, Tile(17, 12, 3)},
    { ViewId::CHAIN_ARMOR, Tile(1, 12, 3)},
    { ViewId::IRON_HELM, Tile(14, 12, 3)},
    { ViewId::LEATHER_BOOTS, Tile(0, 13, 3)},
    { ViewId::IRON_BOOTS, Tile(6, 13, 3)},
    { ViewId::SPEED_BOOTS, Tile(3, 13, 3)},
    { ViewId::LEVITATION_BOOTS, Tile(2, 13, 3)},
    { ViewId::DESTROYED_FURNITURE, Tile('*', brown)},
    { ViewId::BURNT_FURNITURE, Tile('*', darkGray)},
    { ViewId::FALLEN_TREE, Tile(26, 3, 2, true)},
    { ViewId::GUARD_POST, Tile("guardroom")},
    { ViewId::DESTROY_BUTTON, Tile('X', red)},
    { ViewId::MANA, Tile(5, 10, 2)},
    { ViewId::DANGER, Tile(12, 9, 2)},
    { ViewId::FETCH_ICON, Tile(15, 11, 3)}};
  if (tiles.count(id))
    return tiles.at(id);
  else
    FAIL << "unhatndled view id " << (int)id;
  return Tile(' ', white);
}

static vector<Optional<Tile>> tileCache;

Tile getSpriteTile(const ViewObject& obj) {
  if (obj.id() == ViewId::SPECIAL_BEAST)
    return getSpecialCreatureSprite(obj, false);
  if (obj.id() == ViewId::SPECIAL_HUMANOID)
    return getSpecialCreatureSprite(obj, true);
  int numId = int(obj.id());
  if (numId >= tileCache.size())
    tileCache.resize(numId + 1);
  if (!tileCache[numId])
    tileCache[numId] = getSprite(obj.id());
  return *tileCache[numId];
}

static Tile getAscii(ViewId id) {
  switch (id) {
    case ViewId::EMPTY: return Tile(' ', black);
    case ViewId::PLAYER: return Tile('@', white);
    case ViewId::KEEPER: return Tile('@', purple);
    case ViewId::UNKNOWN_MONSTER: return Tile('?', lightGreen);
    case ViewId::ELF: return Tile('@', lightGreen);
    case ViewId::ELF_ARCHER: return Tile('@', green);
    case ViewId::ELF_CHILD: return Tile('@', lightGreen);
    case ViewId::ELF_LORD: return Tile('@', darkGreen);
    case ViewId::ELVEN_SHOPKEEPER: return Tile('@', lightBlue);
    case ViewId::LIZARDMAN: return Tile('@', lightBrown);
    case ViewId::LIZARDLORD: return Tile('@', brown);
    case ViewId::IMP: return Tile('i', lightBrown);
    case ViewId::PRISONER: return Tile('@', lightBrown);
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
    case ViewId::IRON_ORE: return Tile(L'⁂', darkBrown, true);
    case ViewId::STONE: return Tile(L'⁂', lightGray, true);
    case ViewId::SNOW: return Tile(0x25ee, white, true);
    case ViewId::HILL: return Tile(0x1d022, darkGreen, true);
    case ViewId::WOOD_WALL: return Tile('#', darkBrown);
    case ViewId::BLACK_WALL: return Tile('#', lightGray);
    case ViewId::YELLOW_WALL: return Tile('#', yellow);
    case ViewId::LOW_ROCK_WALL: return Tile('#', darkGray);
    case ViewId::HELL_WALL: return Tile('#', red);
    case ViewId::SECRETPASS: return Tile('#', lightGray);
    case ViewId::DUNGEON_ENTRANCE:
    case ViewId::DUNGEON_ENTRANCE_MUD: return Tile(0x2798, brown, true);
    case ViewId::DOWN_STAIRCASE_CELLAR:
    case ViewId::DOWN_STAIRCASE: return Tile(0x2798, almostWhite, true);
    case ViewId::UP_STAIRCASE_CELLAR:
    case ViewId::UP_STAIRCASE: return Tile(0x279a, almostWhite, true);
    case ViewId::DOWN_STAIRCASE_HELL: return Tile(0x2798, red, true);
    case ViewId::UP_STAIRCASE_HELL: return Tile(0x279a, red, true);
    case ViewId::DOWN_STAIRCASE_PYR: return Tile(0x2798, yellow, true);
    case ViewId::UP_STAIRCASE_PYR: return Tile(0x279a, yellow, true);
    case ViewId::WELL: return Tile('0', blue);
    case ViewId::STATUE1:
    case ViewId::STATUE2: return Tile('&', lightGray);
    case ViewId::GREAT_GOBLIN: return Tile('O', purple);
    case ViewId::GOBLIN: return Tile('o', darkBlue);
    case ViewId::BANDIT: return Tile('@', darkBlue);
    case ViewId::DARK_KNIGHT: return Tile('@', purple);
    case ViewId::GREEN_DRAGON: return Tile('D', green);
    case ViewId::RED_DRAGON: return Tile('D', red);
    case ViewId::CYCLOPS: return Tile('C', green);
    case ViewId::WITCH: return Tile('@', brown);
    case ViewId::GHOST: return Tile('&', white);
    case ViewId::SPIRIT: return Tile('&', lightBlue);
    case ViewId::DEVIL: return Tile('&', purple);
    case ViewId::CASTLE_GUARD: return Tile('@', lightGray);
    case ViewId::KNIGHT: return Tile('@', lightGray);
    case ViewId::WARRIOR: return Tile('@', darkGray);
    case ViewId::SHAMAN: return Tile('@', yellow);
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
    case ViewId::GOAT: return Tile('g', gray);
    case ViewId::BOAR: return Tile('b', lightBrown);
    case ViewId::FOX: return Tile('d', orangeBrown);
    case ViewId::WOLF: return Tile('d', darkBlue);
    case ViewId::DOG: return Tile('d', brown);
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
    case ViewId::BURNT_TREE: return Tile('.', darkGray);
    case ViewId::WATER: return Tile('~', lightBlue);
    case ViewId::MAGMA: return Tile('~', red);
    case ViewId::ABYSS: return Tile('~', darkGray);
    case ViewId::DOOR: return Tile('|', brown);
    case ViewId::BARRICADE: return Tile('X', brown);
    case ViewId::DIG_ICON: return Tile(0x26cf, lightGray, true);
    case ViewId::SWORD: return Tile(')', lightGray);
    case ViewId::SPEAR: return Tile('/', lightGray);
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
    case ViewId::FIRE_RESIST_RING: return Tile('=', red);
    case ViewId::POISON_RESIST_RING: return Tile('=', green);
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
    case ViewId::MUSHROOM: return Tile(0x22c6, pink, true);
    case ViewId::FOUNTAIN: return Tile('0', lightBlue);
    case ViewId::GOLD: return Tile('$', yellow);
    case ViewId::TREASURE_CHEST:
    case ViewId::OPENED_CHEST:
    case ViewId::CHEST: return Tile('=', brown);
    case ViewId::OPENED_COFFIN:
    case ViewId::COFFIN: return Tile(L'⚰', darkGray, true);
    case ViewId::BOULDER: return Tile(L'●', lightGray, true);
    case ViewId::PORTAL: return Tile(0x1d6af, lightGreen, true);
    case ViewId::TRAP: return Tile(L'➹', yellow, true);
    case ViewId::GAS_TRAP: return Tile(L'☠', green, true);
    case ViewId::ALARM_TRAP: return Tile(L'^', red, true);
    case ViewId::WEB_TRAP: return Tile('#', white, true);
    case ViewId::SURPRISE_TRAP: return Tile('^', blue, true);
    case ViewId::TERROR_TRAP: return Tile('^', white, true);
    case ViewId::ROCK: return Tile('*', lightGray);
    case ViewId::IRON_ROCK: return Tile('*', orange);
    case ViewId::WOOD_PLANK: return Tile('\\', brown);
    case ViewId::STOCKPILE1: return Tile('.', yellow);
    case ViewId::STOCKPILE2: return Tile('.', lightGreen);
    case ViewId::STOCKPILE3: return Tile('.', lightBlue);
    case ViewId::PRISON: return Tile('.', blue);
    case ViewId::DORM: return Tile('.', brown);
    case ViewId::KEEPER_BED:
    case ViewId::BED: return Tile('=', white);
    case ViewId::DUNGEON_HEART: return Tile(L'♥', white, true);
    case ViewId::TORCH: return Tile('I', orange);
    case ViewId::ALTAR: return Tile(L'Ω', white);
    case ViewId::CREATURE_ALTAR: return Tile(L'Ω', yellow);
    case ViewId::TORTURE_TABLE: return Tile('=', gray);
    case ViewId::IMPALED_HEAD: return Tile(L'𐌒', brown);
    case ViewId::TRAINING_ROOM: return Tile(L'‡', brown, true);
    case ViewId::LIBRARY: return Tile(L'▤', brown, true);
    case ViewId::LABORATORY: return Tile(L'ω', purple, true);
    case ViewId::BEAST_LAIR: return Tile('.', yellow);
    case ViewId::BEAST_CAGE: return Tile(L'▥', lightGray, true);
    case ViewId::WORKSHOP: return Tile('&', lightBlue);
    case ViewId::CEMETERY: return Tile('.', darkBlue);
    case ViewId::GRAVE: return Tile(0x2617, gray, true);
    case ViewId::BARS: return Tile(L'⧻', lightBlue);
    case ViewId::BORDER_GUARD: return Tile(' ', white);
    case ViewId::ROBE: return Tile('[', lightBrown);
    case ViewId::LEATHER_GLOVES: return Tile('[', brown);
    case ViewId::STRENGTH_GLOVES: return Tile('[', red);
    case ViewId::DEXTERITY_GLOVES: return Tile('[', lightBlue);
    case ViewId::LEATHER_ARMOR: return Tile('[', brown);
    case ViewId::LEATHER_HELM: return Tile('[', brown);
    case ViewId::TELEPATHY_HELM: return Tile('[', lightGreen);
    case ViewId::CHAIN_ARMOR: return Tile('[', lightGray);
    case ViewId::IRON_HELM: return Tile('[', lightGray);
    case ViewId::LEATHER_BOOTS: return Tile('[', brown);
    case ViewId::IRON_BOOTS: return Tile('[', lightGray);
    case ViewId::SPEED_BOOTS: return Tile('[', lightBlue);
    case ViewId::LEVITATION_BOOTS: return Tile('[', lightGreen);
    case ViewId::DESTROYED_FURNITURE: return Tile('*', brown);
    case ViewId::BURNT_FURNITURE: return Tile('*', darkGray);
    case ViewId::FALLEN_TREE: return Tile('*', green);
    case ViewId::GUARD_POST: return Tile(L'⚐', yellow, true);
    case ViewId::DESTROY_BUTTON: return Tile('X', red);
    case ViewId::MANA: return Tile('*', blue);
    case ViewId::DANGER: return Tile('*', red);
    case ViewId::FETCH_ICON: return Tile(0x1f44b, lightBrown);
    default: FAIL << "Not handled " << int(id);
  }
  FAIL << "unhandled view id " << (int)id;
  return Tile(' ', white);
}

static vector<Optional<Tile>> tileCache2;

Tile getAsciiTile(const ViewObject& obj) {
  if (obj.id() == ViewId::SPECIAL_BEAST)
    return getSpecialCreature(obj, false);
  if (obj.id() == ViewId::SPECIAL_HUMANOID)
    return getSpecialCreature(obj, true);
  int numId = int(obj.id());
  if (numId >= tileCache2.size())
    tileCache2.resize(numId + 1);
  if (!tileCache2[numId])
    tileCache2[numId] = getAscii(obj.id());
  return *tileCache2[numId];
}

Tile Tile::getTile(const ViewObject& obj, bool sprite) {
  if (sprite)
    return getSpriteTile(obj);
  else
    return getAsciiTile(obj);
}

Color Tile::getColor(const ViewObject& object) {
  if (object.hasModifier(ViewObject::Modifier::INVISIBLE))
    return darkGray;
  if (object.hasModifier(ViewObject::Modifier::HIDDEN))
    return lightGray;
  double bleeding = object.getAttribute(ViewObject::Attribute::BLEEDING);
  if (bleeding > 0)
    bleeding = 0.5 + bleeding / 2;
  bleeding = min(1., bleeding);
  Color color = getAsciiTile(object).color;
  return Color(
      (1 - bleeding) * color.r + bleeding * 255,
      (1 - bleeding) * color.g,
      (1 - bleeding) * color.b);
}


