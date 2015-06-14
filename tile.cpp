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

Tile::Tile(TileCoord c) : color(255, 255, 255), tileCoord(c) {
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

Tile Tile::addConnection(DirSet dirs, TileCoord coord) {
  anyConnections = true;
  connections[dirs] = coord;
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

Tile Tile::setColor(Color col) {
  color = col;
  return *this;
}

Tile Tile::addExtraBorder(DirSet dir, TileCoord coord) {
  extraBorders[dir] = coord;
  return *this;
}

Tile Tile::addExtraBorderId(ViewId id) {
  extraBorderIds.push_back(id);
  anyExtraBorders = true;
  return *this;
}

Tile Tile::addHighlight(TileCoord coord) {
  highlightCoord = coord;
  return *this;
}

optional<Tile::TileCoord> Tile::getHighlightCoord() const {
  return highlightCoord;
}

const vector<ViewId>& Tile::getExtraBorderIds() const {
  return extraBorderIds;
}

bool Tile::hasExtraBorders() const {
  return anyExtraBorders;
}

optional<Tile::TileCoord> Tile::getExtraBorderCoord(DirSet c) const {
  return extraBorders[c];
}

Tile Tile::setTranslucent(double v) {
  translucent = v;
  return *this;
}

bool Tile::hasSpriteCoord() const {
  return !!tileCoord;
}

Tile::TileCoord Tile::getSpriteCoord() const {
  CHECK(tileCoord);
  return *tileCoord;
}

optional<Tile::TileCoord> Tile::getBackgroundCoord() const {
  return backgroundCoord;
}

Tile::TileCoord Tile::getSpriteCoord(DirSet c) const {
  if (connectionOption) {
    if (c.has(connectionOption->first))
      return connectionOption->second;
    else {
      CHECK(tileCoord);
      return *tileCoord;
    }
  }
  if (connections[c])
    return *connections[c];
  else {
    CHECK(tileCoord);
    return *tileCoord;
  }
}

EnumMap<ViewId, optional<Tile>> Tile::tiles;
EnumMap<ViewId, optional<Tile>> Tile::symbols;

void Tile::addTile(ViewId id, Tile tile) {
  tiles[id] = tile;
}

void Tile::addSymbol(ViewId id, Tile tile) {
  symbols[id] = tile;
}


class TileCoordLookup {
  public:
  TileCoordLookup(Renderer& r) : renderer(r) {}

  void loadTiles() {
    genTiles();
    bool bad = false;
    for (ViewId id : ENUM_ALL(ViewId))
      if (!Tile::tiles[id]) {
        Debug() << "ViewId not found: " << EnumInfo<ViewId>::getString(id);
        bad = true;
      }
    CHECK(!bad);
  }

  void loadUnicode() {
    genSymbols();
    bool bad = false;
    for (ViewId id : ENUM_ALL(ViewId))
      if (!Tile::symbols[id]) {
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
    Tile::addTile(ViewId::UNKNOWN_MONSTER, symbol("?", ColorId::LIGHT_GREEN));
    Tile::addTile(ViewId::DIG_MARK, sprite("dig_mark"));
    Tile::addTile(ViewId::FORBID_ZONE, sprite("dig_mark").setColor(colors[ColorId::RED]));
    Tile::addTile(ViewId::DESTROY_BUTTON, sprite("remove"));
    Tile::addTile(ViewId::EMPTY, empty());
    Tile::addTile(ViewId::BORDER_GUARD, empty());
    Tile::addTile(ViewId::VAMPIRE, sprite("vampire"));
    Tile::addTile(ViewId::FALLEN_TREE, sprite("treecut").setNoShadow());
    Tile::addTile(ViewId::DECID_TREE, sprite("tree2").setNoShadow().addHighlight(byName("tree2_mark")));
    Tile::addTile(ViewId::CANIF_TREE, sprite("tree1").setNoShadow().addHighlight(byName("tree1_mark")));
    Tile::addTile(ViewId::TREE_TRUNK, sprite("treecut").setNoShadow());
    Tile::addTile(ViewId::BURNT_TREE, sprite("treeburnt").setNoShadow());
    Tile::addTile(ViewId::PLAYER, sprite(1, 0));
    Tile::addTile(ViewId::KEEPER, sprite("keeper"));
    Tile::addTile(ViewId::UNDEAD_KEEPER, sprite(9, 16));
    Tile::addTile(ViewId::ELF, sprite("elf male"));
    Tile::addTile(ViewId::ELF_ARCHER, sprite("elf archer"));
    Tile::addTile(ViewId::ELF_CHILD, sprite("elf kid"));
    Tile::addTile(ViewId::ELF_LORD, sprite("elf lord"));
    Tile::addTile(ViewId::LIZARDMAN, sprite(8, 8));
    Tile::addTile(ViewId::LIZARDLORD, sprite(11, 8));
    Tile::addTile(ViewId::IMP, sprite("imp"));
    Tile::addTile(ViewId::PRISONER, sprite("prisoner"));
    Tile::addTile(ViewId::OGRE, sprite("troll"));
    Tile::addTile(ViewId::CHICKEN, sprite("chicken"));
    Tile::addTile(ViewId::GNOME, sprite(15, 8));
    Tile::addTile(ViewId::DWARF, sprite(2, 6));
    Tile::addTile(ViewId::DWARF_BARON, sprite(3, 6));
    Tile::addTile(ViewId::SHOPKEEPER, sprite(4, 2));
    Tile::addTile(ViewId::BRIDGE, sprite("bridge").addOption(Dir::S, byName("bridge2")));
    Tile::addTile(ViewId::ROAD, getRoadTile(7));
    Tile::addTile(ViewId::FLOOR, sprite(3, 14, 1));
    Tile::addTile(ViewId::KEEPER_FLOOR, sprite(4, 18, 1));
    Tile::addTile(ViewId::SAND, getExtraBorderTile(14)
        .addExtraBorderId(ViewId::WATER));
    Tile::addTile(ViewId::MUD, getExtraBorderTile(10)
        .addExtraBorderId(ViewId::WATER)
        .addExtraBorderId(ViewId::HILL)
        .addExtraBorderId(ViewId::SAND));
    Tile::addTile(ViewId::GRASS, getExtraBorderTile(18)
        .addExtraBorderId(ViewId::SAND)
        .addExtraBorderId(ViewId::HILL)
        .addExtraBorderId(ViewId::MUD)
        .addExtraBorderId(ViewId::WATER));
    Tile::addTile(ViewId::CROPS, sprite("wheatfield1"));
    Tile::addTile(ViewId::CROPS2, sprite("wheatfield2"));
    Tile::addTile(ViewId::MOUNTAIN2, getMountainTile(9, 18));
    Tile::addTile(ViewId::WALL, getWallTile(2));
    Tile::addTile(ViewId::MOUNTAIN, sprite(17, 2, 2).setNoShadow());
    Tile::addTile(ViewId::GOLD_ORE, getMountainTile(8, 18));
    Tile::addTile(ViewId::IRON_ORE, getMountainTile(7, 18));
    Tile::addTile(ViewId::STONE, getMountainTile(6, 18));
    Tile::addTile(ViewId::SNOW, sprite(16, 2, 2).setNoShadow());
    Tile::addTile(ViewId::HILL, getExtraBorderTile(8)
        .addExtraBorderId(ViewId::SAND)
        .addExtraBorderId(ViewId::WATER));
    Tile::addTile(ViewId::WOOD_WALL, getWallTile(4));
    Tile::addTile(ViewId::BLACK_WALL, getWallTile(2));
    Tile::addTile(ViewId::YELLOW_WALL, getWallTile(8));
    Tile::addTile(ViewId::LOW_ROCK_WALL, getWallTile(21));
    Tile::addTile(ViewId::HELL_WALL, getWallTile(22));
    Tile::addTile(ViewId::CASTLE_WALL, getWallTile(5));
    Tile::addTile(ViewId::MUD_WALL, getWallTile(13));
    Tile::addTile(ViewId::SECRETPASS, sprite(0, 15, 1));
    Tile::addTile(ViewId::DUNGEON_ENTRANCE, sprite(15, 2, 2).setNoShadow());
    Tile::addTile(ViewId::DUNGEON_ENTRANCE_MUD, sprite(19, 2, 2).setNoShadow());
    Tile::addTile(ViewId::DOWN_STAIRCASE, sprite(8, 0, 1).setNoShadow());
    Tile::addTile(ViewId::UP_STAIRCASE, sprite(7, 0, 1).setNoShadow());
    Tile::addTile(ViewId::DOWN_STAIRCASE_CELLAR, sprite(8, 21, 1).setNoShadow());
    Tile::addTile(ViewId::UP_STAIRCASE_CELLAR, sprite(7, 21, 1).setNoShadow());
    Tile::addTile(ViewId::DOWN_STAIRCASE_HELL, sprite(8, 1, 1).setNoShadow());
    Tile::addTile(ViewId::UP_STAIRCASE_HELL, sprite(7, 22, 1).setNoShadow());
    Tile::addTile(ViewId::DOWN_STAIRCASE_PYR, sprite(8, 8, 1).setNoShadow());
    Tile::addTile(ViewId::UP_STAIRCASE_PYR, sprite(7, 8, 1).setNoShadow());
    Tile::addTile(ViewId::WELL, sprite(5, 8, 2).setNoShadow());
    Tile::addTile(ViewId::STATUE1, sprite(6, 5, 2).setNoShadow());
    Tile::addTile(ViewId::STATUE2, sprite(7, 5, 2).setNoShadow());
    Tile::addTile(ViewId::THRONE, sprite(7, 4, 2).setNoShadow());
    Tile::addTile(ViewId::GREAT_ORC, sprite(6, 14));
    Tile::addTile(ViewId::ORC, sprite("orc"));
    Tile::addTile(ViewId::ORC_SHAMAN, sprite("shaman"));
    Tile::addTile(ViewId::HARPY, sprite("harpysmall"));
    Tile::addTile(ViewId::DOPPLEGANGER, sprite("dopple"));
    Tile::addTile(ViewId::SUCCUBUS, sprite("succubussmall"));
    Tile::addTile(ViewId::BANDIT, sprite(0, 2));
    Tile::addTile(ViewId::GHOST, sprite("ghost4"));
    Tile::addTile(ViewId::SPIRIT, sprite(17, 14));
    Tile::addTile(ViewId::DEVIL, sprite(17, 18));
    Tile::addTile(ViewId::DARK_KNIGHT, sprite(12, 14));
    Tile::addTile(ViewId::GREEN_DRAGON, sprite("greendragon"));
    Tile::addTile(ViewId::RED_DRAGON, sprite("reddragon"));
    Tile::addTile(ViewId::CYCLOPS, sprite(10, 14));
    Tile::addTile(ViewId::WITCH, sprite(15, 16));
    Tile::addTile(ViewId::KNIGHT, sprite(0, 0));
    Tile::addTile(ViewId::WARRIOR, sprite(6, 0));
    Tile::addTile(ViewId::SHAMAN, sprite(5, 0));
    Tile::addTile(ViewId::CASTLE_GUARD, sprite(15, 2));
    Tile::addTile(ViewId::AVATAR, sprite(9, 0));
    Tile::addTile(ViewId::ARCHER, sprite(2, 0));
    Tile::addTile(ViewId::PESEANT, sprite("peasant"));
    Tile::addTile(ViewId::PESEANT_WOMAN, sprite("peasantgirl"));
    Tile::addTile(ViewId::CHILD, sprite("peasantkid"));
    Tile::addTile(ViewId::CLAY_GOLEM, sprite(12, 11));
    Tile::addTile(ViewId::STONE_GOLEM, sprite(10, 10));
    Tile::addTile(ViewId::IRON_GOLEM, sprite(12, 10));
    Tile::addTile(ViewId::LAVA_GOLEM, sprite(13, 10));
    Tile::addTile(ViewId::AIR_ELEMENTAL, sprite("airelemental"));
    Tile::addTile(ViewId::FIRE_ELEMENTAL, sprite("fireelemental"));
    Tile::addTile(ViewId::WATER_ELEMENTAL, sprite("waterelemental"));
    Tile::addTile(ViewId::EARTH_ELEMENTAL, sprite("earthelemental"));
    Tile::addTile(ViewId::ENT, sprite("ent"));
    Tile::addTile(ViewId::ANGEL, sprite("angel"));
    Tile::addTile(ViewId::ZOMBIE, sprite(0, 16));
    Tile::addTile(ViewId::SKELETON, sprite("skeleton"));
    Tile::addTile(ViewId::VAMPIRE_LORD, sprite("vampirelord"));
    Tile::addTile(ViewId::MUMMY, sprite(7, 16));
    Tile::addTile(ViewId::MUMMY_LORD, sprite(8, 16));
    Tile::addTile(ViewId::JACKAL, sprite(12, 12));
    Tile::addTile(ViewId::DEER, sprite("deer"));
    Tile::addTile(ViewId::HORSE, sprite("horse"));
    Tile::addTile(ViewId::COW, sprite("cow"));
    Tile::addTile(ViewId::PIG, sprite("pig"));
    Tile::addTile(ViewId::GOAT, sprite("goat"));
    Tile::addTile(ViewId::BOAR, sprite("boar"));
    Tile::addTile(ViewId::FOX, sprite("fox"));
    Tile::addTile(ViewId::WOLF, sprite("wolf"));
    Tile::addTile(ViewId::WEREWOLF, sprite("werewolf2"));
    Tile::addTile(ViewId::DOG, sprite("dog"));
    Tile::addTile(ViewId::KRAKEN_HEAD, sprite("krakenhead"));
    Tile::addTile(ViewId::KRAKEN_LAND, sprite("krakenland1"));
    Tile::addTile(ViewId::KRAKEN_WATER, sprite("krakenwater2"));
    Tile::addTile(ViewId::DEATH, sprite(9, 16));
    Tile::addTile(ViewId::NIGHTMARE, sprite(9, 16));
    Tile::addTile(ViewId::FIRE_SPHERE, sprite(16, 20));
    Tile::addTile(ViewId::BEAR, sprite(8, 18));
    Tile::addTile(ViewId::BAT, sprite(2, 12));
    Tile::addTile(ViewId::GOBLIN, sprite("goblin"));
    Tile::addTile(ViewId::LEPRECHAUN, sprite("leprechaun"));
    Tile::addTile(ViewId::RAT, sprite("rat"));
    Tile::addTile(ViewId::SPIDER, sprite(6, 12));
    Tile::addTile(ViewId::FLY, sprite(10, 12));
    Tile::addTile(ViewId::SCORPION, sprite(11, 18));
    Tile::addTile(ViewId::SNAKE, sprite(9, 12));
    Tile::addTile(ViewId::VULTURE, sprite("vulture"));
    Tile::addTile(ViewId::RAVEN, sprite(17, 12));
    Tile::addTile(ViewId::BODY_PART, sprite("corpse4"));
    Tile::addTile(ViewId::BONE, sprite(3, 0, 2));
    Tile::addTile(ViewId::BUSH, sprite("bush").setNoShadow().addHighlight(byName("bush_mark")));
    Tile::addTile(ViewId::WATER, getWaterTile(5));
    Tile::addTile(ViewId::MAGMA, getWaterTile(11));
    Tile::addTile(ViewId::DOOR, sprite("door").setNoShadow());
    Tile::addTile(ViewId::BARRICADE, sprite("barricade").setNoShadow());
    Tile::addTile(ViewId::DIG_ICON, sprite(8, 10, 2));
    Tile::addTile(ViewId::SWORD, sprite(12, 9, 3));
    Tile::addTile(ViewId::SPEAR, sprite(5, 8, 3));
    Tile::addTile(ViewId::SPECIAL_SWORD, sprite(13, 9, 3));
    Tile::addTile(ViewId::ELVEN_SWORD, sprite(14, 9, 3));
    Tile::addTile(ViewId::KNIFE, sprite(20, 9, 3));
    Tile::addTile(ViewId::WAR_HAMMER, sprite(10, 7, 3));
    Tile::addTile(ViewId::SPECIAL_WAR_HAMMER, sprite(11, 7, 3));
    Tile::addTile(ViewId::BATTLE_AXE, sprite(13, 7, 3));
    Tile::addTile(ViewId::SPECIAL_BATTLE_AXE, sprite(21, 7, 3));
    Tile::addTile(ViewId::BOW, sprite(14, 8, 3));
    Tile::addTile(ViewId::ARROW, sprite(5, 8, 3));
    Tile::addTile(ViewId::CLUB, sprite("club"));
    Tile::addTile(ViewId::HEAVY_CLUB, sprite("heavy club"));
    Tile::addTile(ViewId::SCROLL, sprite(3, 6, 3));
    Tile::addTile(ViewId::STEEL_AMULET, sprite(1, 1, 3));
    Tile::addTile(ViewId::COPPER_AMULET, sprite(2, 1, 3));
    Tile::addTile(ViewId::CRYSTAL_AMULET, sprite(4, 1, 3));
    Tile::addTile(ViewId::WOODEN_AMULET, sprite(0, 1, 3));
    Tile::addTile(ViewId::AMBER_AMULET, sprite(3, 1, 3));
    Tile::addTile(ViewId::FIRE_RESIST_RING, sprite(11, 3, 3));
    Tile::addTile(ViewId::POISON_RESIST_RING, sprite(16, 3, 3));
    Tile::addTile(ViewId::BOOK, sprite(0, 3, 3));
    Tile::addTile(ViewId::FIRST_AID, sprite("medkit2"));
    Tile::addTile(ViewId::TRAP_ITEM, sprite("trapbox"));
    Tile::addTile(ViewId::EFFERVESCENT_POTION, sprite(6, 0, 3));
    Tile::addTile(ViewId::MURKY_POTION, sprite(10, 0, 3));
    Tile::addTile(ViewId::SWIRLY_POTION, sprite(9, 0, 3));
    Tile::addTile(ViewId::VIOLET_POTION, sprite(7, 0, 3));
    Tile::addTile(ViewId::PUCE_POTION, sprite(8, 0, 3));
    Tile::addTile(ViewId::SMOKY_POTION, sprite(11, 0, 3));
    Tile::addTile(ViewId::FIZZY_POTION, sprite(9, 0, 3));
    Tile::addTile(ViewId::MILKY_POTION, sprite(11, 0, 3));
    Tile::addTile(ViewId::MUSHROOM, sprite(5, 4, 3));
    Tile::addTile(ViewId::KEY, sprite(5, 6, 3));
    Tile::addTile(ViewId::FOUNTAIN, sprite(0, 7, 2).setNoShadow());
    Tile::addTile(ViewId::GOLD, sprite(8, 3, 3).setNoShadow());
    Tile::addTile(ViewId::TREASURE_CHEST, sprite("treasurydeco").setNoShadow().addBackground(byName("treasury")));
    Tile::addTile(ViewId::CHEST, sprite(3, 3, 2).setNoShadow());
    Tile::addTile(ViewId::OPENED_CHEST, sprite(6, 3, 2).setNoShadow());
    Tile::addTile(ViewId::COFFIN, sprite(7, 3, 2).setNoShadow());
    Tile::addTile(ViewId::OPENED_COFFIN, sprite(8, 3, 2).setNoShadow());
    Tile::addTile(ViewId::BOULDER, sprite("boulder"));
    Tile::addTile(ViewId::PORTAL, sprite("surprise"));
    Tile::addTile(ViewId::GAS_TRAP, sprite(2, 6, 3));
    Tile::addTile(ViewId::ALARM_TRAP, sprite(16, 5, 3));
    Tile::addTile(ViewId::WEB_TRAP, sprite(4, 1, 2));
    Tile::addTile(ViewId::SURPRISE_TRAP, sprite("surprisetrap"));
    Tile::addTile(ViewId::TERROR_TRAP, sprite(1, 6, 3));
    Tile::addTile(ViewId::ROCK, sprite("stonepile"));
    Tile::addTile(ViewId::IRON_ROCK, sprite("ironpile2"));
    Tile::addTile(ViewId::WOOD_PLANK, sprite("wood2"));
    Tile::addTile(ViewId::STOCKPILE1, sprite("storage1").setFloorBorders());
    Tile::addTile(ViewId::STOCKPILE2, sprite("storage2").setFloorBorders());
    Tile::addTile(ViewId::STOCKPILE3, sprite("storage3").setFloorBorders());
    Tile::addTile(ViewId::PRISON, sprite(6, 2, 1));
    Tile::addTile(ViewId::BED, sprite("sleepdeco").setNoShadow());
    Tile::addTile(ViewId::DORM, sprite("sleep").setFloorBorders());
    Tile::addTile(ViewId::TORCH, sprite(12, 0, 2).setNoShadow().setTranslucent(0.35));
    Tile::addTile(ViewId::DUNGEON_HEART, sprite(6, 10, 2));
    Tile::addTile(ViewId::ALTAR, sprite(2, 7, 2).setNoShadow());
    Tile::addTile(ViewId::CREATURE_ALTAR, sprite(3, 7, 2).setNoShadow());
    Tile::addTile(ViewId::TORTURE_TABLE, empty().addConnection(DirSet::fullSet(), byName("torturedeco"))
        .addBackground(byName("torture"))
        .setFloorBorders());
    Tile::addTile(ViewId::IMPALED_HEAD, sprite("impaledhead").setNoShadow());
    Tile::addTile(ViewId::TRAINING_ROOM, empty().addConnection(DirSet::fullSet(), byName("traindeco"))
        .addBackground(byName("train")).setFloorBorders());
    Tile::addTile(ViewId::RITUAL_ROOM, empty().addConnection(DirSet::fullSet(), byName("ritualroomdeco"))
        .addBackground(byName("ritualroom"))
        .setFloorBorders());
    Tile::addTile(ViewId::LIBRARY, empty().addConnection(DirSet::fullSet(), byName("libdeco"))
        .addBackground(byName("lib")).setFloorBorders());
    Tile::addTile(ViewId::LABORATORY, empty().addConnection(DirSet::fullSet(), byName("labdeco"))
        .addBackground(byName("lab")).setFloorBorders());
    Tile::addTile(ViewId::CAULDRON, sprite("labdeco").setNoShadow());
    Tile::addTile(ViewId::BEAST_LAIR, sprite("lair").setFloorBorders());
    Tile::addTile(ViewId::BEAST_CAGE, sprite("lairdeco").setNoShadow()
        .addBackground(byName("lair")));
    Tile::addTile(ViewId::FORGE, empty().addConnection(DirSet::fullSet(), byName("forgedeco"))
        .addBackground(byName("forge"))
        .setFloorBorders());
    Tile::addTile(ViewId::WORKSHOP, empty().addConnection(DirSet::fullSet(), byName("workshopdeco"))
        .addBackground(byName("workshop"))
        .setFloorBorders());
    Tile::addTile(ViewId::JEWELER, empty().addConnection(DirSet::fullSet(), byName("jewelerdeco"))
        .addBackground(byName("jeweler"))
        .setFloorBorders());
    Tile::addTile(ViewId::CEMETERY, sprite("graveyard").setFloorBorders());
    Tile::addTile(ViewId::GRAVE, sprite("gravedeco").setNoShadow().addBackground(byName("graveyard")));
    Tile::addTile(ViewId::ROBE, sprite(7, 11, 3));
    Tile::addTile(ViewId::LEATHER_GLOVES, sprite(15, 11, 3));
    Tile::addTile(ViewId::DEXTERITY_GLOVES, sprite(19, 11, 3));
    Tile::addTile(ViewId::STRENGTH_GLOVES, sprite(20, 11, 3));
    Tile::addTile(ViewId::LEATHER_ARMOR, sprite(0, 12, 3));
    Tile::addTile(ViewId::LEATHER_HELM, sprite(10, 12, 3));
    Tile::addTile(ViewId::TELEPATHY_HELM, sprite(17, 12, 3));
    Tile::addTile(ViewId::CHAIN_ARMOR, sprite(1, 12, 3));
    Tile::addTile(ViewId::IRON_HELM, sprite(14, 12, 3));
    Tile::addTile(ViewId::LEATHER_BOOTS, sprite(0, 13, 3));
    Tile::addTile(ViewId::IRON_BOOTS, sprite(6, 13, 3));
    Tile::addTile(ViewId::SPEED_BOOTS, sprite(3, 13, 3));
    Tile::addTile(ViewId::LEVITATION_BOOTS, sprite(2, 13, 3));
    Tile::addTile(ViewId::GUARD_POST, sprite("guardroom"));
    Tile::addTile(ViewId::MANA, sprite(5, 10, 2));
    Tile::addTile(ViewId::DANGER, sprite(12, 9, 2));
    Tile::addTile(ViewId::FETCH_ICON, sprite(15, 11, 3));
    Tile::addTile(ViewId::EYEBALL, sprite("eyeball2").setNoShadow());
    Tile::addTile(ViewId::FOG_OF_WAR, getWaterTile(14));
    Tile::addTile(ViewId::FOG_OF_WAR_CORNER, sprite(14, 5, 4)
        .addConnection({Dir::NE}, byCoord(14, 11, 4))
        .addConnection({Dir::NW}, byCoord(13, 11, 4))
        .addConnection({Dir::SE}, byCoord(14, 12, 4))
        .addConnection({Dir::SW}, byCoord(13, 12, 4)));
    Tile::addTile(ViewId::SPECIAL_BEAST, sprite(7, 10));
    Tile::addTile(ViewId::SPECIAL_HUMANOID, sprite(2, 10));
    Tile::addTile(ViewId::TEAM_BUTTON, sprite("team_button"));
    Tile::addTile(ViewId::TEAM_BUTTON_HIGHLIGHT, sprite("team_button_highlight"));
    Tile::addTile(ViewId::STAT_ATT, sprite(12, 9, 3));
    Tile::addTile(ViewId::STAT_DEF, sprite(2, 10, 3));
    Tile::addTile(ViewId::STAT_STR, sprite(9, 12, 3));
    Tile::addTile(ViewId::STAT_DEX, sprite(20, 11, 3));
    Tile::addTile(ViewId::STAT_LVL, sprite(19, 2, 3));
    Tile::addTile(ViewId::STAT_SPD, sprite(7, 4, 3));
    Tile::addTile(ViewId::STAT_ACC, sprite(11, 6, 3));
  }

  Tile symbol(const string& s, ColorId id, bool symbol = false) {
    return Tile::fromString(s, id, symbol);
  }

  Tile symbol(sf::Uint32 i, ColorId id, bool symbol = false) {
    return Tile::unicode(i, id, symbol);
  }

  void genSymbols() {
    Tile::addSymbol(ViewId::EMPTY, symbol(" ", ColorId::BLACK));
    Tile::addSymbol(ViewId::DIG_MARK, symbol(" ", ColorId::BLACK));
    Tile::addSymbol(ViewId::PLAYER, symbol("@", ColorId::WHITE));
    Tile::addSymbol(ViewId::KEEPER, symbol("@", ColorId::PURPLE));
    Tile::addSymbol(ViewId::UNDEAD_KEEPER, symbol("@", ColorId::DARK_GRAY));
    Tile::addSymbol(ViewId::UNKNOWN_MONSTER, symbol("?", ColorId::LIGHT_GREEN));
    Tile::addSymbol(ViewId::ELF, symbol("@", ColorId::LIGHT_GREEN));
    Tile::addSymbol(ViewId::ELF_ARCHER, symbol("@", ColorId::GREEN));
    Tile::addSymbol(ViewId::ELF_CHILD, symbol("@", ColorId::LIGHT_GREEN));
    Tile::addSymbol(ViewId::ELF_LORD, symbol("@", ColorId::DARK_GREEN));
    Tile::addSymbol(ViewId::SHOPKEEPER, symbol("@", ColorId::LIGHT_BLUE));
    Tile::addSymbol(ViewId::LIZARDMAN, symbol("@", ColorId::LIGHT_BROWN));
    Tile::addSymbol(ViewId::LIZARDLORD, symbol("@", ColorId::BROWN));
    Tile::addSymbol(ViewId::IMP, symbol("i", ColorId::LIGHT_BROWN));
    Tile::addSymbol(ViewId::PRISONER, symbol("@", ColorId::LIGHT_BROWN));
    Tile::addSymbol(ViewId::OGRE, symbol("O", ColorId::GREEN));
    Tile::addSymbol(ViewId::CHICKEN, symbol("c", ColorId::YELLOW));
    Tile::addSymbol(ViewId::GNOME, symbol("g", ColorId::GREEN));
    Tile::addSymbol(ViewId::DWARF, symbol("h", ColorId::BLUE));
    Tile::addSymbol(ViewId::DWARF_BARON, symbol("h", ColorId::DARK_BLUE));
    Tile::addSymbol(ViewId::FLOOR, symbol(".", ColorId::LIGHT_GRAY));
    Tile::addSymbol(ViewId::KEEPER_FLOOR, symbol(".", ColorId::WHITE));
    Tile::addSymbol(ViewId::BRIDGE, symbol("_", ColorId::BROWN));
    Tile::addSymbol(ViewId::ROAD, symbol(".", ColorId::LIGHT_GRAY));
    Tile::addSymbol(ViewId::SAND, symbol(".", ColorId::YELLOW));
    Tile::addSymbol(ViewId::MUD, symbol(0x1d0f0, ColorId::BROWN, true));
    Tile::addSymbol(ViewId::GRASS, symbol(0x1d0f0, ColorId::GREEN, true));
    Tile::addSymbol(ViewId::CROPS, symbol(0x1d0f0, ColorId::YELLOW, true));
    Tile::addSymbol(ViewId::CROPS2, symbol(0x1d0f0, ColorId::YELLOW, true));
    Tile::addSymbol(ViewId::CASTLE_WALL, symbol("#", ColorId::LIGHT_GRAY));
    Tile::addSymbol(ViewId::MUD_WALL, symbol("#", ColorId::LIGHT_BROWN));
    Tile::addSymbol(ViewId::WALL, symbol("#", ColorId::LIGHT_GRAY));
    Tile::addSymbol(ViewId::MOUNTAIN, symbol(0x25ee, ColorId::DARK_GRAY, true));
    Tile::addSymbol(ViewId::MOUNTAIN2, symbol("#", ColorId::DARK_GRAY));
    Tile::addSymbol(ViewId::GOLD_ORE, symbol("⁂", ColorId::YELLOW, true));
    Tile::addSymbol(ViewId::IRON_ORE, symbol("⁂", ColorId::DARK_BROWN, true));
    Tile::addSymbol(ViewId::STONE, symbol("⁂", ColorId::LIGHT_GRAY, true));
    Tile::addSymbol(ViewId::SNOW, symbol(0x25ee, ColorId::WHITE, true));
    Tile::addSymbol(ViewId::HILL, symbol(0x1d022, ColorId::DARK_GREEN, true));
    Tile::addSymbol(ViewId::WOOD_WALL, symbol("#", ColorId::DARK_BROWN));
    Tile::addSymbol(ViewId::BLACK_WALL, symbol("#", ColorId::LIGHT_GRAY));
    Tile::addSymbol(ViewId::YELLOW_WALL, symbol("#", ColorId::YELLOW));
    Tile::addSymbol(ViewId::LOW_ROCK_WALL, symbol("#", ColorId::DARK_GRAY));
    Tile::addSymbol(ViewId::HELL_WALL, symbol("#", ColorId::RED));
    Tile::addSymbol(ViewId::SECRETPASS, symbol("#", ColorId::LIGHT_GRAY));
    Tile::addSymbol(ViewId::DUNGEON_ENTRANCE, symbol(0x2798, ColorId::BROWN, true));
    Tile::addSymbol(ViewId::DUNGEON_ENTRANCE_MUD, symbol(0x2798, ColorId::BROWN, true));
    Tile::addSymbol(ViewId::DOWN_STAIRCASE_CELLAR, symbol(0x2798, ColorId::ALMOST_WHITE, true));
    Tile::addSymbol(ViewId::DOWN_STAIRCASE, symbol(0x2798, ColorId::ALMOST_WHITE, true));
    Tile::addSymbol(ViewId::UP_STAIRCASE_CELLAR, symbol(0x279a, ColorId::ALMOST_WHITE, true));
    Tile::addSymbol(ViewId::UP_STAIRCASE, symbol(0x279a, ColorId::ALMOST_WHITE, true));
    Tile::addSymbol(ViewId::DOWN_STAIRCASE_HELL, symbol(0x2798, ColorId::RED, true));
    Tile::addSymbol(ViewId::UP_STAIRCASE_HELL, symbol(0x279a, ColorId::RED, true));
    Tile::addSymbol(ViewId::DOWN_STAIRCASE_PYR, symbol(0x2798, ColorId::YELLOW, true));
    Tile::addSymbol(ViewId::UP_STAIRCASE_PYR, symbol(0x279a, ColorId::YELLOW, true));
    Tile::addSymbol(ViewId::WELL, symbol("0", ColorId::BLUE));
    Tile::addSymbol(ViewId::STATUE1, symbol("&", ColorId::LIGHT_GRAY));
    Tile::addSymbol(ViewId::STATUE2, symbol("&", ColorId::LIGHT_GRAY));
    Tile::addSymbol(ViewId::THRONE, symbol("Ω", ColorId::YELLOW));
    Tile::addSymbol(ViewId::GREAT_ORC, symbol("o", ColorId::PURPLE));
    Tile::addSymbol(ViewId::ORC, symbol("o", ColorId::DARK_BLUE));
    Tile::addSymbol(ViewId::ORC_SHAMAN, symbol("o", ColorId::YELLOW));
    Tile::addSymbol(ViewId::HARPY, symbol("R", ColorId::YELLOW));
    Tile::addSymbol(ViewId::DOPPLEGANGER, symbol("&", ColorId::YELLOW));
    Tile::addSymbol(ViewId::SUCCUBUS, symbol("&", ColorId::RED));
    Tile::addSymbol(ViewId::BANDIT, symbol("@", ColorId::DARK_BLUE));
    Tile::addSymbol(ViewId::DARK_KNIGHT, symbol("@", ColorId::PURPLE));
    Tile::addSymbol(ViewId::GREEN_DRAGON, symbol("D", ColorId::GREEN));
    Tile::addSymbol(ViewId::RED_DRAGON, symbol("D", ColorId::RED));
    Tile::addSymbol(ViewId::CYCLOPS, symbol("C", ColorId::GREEN));
    Tile::addSymbol(ViewId::WITCH, symbol("@", ColorId::BROWN));
    Tile::addSymbol(ViewId::GHOST, symbol("&", ColorId::WHITE));
    Tile::addSymbol(ViewId::SPIRIT, symbol("&", ColorId::LIGHT_BLUE));
    Tile::addSymbol(ViewId::DEVIL, symbol("&", ColorId::PURPLE));
    Tile::addSymbol(ViewId::CASTLE_GUARD, symbol("@", ColorId::LIGHT_GRAY));
    Tile::addSymbol(ViewId::KNIGHT, symbol("@", ColorId::LIGHT_GRAY));
    Tile::addSymbol(ViewId::WARRIOR, symbol("@", ColorId::DARK_GRAY));
    Tile::addSymbol(ViewId::SHAMAN, symbol("@", ColorId::YELLOW));
    Tile::addSymbol(ViewId::AVATAR, symbol("@", ColorId::BLUE));
    Tile::addSymbol(ViewId::ARCHER, symbol("@", ColorId::BROWN));
    Tile::addSymbol(ViewId::PESEANT, symbol("@", ColorId::GREEN));
    Tile::addSymbol(ViewId::PESEANT_WOMAN, symbol("@", ColorId::GREEN));
    Tile::addSymbol(ViewId::CHILD, symbol("@", ColorId::LIGHT_GREEN));
    Tile::addSymbol(ViewId::CLAY_GOLEM, symbol("Y", ColorId::YELLOW));
    Tile::addSymbol(ViewId::STONE_GOLEM, symbol("Y", ColorId::LIGHT_GRAY));
    Tile::addSymbol(ViewId::IRON_GOLEM, symbol("Y", ColorId::ORANGE));
    Tile::addSymbol(ViewId::LAVA_GOLEM, symbol("Y", ColorId::PURPLE));
    Tile::addSymbol(ViewId::AIR_ELEMENTAL, symbol("E", ColorId::LIGHT_BLUE));
    Tile::addSymbol(ViewId::FIRE_ELEMENTAL, symbol("E", ColorId::RED));
    Tile::addSymbol(ViewId::WATER_ELEMENTAL, symbol("E", ColorId::BLUE));
    Tile::addSymbol(ViewId::EARTH_ELEMENTAL, symbol("E", ColorId::GRAY));
    Tile::addSymbol(ViewId::ENT, symbol("E", ColorId::LIGHT_BROWN));
    Tile::addSymbol(ViewId::ANGEL, symbol("A", ColorId::LIGHT_BLUE));
    Tile::addSymbol(ViewId::ZOMBIE, symbol("Z", ColorId::GREEN));
    Tile::addSymbol(ViewId::SKELETON, symbol("Z", ColorId::WHITE));
    Tile::addSymbol(ViewId::VAMPIRE, symbol("V", ColorId::DARK_GRAY));
    Tile::addSymbol(ViewId::VAMPIRE_LORD, symbol("V", ColorId::PURPLE));
    Tile::addSymbol(ViewId::MUMMY, symbol("Z", ColorId::YELLOW));
    Tile::addSymbol(ViewId::MUMMY_LORD, symbol("Z", ColorId::ORANGE));
    Tile::addSymbol(ViewId::JACKAL, symbol("d", ColorId::LIGHT_BROWN));
    Tile::addSymbol(ViewId::DEER, symbol("R", ColorId::DARK_BROWN));
    Tile::addSymbol(ViewId::HORSE, symbol("H", ColorId::LIGHT_BROWN));
    Tile::addSymbol(ViewId::COW, symbol("C", ColorId::WHITE));
    Tile::addSymbol(ViewId::PIG, symbol("p", ColorId::YELLOW));
    Tile::addSymbol(ViewId::GOAT, symbol("g", ColorId::GRAY));
    Tile::addSymbol(ViewId::BOAR, symbol("b", ColorId::LIGHT_BROWN));
    Tile::addSymbol(ViewId::FOX, symbol("d", ColorId::ORANGE_BROWN));
    Tile::addSymbol(ViewId::WOLF, symbol("d", ColorId::DARK_BLUE));
    Tile::addSymbol(ViewId::WEREWOLF, symbol("d", ColorId::WHITE));
    Tile::addSymbol(ViewId::DOG, symbol("d", ColorId::BROWN));
    Tile::addSymbol(ViewId::KRAKEN_HEAD, symbol("S", ColorId::GREEN));
    Tile::addSymbol(ViewId::KRAKEN_WATER, symbol("S", ColorId::DARK_GREEN));
    Tile::addSymbol(ViewId::KRAKEN_LAND, symbol("S", ColorId::DARK_GREEN));
    Tile::addSymbol(ViewId::DEATH, symbol("D", ColorId::DARK_GRAY));
    Tile::addSymbol(ViewId::NIGHTMARE, symbol("n", ColorId::PURPLE));
    Tile::addSymbol(ViewId::FIRE_SPHERE, symbol("e", ColorId::RED));
    Tile::addSymbol(ViewId::BEAR, symbol("N", ColorId::BROWN));
    Tile::addSymbol(ViewId::BAT, symbol("b", ColorId::DARK_GRAY));
    Tile::addSymbol(ViewId::GOBLIN, symbol("o", ColorId::GREEN));
    Tile::addSymbol(ViewId::LEPRECHAUN, symbol("l", ColorId::GREEN));
    Tile::addSymbol(ViewId::RAT, symbol("r", ColorId::BROWN));
    Tile::addSymbol(ViewId::SPIDER, symbol("s", ColorId::BROWN));
    Tile::addSymbol(ViewId::FLY, symbol("b", ColorId::GRAY));
    Tile::addSymbol(ViewId::SCORPION, symbol("s", ColorId::LIGHT_GRAY));
    Tile::addSymbol(ViewId::SNAKE, symbol("S", ColorId::YELLOW));
    Tile::addSymbol(ViewId::VULTURE, symbol("v", ColorId::DARK_GRAY));
    Tile::addSymbol(ViewId::RAVEN, symbol("v", ColorId::DARK_GRAY));
    Tile::addSymbol(ViewId::BODY_PART, symbol("%", ColorId::RED));
    Tile::addSymbol(ViewId::BONE, symbol("%", ColorId::WHITE));
    Tile::addSymbol(ViewId::BUSH, symbol("&", ColorId::DARK_GREEN));
    Tile::addSymbol(ViewId::DECID_TREE, symbol(0x1f70d, ColorId::DARK_GREEN, true));
    Tile::addSymbol(ViewId::CANIF_TREE, symbol(0x2663, ColorId::DARK_GREEN, true));
    Tile::addSymbol(ViewId::TREE_TRUNK, symbol(".", ColorId::BROWN));
    Tile::addSymbol(ViewId::BURNT_TREE, symbol(".", ColorId::DARK_GRAY));
    Tile::addSymbol(ViewId::WATER, symbol("~", ColorId::LIGHT_BLUE));
    Tile::addSymbol(ViewId::MAGMA, symbol("~", ColorId::RED));
    Tile::addSymbol(ViewId::DOOR, symbol("|", ColorId::BROWN));
    Tile::addSymbol(ViewId::BARRICADE, symbol("X", ColorId::BROWN));
    Tile::addSymbol(ViewId::DIG_ICON, symbol(0x26cf, ColorId::LIGHT_GRAY, true));
    Tile::addSymbol(ViewId::SWORD, symbol(")", ColorId::LIGHT_GRAY));
    Tile::addSymbol(ViewId::SPEAR, symbol("/", ColorId::LIGHT_GRAY));
    Tile::addSymbol(ViewId::SPECIAL_SWORD, symbol(")", ColorId::YELLOW));
    Tile::addSymbol(ViewId::ELVEN_SWORD, symbol(")", ColorId::GRAY));
    Tile::addSymbol(ViewId::KNIFE, symbol(")", ColorId::WHITE));
    Tile::addSymbol(ViewId::WAR_HAMMER, symbol(")", ColorId::BLUE));
    Tile::addSymbol(ViewId::SPECIAL_WAR_HAMMER, symbol(")", ColorId::LIGHT_BLUE));
    Tile::addSymbol(ViewId::BATTLE_AXE, symbol(")", ColorId::GREEN));
    Tile::addSymbol(ViewId::SPECIAL_BATTLE_AXE, symbol(")", ColorId::LIGHT_GREEN));
    Tile::addSymbol(ViewId::BOW, symbol(")", ColorId::BROWN));
    Tile::addSymbol(ViewId::CLUB, symbol(")", ColorId::BROWN));
    Tile::addSymbol(ViewId::HEAVY_CLUB, symbol(")", ColorId::BROWN));
    Tile::addSymbol(ViewId::ARROW, symbol("\\", ColorId::BROWN));
    Tile::addSymbol(ViewId::SCROLL, symbol("?", ColorId::WHITE));
    Tile::addSymbol(ViewId::STEEL_AMULET, symbol("\"", ColorId::YELLOW));
    Tile::addSymbol(ViewId::COPPER_AMULET, symbol("\"", ColorId::YELLOW));
    Tile::addSymbol(ViewId::CRYSTAL_AMULET, symbol("\"", ColorId::YELLOW));
    Tile::addSymbol(ViewId::WOODEN_AMULET, symbol("\"", ColorId::YELLOW));
    Tile::addSymbol(ViewId::AMBER_AMULET, symbol("\"", ColorId::YELLOW));
    Tile::addSymbol(ViewId::FIRE_RESIST_RING, symbol("=", ColorId::RED));
    Tile::addSymbol(ViewId::POISON_RESIST_RING, symbol("=", ColorId::GREEN));
    Tile::addSymbol(ViewId::BOOK, symbol("+", ColorId::YELLOW));
    Tile::addSymbol(ViewId::FIRST_AID, symbol("+", ColorId::RED));
    Tile::addSymbol(ViewId::TRAP_ITEM, symbol("+", ColorId::YELLOW));
    Tile::addSymbol(ViewId::EFFERVESCENT_POTION, symbol("!", ColorId::LIGHT_RED));
    Tile::addSymbol(ViewId::MURKY_POTION, symbol("!", ColorId::BLUE));
    Tile::addSymbol(ViewId::SWIRLY_POTION, symbol("!", ColorId::YELLOW));
    Tile::addSymbol(ViewId::VIOLET_POTION, symbol("!", ColorId::VIOLET));
    Tile::addSymbol(ViewId::PUCE_POTION, symbol("!", ColorId::DARK_BROWN));
    Tile::addSymbol(ViewId::SMOKY_POTION, symbol("!", ColorId::LIGHT_GRAY));
    Tile::addSymbol(ViewId::FIZZY_POTION, symbol("!", ColorId::LIGHT_BLUE));
    Tile::addSymbol(ViewId::MILKY_POTION, symbol("!", ColorId::WHITE));
    Tile::addSymbol(ViewId::MUSHROOM, symbol(0x22c6, ColorId::PINK, true));
    Tile::addSymbol(ViewId::KEY, symbol("*", ColorId::YELLOW));
    Tile::addSymbol(ViewId::FOUNTAIN, symbol("0", ColorId::LIGHT_BLUE));
    Tile::addSymbol(ViewId::GOLD, symbol("$", ColorId::YELLOW));
    Tile::addSymbol(ViewId::TREASURE_CHEST, symbol("=", ColorId::BROWN));
    Tile::addSymbol(ViewId::OPENED_CHEST, symbol("=", ColorId::BROWN));
    Tile::addSymbol(ViewId::CHEST, symbol("=", ColorId::BROWN));
    Tile::addSymbol(ViewId::OPENED_COFFIN, symbol("⚰", ColorId::DARK_GRAY, true));
    Tile::addSymbol(ViewId::COFFIN, symbol("⚰", ColorId::DARK_GRAY, true));
    Tile::addSymbol(ViewId::BOULDER, symbol("●", ColorId::LIGHT_GRAY, true));
    Tile::addSymbol(ViewId::PORTAL, symbol(0x1d6af, ColorId::LIGHT_GREEN, true));
    Tile::addSymbol(ViewId::GAS_TRAP, symbol("☠", ColorId::GREEN, true));
    Tile::addSymbol(ViewId::ALARM_TRAP, symbol("^", ColorId::RED, true));
    Tile::addSymbol(ViewId::WEB_TRAP, symbol("#", ColorId::WHITE, true));
    Tile::addSymbol(ViewId::SURPRISE_TRAP, symbol("^", ColorId::BLUE, true));
    Tile::addSymbol(ViewId::TERROR_TRAP, symbol("^", ColorId::WHITE, true));
    Tile::addSymbol(ViewId::ROCK, symbol("✱", ColorId::LIGHT_GRAY, true));
    Tile::addSymbol(ViewId::IRON_ROCK, symbol("✱", ColorId::ORANGE, true));
    Tile::addSymbol(ViewId::WOOD_PLANK, symbol("\\", ColorId::BROWN));
    Tile::addSymbol(ViewId::STOCKPILE1, symbol(".", ColorId::YELLOW));
    Tile::addSymbol(ViewId::STOCKPILE2, symbol(".", ColorId::LIGHT_GREEN));
    Tile::addSymbol(ViewId::STOCKPILE3, symbol(".", ColorId::LIGHT_BLUE));
    Tile::addSymbol(ViewId::PRISON, symbol(".", ColorId::BLUE));
    Tile::addSymbol(ViewId::DORM, symbol(".", ColorId::BROWN));
    Tile::addSymbol(ViewId::BED, symbol("=", ColorId::WHITE));
    Tile::addSymbol(ViewId::DUNGEON_HEART, symbol("♥", ColorId::WHITE, true));
    Tile::addSymbol(ViewId::TORCH, symbol("*", ColorId::YELLOW));
    Tile::addSymbol(ViewId::ALTAR, symbol("Ω", ColorId::WHITE, true));
    Tile::addSymbol(ViewId::CREATURE_ALTAR, symbol("Ω", ColorId::YELLOW, true));
    Tile::addSymbol(ViewId::TORTURE_TABLE, symbol("=", ColorId::GRAY));
    Tile::addSymbol(ViewId::IMPALED_HEAD, symbol("⚲", ColorId::BROWN, true));
    Tile::addSymbol(ViewId::TRAINING_ROOM, symbol("‡", ColorId::BROWN, true));
    Tile::addSymbol(ViewId::RITUAL_ROOM, symbol("Ω", ColorId::PURPLE, true));
    Tile::addSymbol(ViewId::LIBRARY, symbol("▤", ColorId::BROWN, true));
    Tile::addSymbol(ViewId::LABORATORY, symbol("ω", ColorId::PURPLE, true));
    Tile::addSymbol(ViewId::CAULDRON, symbol("ω", ColorId::PURPLE, true));
    Tile::addSymbol(ViewId::BEAST_LAIR, symbol(".", ColorId::YELLOW));
    Tile::addSymbol(ViewId::BEAST_CAGE, symbol("▥", ColorId::LIGHT_GRAY, true));
    Tile::addSymbol(ViewId::WORKSHOP, symbol("&", ColorId::LIGHT_BROWN));
    Tile::addSymbol(ViewId::FORGE, symbol("&", ColorId::LIGHT_BLUE));
    Tile::addSymbol(ViewId::JEWELER, symbol("&", ColorId::YELLOW));
    Tile::addSymbol(ViewId::CEMETERY, symbol(".", ColorId::DARK_BLUE));
    Tile::addSymbol(ViewId::GRAVE, symbol(0x2617, ColorId::GRAY, true));
    Tile::addSymbol(ViewId::BORDER_GUARD, symbol(" ", ColorId::BLACK));
    Tile::addSymbol(ViewId::ROBE, symbol("[", ColorId::LIGHT_BROWN));
    Tile::addSymbol(ViewId::LEATHER_GLOVES, symbol("[", ColorId::BROWN));
    Tile::addSymbol(ViewId::STRENGTH_GLOVES, symbol("[", ColorId::RED));
    Tile::addSymbol(ViewId::DEXTERITY_GLOVES, symbol("[", ColorId::LIGHT_BLUE));
    Tile::addSymbol(ViewId::LEATHER_ARMOR, symbol("[", ColorId::BROWN));
    Tile::addSymbol(ViewId::LEATHER_HELM, symbol("[", ColorId::BROWN));
    Tile::addSymbol(ViewId::TELEPATHY_HELM, symbol("[", ColorId::LIGHT_GREEN));
    Tile::addSymbol(ViewId::CHAIN_ARMOR, symbol("[", ColorId::LIGHT_GRAY));
    Tile::addSymbol(ViewId::IRON_HELM, symbol("[", ColorId::LIGHT_GRAY));
    Tile::addSymbol(ViewId::LEATHER_BOOTS, symbol("[", ColorId::BROWN));
    Tile::addSymbol(ViewId::IRON_BOOTS, symbol("[", ColorId::LIGHT_GRAY));
    Tile::addSymbol(ViewId::SPEED_BOOTS, symbol("[", ColorId::LIGHT_BLUE));
    Tile::addSymbol(ViewId::LEVITATION_BOOTS, symbol("[", ColorId::LIGHT_GREEN));
    Tile::addSymbol(ViewId::FALLEN_TREE, symbol("*", ColorId::GREEN));
    Tile::addSymbol(ViewId::GUARD_POST, symbol("⚐", ColorId::YELLOW, true));
    Tile::addSymbol(ViewId::DESTROY_BUTTON, symbol("X", ColorId::RED));
    Tile::addSymbol(ViewId::FORBID_ZONE, symbol("#", ColorId::RED));
    Tile::addSymbol(ViewId::MANA, symbol("✱", ColorId::BLUE, true));
    Tile::addSymbol(ViewId::EYEBALL, symbol("e", ColorId::BLUE));
    Tile::addSymbol(ViewId::DANGER, symbol("*", ColorId::RED));
    Tile::addSymbol(ViewId::FETCH_ICON, symbol(0x1f44b, ColorId::LIGHT_BROWN, true));
    Tile::addSymbol(ViewId::FOG_OF_WAR, symbol(' ', ColorId::WHITE));
    Tile::addSymbol(ViewId::FOG_OF_WAR_CORNER, symbol(' ', ColorId::WHITE));
    Tile::addSymbol(ViewId::SPECIAL_BEAST, symbol('B', ColorId::PURPLE));
    Tile::addSymbol(ViewId::SPECIAL_HUMANOID, symbol('H', ColorId::PURPLE));
    Tile::addSymbol(ViewId::TEAM_BUTTON, symbol("⚐", ColorId::WHITE, true));
    Tile::addSymbol(ViewId::TEAM_BUTTON_HIGHLIGHT, symbol("⚐", ColorId::GREEN, true));
    Tile::addSymbol(ViewId::STAT_ATT, symbol("A", ColorId::BLUE));
    Tile::addSymbol(ViewId::STAT_DEF, symbol("D", ColorId::BLUE));
    Tile::addSymbol(ViewId::STAT_STR, symbol("S", ColorId::BLUE));
    Tile::addSymbol(ViewId::STAT_DEX, symbol("X", ColorId::BLUE));
    Tile::addSymbol(ViewId::STAT_LVL, symbol("L", ColorId::BLUE));
    Tile::addSymbol(ViewId::STAT_SPD, symbol("P", ColorId::BLUE));
    Tile::addSymbol(ViewId::STAT_ACC, symbol("C", ColorId::BLUE));
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

const Tile& Tile::getTile(ViewId id, bool sprite) {
  if (sprite) {
    CHECK(tiles[id]);
    return *tiles[id];
  }
  else {
    CHECK(symbols[id]);
    return *symbols[id];
  }
}

Color Tile::getColor(const ViewObject& object) {
  if (object.hasModifier(ViewObject::Modifier::LOCKED))
    return colors[ColorId::PURPLE];
  if (object.hasModifier(ViewObject::Modifier::INVISIBLE))
    return colors[ColorId::DARK_GRAY];
  if (object.hasModifier(ViewObject::Modifier::HIDDEN))
    return colors[ColorId::LIGHT_GRAY];
  Color color = symbols[object.id()]->color;
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

Tile Tile::addCorner(DirSet corner, DirSet borders, TileCoord coord) {
  for (DirSet dirs : Range(0, 255))
    if ((dirs & corner) == borders)
      corners[dirs].push_back(coord);
  return *this;
}

bool Tile::hasCorners() const {
  return !corners.empty();
}

const vector<Tile::TileCoord>& Tile::getCornerCoords(DirSet c) const {
  return corners[c];
}

