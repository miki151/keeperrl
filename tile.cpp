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
#include "gui_elem.h"

Tile::Tile() {
}

Tile Tile::unicode(sf::Uint32 ch, ColorId col, bool sym) {
  Tile ret;
  ret.color = colors[col];
  ret.text = ch;
  ret.symFont = sym;
  return ret;
}

Tile Tile::fromString(const string& ch, ColorId colorId, bool symbol) {
  String tmp = Renderer::toUnicode(ch);
  CHECK(tmp.getSize() == 1) << "Symbol text too long: " << ch;
  return unicode(tmp[0], colorId, symbol);
}

Tile::Tile(TileCoord c) : tileCoord(c) {
}

Tile Tile::setNoShadow() {
  noShadow = true;
  return *this;
}

Tile Tile::byCoord(TileCoord c) {
  return Tile(c);
}

Tile Tile::setFloorBorders() {
  floorBorders = true;
  return *this;
}

Tile Tile::addConnection(EnumSet<Dir> c, TileCoord coord) {
  connections.insert({c, coord});
  return *this;
}

Tile Tile::addOption(Dir d, TileCoord coord) {
  connectionOption = {d, coord};
  return *this;
}

Tile Tile::addBackground(TileCoord coord) {
  backgroundCoord = coord;
  return *this;
}

Tile Tile::addExtraBorder(EnumSet<Dir> dir, TileCoord coord) {
  extraBorders[dir] = coord;
  return *this;
}

Tile Tile::addExtraBorderId(ViewId id) {
  extraBorderIds.push_back(id);
  return *this;
}

const vector<ViewId>& Tile::getExtraBorderIds() const {
  return extraBorderIds;
}

bool Tile::hasExtraBorders() const {
  return !extraBorders.empty();
}

Optional<Tile::TileCoord> Tile::getExtraBorderCoord(const EnumSet<Dir>& c) const {
  if (extraBorders.count(c))
    return extraBorders.at(c);
  else
    return Nothing();
}

Tile Tile::setTranslucent(double v) {
  translucent = v;
  return *this;
}

bool Tile::hasSpriteCoord() const {
  return tileCoord;
}

Tile::TileCoord Tile::getSpriteCoord() const {
  return *tileCoord;
}

Optional<Tile::TileCoord> Tile::getBackgroundCoord() const {
  return backgroundCoord;
}

Tile::TileCoord Tile::getSpriteCoord(const EnumSet<Dir>& c) const {
  if (connectionOption) {
    CHECK(connections.empty()) << "Can't have connections and options at the same time";
    if (c[connectionOption->first])
      return connectionOption->second;
    else
      return *tileCoord;
  }
  if (connections.count(c))
    return connections.at(c);
  else return *tileCoord;
}

static map<ViewId, Tile> tiles;
static map<ViewId, Tile> symbols;

typedef EnumSet<Dir> SetOfDir;

namespace {

void addTile(ViewId id, Tile tile) {
  tiles.erase(id);
  tiles.insert({id, tile});
}

void addSymbol(ViewId id, Tile tile) {
  symbols.erase(id);
  symbols.insert({id, tile});
}

}

class TileCoordLookup {
  public:
  TileCoordLookup(Renderer& r) : renderer(r) {}

  void loadTiles() {
    genTiles();
    bool bad = false;
    for (ViewId id : ENUM_ALL(ViewId))
      if (!tiles.count(id)) {
        Debug() << "ViewId not found: " << EnumInfo<ViewId>::getString(id);
        bad = true;
      }
    CHECK(!bad);
  }

  void loadUnicode() {
    genSymbols();
    bool bad = false;
    for (ViewId id : ENUM_ALL(ViewId))
      if (!symbols.count(id)) {
        Debug() << "ViewId not found: " << EnumInfo<ViewId>::getString(id);
        bad = true;
      }
    CHECK(!bad);
  }

  Tile::TileCoord byName(const string& s) {
    return renderer.getTileCoord(s);
  }

  Tile::TileCoord byCoord(int x, int y, int tex) {
    return {Vec2(x, y), tex};
  }

  Tile sprite(int x, int y, int tex) {
    return Tile::byCoord({Vec2(x, y), tex});
  }

  Tile sprite(int x, int y) {
    return sprite(x, y, 0);
  }

  Tile sprite(const string& s) {
    return Tile::byCoord(byName(s));
  }

  Tile empty() {
    return sprite("empty");
  }

  Tile getRoadTile(int pathSet) {
    int tex = 5;
    return sprite(0, pathSet, tex)
      .addConnection({Dir::E, Dir::W}, byCoord(2, pathSet, tex))
      .addConnection({Dir::W}, byCoord(3, pathSet, tex))
      .addConnection({Dir::E}, byCoord(1, pathSet, tex))
      .addConnection({Dir::S}, byCoord(4, pathSet, tex))
      .addConnection({Dir::N, Dir::S}, byCoord(5, pathSet, tex))
      .addConnection({Dir::N}, byCoord(6, pathSet, tex))
      .addConnection({Dir::S, Dir::E}, byCoord(7, pathSet, tex))
      .addConnection({Dir::S, Dir::W}, byCoord(8, pathSet, tex))
      .addConnection({Dir::N, Dir::E}, byCoord(9, pathSet, tex))
      .addConnection({Dir::N, Dir::W}, byCoord(10, pathSet, tex))
      .addConnection({Dir::N, Dir::E, Dir::S, Dir::W}, byCoord(11, pathSet, tex))
      .addConnection({Dir::E, Dir::S, Dir::W}, byCoord(12, pathSet, tex))
      .addConnection({Dir::N, Dir::S, Dir::W}, byCoord(13, pathSet, tex))
      .addConnection({Dir::N, Dir::E, Dir::S}, byCoord(14, pathSet, tex))
      .addConnection({Dir::N, Dir::E, Dir::W}, byCoord(15, pathSet, tex));
  }

  Tile getWallTile(int wallSet) {
    int tex = 1;
    return sprite(9, wallSet, tex).setNoShadow()
      .addConnection({Dir::E, Dir::W}, byCoord(11, wallSet, tex))
      .addConnection({Dir::W}, byCoord(12, wallSet, tex))
      .addConnection({Dir::E}, byCoord(10, wallSet, tex))
      .addConnection({Dir::S}, byCoord(13, wallSet, tex))
      .addConnection({Dir::N, Dir::S}, byCoord(14, wallSet, tex))
      .addConnection({Dir::N}, byCoord(15, wallSet, tex))
      .addConnection({Dir::E, Dir::S}, byCoord(16, wallSet, tex))
      .addConnection({Dir::S, Dir::W}, byCoord(17, wallSet, tex))
      .addConnection({Dir::N, Dir::E}, byCoord(18, wallSet, tex))
      .addConnection({Dir::N, Dir::W}, byCoord(19, wallSet, tex))
      .addConnection({Dir::N, Dir::E, Dir::S, Dir::W}, byCoord(20, wallSet, tex))
      .addConnection({Dir::E, Dir::S, Dir::W}, byCoord(21, wallSet, tex))
      .addConnection({Dir::N, Dir::S, Dir::W}, byCoord(22, wallSet, tex))
      .addConnection({Dir::N, Dir::E, Dir::S}, byCoord(23, wallSet, tex))
      .addConnection({Dir::N, Dir::E, Dir::W}, byCoord(24, wallSet, tex));
  }

