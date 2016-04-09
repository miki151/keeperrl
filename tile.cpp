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
#include "view_id.h"

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

static EnumMap<ViewId, optional<Tile>> tiles;
static EnumMap<ViewId, optional<Tile>> symbols;

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
      if (!tiles[id]) {
        Debug() << "ViewId not found: " << EnumInfo<ViewId>::getString(id);
        bad = true;
      }
    CHECK(!bad);
  }

  void loadUnicode() {
    genSymbols();
    bool bad = false;
    for (ViewId id : ENUM_ALL(ViewId))
      if (!symbols[id]) {
        Debug() << "ViewId not found: " << EnumInfo<ViewId>::getString(id);
        bad = true;
      }
    CHECK(!bad);
  }

  Tile::TileCoord byName(const string& s) {
    return renderer.getTileCoord(s);
  }

  Tile sprite(const string& s) {
    return Tile::byCoord(byName(s));
  }

  Tile empty() {
    return sprite("empty");
  }

  Tile getRoadTile(const string& prefix) {
    return sprite(prefix)
      .addConnection({Dir::E, Dir::W}, byName(prefix + "ew"))
      .addConnection({Dir::W}, byName(prefix + "w"))
      .addConnection({Dir::E}, byName(prefix + "e"))
      .addConnection({Dir::S}, byName(prefix + "s"))
      .addConnection({Dir::N, Dir::S}, byName(prefix + "ns"))
      .addConnection({Dir::N}, byName(prefix + "n"))
      .addConnection({Dir::S, Dir::E}, byName(prefix + "se"))
      .addConnection({Dir::S, Dir::W}, byName(prefix + "sw"))
      .addConnection({Dir::N, Dir::E}, byName(prefix + "ne"))
      .addConnection({Dir::N, Dir::W}, byName(prefix + "nw"))
      .addConnection({Dir::N, Dir::E, Dir::S, Dir::W}, byName(prefix + "nesw"))
      .addConnection({Dir::E, Dir::S, Dir::W}, byName(prefix + "esw"))
      .addConnection({Dir::N, Dir::S, Dir::W}, byName(prefix + "nsw"))
      .addConnection({Dir::N, Dir::E, Dir::S}, byName(prefix + "nes"))
      .addConnection({Dir::N, Dir::E, Dir::W}, byName(prefix + "new"));
  }

  Tile getWallTile(const string& prefix) {
    int tex = 1;
    return sprite(prefix).setNoShadow()
      .addConnection({Dir::E}, byName(prefix + "e"))
      .addConnection({Dir::E, Dir::W}, byName(prefix + "ew"))
      .addConnection({Dir::W}, byName(prefix + "w"))
      .addConnection({Dir::S}, byName(prefix + "s"))
      .addConnection({Dir::N, Dir::S}, byName(prefix + "ns"))
      .addConnection({Dir::N}, byName(prefix + "n"))
      .addConnection({Dir::E, Dir::S}, byName(prefix + "es"))
      .addConnection({Dir::S, Dir::W}, byName(prefix + "sw"))
      .addConnection({Dir::N, Dir::E}, byName(prefix + "ne"))
      .addConnection({Dir::N, Dir::W}, byName(prefix + "nw"))
      .addConnection({Dir::N, Dir::E, Dir::S, Dir::W}, byName(prefix + "nesw"))
      .addConnection({Dir::E, Dir::S, Dir::W}, byName(prefix + "esw"))
      .addConnection({Dir::N, Dir::S, Dir::W}, byName(prefix + "nsw"))
      .addConnection({Dir::N, Dir::E, Dir::S}, byName(prefix + "nes"))
      .addConnection({Dir::N, Dir::E, Dir::W}, byName(prefix + "new"));
  }

  Tile getMountainTile(Tile background, const string& prefix) {
    return background
      .addCorner({Dir::S, Dir::W}, {Dir::W}, byName(prefix + "sw_w"))
      .addCorner({Dir::N, Dir::W}, {Dir::W}, byName(prefix + "nw_w"))
      .addCorner({Dir::S, Dir::E}, {Dir::E}, byName(prefix + "se_e"))
      .addCorner({Dir::N, Dir::E}, {Dir::E}, byName(prefix + "ne_e"))
      .addCorner({Dir::N, Dir::W}, {Dir::N}, byName(prefix + "nw_n"))
      .addCorner({Dir::N, Dir::E}, {Dir::N}, byName(prefix + "ne_n"))
      .addCorner({Dir::S, Dir::W}, {Dir::S}, byName(prefix + "sw_s"))
      .addCorner({Dir::S, Dir::E}, {Dir::S}, byName(prefix + "se_s"))
      .addCorner({Dir::N, Dir::W}, {}, byName(prefix + "nw"))
      .addCorner({Dir::N, Dir::E}, {}, byName(prefix + "ne"))
      .addCorner({Dir::S, Dir::W}, {}, byName(prefix + "sw"))
      .addCorner({Dir::S, Dir::E}, {}, byName(prefix + "se"))
      .addCorner({Dir::S, Dir::E, Dir::SE}, {Dir::S, Dir::E}, byName(prefix + "sese_se"))
      .addCorner({Dir::S, Dir::W, Dir::SW}, {Dir::S, Dir::W}, byName(prefix + "swsw_sw"))
      .addCorner({Dir::N, Dir::E, Dir::NE}, {Dir::N, Dir::E}, byName(prefix + "nene_ne"))
      .addCorner({Dir::N, Dir::W, Dir::NW}, {Dir::N, Dir::W}, byName(prefix + "nwnw_nw"));
  }

  Tile getWaterTile(const string& background, const string& prefix) {
    return empty().addBackground(byName(background))
      .addConnection({Dir::N, Dir::E, Dir::S, Dir::W}, byName("empty"))
      .addConnection({Dir::E, Dir::S, Dir::W}, byName(prefix + "esw"))
      .addConnection({Dir::N, Dir::E, Dir::W}, byName(prefix + "new"))
      .addConnection({Dir::N, Dir::S, Dir::W}, byName(prefix + "nsw"))
      .addConnection({Dir::N, Dir::E, Dir::S}, byName(prefix + "nes"))
      .addConnection({Dir::N, Dir::E}, byName(prefix + "ne"))
      .addConnection({Dir::E, Dir::S}, byName(prefix + "es"))
      .addConnection({Dir::S, Dir::W}, byName(prefix + "sw"))
      .addConnection({Dir::N, Dir::W}, byName(prefix + "nw"))
      .addConnection({Dir::S}, byName(prefix + "s"))
      .addConnection({Dir::N}, byName(prefix + "n"))
      .addConnection({Dir::W}, byName(prefix + "w"))
      .addConnection({Dir::E}, byName(prefix + "e"))
      .addConnection({Dir::N, Dir::S}, byName(prefix + "ns"))
      .addConnection({Dir::E, Dir::W}, byName(prefix + "ew"));
  }

  Tile getExtraBorderTile(const string& prefix) {
    return sprite(prefix)
      .addExtraBorder({Dir::W, Dir::N}, byName(prefix + "wn"))
      .addExtraBorder({Dir::E, Dir::N}, byName(prefix + "en"))
      .addExtraBorder({Dir::E, Dir::S}, byName(prefix + "es"))
      .addExtraBorder({Dir::W, Dir::S}, byName(prefix + "ws"))
      .addExtraBorder({Dir::W, Dir::N, Dir::E}, byName(prefix + "wne"))
      .addExtraBorder({Dir::S, Dir::N, Dir::E}, byName(prefix + "sne"))
      .addExtraBorder({Dir::S, Dir::W, Dir::E}, byName(prefix + "swe"))
      .addExtraBorder({Dir::S, Dir::W, Dir::N}, byName(prefix + "swn"))
      .addExtraBorder({Dir::S, Dir::W, Dir::N, Dir::E}, byName(prefix + "swne"))
      .addExtraBorder({Dir::N}, byName(prefix + "n"))
      .addExtraBorder({Dir::E}, byName(prefix + "e"))
      .addExtraBorder({Dir::S}, byName(prefix + "s"))
      .addExtraBorder({Dir::W}, byName(prefix + "w"));
  }

  void genTiles() {
    Tile::addTile(ViewId::UNKNOWN_MONSTER, sprite("unknown"));
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
    Tile::addTile(ViewId::PLAYER, sprite("adventurer"));
    Tile::addTile(ViewId::KEEPER, sprite("keeper"));
    Tile::addTile(ViewId::RETIRED_KEEPER, sprite("retired_keeper"));
    Tile::addTile(ViewId::ELF, sprite("elf male"));
    Tile::addTile(ViewId::ELF_WOMAN, sprite("elf female"));
    Tile::addTile(ViewId::ELF_ARCHER, sprite("elf archer"));
    Tile::addTile(ViewId::ELF_CHILD, sprite("elf kid"));
    Tile::addTile(ViewId::ELF_LORD, sprite("elf lord"));
    Tile::addTile(ViewId::DARK_ELF, sprite("dark_elf"));
    Tile::addTile(ViewId::DARK_ELF_WOMAN, sprite("dark_elf_female"));
    Tile::addTile(ViewId::DARK_ELF_WARRIOR, sprite("dark_elf_warrior"));
    Tile::addTile(ViewId::DARK_ELF_CHILD, sprite("dark_elf_kid"));
    Tile::addTile(ViewId::DARK_ELF_LORD, sprite("dark_elf_lord"));
    Tile::addTile(ViewId::DRIAD, sprite("driad"));
    Tile::addTile(ViewId::KOBOLD, sprite("kobold"));
    Tile::addTile(ViewId::LIZARDMAN, sprite("lizardman"));
    Tile::addTile(ViewId::LIZARDLORD, sprite("lizardlord"));
    Tile::addTile(ViewId::IMP, sprite("imp"));
    Tile::addTile(ViewId::PRISONER, sprite("prisoner"));
    Tile::addTile(ViewId::OGRE, sprite("troll"));
    Tile::addTile(ViewId::CHICKEN, sprite("hen"));
    Tile::addTile(ViewId::GNOME, sprite("gnome"));
    Tile::addTile(ViewId::GNOME_BOSS, sprite("gnomeboss"));
    Tile::addTile(ViewId::DWARF, sprite("dwarf"));
    Tile::addTile(ViewId::DWARF_BARON, sprite("dwarfboss"));
    Tile::addTile(ViewId::DWARF_FEMALE, sprite("dwarf_f"));
    Tile::addTile(ViewId::SHOPKEEPER, sprite("salesman"));
    Tile::addTile(ViewId::BRIDGE, sprite("bridge").addOption(Dir::S, byName("bridge2")));
    Tile::addTile(ViewId::ROAD, getRoadTile("road"));
    Tile::addTile(ViewId::FLOOR, sprite("floor"));
    Tile::addTile(ViewId::KEEPER_FLOOR, sprite("floor_keeper"));
    Tile::addTile(ViewId::SAND, getExtraBorderTile("sand")
        .addExtraBorderId(ViewId::WATER));
    Tile::addTile(ViewId::MUD, getExtraBorderTile("mud")
        .addExtraBorderId(ViewId::WATER)
        .addExtraBorderId(ViewId::HILL)
        .addExtraBorderId(ViewId::SAND));
    Tile::addTile(ViewId::GRASS, getExtraBorderTile("grass")
        .addExtraBorderId(ViewId::SAND)
        .addExtraBorderId(ViewId::HILL)
        .addExtraBorderId(ViewId::MUD)
        .addExtraBorderId(ViewId::WATER));
    Tile::addTile(ViewId::CROPS, sprite("wheatfield1"));
    Tile::addTile(ViewId::CROPS2, sprite("wheatfield2"));
    Tile::addTile(ViewId::MOUNTAIN, getMountainTile(sprite("mountain_ted").setNoShadow(), "mountain"));
    Tile::addTile(ViewId::WALL, getWallTile("wall"));
    Tile::addTile(ViewId::MAP_MOUNTAIN1, sprite("map_mountain1"));
    Tile::addTile(ViewId::MAP_MOUNTAIN2, sprite("map_mountain2"));
    Tile::addTile(ViewId::MAP_MOUNTAIN3, sprite("map_mountain3"));
    Tile::addTile(ViewId::GOLD_ORE, getMountainTile(sprite("gold_ore").addBackground(byName("mountain_ted"))
          .setNoShadow(), "mountain"));
    Tile::addTile(ViewId::IRON_ORE, getMountainTile(sprite("iron_ore").addBackground(byName("mountain_ted"))
          .setNoShadow(), "mountain"));
    Tile::addTile(ViewId::STONE, getMountainTile(sprite("stone_ore").addBackground(byName("mountain_ted"))
          .setNoShadow(), "mountain"));
    Tile::addTile(ViewId::HILL, getExtraBorderTile("hill")
        .addExtraBorderId(ViewId::SAND)
        .addExtraBorderId(ViewId::WATER));
    Tile::addTile(ViewId::WOOD_WALL, getWallTile("wood_wall"));
    Tile::addTile(ViewId::BLACK_WALL, getWallTile("wall"));
    Tile::addTile(ViewId::CASTLE_WALL, getWallTile("castle_wall"));
    Tile::addTile(ViewId::MUD_WALL, getWallTile("mud_wall"));
    Tile::addTile(ViewId::DOWN_STAIRCASE, sprite("down_stairs").setNoShadow());
    Tile::addTile(ViewId::UP_STAIRCASE, sprite("up_stairs").setNoShadow());
    Tile::addTile(ViewId::WELL, sprite("well").setNoShadow());
    Tile::addTile(ViewId::MINION_STATUE, sprite("statue").setNoShadow());
    Tile::addTile(ViewId::THRONE, sprite("throne").setNoShadow());
    Tile::addTile(ViewId::ORC, sprite("orc"));
    Tile::addTile(ViewId::ORC_CAPTAIN, sprite("orc_captain"));
    Tile::addTile(ViewId::ORC_SHAMAN, sprite("orcshaman"));
    Tile::addTile(ViewId::HARPY, sprite("harpy"));
    Tile::addTile(ViewId::DOPPLEGANGER, sprite("dopple"));
    Tile::addTile(ViewId::SUCCUBUS, sprite("succubussmall"));
    Tile::addTile(ViewId::BANDIT, sprite("bandit"));
    Tile::addTile(ViewId::GHOST, sprite("ghost4"));
    Tile::addTile(ViewId::SPIRIT, sprite("spirit"));
    Tile::addTile(ViewId::GREEN_DRAGON, sprite("greendragon"));
    Tile::addTile(ViewId::RED_DRAGON, sprite("reddragon"));
    Tile::addTile(ViewId::CYCLOPS, sprite("cyclops"));
    Tile::addTile(ViewId::MINOTAUR, sprite("mino"));
    Tile::addTile(ViewId::HYDRA, sprite("hydra"));
    Tile::addTile(ViewId::SHELOB, sprite("szelob"));
    Tile::addTile(ViewId::WITCH, sprite("witch"));
    Tile::addTile(ViewId::WITCHMAN, sprite("witchman"));
    Tile::addTile(ViewId::KNIGHT, sprite("knight"));
    Tile::addTile(ViewId::WARRIOR, sprite("warrior"));
    Tile::addTile(ViewId::SHAMAN, sprite("shaman"));
    Tile::addTile(ViewId::CASTLE_GUARD, sprite("knight"));
    Tile::addTile(ViewId::AVATAR, sprite("knightboss"));
    Tile::addTile(ViewId::ARCHER, sprite("archer"));
    Tile::addTile(ViewId::PESEANT, sprite("peasant"));
    Tile::addTile(ViewId::PESEANT_WOMAN, sprite("peasantgirl"));
    Tile::addTile(ViewId::CHILD, sprite("peasantkid"));
    Tile::addTile(ViewId::CLAY_GOLEM, sprite("clay_golem"));
    Tile::addTile(ViewId::STONE_GOLEM, sprite("stone_golem"));
    Tile::addTile(ViewId::IRON_GOLEM, sprite("iron_golem"));
    Tile::addTile(ViewId::LAVA_GOLEM, sprite("lava_golem"));
    Tile::addTile(ViewId::AUTOMATON, sprite("automaton"));
    Tile::addTile(ViewId::ELEMENTALIST, sprite("elementalist"));
    Tile::addTile(ViewId::AIR_ELEMENTAL, sprite("airelement"));
    Tile::addTile(ViewId::FIRE_ELEMENTAL, sprite("fireelement"));
    Tile::addTile(ViewId::WATER_ELEMENTAL, sprite("waterelement"));
    Tile::addTile(ViewId::EARTH_ELEMENTAL, sprite("earthelement"));
    Tile::addTile(ViewId::ENT, sprite("ent"));
    Tile::addTile(ViewId::ANGEL, sprite("ent"));
    Tile::addTile(ViewId::ZOMBIE, sprite("zombie"));
    Tile::addTile(ViewId::SKELETON, sprite("skeleton"));
    Tile::addTile(ViewId::VAMPIRE_LORD, sprite("vampirelord"));
    Tile::addTile(ViewId::MUMMY, sprite("mummy"));
    Tile::addTile(ViewId::JACKAL, sprite("jackal"));
    Tile::addTile(ViewId::DEER, sprite("deer"));
    Tile::addTile(ViewId::HORSE, sprite("horse"));
    Tile::addTile(ViewId::COW, sprite("cow"));
    Tile::addTile(ViewId::PIG, sprite("pig"));
    Tile::addTile(ViewId::DONKEY, sprite("donkey"));
    Tile::addTile(ViewId::GOAT, sprite("goat"));
    Tile::addTile(ViewId::BOAR, sprite("boar"));
    Tile::addTile(ViewId::FOX, sprite("fox"));
    Tile::addTile(ViewId::WOLF, sprite("wolf"));
    Tile::addTile(ViewId::WEREWOLF, sprite("werewolf2"));
    Tile::addTile(ViewId::DOG, sprite("dog"));
    Tile::addTile(ViewId::KRAKEN_HEAD, sprite("krakenhead"));
    Tile::addTile(ViewId::KRAKEN_LAND, sprite("krakenland1"));
    Tile::addTile(ViewId::KRAKEN_WATER, sprite("krakenwater2"));
    Tile::addTile(ViewId::DEATH, sprite("death"));
    Tile::addTile(ViewId::FIRE_SPHERE, sprite("fire_sphere"));
    Tile::addTile(ViewId::BEAR, sprite("bear"));
    Tile::addTile(ViewId::BAT, sprite("bat"));
    Tile::addTile(ViewId::GOBLIN, sprite("goblin"));
    Tile::addTile(ViewId::LEPRECHAUN, sprite("leprechaun"));
    Tile::addTile(ViewId::RAT, sprite("rat"));
    Tile::addTile(ViewId::SPIDER, sprite("spider"));
    Tile::addTile(ViewId::FLY, sprite("fly"));
    Tile::addTile(ViewId::ANT_WORKER, sprite("antwork"));
    Tile::addTile(ViewId::ANT_SOLDIER, sprite("antw"));
    Tile::addTile(ViewId::ANT_QUEEN, sprite("antq"));
    Tile::addTile(ViewId::SNAKE, sprite("snake"));
    Tile::addTile(ViewId::VULTURE, sprite("vulture"));
    Tile::addTile(ViewId::RAVEN, sprite("raven"));
    Tile::addTile(ViewId::BODY_PART, sprite("corpse4"));
    Tile::addTile(ViewId::BONE, sprite("bone"));
    Tile::addTile(ViewId::BUSH, sprite("bush").setNoShadow().addHighlight(byName("bush_mark")));
    Tile::addTile(ViewId::WATER, getWaterTile("waternesw", "water"));
    Tile::addTile(ViewId::MAGMA, getWaterTile("magmanesw", "magma"));
    Tile::addTile(ViewId::DOOR, sprite("door").setNoShadow());
    Tile::addTile(ViewId::BARRICADE, sprite("barricade").setNoShadow());
    Tile::addTile(ViewId::DIG_ICON, sprite("dig_icon"));
    Tile::addTile(ViewId::SWORD, sprite("sword"));
    Tile::addTile(ViewId::SPEAR, sprite("spear"));
    Tile::addTile(ViewId::SPECIAL_SWORD, sprite("special_sword"));
    Tile::addTile(ViewId::ELVEN_SWORD, sprite("elven_sword"));
    Tile::addTile(ViewId::KNIFE, sprite("knife"));
    Tile::addTile(ViewId::WAR_HAMMER, sprite("warhammer"));
    Tile::addTile(ViewId::SPECIAL_WAR_HAMMER, sprite("special_warhammer"));
    Tile::addTile(ViewId::BATTLE_AXE, sprite("battle_axe"));
    Tile::addTile(ViewId::SPECIAL_BATTLE_AXE, sprite("special_battle_axe"));
    Tile::addTile(ViewId::BOW, sprite("bow"));
    Tile::addTile(ViewId::ARROW, sprite("arrow"));
    Tile::addTile(ViewId::CLUB, sprite("club"));
    Tile::addTile(ViewId::HEAVY_CLUB, sprite("heavy club"));
    Tile::addTile(ViewId::SCROLL, sprite("scroll"));
    Tile::addTile(ViewId::AMULET1, sprite("amulet1"));
    Tile::addTile(ViewId::AMULET2, sprite("amulet2"));
    Tile::addTile(ViewId::AMULET3, sprite("amulet3"));
    Tile::addTile(ViewId::AMULET4, sprite("amulet4"));
    Tile::addTile(ViewId::AMULET5, sprite("amulet5"));
    Tile::addTile(ViewId::FIRE_RESIST_RING, sprite("ring_red"));
    Tile::addTile(ViewId::POISON_RESIST_RING, sprite("ring_green"));
    Tile::addTile(ViewId::BOOK, sprite("book"));
    Tile::addTile(ViewId::FIRST_AID, sprite("medkit"));
    Tile::addTile(ViewId::TRAP_ITEM, sprite("trapbox"));
    Tile::addTile(ViewId::POTION1, sprite("potion1"));
    Tile::addTile(ViewId::POTION2, sprite("potion2"));
    Tile::addTile(ViewId::POTION3, sprite("potion3"));
    Tile::addTile(ViewId::POTION4, sprite("potion4"));
    Tile::addTile(ViewId::POTION5, sprite("potion5"));
    Tile::addTile(ViewId::POTION6, sprite("potion6"));
    Tile::addTile(ViewId::MUSHROOM, sprite("mushroom1"));
    Tile::addTile(ViewId::KEY, sprite("key"));
    Tile::addTile(ViewId::FOUNTAIN, sprite("fountain").setNoShadow());
    Tile::addTile(ViewId::GOLD, sprite("gold").setNoShadow());
    Tile::addTile(ViewId::TREASURE_CHEST, sprite("treasurydeco").setNoShadow().addBackground(byName("treasury")));
    Tile::addTile(ViewId::CHEST, sprite("chest").setNoShadow());
    Tile::addTile(ViewId::OPENED_CHEST, sprite("chest_opened").setNoShadow());
    Tile::addTile(ViewId::COFFIN, sprite("coffin").setNoShadow());
    Tile::addTile(ViewId::OPENED_COFFIN, sprite("coffin_opened").setNoShadow());
    Tile::addTile(ViewId::BOULDER, sprite("boulder"));
    Tile::addTile(ViewId::PORTAL, sprite("surprise"));
    Tile::addTile(ViewId::GAS_TRAP, sprite("gas_trap"));
    Tile::addTile(ViewId::ALARM_TRAP, sprite("alarm_trap"));
    Tile::addTile(ViewId::WEB_TRAP, sprite("web_trap"));
    Tile::addTile(ViewId::SURPRISE_TRAP, sprite("surprisetrap"));
    Tile::addTile(ViewId::TERROR_TRAP, sprite("terror_trap"));
    Tile::addTile(ViewId::ROCK, sprite("stonepile"));
    Tile::addTile(ViewId::IRON_ROCK, sprite("ironpile"));
    Tile::addTile(ViewId::WOOD_PLANK, sprite("wood2"));
    Tile::addTile(ViewId::STOCKPILE1, sprite("storage1").setFloorBorders());
    Tile::addTile(ViewId::STOCKPILE2, sprite("storage2").setFloorBorders());
    Tile::addTile(ViewId::STOCKPILE3, sprite("storage3").setFloorBorders());
    Tile::addTile(ViewId::PRISON, sprite("prison"));
    Tile::addTile(ViewId::BED, sprite("sleepdeco").setNoShadow());
    Tile::addTile(ViewId::DORM, sprite("sleep").setFloorBorders());
    Tile::addTile(ViewId::TORCH, sprite("torch").setNoShadow().setTranslucent(0.35));
    Tile::addTile(ViewId::ALTAR, sprite("altar").setNoShadow());
    Tile::addTile(ViewId::CREATURE_ALTAR, sprite("altar2").setNoShadow());
    Tile::addTile(ViewId::TORTURE_TABLE, empty().addConnection(DirSet::fullSet(), byName("torturedeco"))
        .addBackground(byName("torture"))
        .setFloorBorders());
    Tile::addTile(ViewId::IMPALED_HEAD, sprite("impaledhead").setNoShadow());
    Tile::addTile(ViewId::WHIPPING_POST, sprite("whipping_post").setNoShadow());
    Tile::addTile(ViewId::NOTICE_BOARD, sprite("board").setNoShadow());
    Tile::addTile(ViewId::SOKOBAN_HOLE, sprite("hole").setNoShadow());
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
    Tile::addTile(ViewId::GRAVE, sprite("RIP").setNoShadow());
    Tile::addTile(ViewId::ROBE, sprite("robe"));
    Tile::addTile(ViewId::LEATHER_GLOVES, sprite("leather_gloves"));
    Tile::addTile(ViewId::DEXTERITY_GLOVES, sprite("blue_gloves"));
    Tile::addTile(ViewId::STRENGTH_GLOVES, sprite("red_gloves"));
    Tile::addTile(ViewId::LEATHER_ARMOR, sprite("leather_armor"));
    Tile::addTile(ViewId::LEATHER_HELM, sprite("leather_helm"));
    Tile::addTile(ViewId::TELEPATHY_HELM, sprite("blue_helm"));
    Tile::addTile(ViewId::CHAIN_ARMOR, sprite("chain_armor"));
    Tile::addTile(ViewId::IRON_HELM, sprite("iron_helm"));
    Tile::addTile(ViewId::LEATHER_BOOTS, sprite("leather_boots"));
    Tile::addTile(ViewId::IRON_BOOTS, sprite("iron_boots"));
    Tile::addTile(ViewId::SPEED_BOOTS, sprite("blue_boots"));
    Tile::addTile(ViewId::LEVITATION_BOOTS, sprite("green_boots"));
    Tile::addTile(ViewId::GUARD_POST, sprite("guardroom"));
    Tile::addTile(ViewId::MANA, sprite("mana"));
    Tile::addTile(ViewId::FETCH_ICON, sprite("leather_gloves"));
    Tile::addTile(ViewId::EYEBALL, sprite("eyeball2").setNoShadow());
    Tile::addTile(ViewId::FOG_OF_WAR, getWaterTile("empty", "fogofwar"));
    Tile::addTile(ViewId::CREATURE_HIGHLIGHT, sprite("creature_highlight"));
    Tile::addTile(ViewId::SQUARE_HIGHLIGHT, sprite("square_highlight"));
    Tile::addTile(ViewId::ROUND_SHADOW, sprite("round_shadow"));
    Tile::addTile(ViewId::FOG_OF_WAR_CORNER, sprite("fogofwar")
        .addConnection({Dir::NE}, byName("fogofwarcornne"))
        .addConnection({Dir::NW}, byName("fogofwarcornnw"))
        .addConnection({Dir::SE}, byName("fogofwarcornse"))
        .addConnection({Dir::SW}, byName("fogofwarcornsw")));
    Tile::addTile(ViewId::SPECIAL_BLBN, sprite("special_blbn"));
    Tile::addTile(ViewId::SPECIAL_BLBW, sprite("special_blbw"));
    Tile::addTile(ViewId::SPECIAL_BLGN, sprite("special_blgn"));
    Tile::addTile(ViewId::SPECIAL_BLGW, sprite("special_blgw"));
    Tile::addTile(ViewId::SPECIAL_BMBN, sprite("special_bmbn"));
    Tile::addTile(ViewId::SPECIAL_BMBW, sprite("special_bmbw"));
    Tile::addTile(ViewId::SPECIAL_BMGN, sprite("special_bmgn"));
    Tile::addTile(ViewId::SPECIAL_BMGW, sprite("special_bmgw"));
    Tile::addTile(ViewId::SPECIAL_HLBN, sprite("special_hlbn"));
    Tile::addTile(ViewId::SPECIAL_HLBW, sprite("special_hlbw"));
    Tile::addTile(ViewId::SPECIAL_HLGN, sprite("special_hlgn"));
    Tile::addTile(ViewId::SPECIAL_HLGW, sprite("special_hlgw"));
    Tile::addTile(ViewId::SPECIAL_HMBN, sprite("special_hmbn"));
    Tile::addTile(ViewId::SPECIAL_HMBW, sprite("special_hmbw"));
    Tile::addTile(ViewId::SPECIAL_HMGN, sprite("special_hmgn"));
    Tile::addTile(ViewId::SPECIAL_HMGW, sprite("special_hmgw"));
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
    Tile::addSymbol(ViewId::RETIRED_KEEPER, symbol("@", ColorId::BLUE));
    Tile::addSymbol(ViewId::UNKNOWN_MONSTER, symbol("?", ColorId::LIGHT_GREEN));
    Tile::addSymbol(ViewId::ELF, symbol("@", ColorId::LIGHT_GREEN));
    Tile::addSymbol(ViewId::ELF_WOMAN, symbol("@", ColorId::LIGHT_GREEN));
    Tile::addSymbol(ViewId::ELF_ARCHER, symbol("@", ColorId::GREEN));
    Tile::addSymbol(ViewId::ELF_CHILD, symbol("@", ColorId::LIGHT_GREEN));
    Tile::addSymbol(ViewId::ELF_LORD, symbol("@", ColorId::DARK_GREEN));
    Tile::addSymbol(ViewId::DARK_ELF, symbol("@", ColorId::ALMOST_DARK_GRAY));
    Tile::addSymbol(ViewId::DARK_ELF_WOMAN, symbol("@", ColorId::ALMOST_DARK_GRAY));
    Tile::addSymbol(ViewId::DARK_ELF_WARRIOR, symbol("@", ColorId::GRAY));
    Tile::addSymbol(ViewId::DARK_ELF_CHILD, symbol("@", ColorId::ALMOST_GRAY));
    Tile::addSymbol(ViewId::DARK_ELF_LORD, symbol("@", ColorId::LIGHT_GRAY));
    Tile::addSymbol(ViewId::DRIAD, symbol("@", ColorId::LIGHT_BROWN));
    Tile::addSymbol(ViewId::KOBOLD, symbol("k", ColorId::LIGHT_BROWN));
    Tile::addSymbol(ViewId::SHOPKEEPER, symbol("@", ColorId::LIGHT_BLUE));
    Tile::addSymbol(ViewId::LIZARDMAN, symbol("@", ColorId::LIGHT_BROWN));
    Tile::addSymbol(ViewId::LIZARDLORD, symbol("@", ColorId::BROWN));
    Tile::addSymbol(ViewId::IMP, symbol("i", ColorId::LIGHT_BROWN));
    Tile::addSymbol(ViewId::PRISONER, symbol("@", ColorId::LIGHT_BROWN));
    Tile::addSymbol(ViewId::OGRE, symbol("O", ColorId::GREEN));
    Tile::addSymbol(ViewId::CHICKEN, symbol("c", ColorId::YELLOW));
    Tile::addSymbol(ViewId::GNOME, symbol("g", ColorId::GREEN));
    Tile::addSymbol(ViewId::GNOME_BOSS, symbol("g", ColorId::DARK_BLUE));
    Tile::addSymbol(ViewId::DWARF, symbol("h", ColorId::BLUE));
    Tile::addSymbol(ViewId::DWARF_BARON, symbol("h", ColorId::DARK_BLUE));
    Tile::addSymbol(ViewId::DWARF_FEMALE, symbol("h", ColorId::LIGHT_BLUE));
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
    Tile::addSymbol(ViewId::MOUNTAIN, symbol("#", ColorId::DARK_GRAY));
    Tile::addSymbol(ViewId::MAP_MOUNTAIN1, symbol("^", ColorId::DARK_GRAY));
    Tile::addSymbol(ViewId::MAP_MOUNTAIN2, symbol("^", ColorId::GRAY));
    Tile::addSymbol(ViewId::MAP_MOUNTAIN3, symbol("^", ColorId::LIGHT_GRAY));
    Tile::addSymbol(ViewId::GOLD_ORE, symbol("⁂", ColorId::YELLOW, true));
    Tile::addSymbol(ViewId::IRON_ORE, symbol("⁂", ColorId::DARK_BROWN, true));
    Tile::addSymbol(ViewId::STONE, symbol("⁂", ColorId::LIGHT_GRAY, true));
    Tile::addSymbol(ViewId::HILL, symbol(0x1d022, ColorId::DARK_GREEN, true));
    Tile::addSymbol(ViewId::WOOD_WALL, symbol("#", ColorId::DARK_BROWN));
    Tile::addSymbol(ViewId::BLACK_WALL, symbol("#", ColorId::LIGHT_GRAY));
    Tile::addSymbol(ViewId::DOWN_STAIRCASE, symbol(0x2798, ColorId::ALMOST_WHITE, true));
    Tile::addSymbol(ViewId::UP_STAIRCASE, symbol(0x279a, ColorId::ALMOST_WHITE, true));
    Tile::addSymbol(ViewId::WELL, symbol("0", ColorId::BLUE));
    Tile::addSymbol(ViewId::MINION_STATUE, symbol("&", ColorId::LIGHT_GRAY));
    Tile::addSymbol(ViewId::THRONE, symbol("Ω", ColorId::YELLOW));
    Tile::addSymbol(ViewId::ORC, symbol("o", ColorId::DARK_BLUE));
    Tile::addSymbol(ViewId::ORC_CAPTAIN, symbol("o", ColorId::PURPLE));
    Tile::addSymbol(ViewId::ORC_SHAMAN, symbol("o", ColorId::YELLOW));
    Tile::addSymbol(ViewId::HARPY, symbol("R", ColorId::YELLOW));
    Tile::addSymbol(ViewId::DOPPLEGANGER, symbol("&", ColorId::YELLOW));
    Tile::addSymbol(ViewId::SUCCUBUS, symbol("&", ColorId::RED));
    Tile::addSymbol(ViewId::BANDIT, symbol("@", ColorId::DARK_BLUE));
    Tile::addSymbol(ViewId::GREEN_DRAGON, symbol("D", ColorId::GREEN));
    Tile::addSymbol(ViewId::RED_DRAGON, symbol("D", ColorId::RED));
    Tile::addSymbol(ViewId::CYCLOPS, symbol("C", ColorId::GREEN));
    Tile::addSymbol(ViewId::MINOTAUR, symbol("C", ColorId::PURPLE));
    Tile::addSymbol(ViewId::HYDRA, symbol("H", ColorId::PURPLE));
    Tile::addSymbol(ViewId::SHELOB, symbol("S", ColorId::PURPLE));
    Tile::addSymbol(ViewId::WITCH, symbol("@", ColorId::BROWN));
    Tile::addSymbol(ViewId::WITCHMAN, symbol("@", ColorId::WHITE));
    Tile::addSymbol(ViewId::GHOST, symbol("&", ColorId::WHITE));
    Tile::addSymbol(ViewId::SPIRIT, symbol("&", ColorId::LIGHT_BLUE));
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
    Tile::addSymbol(ViewId::AUTOMATON, symbol("A", ColorId::LIGHT_GRAY));
    Tile::addSymbol(ViewId::ELEMENTALIST, symbol("@", ColorId::YELLOW));
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
    Tile::addSymbol(ViewId::JACKAL, symbol("d", ColorId::LIGHT_BROWN));
    Tile::addSymbol(ViewId::DEER, symbol("R", ColorId::DARK_BROWN));
    Tile::addSymbol(ViewId::HORSE, symbol("H", ColorId::LIGHT_BROWN));
    Tile::addSymbol(ViewId::COW, symbol("C", ColorId::WHITE));
    Tile::addSymbol(ViewId::PIG, symbol("p", ColorId::YELLOW));
    Tile::addSymbol(ViewId::DONKEY, symbol("D", ColorId::GRAY));
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
    Tile::addSymbol(ViewId::FIRE_SPHERE, symbol("e", ColorId::RED));
    Tile::addSymbol(ViewId::BEAR, symbol("N", ColorId::BROWN));
    Tile::addSymbol(ViewId::BAT, symbol("b", ColorId::DARK_GRAY));
    Tile::addSymbol(ViewId::GOBLIN, symbol("o", ColorId::GREEN));
    Tile::addSymbol(ViewId::LEPRECHAUN, symbol("l", ColorId::GREEN));
    Tile::addSymbol(ViewId::RAT, symbol("r", ColorId::BROWN));
    Tile::addSymbol(ViewId::SPIDER, symbol("s", ColorId::BROWN));
    Tile::addSymbol(ViewId::ANT_WORKER, symbol("a", ColorId::BROWN));
    Tile::addSymbol(ViewId::ANT_SOLDIER, symbol("a", ColorId::YELLOW));
    Tile::addSymbol(ViewId::ANT_QUEEN, symbol("a", ColorId::PURPLE));
    Tile::addSymbol(ViewId::FLY, symbol("b", ColorId::GRAY));
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
    Tile::addSymbol(ViewId::AMULET1, symbol("\"", ColorId::YELLOW));
    Tile::addSymbol(ViewId::AMULET2, symbol("\"", ColorId::YELLOW));
    Tile::addSymbol(ViewId::AMULET3, symbol("\"", ColorId::YELLOW));
    Tile::addSymbol(ViewId::AMULET4, symbol("\"", ColorId::YELLOW));
    Tile::addSymbol(ViewId::AMULET5, symbol("\"", ColorId::YELLOW));
    Tile::addSymbol(ViewId::FIRE_RESIST_RING, symbol("=", ColorId::RED));
    Tile::addSymbol(ViewId::POISON_RESIST_RING, symbol("=", ColorId::GREEN));
    Tile::addSymbol(ViewId::BOOK, symbol("+", ColorId::YELLOW));
    Tile::addSymbol(ViewId::FIRST_AID, symbol("+", ColorId::RED));
    Tile::addSymbol(ViewId::TRAP_ITEM, symbol("+", ColorId::YELLOW));
    Tile::addSymbol(ViewId::POTION1, symbol("!", ColorId::LIGHT_RED));
    Tile::addSymbol(ViewId::POTION2, symbol("!", ColorId::BLUE));
    Tile::addSymbol(ViewId::POTION3, symbol("!", ColorId::YELLOW));
    Tile::addSymbol(ViewId::POTION4, symbol("!", ColorId::VIOLET));
    Tile::addSymbol(ViewId::POTION5, symbol("!", ColorId::DARK_BROWN));
    Tile::addSymbol(ViewId::POTION6, symbol("!", ColorId::LIGHT_GRAY));
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
    Tile::addSymbol(ViewId::TORCH, symbol("*", ColorId::YELLOW));
    Tile::addSymbol(ViewId::ALTAR, symbol("Ω", ColorId::WHITE, true));
    Tile::addSymbol(ViewId::CREATURE_ALTAR, symbol("Ω", ColorId::YELLOW, true));
    Tile::addSymbol(ViewId::TORTURE_TABLE, symbol("=", ColorId::GRAY));
    Tile::addSymbol(ViewId::IMPALED_HEAD, symbol("⚲", ColorId::BROWN, true));
    Tile::addSymbol(ViewId::WHIPPING_POST, symbol("}", ColorId::BROWN, true));
    Tile::addSymbol(ViewId::NOTICE_BOARD, symbol("|", ColorId::BROWN));
    Tile::addSymbol(ViewId::SOKOBAN_HOLE, symbol("0", ColorId::DARK_BLUE));
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
    Tile::addSymbol(ViewId::FETCH_ICON, symbol(0x1f44b, ColorId::LIGHT_BROWN, true));
    Tile::addSymbol(ViewId::FOG_OF_WAR, symbol(' ', ColorId::WHITE));
    Tile::addSymbol(ViewId::CREATURE_HIGHLIGHT, symbol(' ', ColorId::WHITE));
    Tile::addSymbol(ViewId::SQUARE_HIGHLIGHT, symbol(' ', ColorId::WHITE));
    Tile::addSymbol(ViewId::ROUND_SHADOW, symbol(' ', ColorId::WHITE));
    Tile::addSymbol(ViewId::FOG_OF_WAR_CORNER, symbol(' ', ColorId::WHITE));
    Tile::addSymbol(ViewId::SPECIAL_BLBN, symbol('B', ColorId::PURPLE));
    Tile::addSymbol(ViewId::SPECIAL_BLBW, symbol('B', ColorId::LIGHT_RED));
    Tile::addSymbol(ViewId::SPECIAL_BLGN, symbol('B', ColorId::LIGHT_GRAY));
    Tile::addSymbol(ViewId::SPECIAL_BLGW, symbol('B', ColorId::WHITE));
    Tile::addSymbol(ViewId::SPECIAL_BMBN, symbol('B', ColorId::YELLOW));
    Tile::addSymbol(ViewId::SPECIAL_BMBW, symbol('B', ColorId::ORANGE));
    Tile::addSymbol(ViewId::SPECIAL_BMGN, symbol('B', ColorId::GREEN));
    Tile::addSymbol(ViewId::SPECIAL_BMGW, symbol('B', ColorId::LIGHT_GREEN));
    Tile::addSymbol(ViewId::SPECIAL_HLBN, symbol('H', ColorId::PURPLE));
    Tile::addSymbol(ViewId::SPECIAL_HLBW, symbol('H', ColorId::LIGHT_RED));
    Tile::addSymbol(ViewId::SPECIAL_HLGN, symbol('H', ColorId::LIGHT_GRAY));
    Tile::addSymbol(ViewId::SPECIAL_HLGW, symbol('H', ColorId::WHITE));
    Tile::addSymbol(ViewId::SPECIAL_HMBN, symbol('H', ColorId::YELLOW));
    Tile::addSymbol(ViewId::SPECIAL_HMBW, symbol('H', ColorId::ORANGE));
    Tile::addSymbol(ViewId::SPECIAL_HMGN, symbol('H', ColorId::GREEN));
    Tile::addSymbol(ViewId::SPECIAL_HMGW, symbol('H', ColorId::LIGHT_GREEN));
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
  if (sprite && tiles[id]) {
    return *tiles[id];
  }
  else {
    CHECK(symbols[id]);
    return *symbols[id];
  }
}

const Tile& Tile::getTile(ViewId id) {
  if (tiles[id])
    return *tiles[id];
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