  Tile getMountainTile(int numT, int set) {
    int tex = 1;
    return sprite(numT, set, tex).setNoShadow()
      .addCorner({Dir::S, Dir::W}, {Dir::W}, byCoord(10, set, tex))
      .addCorner({Dir::N, Dir::W}, {Dir::W}, byCoord(11, set, tex))
      .addCorner({Dir::S, Dir::E}, {Dir::E}, byCoord(12, set, tex))
      .addCorner({Dir::N, Dir::E}, {Dir::E}, byCoord(13, set, tex))
      .addCorner({Dir::N, Dir::W}, {Dir::N}, byCoord(14, set, tex))
      .addCorner({Dir::N, Dir::E}, {Dir::N}, byCoord(15, set, tex))
      .addCorner({Dir::S, Dir::W}, {Dir::S}, byCoord(16, set, tex))
      .addCorner({Dir::S, Dir::E}, {Dir::S}, byCoord(17, set, tex))
      .addCorner({Dir::N, Dir::W}, {}, byCoord(18, set, tex))
      .addCorner({Dir::N, Dir::E}, {}, byCoord(19, set, tex))
      .addCorner({Dir::S, Dir::W}, {}, byCoord(20, set, tex))
      .addCorner({Dir::S, Dir::E}, {}, byCoord(21, set, tex))
      .addCorner({Dir::S, Dir::E, Dir::SE}, {Dir::S, Dir::E}, byCoord(22, set, tex))
      .addCorner({Dir::S, Dir::W, Dir::SW}, {Dir::S, Dir::W}, byCoord(23, set, tex))
      .addCorner({Dir::N, Dir::E, Dir::NE}, {Dir::N, Dir::E}, byCoord(24, set, tex))
      .addCorner({Dir::N, Dir::W, Dir::NW}, {Dir::N, Dir::W}, byCoord(25, set, tex));
  }

  Tile getWaterTile(int leftX) {
    int tex = 4;
    return sprite(leftX, 5, tex)
      .addConnection({Dir::N, Dir::E, Dir::S, Dir::W}, byCoord(leftX - 1, 7, tex))
      .addConnection({Dir::E, Dir::S, Dir::W}, byCoord(leftX, 4, tex))
      .addConnection({Dir::N, Dir::E, Dir::W}, byCoord(leftX, 6, tex))
      .addConnection({Dir::N, Dir::S, Dir::W}, byCoord(leftX + 1, 5, tex))
      .addConnection({Dir::N, Dir::E, Dir::S}, byCoord(leftX - 1, 5, tex))
      .addConnection({Dir::N, Dir::E}, byCoord(leftX - 1, 6, tex))
      .addConnection({Dir::E, Dir::S}, byCoord(leftX - 1, 4, tex))
      .addConnection({Dir::S, Dir::W}, byCoord(leftX + 1, 4, tex))
      .addConnection({Dir::N, Dir::W}, byCoord(leftX + 1, 6, tex))
      .addConnection({Dir::S}, byCoord(leftX, 7, tex))
      .addConnection({Dir::N}, byCoord(leftX, 8, tex))
      .addConnection({Dir::W}, byCoord(leftX + 1, 7, tex))
      .addConnection({Dir::E}, byCoord(leftX + 1, 8, tex))
      .addConnection({Dir::N, Dir::S}, byCoord(leftX + 1, 12, tex))
      .addConnection({Dir::E, Dir::W}, byCoord(leftX + 1, 11, tex));
  }

  Tile getExtraBorderTile(int set) {
    int tex = 5;
    return sprite(1, set, tex)
      .addExtraBorder({Dir::W, Dir::N}, byCoord(10, set, tex))
      .addExtraBorder({Dir::E, Dir::N}, byCoord(11, set, tex))
      .addExtraBorder({Dir::E, Dir::S}, byCoord(12, set, tex))
      .addExtraBorder({Dir::W, Dir::S}, byCoord(13, set, tex))
      .addExtraBorder({Dir::W, Dir::N, Dir::E}, byCoord(14, set, tex))
      .addExtraBorder({Dir::S, Dir::N, Dir::E}, byCoord(15, set, tex))
      .addExtraBorder({Dir::S, Dir::W, Dir::E}, byCoord(16, set, tex))
      .addExtraBorder({Dir::S, Dir::W, Dir::N}, byCoord(17, set, tex))
      .addExtraBorder({Dir::S, Dir::W, Dir::N, Dir::E}, byCoord(18, set, tex))
      .addExtraBorder({Dir::N}, byCoord(19, set, tex))
      .addExtraBorder({Dir::E}, byCoord(20, set, tex))
      .addExtraBorder({Dir::S}, byCoord(21, set, tex))
      .addExtraBorder({Dir::W}, byCoord(22, set, tex));
  }

  void genTiles() {
    addTile(ViewId::UNKNOWN_MONSTER, symbol("?", ColorId::LIGHT_GREEN));
    addTile(ViewId::SHEEP, symbol("s", ColorId::WHITE));
    addTile(ViewId::VODNIK, symbol("f", ColorId::GREEN));
    addTile(ViewId::ABYSS, symbol("~", ColorId::DARK_GRAY));
    addTile(ViewId::TRAP, symbol("➹", ColorId::YELLOW, true));
    addTile(ViewId::BARS, symbol("⧻", ColorId::LIGHT_BLUE));
    addTile(ViewId::DESTROYED_FURNITURE, symbol("*", ColorId::BROWN));
    addTile(ViewId::BURNT_FURNITURE, symbol("*", ColorId::DARK_GRAY));
    addTile(ViewId::DESTROY_BUTTON, sprite("remove"));
    addTile(ViewId::EMPTY, empty());
    addTile(ViewId::BORDER_GUARD, empty());
    addTile(ViewId::VAMPIRE, sprite("vampire"));
    addTile(ViewId::FALLEN_TREE, sprite("treecut").setNoShadow());
    addTile(ViewId::DECID_TREE, sprite("tree2").setNoShadow());
    addTile(ViewId::CANIF_TREE, sprite("tree1").setNoShadow());
    addTile(ViewId::TREE_TRUNK, sprite("treecut").setNoShadow());
    addTile(ViewId::BURNT_TREE, sprite("treeburnt").setNoShadow());
    addTile(ViewId::PLAYER, sprite(1, 0));
    addTile(ViewId::KEEPER, sprite("keeper"));
    addTile(ViewId::UNDEAD_KEEPER, sprite(9, 16));
    addTile(ViewId::ELF, sprite("elf male"));
    addTile(ViewId::ELF_ARCHER, sprite("elf archer"));
    addTile(ViewId::ELF_CHILD, sprite("elf kid"));
    addTile(ViewId::ELF_LORD, sprite("elf lord"));
    addTile(ViewId::LIZARDMAN, sprite(8, 8));
    addTile(ViewId::LIZARDLORD, sprite(11, 8));
    addTile(ViewId::IMP, sprite("imp"));
    addTile(ViewId::PRISONER, sprite("prisoner"));
    addTile(ViewId::OGRE, sprite("troll"));
    addTile(ViewId::CHICKEN, sprite("chicken"));
    addTile(ViewId::GNOME, sprite(15, 8));
    addTile(ViewId::DWARF, sprite(2, 6));
    addTile(ViewId::DWARF_BARON, sprite(3, 6));
    addTile(ViewId::SHOPKEEPER, sprite(4, 2));
    addTile(ViewId::BRIDGE, sprite("bridge").addOption(Dir::S, byName("bridge2")));
    addTile(ViewId::ROAD, getRoadTile(7));
    addTile(ViewId::FLOOR, sprite(3, 14, 1));
    addTile(ViewId::KEEPER_FLOOR, sprite(4, 18, 1));
    addTile(ViewId::SAND, getExtraBorderTile(14)
        .addExtraBorderId(ViewId::WATER));
    addTile(ViewId::MUD, getExtraBorderTile(10)
        .addExtraBorderId(ViewId::WATER)
        .addExtraBorderId(ViewId::HILL)
        .addExtraBorderId(ViewId::SAND));
    addTile(ViewId::GRASS, getExtraBorderTile(18)
        .addExtraBorderId(ViewId::SAND)
        .addExtraBorderId(ViewId::HILL)
        .addExtraBorderId(ViewId::MUD)
        .addExtraBorderId(ViewId::WATER));
    addTile(ViewId::CROPS, sprite("wheatfield1"));
    addTile(ViewId::CROPS2, sprite("wheatfield2"));
    addTile(ViewId::MOUNTAIN2, getMountainTile(9, 18));
    addTile(ViewId::WALL, getWallTile(2));
    addTile(ViewId::MOUNTAIN, sprite(17, 2, 2).setNoShadow());
    addTile(ViewId::GOLD_ORE, getMountainTile(8, 18));
    addTile(ViewId::IRON_ORE, getMountainTile(7, 18));
    addTile(ViewId::STONE, getMountainTile(6, 18));
    addTile(ViewId::SNOW, sprite(16, 2, 2).setNoShadow());
    addTile(ViewId::HILL, getExtraBorderTile(8)
        .addExtraBorderId(ViewId::SAND)
        .addExtraBorderId(ViewId::WATER));
    addTile(ViewId::WOOD_WALL, getWallTile(4));
    addTile(ViewId::BLACK_WALL, getWallTile(2));
    addTile(ViewId::YELLOW_WALL, getWallTile(8));
    addTile(ViewId::LOW_ROCK_WALL, getWallTile(21));
    addTile(ViewId::HELL_WALL, getWallTile(22));
    addTile(ViewId::CASTLE_WALL, getWallTile(5));
    addTile(ViewId::MUD_WALL, getWallTile(13));
    addTile(ViewId::SECRETPASS, sprite(0, 15, 1));
    addTile(ViewId::DUNGEON_ENTRANCE, sprite(15, 2, 2).setNoShadow());
    addTile(ViewId::DUNGEON_ENTRANCE_MUD, sprite(19, 2, 2).setNoShadow());
    addTile(ViewId::DOWN_STAIRCASE, sprite(8, 0, 1).setNoShadow());
    addTile(ViewId::UP_STAIRCASE, sprite(7, 0, 1).setNoShadow());
    addTile(ViewId::DOWN_STAIRCASE_CELLAR, sprite(8, 21, 1).setNoShadow());
    addTile(ViewId::UP_STAIRCASE_CELLAR, sprite(7, 21, 1).setNoShadow());
    addTile(ViewId::DOWN_STAIRCASE_HELL, sprite(8, 1, 1).setNoShadow());
    addTile(ViewId::UP_STAIRCASE_HELL, sprite(7, 22, 1).setNoShadow());
    addTile(ViewId::DOWN_STAIRCASE_PYR, sprite(8, 8, 1).setNoShadow());
    addTile(ViewId::UP_STAIRCASE_PYR, sprite(7, 8, 1).setNoShadow());
    addTile(ViewId::WELL, sprite(5, 8, 2).setNoShadow());
    addTile(ViewId::STATUE1, sprite(6, 5, 2).setNoShadow());
    addTile(ViewId::STATUE2, sprite(7, 5, 2).setNoShadow());
    addTile(ViewId::GREAT_ORC, sprite(6, 14));
    addTile(ViewId::ORC, sprite("orc"));
    addTile(ViewId::ORC_SHAMAN, sprite("shaman"));
    addTile(ViewId::HARPY, sprite("harpysmall"));
    addTile(ViewId::DOPPLEGANGER, sprite("dopple"));
    addTile(ViewId::SUCCUBUS, sprite("succubussmall"));
    addTile(ViewId::BANDIT, sprite(0, 2));
    addTile(ViewId::GHOST, sprite("ghost4"));
    addTile(ViewId::SPIRIT, sprite(17, 14));
    addTile(ViewId::DEVIL, sprite(17, 18));
    addTile(ViewId::DARK_KNIGHT, sprite(12, 14));
    addTile(ViewId::GREEN_DRAGON, sprite("greendragon"));
    addTile(ViewId::RED_DRAGON, sprite("reddragon"));
    addTile(ViewId::CYCLOPS, sprite(10, 14));
    addTile(ViewId::WITCH, sprite(15, 16));
    addTile(ViewId::KNIGHT, sprite(0, 0));
    addTile(ViewId::WARRIOR, sprite(6, 0));
    addTile(ViewId::SHAMAN, sprite(5, 0));
    addTile(ViewId::CASTLE_GUARD, sprite(15, 2));
    addTile(ViewId::AVATAR, sprite(9, 0));
    addTile(ViewId::ARCHER, sprite(2, 0));
    addTile(ViewId::PESEANT, sprite("peasant"));
    addTile(ViewId::PESEANT_WOMAN, sprite("peasantgirl"));
    addTile(ViewId::CHILD, sprite("peasantkid"));
    addTile(ViewId::CLAY_GOLEM, sprite(12, 11));
    addTile(ViewId::STONE_GOLEM, sprite(10, 10));
    addTile(ViewId::IRON_GOLEM, sprite(12, 10));
    addTile(ViewId::LAVA_GOLEM, sprite(13, 10));
    addTile(ViewId::AIR_ELEMENTAL, sprite("airelemental"));
    addTile(ViewId::FIRE_ELEMENTAL, sprite("fireelemental"));
    addTile(ViewId::WATER_ELEMENTAL, sprite("waterelemental"));
    addTile(ViewId::EARTH_ELEMENTAL, sprite("earthelemental"));
    addTile(ViewId::ENT, sprite("ent"));
    addTile(ViewId::ANGEL, sprite("angel"));
    addTile(ViewId::ZOMBIE, sprite(0, 16));
    addTile(ViewId::SKELETON, sprite("skeleton"));
    addTile(ViewId::VAMPIRE_LORD, sprite("vampirelord"));
    addTile(ViewId::MUMMY, sprite(7, 16));
    addTile(ViewId::MUMMY_LORD, sprite(8, 16));
    addTile(ViewId::JACKAL, sprite(12, 12));
    addTile(ViewId::DEER, sprite("deer"));
    addTile(ViewId::HORSE, sprite("horse"));
    addTile(ViewId::COW, sprite("cow"));
    addTile(ViewId::PIG, sprite("pig"));
    addTile(ViewId::GOAT, sprite("goat"));
    addTile(ViewId::BOAR, sprite("boar"));
    addTile(ViewId::FOX, sprite("fox"));
    addTile(ViewId::WOLF, sprite("wolf"));
    addTile(ViewId::WEREWOLF, sprite("werewolf2"));
    addTile(ViewId::DOG, sprite("dog"));
    addTile(ViewId::KRAKEN_HEAD, sprite("krakenhead"));
    addTile(ViewId::KRAKEN_LAND, sprite("krakenland1"));
    addTile(ViewId::KRAKEN_WATER, sprite("krakenwater2"));
    addTile(ViewId::DEATH, sprite(9, 16));
    addTile(ViewId::NIGHTMARE, sprite(9, 16));
    addTile(ViewId::FIRE_SPHERE, sprite(16, 20));
    addTile(ViewId::BEAR, sprite(8, 18));
    addTile(ViewId::BAT, sprite(2, 12));
    addTile(ViewId::GOBLIN, sprite("goblin"));
    addTile(ViewId::LEPRECHAUN, sprite("leprechaun"));
    addTile(ViewId::RAT, sprite("rat"));
    addTile(ViewId::SPIDER, sprite(6, 12));
    addTile(ViewId::FLY, sprite(10, 12));
    addTile(ViewId::SCORPION, sprite(11, 18));
    addTile(ViewId::SNAKE, sprite(9, 12));
    addTile(ViewId::VULTURE, sprite("vulture"));
    addTile(ViewId::RAVEN, sprite(17, 12));
    addTile(ViewId::BODY_PART, sprite("corpse4"));
    addTile(ViewId::BONE, sprite(3, 0, 2));
    addTile(ViewId::BUSH, sprite(17, 0, 2).setNoShadow());
    addTile(ViewId::WATER, getWaterTile(5));
    addTile(ViewId::MAGMA, getWaterTile(11));
    addTile(ViewId::DOOR, sprite("door").setNoShadow());
    addTile(ViewId::BARRICADE, sprite("barricade").setNoShadow());
    addTile(ViewId::DIG_ICON, sprite(8, 10, 2));
    addTile(ViewId::SWORD, sprite(12, 9, 3));
    addTile(ViewId::SPEAR, sprite(5, 8, 3));
    addTile(ViewId::SPECIAL_SWORD, sprite(13, 9, 3));
    addTile(ViewId::ELVEN_SWORD, sprite(14, 9, 3));
    addTile(ViewId::KNIFE, sprite(20, 9, 3));
    addTile(ViewId::WAR_HAMMER, sprite(10, 7, 3));
    addTile(ViewId::SPECIAL_WAR_HAMMER, sprite(11, 7, 3));
    addTile(ViewId::BATTLE_AXE, sprite(13, 7, 3));
    addTile(ViewId::SPECIAL_BATTLE_AXE, sprite(21, 7, 3));
    addTile(ViewId::BOW, sprite(14, 8, 3));
    addTile(ViewId::ARROW, sprite(5, 8, 3));
    addTile(ViewId::CLUB, sprite("club"));
    addTile(ViewId::HEAVY_CLUB, sprite("heavy club"));
    addTile(ViewId::SCROLL, sprite(3, 6, 3));
    addTile(ViewId::STEEL_AMULET, sprite(1, 1, 3));
    addTile(ViewId::COPPER_AMULET, sprite(2, 1, 3));
    addTile(ViewId::CRYSTAL_AMULET, sprite(4, 1, 3));
    addTile(ViewId::WOODEN_AMULET, sprite(0, 1, 3));
    addTile(ViewId::AMBER_AMULET, sprite(3, 1, 3));
    addTile(ViewId::FIRE_RESIST_RING, sprite(11, 3, 3));
    addTile(ViewId::POISON_RESIST_RING, sprite(16, 3, 3));
    addTile(ViewId::BOOK, sprite(0, 3, 3));
    addTile(ViewId::FIRST_AID, sprite("medkit2"));
    addTile(ViewId::TRAP_ITEM, sprite("trapbox"));
    addTile(ViewId::EFFERVESCENT_POTION, sprite(6, 0, 3));
    addTile(ViewId::MURKY_POTION, sprite(10, 0, 3));
    addTile(ViewId::SWIRLY_POTION, sprite(9, 0, 3));
    addTile(ViewId::VIOLET_POTION, sprite(7, 0, 3));
    addTile(ViewId::PUCE_POTION, sprite(8, 0, 3));
    addTile(ViewId::SMOKY_POTION, sprite(11, 0, 3));
    addTile(ViewId::FIZZY_POTION, sprite(9, 0, 3));
    addTile(ViewId::MILKY_POTION, sprite(11, 0, 3));
    addTile(ViewId::MUSHROOM, sprite(5, 4, 3));
    addTile(ViewId::FOUNTAIN, sprite(0, 7, 2).setNoShadow());
    addTile(ViewId::GOLD, sprite(8, 3, 3).setNoShadow());
    addTile(ViewId::TREASURE_CHEST, sprite("treasurydeco").setNoShadow().addBackground(byName("treasury")));
    addTile(ViewId::CHEST, sprite(3, 3, 2).setNoShadow());
    addTile(ViewId::OPENED_CHEST, sprite(6, 3, 2).setNoShadow());
    addTile(ViewId::COFFIN, sprite(7, 3, 2).setNoShadow());
    addTile(ViewId::OPENED_COFFIN, sprite(8, 3, 2).setNoShadow());
    addTile(ViewId::BOULDER, sprite("boulder"));
    addTile(ViewId::PORTAL, sprite("surprise"));
    addTile(ViewId::GAS_TRAP, sprite(2, 6, 3));
    addTile(ViewId::ALARM_TRAP, sprite(16, 5, 3));
    addTile(ViewId::WEB_TRAP, sprite(4, 1, 2));
    addTile(ViewId::SURPRISE_TRAP, sprite("surprisetrap"));
    addTile(ViewId::TERROR_TRAP, sprite(1, 6, 3));
    addTile(ViewId::ROCK, sprite("stonepile"));
    addTile(ViewId::IRON_ROCK, sprite("ironpile2"));
    addTile(ViewId::WOOD_PLANK, sprite("wood2"));
    addTile(ViewId::STOCKPILE1, sprite("storage1").setFloorBorders());
    addTile(ViewId::STOCKPILE2, sprite("storage2").setFloorBorders());
    addTile(ViewId::STOCKPILE3, sprite("storage3").setFloorBorders());
    addTile(ViewId::PRISON, sprite(6, 2, 1));
    addTile(ViewId::BED, sprite("sleepdeco").setNoShadow());
    addTile(ViewId::DORM, sprite("sleep").setFloorBorders());
    addTile(ViewId::TORCH, sprite(12, 0, 2).setNoShadow().setTranslucent(0.35));
    addTile(ViewId::DUNGEON_HEART, sprite(6, 10, 2));
    addTile(ViewId::ALTAR, sprite(2, 7, 2).setNoShadow());
    addTile(ViewId::CREATURE_ALTAR, sprite(3, 7, 2).setNoShadow());
    addTile(ViewId::TORTURE_TABLE, empty().addConnection(SetOfDir::fullSet(), byName("torturedeco"))
        .addBackground(byName("torture"))
        .setFloorBorders());
    addTile(ViewId::IMPALED_HEAD, sprite("impaledhead").setNoShadow());
    addTile(ViewId::TRAINING_ROOM, empty().addConnection(SetOfDir::fullSet(), byName("traindeco"))
        .addBackground(byName("train")).setFloorBorders());
    addTile(ViewId::RITUAL_ROOM, empty().addConnection(SetOfDir::fullSet(), byName("ritualroomdeco"))
        .addBackground(byName("ritualroom"))
        .setFloorBorders());
    addTile(ViewId::LIBRARY, empty().addConnection(SetOfDir::fullSet(), byName("libdeco"))
        .addBackground(byName("lib")).setFloorBorders());
    addTile(ViewId::LABORATORY, empty().addConnection(SetOfDir::fullSet(), byName("labdeco"))
        .addBackground(byName("lab")).setFloorBorders());
    addTile(ViewId::CAULDRON, sprite("labdeco").setNoShadow());
    addTile(ViewId::BEAST_LAIR, sprite("lair").setFloorBorders());
    addTile(ViewId::BEAST_CAGE, sprite("lairdeco").setNoShadow()
        .addBackground(byName("lair")));
    addTile(ViewId::FORGE, empty().addConnection(SetOfDir::fullSet(), byName("forgedeco"))
        .addBackground(byName("forge"))
        .setFloorBorders());
    addTile(ViewId::WORKSHOP, empty().addConnection(SetOfDir::fullSet(), byName("workshopdeco"))
        .addBackground(byName("workshop"))
        .setFloorBorders());
    addTile(ViewId::JEWELER, empty().addConnection(SetOfDir::fullSet(), byName("jewelerdeco"))
        .addBackground(byName("jeweler"))
        .setFloorBorders());
    addTile(ViewId::CEMETERY, sprite("graveyard").setFloorBorders());
    addTile(ViewId::GRAVE, sprite("gravedeco").setNoShadow().addBackground(byName("graveyard")));
    addTile(ViewId::ROBE, sprite(7, 11, 3));
    addTile(ViewId::LEATHER_GLOVES, sprite(15, 11, 3));
    addTile(ViewId::DEXTERITY_GLOVES, sprite(19, 11, 3));
    addTile(ViewId::STRENGTH_GLOVES, sprite(20, 11, 3));
    addTile(ViewId::LEATHER_ARMOR, sprite(0, 12, 3));
    addTile(ViewId::LEATHER_HELM, sprite(10, 12, 3));
    addTile(ViewId::TELEPATHY_HELM, sprite(17, 12, 3));
    addTile(ViewId::CHAIN_ARMOR, sprite(1, 12, 3));
    addTile(ViewId::IRON_HELM, sprite(14, 12, 3));
    addTile(ViewId::LEATHER_BOOTS, sprite(0, 13, 3));
    addTile(ViewId::IRON_BOOTS, sprite(6, 13, 3));
    addTile(ViewId::SPEED_BOOTS, sprite(3, 13, 3));
    addTile(ViewId::LEVITATION_BOOTS, sprite(2, 13, 3));
    addTile(ViewId::GUARD_POST, sprite("guardroom"));
    addTile(ViewId::MANA, sprite(5, 10, 2));
    addTile(ViewId::DANGER, sprite(12, 9, 2));
    addTile(ViewId::FETCH_ICON, sprite(15, 11, 3));
    addTile(ViewId::EYEBALL, sprite("eyeball2").setNoShadow());
    addTile(ViewId::FOG_OF_WAR, getWaterTile(14));
    addTile(ViewId::FOG_OF_WAR_CORNER, sprite(14, 5, 4)
        .addConnection({Dir::NE}, byCoord(14, 11, 4))
        .addConnection({Dir::NW}, byCoord(13, 11, 4))
        .addConnection({Dir::SE}, byCoord(14, 12, 4))
        .addConnection({Dir::SW}, byCoord(13, 12, 4)));
    addTile(ViewId::SPECIAL_BEAST, sprite(7, 10));
    addTile(ViewId::SPECIAL_HUMANOID, sprite(2, 10));
    addTile(ViewId::TEAM_BUTTON, sprite("team_button"));
    addTile(ViewId::TEAM_BUTTON_HIGHLIGHT, sprite("team_button_highlight"));
  }

  Tile symbol(const string& s, ColorId id, bool symbol = false) {
    return Tile::fromString(s, id, symbol);
  }

  Tile symbol(sf::Uint32 i, ColorId id, bool symbol = false) {
    return Tile::unicode(i, id, symbol);
  }

  void genSymbols() {
    addSymbol(ViewId::EMPTY, symbol(" ", ColorId::BLACK));
    addSymbol(ViewId::PLAYER, symbol("@", ColorId::WHITE));
    addSymbol(ViewId::KEEPER, symbol("@", ColorId::PURPLE));
    addSymbol(ViewId::UNDEAD_KEEPER, symbol("@", ColorId::DARK_GRAY));
    addSymbol(ViewId::UNKNOWN_MONSTER, symbol("?", ColorId::LIGHT_GREEN));
    addSymbol(ViewId::ELF, symbol("@", ColorId::LIGHT_GREEN));
    addSymbol(ViewId::ELF_ARCHER, symbol("@", ColorId::GREEN));
    addSymbol(ViewId::ELF_CHILD, symbol("@", ColorId::LIGHT_GREEN));
    addSymbol(ViewId::ELF_LORD, symbol("@", ColorId::DARK_GREEN));
    addSymbol(ViewId::SHOPKEEPER, symbol("@", ColorId::LIGHT_BLUE));
    addSymbol(ViewId::LIZARDMAN, symbol("@", ColorId::LIGHT_BROWN));
    addSymbol(ViewId::LIZARDLORD, symbol("@", ColorId::BROWN));
    addSymbol(ViewId::IMP, symbol("i", ColorId::LIGHT_BROWN));
    addSymbol(ViewId::PRISONER, symbol("@", ColorId::LIGHT_BROWN));
    addSymbol(ViewId::OGRE, symbol("O", ColorId::GREEN));
    addSymbol(ViewId::CHICKEN, symbol("c", ColorId::YELLOW));
    addSymbol(ViewId::GNOME, symbol("g", ColorId::GREEN));
    addSymbol(ViewId::DWARF, symbol("h", ColorId::BLUE));
    addSymbol(ViewId::DWARF_BARON, symbol("h", ColorId::DARK_BLUE));
    addSymbol(ViewId::FLOOR, symbol(".", ColorId::LIGHT_GRAY));
    addSymbol(ViewId::KEEPER_FLOOR, symbol(".", ColorId::WHITE));
    addSymbol(ViewId::BRIDGE, symbol("_", ColorId::BROWN));
    addSymbol(ViewId::ROAD, symbol(".", ColorId::LIGHT_GRAY));
    addSymbol(ViewId::SAND, symbol(".", ColorId::YELLOW));
    addSymbol(ViewId::MUD, symbol(0x1d0f0, ColorId::BROWN, true));
    addSymbol(ViewId::GRASS, symbol(0x1d0f0, ColorId::GREEN, true));
    addSymbol(ViewId::CROPS, symbol(0x1d0f0, ColorId::YELLOW, true));
    addSymbol(ViewId::CROPS2, symbol(0x1d0f0, ColorId::YELLOW, true));
    addSymbol(ViewId::CASTLE_WALL, symbol("#", ColorId::LIGHT_GRAY));
    addSymbol(ViewId::MUD_WALL, symbol("#", ColorId::LIGHT_BROWN));
    addSymbol(ViewId::WALL, symbol("#", ColorId::LIGHT_GRAY));
    addSymbol(ViewId::MOUNTAIN, symbol(0x25ee, ColorId::DARK_GRAY, true));
    addSymbol(ViewId::MOUNTAIN2, symbol("#", ColorId::DARK_GRAY));
    addSymbol(ViewId::GOLD_ORE, symbol("⁂", ColorId::YELLOW, true));
    addSymbol(ViewId::IRON_ORE, symbol("⁂", ColorId::DARK_BROWN, true));
    addSymbol(ViewId::STONE, symbol("⁂", ColorId::LIGHT_GRAY, true));
    addSymbol(ViewId::SNOW, symbol(0x25ee, ColorId::WHITE, true));
    addSymbol(ViewId::HILL, symbol(0x1d022, ColorId::DARK_GREEN, true));
    addSymbol(ViewId::WOOD_WALL, symbol("#", ColorId::DARK_BROWN));
    addSymbol(ViewId::BLACK_WALL, symbol("#", ColorId::LIGHT_GRAY));
    addSymbol(ViewId::YELLOW_WALL, symbol("#", ColorId::YELLOW));
    addSymbol(ViewId::LOW_ROCK_WALL, symbol("#", ColorId::DARK_GRAY));
    addSymbol(ViewId::HELL_WALL, symbol("#", ColorId::RED));
    addSymbol(ViewId::SECRETPASS, symbol("#", ColorId::LIGHT_GRAY));
    addSymbol(ViewId::DUNGEON_ENTRANCE, symbol(0x2798, ColorId::BROWN, true));
    addSymbol(ViewId::DUNGEON_ENTRANCE_MUD, symbol(0x2798, ColorId::BROWN, true));
    addSymbol(ViewId::DOWN_STAIRCASE_CELLAR, symbol(0x2798, ColorId::ALMOST_WHITE, true));
    addSymbol(ViewId::DOWN_STAIRCASE, symbol(0x2798, ColorId::ALMOST_WHITE, true));
    addSymbol(ViewId::UP_STAIRCASE_CELLAR, symbol(0x279a, ColorId::ALMOST_WHITE, true));
    addSymbol(ViewId::UP_STAIRCASE, symbol(0x279a, ColorId::ALMOST_WHITE, true));
    addSymbol(ViewId::DOWN_STAIRCASE_HELL, symbol(0x2798, ColorId::RED, true));
    addSymbol(ViewId::UP_STAIRCASE_HELL, symbol(0x279a, ColorId::RED, true));
    addSymbol(ViewId::DOWN_STAIRCASE_PYR, symbol(0x2798, ColorId::YELLOW, true));
    addSymbol(ViewId::UP_STAIRCASE_PYR, symbol(0x279a, ColorId::YELLOW, true));
    addSymbol(ViewId::WELL, symbol("0", ColorId::BLUE));
    addSymbol(ViewId::STATUE1, symbol("&", ColorId::LIGHT_GRAY));
    addSymbol(ViewId::STATUE2, symbol("&", ColorId::LIGHT_GRAY));
    addSymbol(ViewId::GREAT_ORC, symbol("o", ColorId::PURPLE));
    addSymbol(ViewId::ORC, symbol("o", ColorId::DARK_BLUE));
    addSymbol(ViewId::ORC_SHAMAN, symbol("o", ColorId::YELLOW));
    addSymbol(ViewId::HARPY, symbol("R", ColorId::YELLOW));
    addSymbol(ViewId::DOPPLEGANGER, symbol("&", ColorId::YELLOW));
    addSymbol(ViewId::SUCCUBUS, symbol("&", ColorId::RED));
    addSymbol(ViewId::BANDIT, symbol("@", ColorId::DARK_BLUE));
    addSymbol(ViewId::DARK_KNIGHT, symbol("@", ColorId::PURPLE));
    addSymbol(ViewId::GREEN_DRAGON, symbol("D", ColorId::GREEN));
    addSymbol(ViewId::RED_DRAGON, symbol("D", ColorId::RED));
    addSymbol(ViewId::CYCLOPS, symbol("C", ColorId::GREEN));
    addSymbol(ViewId::WITCH, symbol("@", ColorId::BROWN));
    addSymbol(ViewId::GHOST, symbol("&", ColorId::WHITE));
    addSymbol(ViewId::SPIRIT, symbol("&", ColorId::LIGHT_BLUE));
    addSymbol(ViewId::DEVIL, symbol("&", ColorId::PURPLE));
    addSymbol(ViewId::CASTLE_GUARD, symbol("@", ColorId::LIGHT_GRAY));
    addSymbol(ViewId::KNIGHT, symbol("@", ColorId::LIGHT_GRAY));
    addSymbol(ViewId::WARRIOR, symbol("@", ColorId::DARK_GRAY));
    addSymbol(ViewId::SHAMAN, symbol("@", ColorId::YELLOW));
    addSymbol(ViewId::AVATAR, symbol("@", ColorId::BLUE));
    addSymbol(ViewId::ARCHER, symbol("@", ColorId::BROWN));
    addSymbol(ViewId::PESEANT, symbol("@", ColorId::GREEN));
    addSymbol(ViewId::PESEANT_WOMAN, symbol("@", ColorId::GREEN));
    addSymbol(ViewId::CHILD, symbol("@", ColorId::LIGHT_GREEN));
    addSymbol(ViewId::CLAY_GOLEM, symbol("Y", ColorId::YELLOW));
    addSymbol(ViewId::STONE_GOLEM, symbol("Y", ColorId::LIGHT_GRAY));
    addSymbol(ViewId::IRON_GOLEM, symbol("Y", ColorId::ORANGE));
    addSymbol(ViewId::LAVA_GOLEM, symbol("Y", ColorId::PURPLE));
    addSymbol(ViewId::AIR_ELEMENTAL, symbol("E", ColorId::LIGHT_BLUE));
    addSymbol(ViewId::FIRE_ELEMENTAL, symbol("E", ColorId::RED));
    addSymbol(ViewId::WATER_ELEMENTAL, symbol("E", ColorId::BLUE));
    addSymbol(ViewId::EARTH_ELEMENTAL, symbol("E", ColorId::GRAY));
    addSymbol(ViewId::ENT, symbol("E", ColorId::LIGHT_BROWN));
    addSymbol(ViewId::ANGEL, symbol("A", ColorId::LIGHT_BLUE));
    addSymbol(ViewId::ZOMBIE, symbol("Z", ColorId::GREEN));
    addSymbol(ViewId::SKELETON, symbol("Z", ColorId::WHITE));
    addSymbol(ViewId::VAMPIRE, symbol("V", ColorId::DARK_GRAY));
    addSymbol(ViewId::VAMPIRE_LORD, symbol("V", ColorId::PURPLE));
    addSymbol(ViewId::MUMMY, symbol("Z", ColorId::YELLOW));
    addSymbol(ViewId::MUMMY_LORD, symbol("Z", ColorId::ORANGE));
    addSymbol(ViewId::JACKAL, symbol("d", ColorId::LIGHT_BROWN));
    addSymbol(ViewId::DEER, symbol("R", ColorId::DARK_BROWN));
    addSymbol(ViewId::HORSE, symbol("H", ColorId::LIGHT_BROWN));
    addSymbol(ViewId::COW, symbol("C", ColorId::WHITE));
    addSymbol(ViewId::SHEEP, symbol("s", ColorId::WHITE));
    addSymbol(ViewId::PIG, symbol("p", ColorId::YELLOW));
    addSymbol(ViewId::GOAT, symbol("g", ColorId::GRAY));
    addSymbol(ViewId::BOAR, symbol("b", ColorId::LIGHT_BROWN));
    addSymbol(ViewId::FOX, symbol("d", ColorId::ORANGE_BROWN));
    addSymbol(ViewId::WOLF, symbol("d", ColorId::DARK_BLUE));
    addSymbol(ViewId::WEREWOLF, symbol("d", ColorId::WHITE));
    addSymbol(ViewId::DOG, symbol("d", ColorId::BROWN));
    addSymbol(ViewId::VODNIK, symbol("f", ColorId::GREEN));
    addSymbol(ViewId::KRAKEN_HEAD, symbol("S", ColorId::GREEN));
    addSymbol(ViewId::KRAKEN_WATER, symbol("S", ColorId::DARK_GREEN));
    addSymbol(ViewId::KRAKEN_LAND, symbol("S", ColorId::DARK_GREEN));
    addSymbol(ViewId::DEATH, symbol("D", ColorId::DARK_GRAY));
    addSymbol(ViewId::NIGHTMARE, symbol("n", ColorId::PURPLE));
    addSymbol(ViewId::FIRE_SPHERE, symbol("e", ColorId::RED));
    addSymbol(ViewId::BEAR, symbol("N", ColorId::BROWN));
    addSymbol(ViewId::BAT, symbol("b", ColorId::DARK_GRAY));
    addSymbol(ViewId::GOBLIN, symbol("o", ColorId::GREEN));
    addSymbol(ViewId::LEPRECHAUN, symbol("l", ColorId::GREEN));
    addSymbol(ViewId::RAT, symbol("r", ColorId::BROWN));
    addSymbol(ViewId::SPIDER, symbol("s", ColorId::BROWN));
    addSymbol(ViewId::FLY, symbol("b", ColorId::GRAY));
    addSymbol(ViewId::SCORPION, symbol("s", ColorId::LIGHT_GRAY));
    addSymbol(ViewId::SNAKE, symbol("S", ColorId::YELLOW));
    addSymbol(ViewId::VULTURE, symbol("v", ColorId::DARK_GRAY));
    addSymbol(ViewId::RAVEN, symbol("v", ColorId::DARK_GRAY));
    addSymbol(ViewId::BODY_PART, symbol("%", ColorId::RED));
    addSymbol(ViewId::BONE, symbol("%", ColorId::WHITE));
    addSymbol(ViewId::BUSH, symbol("&", ColorId::DARK_GREEN));
    addSymbol(ViewId::DECID_TREE, symbol(0x1f70d, ColorId::DARK_GREEN, true));
    addSymbol(ViewId::CANIF_TREE, symbol(0x2663, ColorId::DARK_GREEN, true));
    addSymbol(ViewId::TREE_TRUNK, symbol(".", ColorId::BROWN));
    addSymbol(ViewId::BURNT_TREE, symbol(".", ColorId::DARK_GRAY));
    addSymbol(ViewId::WATER, symbol("~", ColorId::LIGHT_BLUE));
    addSymbol(ViewId::MAGMA, symbol("~", ColorId::RED));
    addSymbol(ViewId::ABYSS, symbol("~", ColorId::DARK_GRAY));
    addSymbol(ViewId::DOOR, symbol("|", ColorId::BROWN));
    addSymbol(ViewId::BARRICADE, symbol("X", ColorId::BROWN));
    addSymbol(ViewId::DIG_ICON, symbol(0x26cf, ColorId::LIGHT_GRAY, true));
    addSymbol(ViewId::SWORD, symbol(")", ColorId::LIGHT_GRAY));
    addSymbol(ViewId::SPEAR, symbol("/", ColorId::LIGHT_GRAY));
    addSymbol(ViewId::SPECIAL_SWORD, symbol(")", ColorId::YELLOW));
    addSymbol(ViewId::ELVEN_SWORD, symbol(")", ColorId::GRAY));
    addSymbol(ViewId::KNIFE, symbol(")", ColorId::WHITE));
    addSymbol(ViewId::WAR_HAMMER, symbol(")", ColorId::BLUE));
    addSymbol(ViewId::SPECIAL_WAR_HAMMER, symbol(")", ColorId::LIGHT_BLUE));
    addSymbol(ViewId::BATTLE_AXE, symbol(")", ColorId::GREEN));
    addSymbol(ViewId::SPECIAL_BATTLE_AXE, symbol(")", ColorId::LIGHT_GREEN));
    addSymbol(ViewId::BOW, symbol(")", ColorId::BROWN));
    addSymbol(ViewId::CLUB, symbol(")", ColorId::BROWN));
    addSymbol(ViewId::HEAVY_CLUB, symbol(")", ColorId::BROWN));
    addSymbol(ViewId::ARROW, symbol("\\", ColorId::BROWN));
    addSymbol(ViewId::SCROLL, symbol("?", ColorId::WHITE));
    addSymbol(ViewId::STEEL_AMULET, symbol("\"", ColorId::YELLOW));
    addSymbol(ViewId::COPPER_AMULET, symbol("\"", ColorId::YELLOW));
    addSymbol(ViewId::CRYSTAL_AMULET, symbol("\"", ColorId::YELLOW));
    addSymbol(ViewId::WOODEN_AMULET, symbol("\"", ColorId::YELLOW));
    addSymbol(ViewId::AMBER_AMULET, symbol("\"", ColorId::YELLOW));
    addSymbol(ViewId::FIRE_RESIST_RING, symbol("=", ColorId::RED));
    addSymbol(ViewId::POISON_RESIST_RING, symbol("=", ColorId::GREEN));
    addSymbol(ViewId::BOOK, symbol("+", ColorId::YELLOW));
    addSymbol(ViewId::FIRST_AID, symbol("+", ColorId::RED));
    addSymbol(ViewId::TRAP_ITEM, symbol("+", ColorId::YELLOW));
    addSymbol(ViewId::EFFERVESCENT_POTION, symbol("!", ColorId::LIGHT_RED));
    addSymbol(ViewId::MURKY_POTION, symbol("!", ColorId::BLUE));
    addSymbol(ViewId::SWIRLY_POTION, symbol("!", ColorId::YELLOW));
    addSymbol(ViewId::VIOLET_POTION, symbol("!", ColorId::VIOLET));
    addSymbol(ViewId::PUCE_POTION, symbol("!", ColorId::DARK_BROWN));
    addSymbol(ViewId::SMOKY_POTION, symbol("!", ColorId::LIGHT_GRAY));
    addSymbol(ViewId::FIZZY_POTION, symbol("!", ColorId::LIGHT_BLUE));
    addSymbol(ViewId::MILKY_POTION, symbol("!", ColorId::WHITE));
    addSymbol(ViewId::MUSHROOM, symbol(0x22c6, ColorId::PINK, true));
    addSymbol(ViewId::FOUNTAIN, symbol("0", ColorId::LIGHT_BLUE));
    addSymbol(ViewId::GOLD, symbol("$", ColorId::YELLOW));
    addSymbol(ViewId::TREASURE_CHEST, symbol("=", ColorId::BROWN));
    addSymbol(ViewId::OPENED_CHEST, symbol("=", ColorId::BROWN));
    addSymbol(ViewId::CHEST, symbol("=", ColorId::BROWN));
    addSymbol(ViewId::OPENED_COFFIN, symbol("⚰", ColorId::DARK_GRAY, true));
    addSymbol(ViewId::COFFIN, symbol("⚰", ColorId::DARK_GRAY, true));
    addSymbol(ViewId::BOULDER, symbol("●", ColorId::LIGHT_GRAY, true));
    addSymbol(ViewId::PORTAL, symbol(0x1d6af, ColorId::LIGHT_GREEN, true));
    addSymbol(ViewId::TRAP, symbol("➹", ColorId::YELLOW, true));
    addSymbol(ViewId::GAS_TRAP, symbol("☠", ColorId::GREEN, true));
    addSymbol(ViewId::ALARM_TRAP, symbol("^", ColorId::RED, true));
    addSymbol(ViewId::WEB_TRAP, symbol("#", ColorId::WHITE, true));
    addSymbol(ViewId::SURPRISE_TRAP, symbol("^", ColorId::BLUE, true));
    addSymbol(ViewId::TERROR_TRAP, symbol("^", ColorId::WHITE, true));
    addSymbol(ViewId::ROCK, symbol("✱", ColorId::LIGHT_GRAY, true));
    addSymbol(ViewId::IRON_ROCK, symbol("✱", ColorId::ORANGE, true));
    addSymbol(ViewId::WOOD_PLANK, symbol("\\", ColorId::BROWN));
    addSymbol(ViewId::STOCKPILE1, symbol(".", ColorId::YELLOW));
    addSymbol(ViewId::STOCKPILE2, symbol(".", ColorId::LIGHT_GREEN));
    addSymbol(ViewId::STOCKPILE3, symbol(".", ColorId::LIGHT_BLUE));
    addSymbol(ViewId::PRISON, symbol(".", ColorId::BLUE));
    addSymbol(ViewId::DORM, symbol(".", ColorId::BROWN));
    addSymbol(ViewId::BED, symbol("=", ColorId::WHITE));
    addSymbol(ViewId::DUNGEON_HEART, symbol("♥", ColorId::WHITE, true));
    addSymbol(ViewId::TORCH, symbol("*", ColorId::YELLOW));
    addSymbol(ViewId::ALTAR, symbol("Ω", ColorId::WHITE, true));
    addSymbol(ViewId::CREATURE_ALTAR, symbol("Ω", ColorId::YELLOW, true));
    addSymbol(ViewId::TORTURE_TABLE, symbol("=", ColorId::GRAY));
    addSymbol(ViewId::IMPALED_HEAD, symbol("⚲", ColorId::BROWN, true));
    addSymbol(ViewId::TRAINING_ROOM, symbol("‡", ColorId::BROWN, true));
    addSymbol(ViewId::RITUAL_ROOM, symbol("Ω", ColorId::PURPLE, true));
    addSymbol(ViewId::LIBRARY, symbol("▤", ColorId::BROWN, true));
    addSymbol(ViewId::LABORATORY, symbol("ω", ColorId::PURPLE, true));
    addSymbol(ViewId::CAULDRON, symbol("ω", ColorId::PURPLE, true));
    addSymbol(ViewId::BEAST_LAIR, symbol(".", ColorId::YELLOW));
    addSymbol(ViewId::BEAST_CAGE, symbol("▥", ColorId::LIGHT_GRAY, true));
    addSymbol(ViewId::WORKSHOP, symbol("&", ColorId::LIGHT_BROWN));
    addSymbol(ViewId::FORGE, symbol("&", ColorId::LIGHT_BLUE));
    addSymbol(ViewId::JEWELER, symbol("&", ColorId::YELLOW));
    addSymbol(ViewId::CEMETERY, symbol(".", ColorId::DARK_BLUE));
    addSymbol(ViewId::GRAVE, symbol(0x2617, ColorId::GRAY, true));
    addSymbol(ViewId::BARS, symbol("⧻", ColorId::LIGHT_BLUE));
    addSymbol(ViewId::BORDER_GUARD, symbol(" ", ColorId::WHITE));
    addSymbol(ViewId::ROBE, symbol("[", ColorId::LIGHT_BROWN));
    addSymbol(ViewId::LEATHER_GLOVES, symbol("[", ColorId::BROWN));
    addSymbol(ViewId::STRENGTH_GLOVES, symbol("[", ColorId::RED));
    addSymbol(ViewId::DEXTERITY_GLOVES, symbol("[", ColorId::LIGHT_BLUE));
    addSymbol(ViewId::LEATHER_ARMOR, symbol("[", ColorId::BROWN));
    addSymbol(ViewId::LEATHER_HELM, symbol("[", ColorId::BROWN));
    addSymbol(ViewId::TELEPATHY_HELM, symbol("[", ColorId::LIGHT_GREEN));
    addSymbol(ViewId::CHAIN_ARMOR, symbol("[", ColorId::LIGHT_GRAY));
    addSymbol(ViewId::IRON_HELM, symbol("[", ColorId::LIGHT_GRAY));
    addSymbol(ViewId::LEATHER_BOOTS, symbol("[", ColorId::BROWN));
    addSymbol(ViewId::IRON_BOOTS, symbol("[", ColorId::LIGHT_GRAY));
    addSymbol(ViewId::SPEED_BOOTS, symbol("[", ColorId::LIGHT_BLUE));
    addSymbol(ViewId::LEVITATION_BOOTS, symbol("[", ColorId::LIGHT_GREEN));
    addSymbol(ViewId::DESTROYED_FURNITURE, symbol("*", ColorId::BROWN));
    addSymbol(ViewId::BURNT_FURNITURE, symbol("*", ColorId::DARK_GRAY));
    addSymbol(ViewId::FALLEN_TREE, symbol("*", ColorId::GREEN));
    addSymbol(ViewId::GUARD_POST, symbol("⚐", ColorId::YELLOW, true));
    addSymbol(ViewId::DESTROY_BUTTON, symbol("X", ColorId::RED));
    addSymbol(ViewId::MANA, symbol("✱", ColorId::BLUE, true));
    addSymbol(ViewId::EYEBALL, symbol("e", ColorId::BLUE));
    addSymbol(ViewId::DANGER, symbol("*", ColorId::RED));
    addSymbol(ViewId::FETCH_ICON, symbol(0x1f44b, ColorId::LIGHT_BROWN, true));
    addSymbol(ViewId::FOG_OF_WAR, symbol(' ', ColorId::WHITE));
    addSymbol(ViewId::FOG_OF_WAR_CORNER, symbol(' ', ColorId::WHITE));
    addSymbol(ViewId::SPECIAL_BEAST, symbol('B', ColorId::PURPLE));
    addSymbol(ViewId::SPECIAL_HUMANOID, symbol('H', ColorId::PURPLE));
    addSymbol(ViewId::TEAM_BUTTON, symbol("⚐", ColorId::WHITE, true));
    addSymbol(ViewId::TEAM_BUTTON_HIGHLIGHT, symbol("⚐", ColorId::GREEN, true));
  }
 
  private:
  Renderer& renderer;
};

void Tile::initialize(Renderer& renderer, bool useTiles) {
  TileCoordLookup lookup(renderer);
  lookup.loadUnicode();
  if (useTiles)
    lookup.loadTiles();
}

const Tile& Tile::fromViewId(ViewId id) {
  CHECK(tiles.count(id));
  return tiles.at(id);
}

const Tile& getSpriteTile(ViewId id) {
  if (!tiles.count(id))
    FAIL << "unhandled view id " << EnumInfo<ViewId>::getString(id);
  return tiles.at(id);
}

const Tile& getAsciiTile(ViewId id) {
  if (!symbols.count(id))
    FAIL << "unhandled view id " << EnumInfo<ViewId>::getString(id);
  return symbols.at(id);
}

const Tile& Tile::getTile(ViewId id, bool sprite) {
  if (sprite)
    return getSpriteTile(id);
  else
    return getAsciiTile(id);
}

Color Tile::getColor(const ViewObject& object) {
  if (object.hasModifier(ViewObject::Modifier::INVISIBLE))
    return colors[ColorId::DARK_GRAY];
  if (object.hasModifier(ViewObject::Modifier::HIDDEN))
    return colors[ColorId::LIGHT_GRAY];
  Color color = getAsciiTile(object.id()).color;
  if (object.hasModifier(ViewObject::Modifier::PLANNED)) {
    return Color((color.r) / 2, (color.g) / 2, (color.b) / 2);
  }
  double bleeding = object.getAttribute(ViewObject::Attribute::BLEEDING);
  if (bleeding > 0) {
    bleeding = 0.5 + bleeding / 2;
    bleeding = min(1., bleeding);
    return Color(
        (1 - bleeding) * color.r + bleeding * 255,
        (1 - bleeding) * color.g,
        (1 - bleeding) * color.b);
  } else
    return color;
}

Tile Tile::addCorner(EnumSet<Dir> cornerDef, EnumSet<Dir> borders, TileCoord coord) {
  corners.push_back({cornerDef, borders, coord});
  return *this;
}

bool Tile::hasCorners() const {
  return !corners.empty();
}

vector<Tile::TileCoord> Tile::getCornerCoords(const EnumSet<Dir>& c) const {
  vector<TileCoord> ret;
  for (auto& elem : corners) {
    if (elem.cornerDef.intersection(c) == elem.borders)
      ret.push_back(elem.tileCoord);
  }
  return ret;
}

