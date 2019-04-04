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
#include "view_object.h"

Tile::Tile() {
}

Tile Tile::fromString(const string& ch, Color color, bool symbol) {
  Tile ret;
  ret.color = color;
  ret.text = ch;
  ret.symFont = symbol;
  return ret;
}

Tile::Tile(const vector<TileCoord>& c) : color{255, 255, 255}, tileCoord(c) {
}

Tile Tile::setRoundShadow() {
  roundShadow = true;
  moveUp = true;
  return *this;
}

Tile Tile::setMoveUp() {
  moveUp = true;
  return *this;
}

Tile Tile::setWallShadow() {
  wallShadow = true;
  return *this;
}

Tile Tile::setFX(FXVariantName f) {
  fx = f;
  return *this;
}

Tile Tile::setConnectionId(ViewId id) {
  connectionId = id;
  return *this;
}

Tile Tile::byCoord(const vector<TileCoord>& c) {
  return Tile(c);
}

Tile Tile::setFloorBorders() {
  floorBorders = true;
  return *this;
}

Tile Tile::addConnection(DirSet dirs, vector<TileCoord> coord) {
  connections[dirs] = coord;
  if (dirs & (~connectionsMask))
    connectionsMask = DirSet::fullSet();
  anyConnections = true;
  return *this;
}

Tile Tile::addOption(Dir d, vector<TileCoord> coord) {
  connectionOption = make_pair(d, coord);
  anyConnections = true;
  return *this;
}

Tile Tile::addBackground(vector<TileCoord> coord) {
  backgroundCoord = coord;
  return *this;
}

Tile Tile::setColor(Color col) {
  color = col;
  return *this;
}

Tile Tile::addExtraBorder(DirSet dir, const vector<TileCoord>& coord) {
  extraBorders[dir] = coord;
  return *this;
}

Tile Tile::addExtraBorderId(ViewId id) {
  extraBorderIds.insert(id);
  anyExtraBorders = true;
  return *this;
}

const unordered_set<ViewId, CustomHash<ViewId>>& Tile::getExtraBorderIds() const {
  return extraBorderIds;
}

bool Tile::hasExtraBorders() const {
  return anyExtraBorders;
}

const vector<Tile::TileCoord>& Tile::getExtraBorderCoord(DirSet c) const {
  return extraBorders[c];
}

const optional<ViewId>&Tile::getConnectionId() const {
  return connectionId;
}

Tile Tile::setTranslucent(double v) {
  translucent = v;
  return *this;
}

bool Tile::hasSpriteCoord() const {
  return !tileCoord.empty();
}

const vector<Tile::TileCoord>& Tile::getSpriteCoord() const {
  CHECK(!tileCoord.empty());
  return tileCoord;
}

const vector<Tile::TileCoord>& Tile::getBackgroundCoord() const {
  return backgroundCoord;
}

bool Tile::hasAnyConnections() const {
  return anyConnections;
}

const vector<Tile::TileCoord>& Tile::getSpriteCoord(DirSet c) const {
  if (connectionOption) {
    if (c.has(connectionOption->first))
      return connectionOption->second;
    else {
      CHECK(!tileCoord.empty());
      return tileCoord;
    }
  }
  c = c & connectionsMask;
  if (!connections[c].empty())
    return connections[c];
  else {
    CHECK(!tileCoord.empty());
    return tileCoord;
  }
}

static unordered_map<ViewId, Tile, CustomHash<ViewId>> tiles;
static unordered_map<ViewId, Tile, CustomHash<ViewId>> symbols;

void Tile::addTile(ViewId id, Tile tile) {
  tiles.insert(make_pair(id, std::move(tile)));
}

void Tile::addSymbol(ViewId id, Tile tile) {
  symbols.insert(make_pair(std::move(id), std::move(tile)));
}


class TileCoordLookup {
  public:
  TileCoordLookup(Renderer& r) : renderer(r) {}

  void loadTiles() {
    genTiles1();
    genTiles2();
    genTiles3();
    genTiles4();
    genTiles5();
  }

  void loadUnicode() {
    genSymbols1();
    genSymbols2();
    genSymbols3();
    genSymbols4();
    genSymbols5();
  }

  const vector<Tile::TileCoord>& byName(const string& s) {
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
    return sprite(prefix)
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
      .addConnection({Dir::N, Dir::E, Dir::W}, byName(prefix + "new"))
      .setConnectionId(ViewId("wall"));
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
      .addCorner({Dir::N, Dir::W, Dir::NW}, {Dir::N, Dir::W}, byName(prefix + "nwnw_nw"))
      .setConnectionId(ViewId("mountain"));
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
      .addConnection({Dir::E, Dir::W}, byName(prefix + "ew"))
      .addConnection({}, byName(prefix));
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

  void genTiles1() {
    Tile::addTile(ViewId("unknown_monster"), sprite("unknown"));
    Tile::addTile(ViewId("dig_mark"), sprite("dig_mark"));
    Tile::addTile(ViewId("dig_mark2"), sprite("dig_mark2"));
    Tile::addTile(ViewId("forbid_zone"), sprite("dig_mark").setColor(Color::RED));
    Tile::addTile(ViewId("destroy_button"), sprite("remove"));
    Tile::addTile(ViewId("empty"), empty());
    Tile::addTile(ViewId("border_guard"), empty());
    Tile::addTile(ViewId("demon_dweller"), sprite("demon_dweller"));
    Tile::addTile(ViewId("demon_lord"), sprite("demon_lord"));
    Tile::addTile(ViewId("vampire"), sprite("vampire"));
    Tile::addTile(ViewId("fallen_tree"), sprite("treecut"));
    Tile::addTile(ViewId("decid_tree"), sprite("tree2").setRoundShadow());
    Tile::addTile(ViewId("canif_tree"), sprite("tree1").setRoundShadow());
    Tile::addTile(ViewId("tree_trunk"), sprite("treecut"));
    Tile::addTile(ViewId("unicorn"), sprite("unicorn"));
    Tile::addTile(ViewId("burnt_tree"), sprite("treeburnt")
        .setRoundShadow());
    Tile::addTile(ViewId("player"), sprite("adventurer"));
    Tile::addTile(ViewId("player_f"), sprite("adventurer_female"));
    Tile::addTile(ViewId("keeper1"), sprite("keeper1"));
    Tile::addTile(ViewId("keeper2"), sprite("keeper2"));
    Tile::addTile(ViewId("keeper3"), sprite("keeper3"));
    Tile::addTile(ViewId("keeper4"), sprite("keeper4"));
    Tile::addTile(ViewId("keeper_f1"), sprite("keeper_female1"));
    Tile::addTile(ViewId("keeper_f2"), sprite("keeper_female2"));
    Tile::addTile(ViewId("keeper_f3"), sprite("keeper_female3"));
    Tile::addTile(ViewId("keeper_f4"), sprite("keeper_female4"));
    Tile::addTile(ViewId("keeper_knight1"), sprite("keeper_knight1"));
    Tile::addTile(ViewId("keeper_knight2"), sprite("keeper_knight2"));
    Tile::addTile(ViewId("keeper_knight3"), sprite("keeper_knight3"));
    Tile::addTile(ViewId("keeper_knight4"), sprite("keeper_knight4"));
    Tile::addTile(ViewId("keeper_knight_f1"), sprite("keeper_knight_female1"));
    Tile::addTile(ViewId("keeper_knight_f2"), sprite("keeper_knight_female2"));
    Tile::addTile(ViewId("keeper_knight_f3"), sprite("keeper_knight_female3"));
    Tile::addTile(ViewId("keeper_knight_f4"), sprite("keeper_knight_female4"));
    Tile::addTile(ViewId("elf"), sprite("elf male"));
    Tile::addTile(ViewId("elf_woman"), sprite("elf female"));
    Tile::addTile(ViewId("elf_archer"), sprite("elf archer"));
    Tile::addTile(ViewId("elf_child"), sprite("elf kid"));
    Tile::addTile(ViewId("elf_lord"), sprite("elf lord"));
    Tile::addTile(ViewId("dark_elf"), sprite("dark_elf"));
    Tile::addTile(ViewId("dark_elf_woman"), sprite("dark_elf_female"));
    Tile::addTile(ViewId("dark_elf_warrior"), sprite("dark_elf_warrior"));
    Tile::addTile(ViewId("dark_elf_child"), sprite("dark_elf_kid"));
    Tile::addTile(ViewId("dark_elf_lord"), sprite("dark_elf_lord"));
    Tile::addTile(ViewId("driad"), sprite("driad"));
    Tile::addTile(ViewId("kobold"), sprite("kobold"));
    Tile::addTile(ViewId("lizardman"), sprite("lizardman"));
    Tile::addTile(ViewId("lizardlord"), sprite("lizardlord"));
    Tile::addTile(ViewId("imp"), sprite("imp"));
    Tile::addTile(ViewId("prisoner"), sprite("prisoner"));
    Tile::addTile(ViewId("ogre"), sprite("troll"));
    Tile::addTile(ViewId("chicken"), sprite("hen"));
  }
  void genTiles2() {
    Tile::addTile(ViewId("gnome"), sprite("gnome"));
    Tile::addTile(ViewId("gnome_boss"), sprite("gnomeboss"));
    Tile::addTile(ViewId("dwarf"), sprite("dwarf"));
    Tile::addTile(ViewId("dwarf_baron"), sprite("dwarfboss"));
    Tile::addTile(ViewId("dwarf_female"), sprite("dwarf_f"));
    Tile::addTile(ViewId("shopkeeper"), sprite("salesman"));
    Tile::addTile(ViewId("bridge"), sprite("bridge").addOption(Dir::S, byName("bridge2")));
    Tile::addTile(ViewId("road"), getRoadTile("road"));
    Tile::addTile(ViewId("floor"), sprite("floor"));
    Tile::addTile(ViewId("keeper_floor"), sprite("floor_keeper"));
    Tile::addTile(ViewId("wood_floor1"), sprite("floor_wood1"));
    Tile::addTile(ViewId("wood_floor2"), sprite("floor_wood2"));
    Tile::addTile(ViewId("wood_floor3"), sprite("floor_wood3"));
    Tile::addTile(ViewId("wood_floor4"), sprite("floor_wood4"));
    Tile::addTile(ViewId("wood_floor5"), sprite("floor_wood5"));
    Tile::addTile(ViewId("stone_floor1"), sprite("floor_stone1"));
    Tile::addTile(ViewId("stone_floor2"), sprite("floor_stone2"));
    Tile::addTile(ViewId("stone_floor3"), sprite("floor_stone3"));
    Tile::addTile(ViewId("stone_floor4"), sprite("floor_stone4"));
    Tile::addTile(ViewId("stone_floor5"), sprite("floor_stone5"));
    Tile::addTile(ViewId("carpet_floor1"), sprite("floor_carpet1"));
    Tile::addTile(ViewId("carpet_floor2"), sprite("floor_carpet2"));
    Tile::addTile(ViewId("carpet_floor3"), sprite("floor_carpet3"));
    Tile::addTile(ViewId("carpet_floor4"), sprite("floor_carpet4"));
    Tile::addTile(ViewId("carpet_floor5"), sprite("floor_carpet5"));
    Tile::addTile(ViewId("buff_floor1"), sprite("floor_buff1"));
    Tile::addTile(ViewId("buff_floor2"), sprite("floor_buff2"));
    Tile::addTile(ViewId("buff_floor3"), sprite("floor_buff3"));
    Tile::addTile(ViewId("sand"), getExtraBorderTile("sand")
        .addExtraBorderId(ViewId("water")));
    Tile::addTile(ViewId("mud"), getExtraBorderTile("mud")
        .addExtraBorderId(ViewId("water"))
        .addExtraBorderId(ViewId("hill"))
        .addExtraBorderId(ViewId("sand")));
    Tile::addTile(ViewId("grass"), getExtraBorderTile("grass")
        .addExtraBorderId(ViewId("sand"))
        .addExtraBorderId(ViewId("hill"))
        .addExtraBorderId(ViewId("mud"))
        .addExtraBorderId(ViewId("water")));
    Tile::addTile(ViewId("crops"), sprite("wheatfield1"));
    Tile::addTile(ViewId("crops2"), sprite("wheatfield2"));
    Tile::addTile(ViewId("mountain"), getMountainTile(sprite("mountain_ted"), "mountain").setWallShadow());
    Tile::addTile(ViewId("mountain2"), getMountainTile(sprite("mountain_ted2"), "mountain").setWallShadow());
    Tile::addTile(ViewId("dungeon_wall"), getMountainTile(sprite("mountain_ted"), "dungeonwall").setWallShadow());
    Tile::addTile(ViewId("dungeon_wall2"), getMountainTile(sprite("mountain_ted2"), "dungeonwall").setWallShadow());
    Tile::addTile(ViewId("wall"), getWallTile("wall"));
    Tile::addTile(ViewId("map_mountain1"), sprite("map_mountain1"));
    Tile::addTile(ViewId("map_mountain2"), sprite("map_mountain2"));
    Tile::addTile(ViewId("map_mountain3"), sprite("map_mountain3"));
  }
  void genTiles3() {
    Tile::addTile(ViewId("gold_ore"), getMountainTile(sprite("gold_ore")
          .addBackground(byName("mountain_ted2")).setWallShadow(), "mountain"));
    Tile::addTile(ViewId("iron_ore"), getMountainTile(sprite("iron_ore")
          .addBackground(byName("mountain_ted2")).setWallShadow(), "mountain"));
    Tile::addTile(ViewId("stone"), getMountainTile(sprite("stone_ore")
          .addBackground(byName("mountain_ted2")).setWallShadow(), "mountain"));
    Tile::addTile(ViewId("adamantium_ore"), getMountainTile(sprite("adamantium")
          .addBackground(byName("mountain_ted2")).setWallShadow(), "mountain"));
    Tile::addTile(ViewId("hill"), getExtraBorderTile("hill")
        .addExtraBorderId(ViewId("sand"))
        .addExtraBorderId(ViewId("floor"))
        .addExtraBorderId(ViewId("keeper_floor"))
        .addExtraBorderId(ViewId("water")));
    Tile::addTile(ViewId("wood_wall"), getWallTile("wood_wall").setWallShadow());
    Tile::addTile(ViewId("castle_wall"), getWallTile("castle_wall").setWallShadow());
    Tile::addTile(ViewId("mud_wall"), getWallTile("mud_wall").setWallShadow());
    Tile::addTile(ViewId("ruin_wall"), getWallTile("ruin_wall"));
    Tile::addTile(ViewId("down_staircase"), sprite("down_stairs"));
    Tile::addTile(ViewId("up_staircase"), sprite("up_stairs"));
    Tile::addTile(ViewId("well"), sprite("well")
        .setRoundShadow());
    Tile::addTile(ViewId("minion_statue"), sprite("statue")
        .setRoundShadow());
    Tile::addTile(ViewId("stone_minion_statue"), sprite("statue_stone")
        .setRoundShadow());
    Tile::addTile(ViewId("throne"), sprite("throne").setRoundShadow());
    Tile::addTile(ViewId("orc"), sprite("orc"));
    Tile::addTile(ViewId("orc_captain"), sprite("orc_captain"));
    Tile::addTile(ViewId("orc_shaman"), sprite("orcshaman"));
    Tile::addTile(ViewId("harpy"), sprite("harpy"));
    Tile::addTile(ViewId("doppleganger"), sprite("dopple"));
    Tile::addTile(ViewId("succubus"), sprite("succubussmall"));
    Tile::addTile(ViewId("bandit"), sprite("bandit"));
    Tile::addTile(ViewId("ghost"), sprite("ghost4"));
    Tile::addTile(ViewId("spirit"), sprite("spirit"));
    Tile::addTile(ViewId("green_dragon"), sprite("greendragon"));
    Tile::addTile(ViewId("red_dragon"), sprite("reddragon"));
    Tile::addTile(ViewId("cyclops"), sprite("cyclops"));
    Tile::addTile(ViewId("minotaur"), sprite("mino"));
    Tile::addTile(ViewId("soft_monster"), sprite("softmonster"));
    Tile::addTile(ViewId("hydra"), sprite("hydra"));
    Tile::addTile(ViewId("shelob"), sprite("szelob"));
    Tile::addTile(ViewId("witch"), sprite("witch"));
    Tile::addTile(ViewId("witchman"), sprite("witchman"));
    Tile::addTile(ViewId("knight"), sprite("knight"));
    Tile::addTile(ViewId("jester"), sprite("jester"));
    Tile::addTile(ViewId("priest"), sprite("priest"));
    Tile::addTile(ViewId("warrior"), sprite("warrior"));
    Tile::addTile(ViewId("shaman"), sprite("shaman"));
    Tile::addTile(ViewId("duke1"), sprite("knightboss1"));
    Tile::addTile(ViewId("duke2"), sprite("knightboss2"));
    Tile::addTile(ViewId("duke3"), sprite("knightboss3"));
    Tile::addTile(ViewId("duke4"), sprite("knightboss4"));
    Tile::addTile(ViewId("duke_f1"), sprite("knightboss_f1"));
    Tile::addTile(ViewId("duke_f2"), sprite("knightboss_f2"));
    Tile::addTile(ViewId("duke_f3"), sprite("knightboss_f3"));
    Tile::addTile(ViewId("duke_f4"), sprite("knightboss_f4"));
    Tile::addTile(ViewId("archer"), sprite("archer"));
    Tile::addTile(ViewId("peseant"), sprite("peasant"));
    Tile::addTile(ViewId("peseant_woman"), sprite("peasantgirl"));
    Tile::addTile(ViewId("child"), sprite("peasantkid"));
    Tile::addTile(ViewId("clay_golem"), sprite("clay_golem"));
    Tile::addTile(ViewId("stone_golem"), sprite("stone_golem"));
    Tile::addTile(ViewId("iron_golem"), sprite("iron_golem"));
    Tile::addTile(ViewId("lava_golem"), sprite("lava_golem"));
    Tile::addTile(ViewId("ada_golem"), sprite("ada_golem"));
    Tile::addTile(ViewId("automaton"), sprite("automaton"));
    Tile::addTile(ViewId("elementalist"), sprite("elementalist"));
    Tile::addTile(ViewId("air_elemental"), sprite("airelement"));
    Tile::addTile(ViewId("fire_elemental"), sprite("fireelement"));
    Tile::addTile(ViewId("water_elemental"), sprite("waterelement"));
    Tile::addTile(ViewId("earth_elemental"), sprite("earthelement"));
    Tile::addTile(ViewId("ent"), sprite("ent"));
    Tile::addTile(ViewId("angel"), sprite("angel"));
    Tile::addTile(ViewId("zombie"), sprite("zombie"));
    Tile::addTile(ViewId("skeleton"), sprite("skeleton"));
    Tile::addTile(ViewId("vampire_lord"), sprite("vampirelord"));
    Tile::addTile(ViewId("mummy"), sprite("mummy"));
    Tile::addTile(ViewId("jackal"), sprite("jackal"));
    Tile::addTile(ViewId("deer"), sprite("deer"));
    Tile::addTile(ViewId("horse"), sprite("horse"));
    Tile::addTile(ViewId("cow"), sprite("cow"));
    Tile::addTile(ViewId("pig"), sprite("pig"));
    Tile::addTile(ViewId("donkey"), sprite("donkey"));
    Tile::addTile(ViewId("goat"), sprite("goat"));
    Tile::addTile(ViewId("boar"), sprite("boar"));
    Tile::addTile(ViewId("fox"), sprite("fox"));
    Tile::addTile(ViewId("wolf"), sprite("wolf"));
    Tile::addTile(ViewId("werewolf"), sprite("werewolf2"));
    Tile::addTile(ViewId("dog"), sprite("dog"));
    Tile::addTile(ViewId("kraken_head"), sprite("krakenhead"));
    Tile::addTile(ViewId("kraken_land"), sprite("krakenland1"));
    Tile::addTile(ViewId("kraken_water"), sprite("krakenwater2"));
    Tile::addTile(ViewId("death"), sprite("death"));
    Tile::addTile(ViewId("fire_sphere"), sprite("fire_sphere").setFX(FXVariantName::FIRE_SPHERE));
    Tile::addTile(ViewId("bear"), sprite("bear"));
    Tile::addTile(ViewId("bat"), sprite("bat"));
    Tile::addTile(ViewId("goblin"), sprite("goblin"));
    Tile::addTile(ViewId("rat"), sprite("rat"));
    Tile::addTile(ViewId("rat_soldier"), sprite("rat_soldier"));
    Tile::addTile(ViewId("rat_lady"), sprite("rat_lady"));
    Tile::addTile(ViewId("rat_king"), sprite("rat_king"));
    Tile::addTile(ViewId("spider"), sprite("spider"));
    Tile::addTile(ViewId("fly"), sprite("fly"));
    Tile::addTile(ViewId("ant_worker"), sprite("antwork"));
    Tile::addTile(ViewId("ant_soldier"), sprite("antw"));
    Tile::addTile(ViewId("ant_queen"), sprite("antq"));
    Tile::addTile(ViewId("snake"), sprite("snake"));
  }
  void genTiles4() {
    Tile::addTile(ViewId("vulture"), sprite("vulture"));
    Tile::addTile(ViewId("raven"), sprite("raven"));
    Tile::addTile(ViewId("body_part"), sprite("corpse4"));
    Tile::addTile(ViewId("bone"), sprite("bone"));
    Tile::addTile(ViewId("bush"), sprite("bush").setRoundShadow());
    Tile::addTile(ViewId("water"), getWaterTile("wateranim", "water"));
    Tile::addTile(ViewId("magma"), getWaterTile("magmaanim", "magma"));
    Tile::addTile(ViewId("wood_door"), sprite("door_wood").setWallShadow());
    Tile::addTile(ViewId("iron_door"), sprite("door_iron").setWallShadow());
    Tile::addTile(ViewId("ada_door"), sprite("door_steel").setWallShadow());
    Tile::addTile(ViewId("barricade"), sprite("barricade").setRoundShadow());
    Tile::addTile(ViewId("dig_icon"), sprite("dig_icon"));
    Tile::addTile(ViewId("sword"), sprite("sword"));
    Tile::addTile(ViewId("ada_sword"), sprite("steel_sword"));
    Tile::addTile(ViewId("spear"), sprite("spear"));
    Tile::addTile(ViewId("special_sword"), sprite("special_sword"));
    Tile::addTile(ViewId("elven_sword"), sprite("elven_sword"));
    Tile::addTile(ViewId("knife"), sprite("knife"));
    Tile::addTile(ViewId("war_hammer"), sprite("warhammer"));
    Tile::addTile(ViewId("ada_war_hammer"), sprite("ada_warhammer"));
    Tile::addTile(ViewId("special_war_hammer"), sprite("special_warhammer"));
    Tile::addTile(ViewId("battle_axe"), sprite("battle_axe"));
    Tile::addTile(ViewId("ada_battle_axe"), sprite("steel_battle_axe"));
    Tile::addTile(ViewId("special_battle_axe"), sprite("special_battle_axe"));
    Tile::addTile(ViewId("bow"), sprite("bow"));
    Tile::addTile(ViewId("elven_bow"), sprite("bow")); //For now
    Tile::addTile(ViewId("arrow"), sprite("arrow_e"));
    Tile::addTile(ViewId("wooden_staff"), sprite("staff_wooden"));
    Tile::addTile(ViewId("iron_staff"), sprite("staff_iron"));
    Tile::addTile(ViewId("golden_staff"), sprite("staff_gold"));
    Tile::addTile(ViewId("force_bolt"), sprite("force_bolt"));
    Tile::addTile(ViewId("fireball"), sprite("fireball"));
    Tile::addTile(ViewId("air_blast"), sprite("air_blast"));
    Tile::addTile(ViewId("stun_ray"), sprite("stun_ray"));
    Tile::addTile(ViewId("club"), sprite("club"));
    Tile::addTile(ViewId("heavy_club"), sprite("heavy club"));
    Tile::addTile(ViewId("scroll"), sprite("scroll"));
    Tile::addTile(ViewId("amulet1"), sprite("amulet1"));
    Tile::addTile(ViewId("amulet2"), sprite("amulet2"));
    Tile::addTile(ViewId("amulet3"), sprite("amulet3"));
    Tile::addTile(ViewId("amulet4"), sprite("amulet4"));
    Tile::addTile(ViewId("amulet5"), sprite("amulet5"));
    Tile::addTile(ViewId("fire_resist_ring"), sprite("ring_red"));
    Tile::addTile(ViewId("poison_resist_ring"), sprite("ring_green"));
    Tile::addTile(ViewId("book"), sprite("book"));
    Tile::addTile(ViewId("hand_torch"), sprite("hand_torch"));
    Tile::addTile(ViewId("first_aid"), sprite("medkit"));
    Tile::addTile(ViewId("trap_item"), sprite("trapbox"));
    Tile::addTile(ViewId("potion1"), sprite("potion1"));
    Tile::addTile(ViewId("potion2"), sprite("potion2"));
    Tile::addTile(ViewId("potion3"), sprite("potion3"));
    Tile::addTile(ViewId("potion4"), sprite("potion4"));
    Tile::addTile(ViewId("potion5"), sprite("potion5"));
    Tile::addTile(ViewId("potion6"), sprite("potion6"));
    Tile::addTile(ViewId("rune1"), sprite("rune1"));
    Tile::addTile(ViewId("rune2"), sprite("rune2"));
    Tile::addTile(ViewId("rune3"), sprite("rune3"));
    Tile::addTile(ViewId("rune4"), sprite("rune4"));
    Tile::addTile(ViewId("mushroom1"), sprite("mushroom1").setRoundShadow());
    Tile::addTile(ViewId("mushroom2"), sprite("mushroom2").setRoundShadow());
    Tile::addTile(ViewId("mushroom3"), sprite("mushroom3").setRoundShadow());
    Tile::addTile(ViewId("mushroom4"), sprite("mushroom4").setRoundShadow());
    Tile::addTile(ViewId("mushroom5"), sprite("mushroom5").setRoundShadow());
    Tile::addTile(ViewId("mushroom6"), sprite("mushroom6").setRoundShadow());
    Tile::addTile(ViewId("mushroom7"), sprite("mushroom2").setRoundShadow().setColor(Color::LIGHT_BLUE));
    Tile::addTile(ViewId("key"), sprite("key"));
    Tile::addTile(ViewId("fountain"), sprite("fountain").setRoundShadow());
    Tile::addTile(ViewId("gold"), sprite("gold"));
    Tile::addTile(ViewId("treasure_chest"), sprite("treasurydeco"));
    Tile::addTile(ViewId("chest"), sprite("chest").setRoundShadow());
    Tile::addTile(ViewId("opened_chest"), sprite("chest_opened").setRoundShadow());
    Tile::addTile(ViewId("coffin1"), sprite("coffin"));
    Tile::addTile(ViewId("coffin2"), sprite("coffin2"));
    Tile::addTile(ViewId("coffin3"), sprite("coffin3"));
    Tile::addTile(ViewId("loot_coffin"), sprite("coffin"));
    Tile::addTile(ViewId("opened_coffin"), sprite("coffin_opened"));
    Tile::addTile(ViewId("boulder"), sprite("boulder").setRoundShadow());
    Tile::addTile(ViewId("portal"), sprite("surprise").setRoundShadow());
    Tile::addTile(ViewId("gas_trap"), sprite("gas_trap"));
    Tile::addTile(ViewId("alarm_trap"), sprite("alarm_trap"));
    Tile::addTile(ViewId("web_trap"), sprite("web_trap"));
    Tile::addTile(ViewId("surprise_trap"), sprite("surprisetrap"));
    Tile::addTile(ViewId("terror_trap"), sprite("terror_trap"));
    Tile::addTile(ViewId("fire_trap"), sprite("fire_trap"));
    Tile::addTile(ViewId("rock"), sprite("stonepile"));
    Tile::addTile(ViewId("iron_rock"), sprite("ironpile"));
    Tile::addTile(ViewId("ada_ore"), sprite("steelpile"));
    Tile::addTile(ViewId("wood_plank"), sprite("wood2"));
    Tile::addTile(ViewId("storage_equipment"), sprite("dig_mark").setColor(Color::BLUE.transparency(120)));
    Tile::addTile(ViewId("storage_resources"), sprite("dig_mark").setColor(Color::GREEN.transparency(120)));
    Tile::addTile(ViewId("quarters1"), sprite("dig_mark2").setColor(Color::PINK.transparency(120)));
    Tile::addTile(ViewId("quarters2"), sprite("dig_mark2").setColor(Color::SKY_BLUE.transparency(120)));
    Tile::addTile(ViewId("quarters3"), sprite("dig_mark2").setColor(Color::ORANGE.transparency(120)));
    Tile::addTile(ViewId("leisure"), sprite("dig_mark2").setColor(Color::DARK_BLUE.transparency(120)));
    Tile::addTile(ViewId("prison"), sprite("prison"));
    Tile::addTile(ViewId("bed1"), sprite("bed1").setRoundShadow());
    Tile::addTile(ViewId("bed2"), sprite("bed2").setRoundShadow());
    Tile::addTile(ViewId("bed3"), sprite("bed3").setRoundShadow());
    Tile::addTile(ViewId("dorm"), sprite("sleep").setFloorBorders());
    Tile::addTile(ViewId("torch"), sprite("torch"));
    Tile::addTile(ViewId("candelabrum_ns"), sprite("candelabrum_ns"));
    Tile::addTile(ViewId("candelabrum_w"), sprite("candelabrum_w"));
    Tile::addTile(ViewId("candelabrum_e"), sprite("candelabrum_e"));
    Tile::addTile(ViewId("standing_torch"), sprite("standing_torch").setMoveUp());
    Tile::addTile(ViewId("altar"), sprite("altar").setRoundShadow());
  }
  void genTiles5() {
    Tile::addTile(ViewId("creature_altar"), sprite("altar2").setRoundShadow());
    Tile::addTile(ViewId("altar_des"), sprite("altar_des").setRoundShadow());
    Tile::addTile(ViewId("torture_table"), sprite("torturedeco").setRoundShadow());
    Tile::addTile(ViewId("impaled_head"), sprite("impaledhead").setRoundShadow());
    Tile::addTile(ViewId("whipping_post"), sprite("whipping_post").setRoundShadow());
    Tile::addTile(ViewId("gallows"), sprite("gallows").setRoundShadow());
    Tile::addTile(ViewId("notice_board"), sprite("board").setRoundShadow());
    Tile::addTile(ViewId("sokoban_hole"), sprite("hole"));
    Tile::addTile(ViewId("training_wood"), sprite("train_wood").setRoundShadow());
    Tile::addTile(ViewId("training_iron"), sprite("train_iron").setRoundShadow());
    Tile::addTile(ViewId("training_ada"), sprite("train_steel").setRoundShadow());
    Tile::addTile(ViewId("archery_range"), sprite("archery_range").setRoundShadow());
    Tile::addTile(ViewId("demon_shrine"), sprite("demon_shrine").setRoundShadow());
    Tile::addTile(ViewId("bookcase_wood"), sprite("library_wood").setRoundShadow());
    Tile::addTile(ViewId("bookcase_iron"), sprite("library_iron").setRoundShadow());
    Tile::addTile(ViewId("bookcase_gold"), sprite("library_gold").setRoundShadow());
    Tile::addTile(ViewId("laboratory"), sprite("labdeco").setRoundShadow());
    Tile::addTile(ViewId("cauldron"), sprite("labdeco"));
    Tile::addTile(ViewId("beast_lair"), sprite("lair").setFloorBorders());
    Tile::addTile(ViewId("beast_cage"), sprite("lairdeco").setRoundShadow());
    Tile::addTile(ViewId("forge"), sprite("forgedeco").setRoundShadow());
    Tile::addTile(ViewId("workshop"), sprite("workshopdeco").setRoundShadow());
    Tile::addTile(ViewId("jeweller"), sprite("jewelerdeco").setRoundShadow());
    Tile::addTile(ViewId("furnace"), sprite("steel_furnace").setRoundShadow());
    Tile::addTile(ViewId("grave"), sprite("RIP").setRoundShadow());
    Tile::addTile(ViewId("robe"), sprite("robe"));
    Tile::addTile(ViewId("leather_gloves"), sprite("leather_gloves"));
    Tile::addTile(ViewId("iron_gloves"), sprite("iron_gloves"));
    Tile::addTile(ViewId("ada_gloves"), sprite("ada_gloves"));
    Tile::addTile(ViewId("dexterity_gloves"), sprite("blue_gloves"));
    Tile::addTile(ViewId("strength_gloves"), sprite("red_gloves"));
    Tile::addTile(ViewId("leather_armor"), sprite("leather_armor"));
    Tile::addTile(ViewId("leather_helm"), sprite("leather_helm"));
    Tile::addTile(ViewId("telepathy_helm"), sprite("blue_helm"));
    Tile::addTile(ViewId("chain_armor"), sprite("chain_armor"));
    Tile::addTile(ViewId("ada_armor"), sprite("steel_armor"));
    Tile::addTile(ViewId("iron_helm"), sprite("iron_helm"));
    Tile::addTile(ViewId("ada_helm"), sprite("ada_helm"));
    Tile::addTile(ViewId("leather_boots"), sprite("leather_boots"));
    Tile::addTile(ViewId("iron_boots"), sprite("iron_boots"));
    Tile::addTile(ViewId("ada_boots"), sprite("ada_boots"));
    Tile::addTile(ViewId("speed_boots"), sprite("blue_boots"));
    Tile::addTile(ViewId("levitation_boots"), sprite("green_boots"));
    Tile::addTile(ViewId("wooden_shield"), sprite("wooden_shield"));
    Tile::addTile(ViewId("guard_post"), sprite("guardroom").setRoundShadow());
    Tile::addTile(ViewId("mana"), sprite("mana"));
    Tile::addTile(ViewId("fetch_icon"), sprite("leather_gloves"));
    Tile::addTile(ViewId("eyeball"), sprite("eyeball2").setRoundShadow());
    Tile::addTile(ViewId("fog_of_war"), getWaterTile("empty", "fogofwar"));
    Tile::addTile(ViewId("pit"), sprite("pit"));
    Tile::addTile(ViewId("creature_highlight"), sprite("creature_highlight"));
    Tile::addTile(ViewId("square_highlight"), sprite("square_highlight"));
    Tile::addTile(ViewId("round_shadow"), sprite("round_shadow"));
    Tile::addTile(ViewId("campaign_defeated"), sprite("campaign_defeated"));
    Tile::addTile(ViewId("accept_immigrant"), symbol(u8"✓", Color::GREEN, true));
    Tile::addTile(ViewId("reject_immigrant"), symbol(u8"✘", Color::RED, true));
    Tile::addTile(ViewId("fog_of_war_corner"), sprite("fogofwar")
        .addConnection({Dir::NE}, byName("fogofwarcornne"))
        .addConnection({Dir::NW}, byName("fogofwarcornnw"))
        .addConnection({Dir::SE}, byName("fogofwarcornse"))
        .addConnection({Dir::SW}, byName("fogofwarcornsw")));
    Tile::addTile(ViewId("special_blbn"), sprite("special_blbn"));
    Tile::addTile(ViewId("special_blbw"), sprite("special_blbw"));
    Tile::addTile(ViewId("special_blgn"), sprite("special_blgn"));
    Tile::addTile(ViewId("special_blgw"), sprite("special_blgw"));
    Tile::addTile(ViewId("special_bmbn"), sprite("special_bmbn"));
    Tile::addTile(ViewId("special_bmbw"), sprite("special_bmbw"));
    Tile::addTile(ViewId("special_bmgn"), sprite("special_bmgn"));
    Tile::addTile(ViewId("special_bmgw"), sprite("special_bmgw"));
    Tile::addTile(ViewId("special_hlbn"), sprite("special_hlbn"));
    Tile::addTile(ViewId("special_hlbw"), sprite("special_hlbw"));
    Tile::addTile(ViewId("special_hlgn"), sprite("special_hlgn"));
    Tile::addTile(ViewId("special_hlgw"), sprite("special_hlgw"));
    Tile::addTile(ViewId("special_hmbn"), sprite("special_hmbn"));
    Tile::addTile(ViewId("special_hmbw"), sprite("special_hmbw"));
    Tile::addTile(ViewId("special_hmgn"), sprite("special_hmgn"));
    Tile::addTile(ViewId("special_hmgw"), sprite("special_hmgw"));
    Tile::addTile(ViewId("halloween_kid1"), sprite("halloween_kid1"));
    Tile::addTile(ViewId("halloween_kid2"), sprite("halloween_kid2"));
    Tile::addTile(ViewId("halloween_kid3"), sprite("halloween_kid3"));
    Tile::addTile(ViewId("halloween_kid4"), sprite("halloween_kid4"));
    Tile::addTile(ViewId("halloween_costume"), sprite("halloween_costume"));
    Tile::addTile(ViewId("bag_of_candy"), sprite("halloween_candies"));
    Tile::addTile(ViewId("horn_attack"), sprite("horn_attack"));
    Tile::addTile(ViewId("beak_attack"), sprite("beak_attack"));
    Tile::addTile(ViewId("touch_attack"), sprite("touch_attack"));
    Tile::addTile(ViewId("bite_attack"), sprite("bite_attack"));
    Tile::addTile(ViewId("claws_attack"), sprite("claws_attack"));
    Tile::addTile(ViewId("leg_attack"), sprite("leg_attack"));
    Tile::addTile(ViewId("fist_attack"), sprite("fist_attack"));
    Tile::addTile(ViewId("item_aura"), sprite("aura"));
#ifndef RELEASE
    Tile::addTile(ViewId("tutorial_entrance"), symbol(u8"?", Color::YELLOW));
#else
    Tile::addTile(ViewId("tutorial_entrance"), sprite("empty"));
#endif
  }

  Tile symbol(const string& s, Color id, bool symbol = false) {
    return Tile::fromString(s, id, symbol);
  }

  void genSymbols1() {
    Tile::addSymbol(ViewId("demon_dweller"), symbol(u8"U", Color::PURPLE));
    Tile::addSymbol(ViewId("demon_lord"), symbol(u8"U", Color::YELLOW));
    Tile::addSymbol(ViewId("empty"), symbol(u8" ", Color::BLACK));
    Tile::addSymbol(ViewId("dig_mark"), symbol(u8" ", Color::BLACK));
    Tile::addSymbol(ViewId("dig_mark2"), symbol(u8" ", Color::BLACK));
    Tile::addSymbol(ViewId("player"), symbol(u8"@", Color::WHITE));
    Tile::addSymbol(ViewId("player_f"), symbol(u8"@", Color::YELLOW));
    Tile::addSymbol(ViewId("keeper1"), symbol(u8"@", Color::PURPLE));
    Tile::addSymbol(ViewId("keeper2"), symbol(u8"@", Color::PURPLE));
    Tile::addSymbol(ViewId("keeper3"), symbol(u8"@", Color::PURPLE));
    Tile::addSymbol(ViewId("keeper4"), symbol(u8"@", Color::PURPLE));
    Tile::addSymbol(ViewId("keeper_f1"), symbol(u8"@", Color::PINK));
    Tile::addSymbol(ViewId("keeper_f2"), symbol(u8"@", Color::PINK));
    Tile::addSymbol(ViewId("keeper_f3"), symbol(u8"@", Color::PINK));
    Tile::addSymbol(ViewId("keeper_f4"), symbol(u8"@", Color::PINK));
    Tile::addSymbol(ViewId("keeper_knight1"), symbol(u8"@", Color::YELLOW));
    Tile::addSymbol(ViewId("keeper_knight2"), symbol(u8"@", Color::YELLOW));
    Tile::addSymbol(ViewId("keeper_knight3"), symbol(u8"@", Color::YELLOW));
    Tile::addSymbol(ViewId("keeper_knight4"), symbol(u8"@", Color::YELLOW));
    Tile::addSymbol(ViewId("keeper_knight_f1"), symbol(u8"@", Color::YELLOW));
    Tile::addSymbol(ViewId("keeper_knight_f2"), symbol(u8"@", Color::YELLOW));
    Tile::addSymbol(ViewId("keeper_knight_f3"), symbol(u8"@", Color::YELLOW));
    Tile::addSymbol(ViewId("keeper_knight_f4"), symbol(u8"@", Color::YELLOW));
    Tile::addSymbol(ViewId("unknown_monster"), symbol(u8"?", Color::LIGHT_GREEN));
    Tile::addSymbol(ViewId("elf"), symbol(u8"@", Color::LIGHT_GREEN));
    Tile::addSymbol(ViewId("elf_woman"), symbol(u8"@", Color::LIGHT_GREEN));
    Tile::addSymbol(ViewId("elf_archer"), symbol(u8"@", Color::GREEN));
    Tile::addSymbol(ViewId("elf_child"), symbol(u8"@", Color::LIGHT_GREEN));
    Tile::addSymbol(ViewId("elf_lord"), symbol(u8"@", Color::DARK_GREEN));
    Tile::addSymbol(ViewId("dark_elf"), symbol(u8"@", Color::ALMOST_DARK_GRAY));
    Tile::addSymbol(ViewId("dark_elf_woman"), symbol(u8"@", Color::ALMOST_DARK_GRAY));
    Tile::addSymbol(ViewId("dark_elf_warrior"), symbol(u8"@", Color::GRAY));
    Tile::addSymbol(ViewId("dark_elf_child"), symbol(u8"@", Color::ALMOST_GRAY));
    Tile::addSymbol(ViewId("dark_elf_lord"), symbol(u8"@", Color::LIGHT_GRAY));
    Tile::addSymbol(ViewId("driad"), symbol(u8"@", Color::LIGHT_BROWN));
    Tile::addSymbol(ViewId("unicorn"), symbol(u8"h", Color::WHITE));
    Tile::addSymbol(ViewId("kobold"), symbol(u8"k", Color::LIGHT_BROWN));
    Tile::addSymbol(ViewId("shopkeeper"), symbol(u8"@", Color::LIGHT_BLUE));
    Tile::addSymbol(ViewId("lizardman"), symbol(u8"@", Color::LIGHT_BROWN));
    Tile::addSymbol(ViewId("lizardlord"), symbol(u8"@", Color::BROWN));
    Tile::addSymbol(ViewId("imp"), symbol(u8"i", Color::LIGHT_BROWN));
    Tile::addSymbol(ViewId("prisoner"), symbol(u8"@", Color::LIGHT_BROWN));
    Tile::addSymbol(ViewId("ogre"), symbol(u8"O", Color::GREEN));
    Tile::addSymbol(ViewId("chicken"), symbol(u8"c", Color::YELLOW));
    Tile::addSymbol(ViewId("gnome"), symbol(u8"g", Color::GREEN));
    Tile::addSymbol(ViewId("gnome_boss"), symbol(u8"g", Color::DARK_BLUE));
    Tile::addSymbol(ViewId("dwarf"), symbol(u8"h", Color::BLUE));
    Tile::addSymbol(ViewId("dwarf_baron"), symbol(u8"h", Color::DARK_BLUE));
    Tile::addSymbol(ViewId("dwarf_female"), symbol(u8"h", Color::LIGHT_BLUE));
    Tile::addSymbol(ViewId("floor"), symbol(u8".", Color::LIGHT_GRAY));
    Tile::addSymbol(ViewId("keeper_floor"), symbol(u8".", Color::WHITE));
    Tile::addSymbol(ViewId("wood_floor1"), symbol(u8".", Color::LIGHT_BROWN));
    Tile::addSymbol(ViewId("wood_floor2"), symbol(u8".", Color::BROWN));
    Tile::addSymbol(ViewId("wood_floor3"), symbol(u8".", Color::LIGHT_BROWN));
    Tile::addSymbol(ViewId("wood_floor4"), symbol(u8".", Color::BROWN));
    Tile::addSymbol(ViewId("wood_floor5"), symbol(u8".", Color::BROWN));
    Tile::addSymbol(ViewId("stone_floor1"), symbol(u8".", Color::LIGHT_GRAY));
    Tile::addSymbol(ViewId("stone_floor2"), symbol(u8".", Color::GRAY));
    Tile::addSymbol(ViewId("stone_floor3"), symbol(u8".", Color::LIGHT_GRAY));
    Tile::addSymbol(ViewId("stone_floor4"), symbol(u8".", Color::GRAY));
    Tile::addSymbol(ViewId("stone_floor5"), symbol(u8".", Color::GRAY));
    Tile::addSymbol(ViewId("carpet_floor1"), symbol(u8".", Color::PURPLE));
    Tile::addSymbol(ViewId("carpet_floor2"), symbol(u8".", Color::PINK));
    Tile::addSymbol(ViewId("carpet_floor3"), symbol(u8".", Color::PURPLE));
    Tile::addSymbol(ViewId("carpet_floor4"), symbol(u8".", Color::PINK));
    Tile::addSymbol(ViewId("carpet_floor5"), symbol(u8".", Color::PINK));
    Tile::addSymbol(ViewId("buff_floor1"), symbol(u8".", Color::PURPLE));
    Tile::addSymbol(ViewId("buff_floor2"), symbol(u8".", Color::PINK));
    Tile::addSymbol(ViewId("buff_floor3"), symbol(u8".", Color::PURPLE));
    Tile::addSymbol(ViewId("bridge"), symbol(u8"_", Color::BROWN));
    Tile::addSymbol(ViewId("road"), symbol(u8".", Color::LIGHT_GRAY));
    Tile::addSymbol(ViewId("sand"), symbol(u8".", Color::YELLOW));
    Tile::addSymbol(ViewId("mud"), symbol(u8"𝃰", Color::BROWN, true));
    Tile::addSymbol(ViewId("grass"), symbol(u8"𝃰", Color::GREEN, true));
  }
  void genSymbols2() {
    Tile::addSymbol(ViewId("crops"), symbol(u8"𝃰", Color::YELLOW, true));
    Tile::addSymbol(ViewId("crops2"), symbol(u8"𝃰", Color::YELLOW, true));
    Tile::addSymbol(ViewId("castle_wall"), symbol(u8"#", Color::LIGHT_GRAY));
    Tile::addSymbol(ViewId("mud_wall"), symbol(u8"#", Color::LIGHT_BROWN));
    Tile::addSymbol(ViewId("ruin_wall"), symbol(u8"#", Color::DARK_GRAY));
    Tile::addSymbol(ViewId("wall"), symbol(u8"#", Color::LIGHT_GRAY));
    Tile::addSymbol(ViewId("mountain"), symbol(u8"#", Color::GRAY));
    Tile::addSymbol(ViewId("mountain2"), symbol(u8"#", Color::DARK_GRAY));
    Tile::addSymbol(ViewId("dungeon_wall"), symbol(u8"#", Color::LIGHT_GRAY));
    Tile::addSymbol(ViewId("dungeon_wall2"), symbol(u8"#", Color::DARK_BLUE));
    Tile::addSymbol(ViewId("map_mountain1"), symbol(u8"^", Color::DARK_GRAY));
    Tile::addSymbol(ViewId("map_mountain2"), symbol(u8"^", Color::GRAY));
    Tile::addSymbol(ViewId("map_mountain3"), symbol(u8"^", Color::LIGHT_GRAY));
    Tile::addSymbol(ViewId("gold_ore"), symbol(u8"⁂", Color::YELLOW, true));
    Tile::addSymbol(ViewId("iron_ore"), symbol(u8"⁂", Color::DARK_BROWN, true));
    Tile::addSymbol(ViewId("stone"), symbol(u8"⁂", Color::LIGHT_GRAY, true));
    Tile::addSymbol(ViewId("adamantium_ore"), symbol(u8"⁂", Color::LIGHT_BLUE, true));
    Tile::addSymbol(ViewId("hill"), symbol(u8"𝀢", Color::DARK_GREEN, true));
    Tile::addSymbol(ViewId("wood_wall"), symbol(u8"#", Color::DARK_BROWN));
    Tile::addSymbol(ViewId("down_staircase"), symbol(u8"➘", Color::ALMOST_WHITE, true));
    Tile::addSymbol(ViewId("up_staircase"), symbol(u8"➚", Color::ALMOST_WHITE, true));
    Tile::addSymbol(ViewId("well"), symbol(u8"0", Color::BLUE));
    Tile::addSymbol(ViewId("minion_statue"), symbol(u8"&", Color::LIGHT_GRAY));
    Tile::addSymbol(ViewId("stone_minion_statue"), symbol(u8"&", Color::ALMOST_WHITE));
    Tile::addSymbol(ViewId("throne"), symbol(u8"Ω", Color::YELLOW, true));
    Tile::addSymbol(ViewId("orc"), symbol(u8"o", Color::DARK_BLUE));
    Tile::addSymbol(ViewId("orc_captain"), symbol(u8"o", Color::PURPLE));
    Tile::addSymbol(ViewId("orc_shaman"), symbol(u8"o", Color::YELLOW));
    Tile::addSymbol(ViewId("harpy"), symbol(u8"R", Color::YELLOW));
    Tile::addSymbol(ViewId("doppleganger"), symbol(u8"&", Color::YELLOW));
    Tile::addSymbol(ViewId("succubus"), symbol(u8"&", Color::RED));
    Tile::addSymbol(ViewId("bandit"), symbol(u8"@", Color::DARK_BLUE));
    Tile::addSymbol(ViewId("green_dragon"), symbol(u8"D", Color::GREEN));
    Tile::addSymbol(ViewId("red_dragon"), symbol(u8"D", Color::RED));
    Tile::addSymbol(ViewId("cyclops"), symbol(u8"C", Color::GREEN));
    Tile::addSymbol(ViewId("minotaur"), symbol(u8"M", Color::PURPLE));
    Tile::addSymbol(ViewId("soft_monster"), symbol(u8"M", Color::PINK));
    Tile::addSymbol(ViewId("hydra"), symbol(u8"H", Color::PURPLE));
    Tile::addSymbol(ViewId("shelob"), symbol(u8"S", Color::PURPLE));
    Tile::addSymbol(ViewId("witch"), symbol(u8"@", Color::BROWN));
    Tile::addSymbol(ViewId("witchman"), symbol(u8"@", Color::WHITE));
    Tile::addSymbol(ViewId("ghost"), symbol(u8"&", Color::WHITE));
    Tile::addSymbol(ViewId("spirit"), symbol(u8"&", Color::LIGHT_BLUE));
    Tile::addSymbol(ViewId("knight"), symbol(u8"@", Color::LIGHT_GRAY));
    Tile::addSymbol(ViewId("priest"), symbol(u8"@", Color::SKY_BLUE));
    Tile::addSymbol(ViewId("jester"), symbol(u8"@", Color::PINK));
    Tile::addSymbol(ViewId("warrior"), symbol(u8"@", Color::DARK_GRAY));
    Tile::addSymbol(ViewId("shaman"), symbol(u8"@", Color::YELLOW));
    Tile::addSymbol(ViewId("duke1"), symbol(u8"@", Color::BLUE));
    Tile::addSymbol(ViewId("duke2"), symbol(u8"@", Color::BLUE));
    Tile::addSymbol(ViewId("duke3"), symbol(u8"@", Color::BLUE));
    Tile::addSymbol(ViewId("duke4"), symbol(u8"@", Color::BLUE));
    Tile::addSymbol(ViewId("duke_f1"), symbol(u8"@", Color::BLUE));
    Tile::addSymbol(ViewId("duke_f2"), symbol(u8"@", Color::BLUE));
    Tile::addSymbol(ViewId("duke_f3"), symbol(u8"@", Color::BLUE));
    Tile::addSymbol(ViewId("duke_f4"), symbol(u8"@", Color::BLUE));
    Tile::addSymbol(ViewId("archer"), symbol(u8"@", Color::BROWN));
    Tile::addSymbol(ViewId("peseant"), symbol(u8"@", Color::GREEN));
    Tile::addSymbol(ViewId("peseant_woman"), symbol(u8"@", Color::GREEN));
    Tile::addSymbol(ViewId("child"), symbol(u8"@", Color::LIGHT_GREEN));
    Tile::addSymbol(ViewId("clay_golem"), symbol(u8"Y", Color::YELLOW));
    Tile::addSymbol(ViewId("stone_golem"), symbol(u8"Y", Color::LIGHT_GRAY));
    Tile::addSymbol(ViewId("iron_golem"), symbol(u8"Y", Color::ORANGE));
    Tile::addSymbol(ViewId("lava_golem"), symbol(u8"Y", Color::PURPLE));
    Tile::addSymbol(ViewId("ada_golem"), symbol(u8"Y", Color::LIGHT_BLUE));
    Tile::addSymbol(ViewId("automaton"), symbol(u8"A", Color::LIGHT_GRAY));
    Tile::addSymbol(ViewId("elementalist"), symbol(u8"@", Color::YELLOW));
    Tile::addSymbol(ViewId("air_elemental"), symbol(u8"E", Color::LIGHT_BLUE));
    Tile::addSymbol(ViewId("fire_elemental"), symbol(u8"E", Color::RED));
    Tile::addSymbol(ViewId("water_elemental"), symbol(u8"E", Color::BLUE));
    Tile::addSymbol(ViewId("earth_elemental"), symbol(u8"E", Color::GRAY));
    Tile::addSymbol(ViewId("ent"), symbol(u8"E", Color::LIGHT_BROWN));
    Tile::addSymbol(ViewId("angel"), symbol(u8"A", Color::LIGHT_BLUE));
    Tile::addSymbol(ViewId("zombie"), symbol(u8"Z", Color::GREEN));
    Tile::addSymbol(ViewId("skeleton"), symbol(u8"Z", Color::WHITE));
    Tile::addSymbol(ViewId("vampire"), symbol(u8"V", Color::DARK_GRAY));
    Tile::addSymbol(ViewId("vampire_lord"), symbol(u8"V", Color::PURPLE));
    Tile::addSymbol(ViewId("mummy"), symbol(u8"Z", Color::YELLOW));
  }
  void genSymbols3() {
    Tile::addSymbol(ViewId("jackal"), symbol(u8"d", Color::LIGHT_BROWN));
    Tile::addSymbol(ViewId("deer"), symbol(u8"R", Color::DARK_BROWN));
    Tile::addSymbol(ViewId("horse"), symbol(u8"H", Color::LIGHT_BROWN));
    Tile::addSymbol(ViewId("cow"), symbol(u8"C", Color::WHITE));
    Tile::addSymbol(ViewId("pig"), symbol(u8"p", Color::YELLOW));
    Tile::addSymbol(ViewId("donkey"), symbol(u8"D", Color::GRAY));
    Tile::addSymbol(ViewId("goat"), symbol(u8"g", Color::GRAY));
    Tile::addSymbol(ViewId("boar"), symbol(u8"b", Color::LIGHT_BROWN));
    Tile::addSymbol(ViewId("fox"), symbol(u8"d", Color::ORANGE_BROWN));
    Tile::addSymbol(ViewId("wolf"), symbol(u8"d", Color::DARK_BLUE));
    Tile::addSymbol(ViewId("werewolf"), symbol(u8"d", Color::WHITE));
    Tile::addSymbol(ViewId("dog"), symbol(u8"d", Color::BROWN));
    Tile::addSymbol(ViewId("kraken_head"), symbol(u8"S", Color::GREEN));
    Tile::addSymbol(ViewId("kraken_water"), symbol(u8"S", Color::DARK_GREEN));
    Tile::addSymbol(ViewId("kraken_land"), symbol(u8"S", Color::DARK_GREEN));
    Tile::addSymbol(ViewId("death"), symbol(u8"D", Color::DARK_GRAY));
    Tile::addSymbol(ViewId("fire_sphere"), symbol(u8"e", Color::RED));
    Tile::addSymbol(ViewId("bear"), symbol(u8"N", Color::BROWN));
    Tile::addSymbol(ViewId("bat"), symbol(u8"b", Color::DARK_GRAY));
    Tile::addSymbol(ViewId("goblin"), symbol(u8"o", Color::GREEN));
    Tile::addSymbol(ViewId("rat"), symbol(u8"r", Color::BROWN));
    Tile::addSymbol(ViewId("rat_king"), symbol(u8"@", Color::LIGHT_BROWN));
    Tile::addSymbol(ViewId("rat_soldier"), symbol(u8"@", Color::BROWN));
    Tile::addSymbol(ViewId("rat_lady"), symbol(u8"@", Color::DARK_BROWN));
    Tile::addSymbol(ViewId("spider"), symbol(u8"s", Color::BROWN));
    Tile::addSymbol(ViewId("ant_worker"), symbol(u8"a", Color::BROWN));
    Tile::addSymbol(ViewId("ant_soldier"), symbol(u8"a", Color::YELLOW));
    Tile::addSymbol(ViewId("ant_queen"), symbol(u8"a", Color::PURPLE));
    Tile::addSymbol(ViewId("fly"), symbol(u8"b", Color::GRAY));
    Tile::addSymbol(ViewId("snake"), symbol(u8"S", Color::YELLOW));
    Tile::addSymbol(ViewId("vulture"), symbol(u8"v", Color::DARK_GRAY));
    Tile::addSymbol(ViewId("raven"), symbol(u8"v", Color::DARK_GRAY));
    Tile::addSymbol(ViewId("body_part"), symbol(u8"%", Color::RED));
    Tile::addSymbol(ViewId("bone"), symbol(u8"%", Color::WHITE));
    Tile::addSymbol(ViewId("bush"), symbol(u8"&", Color::DARK_GREEN));
    Tile::addSymbol(ViewId("decid_tree"), symbol(u8"🜍", Color::DARK_GREEN, true));
    Tile::addSymbol(ViewId("canif_tree"), symbol(u8"♣", Color::DARK_GREEN, true));
    Tile::addSymbol(ViewId("tree_trunk"), symbol(u8".", Color::BROWN));
    Tile::addSymbol(ViewId("burnt_tree"), symbol(u8".", Color::DARK_GRAY));
    Tile::addSymbol(ViewId("water"), symbol(u8"~", Color::LIGHT_BLUE));
    Tile::addSymbol(ViewId("magma"), symbol(u8"~", Color::RED));
    Tile::addSymbol(ViewId("wood_door"), symbol(u8"|", Color::BROWN));
    Tile::addSymbol(ViewId("iron_door"), symbol(u8"|", Color::LIGHT_GRAY));
    Tile::addSymbol(ViewId("ada_door"), symbol(u8"|", Color::LIGHT_BLUE));
    Tile::addSymbol(ViewId("barricade"), symbol(u8"X", Color::BROWN));
    Tile::addSymbol(ViewId("dig_icon"), symbol(u8"⛏", Color::LIGHT_GRAY, true));
    Tile::addSymbol(ViewId("ada_sword"), symbol(u8")", Color::LIGHT_BLUE));
    Tile::addSymbol(ViewId("sword"), symbol(u8")", Color::LIGHT_GRAY));
    Tile::addSymbol(ViewId("spear"), symbol(u8"/", Color::LIGHT_GRAY));
    Tile::addSymbol(ViewId("special_sword"), symbol(u8")", Color::YELLOW));
    Tile::addSymbol(ViewId("elven_sword"), symbol(u8")", Color::GRAY));
    Tile::addSymbol(ViewId("knife"), symbol(u8")", Color::WHITE));
    Tile::addSymbol(ViewId("war_hammer"), symbol(u8")", Color::BLUE));
    Tile::addSymbol(ViewId("ada_war_hammer"), symbol(u8")", Color::LIGHT_BLUE));
    Tile::addSymbol(ViewId("special_war_hammer"), symbol(u8")", Color::LIGHT_BLUE));
    Tile::addSymbol(ViewId("battle_axe"), symbol(u8")", Color::GREEN));
    Tile::addSymbol(ViewId("ada_battle_axe"), symbol(u8")", Color::LIGHT_BLUE));
    Tile::addSymbol(ViewId("special_battle_axe"), symbol(u8")", Color::LIGHT_GREEN));
    Tile::addSymbol(ViewId("elven_bow"), symbol(u8")", Color::YELLOW));
    Tile::addSymbol(ViewId("bow"), symbol(u8")", Color::BROWN));
    Tile::addSymbol(ViewId("wooden_staff"), symbol(u8")", Color::YELLOW));
    Tile::addSymbol(ViewId("iron_staff"), symbol(u8")", Color::ORANGE));
    Tile::addSymbol(ViewId("golden_staff"), symbol(u8")", Color::YELLOW));
    Tile::addSymbol(ViewId("club"), symbol(u8")", Color::BROWN));
    Tile::addSymbol(ViewId("heavy_club"), symbol(u8")", Color::BROWN));
    Tile::addSymbol(ViewId("arrow"), symbol(u8"/", Color::BROWN));
    Tile::addSymbol(ViewId("force_bolt"), symbol(u8"*", Color::LIGHT_BLUE));
    Tile::addSymbol(ViewId("fireball"), symbol(u8"*", Color::ORANGE));
    Tile::addSymbol(ViewId("air_blast"), symbol(u8"*", Color::WHITE));
    Tile::addSymbol(ViewId("stun_ray"), symbol(u8"*", Color::LIGHT_GREEN));
    Tile::addSymbol(ViewId("scroll"), symbol(u8"?", Color::WHITE));
    Tile::addSymbol(ViewId("amulet1"), symbol(u8"\"", Color::YELLOW));
  }
  void genSymbols4() {
    Tile::addSymbol(ViewId("amulet2"), symbol(u8"\"", Color::YELLOW));
    Tile::addSymbol(ViewId("amulet3"), symbol(u8"\"", Color::YELLOW));
    Tile::addSymbol(ViewId("amulet4"), symbol(u8"\"", Color::YELLOW));
    Tile::addSymbol(ViewId("amulet5"), symbol(u8"\"", Color::YELLOW));
    Tile::addSymbol(ViewId("fire_resist_ring"), symbol(u8"=", Color::RED));
    Tile::addSymbol(ViewId("poison_resist_ring"), symbol(u8"=", Color::GREEN));
    Tile::addSymbol(ViewId("book"), symbol(u8"+", Color::YELLOW));
    Tile::addSymbol(ViewId("hand_torch"), symbol(u8"/", Color::YELLOW));
    Tile::addSymbol(ViewId("first_aid"), symbol(u8"+", Color::RED));
    Tile::addSymbol(ViewId("trap_item"), symbol(u8"+", Color::YELLOW));
    Tile::addSymbol(ViewId("potion1"), symbol(u8"!", Color::LIGHT_RED));
    Tile::addSymbol(ViewId("potion2"), symbol(u8"!", Color::BLUE));
    Tile::addSymbol(ViewId("potion3"), symbol(u8"!", Color::YELLOW));
    Tile::addSymbol(ViewId("potion4"), symbol(u8"!", Color::VIOLET));
    Tile::addSymbol(ViewId("potion5"), symbol(u8"!", Color::DARK_BROWN));
    Tile::addSymbol(ViewId("potion6"), symbol(u8"!", Color::LIGHT_GRAY));
    Tile::addSymbol(ViewId("rune1"), symbol(u8"~", Color::GREEN));
    Tile::addSymbol(ViewId("rune2"), symbol(u8"~", Color::PURPLE));
    Tile::addSymbol(ViewId("rune3"), symbol(u8"~", Color::RED));
    Tile::addSymbol(ViewId("rune4"), symbol(u8"~", Color::BLUE));
    Tile::addSymbol(ViewId("mushroom1"), symbol(u8"⋆", Color::PINK, true));
    Tile::addSymbol(ViewId("mushroom2"), symbol(u8"⋆", Color::YELLOW, true));
    Tile::addSymbol(ViewId("mushroom3"), symbol(u8"⋆", Color::PURPLE, true));
    Tile::addSymbol(ViewId("mushroom4"), symbol(u8"⋆", Color::BROWN, true));
    Tile::addSymbol(ViewId("mushroom5"), symbol(u8"⋆", Color::LIGHT_GRAY, true));
    Tile::addSymbol(ViewId("mushroom6"), symbol(u8"⋆", Color::ORANGE, true));
    Tile::addSymbol(ViewId("mushroom7"), symbol(u8"⋆", Color::LIGHT_BLUE, true));
    Tile::addSymbol(ViewId("key"), symbol(u8"*", Color::YELLOW));
    Tile::addSymbol(ViewId("fountain"), symbol(u8"0", Color::LIGHT_BLUE));
    Tile::addSymbol(ViewId("gold"), symbol(u8"$", Color::YELLOW));
    Tile::addSymbol(ViewId("treasure_chest"), symbol(u8"=", Color::BROWN));
    Tile::addSymbol(ViewId("opened_chest"), symbol(u8"=", Color::BROWN));
    Tile::addSymbol(ViewId("chest"), symbol(u8"=", Color::BROWN));
    Tile::addSymbol(ViewId("opened_coffin"), symbol(u8"⚰", Color::DARK_BROWN, true));
    Tile::addSymbol(ViewId("loot_coffin"), symbol(u8"⚰", Color::BROWN, true));
    Tile::addSymbol(ViewId("coffin1"), symbol(u8"⚰", Color::BROWN, true));
    Tile::addSymbol(ViewId("coffin2"), symbol(u8"⚰", Color::GRAY, true));
    Tile::addSymbol(ViewId("coffin3"), symbol(u8"⚰", Color::YELLOW, true));
    Tile::addSymbol(ViewId("boulder"), symbol(u8"●", Color::LIGHT_GRAY, true));
    Tile::addSymbol(ViewId("portal"), symbol(u8"𝚯", Color::WHITE, true));
    Tile::addSymbol(ViewId("gas_trap"), symbol(u8"☠", Color::GREEN, true));
    Tile::addSymbol(ViewId("alarm_trap"), symbol(u8"^", Color::RED, true));
    Tile::addSymbol(ViewId("web_trap"), symbol(u8"#", Color::WHITE, true));
    Tile::addSymbol(ViewId("surprise_trap"), symbol(u8"^", Color::BLUE, true));
    Tile::addSymbol(ViewId("terror_trap"), symbol(u8"^", Color::WHITE, true));
    Tile::addSymbol(ViewId("fire_trap"), symbol(u8"^", Color::RED, true));
    Tile::addSymbol(ViewId("rock"), symbol(u8"✱", Color::LIGHT_GRAY, true));
    Tile::addSymbol(ViewId("iron_rock"), symbol(u8"✱", Color::ORANGE, true));
    Tile::addSymbol(ViewId("ada_ore"), symbol(u8"✱", Color::LIGHT_BLUE, true));
    Tile::addSymbol(ViewId("wood_plank"), symbol(u8"\\", Color::BROWN));
    Tile::addSymbol(ViewId("storage_equipment"), symbol(u8".", Color::GREEN));
    Tile::addSymbol(ViewId("storage_resources"), symbol(u8".", Color::BLUE));
    Tile::addSymbol(ViewId("quarters1"), symbol(u8".", Color::PINK));
    Tile::addSymbol(ViewId("quarters2"), symbol(u8".", Color::SKY_BLUE));
    Tile::addSymbol(ViewId("quarters3"), symbol(u8".", Color::ORANGE));
    Tile::addSymbol(ViewId("leisure"), symbol(u8".", Color::DARK_BLUE));
    Tile::addSymbol(ViewId("prison"), symbol(u8".", Color::BLUE));
    Tile::addSymbol(ViewId("dorm"), symbol(u8".", Color::BROWN));
    Tile::addSymbol(ViewId("bed1"), symbol(u8"=", Color::WHITE));
    Tile::addSymbol(ViewId("bed2"), symbol(u8"=", Color::YELLOW));
    Tile::addSymbol(ViewId("bed3"), symbol(u8"=", Color::PURPLE));
    Tile::addSymbol(ViewId("torch"), symbol(u8"*", Color::YELLOW));
    Tile::addSymbol(ViewId("standing_torch"), symbol(u8"*", Color::YELLOW));
    Tile::addSymbol(ViewId("candelabrum_ns"), symbol(u8"*", Color::ORANGE));
    Tile::addSymbol(ViewId("candelabrum_w"), symbol(u8"*", Color::ORANGE));
    Tile::addSymbol(ViewId("candelabrum_e"), symbol(u8"*", Color::ORANGE));
    Tile::addSymbol(ViewId("altar"), symbol(u8"Ω", Color::WHITE, true));
    Tile::addSymbol(ViewId("altar_des"), symbol(u8"Ω", Color::RED, true));
    Tile::addSymbol(ViewId("creature_altar"), symbol(u8"Ω", Color::YELLOW, true));
    Tile::addSymbol(ViewId("torture_table"), symbol(u8"=", Color::GRAY));
    Tile::addSymbol(ViewId("impaled_head"), symbol(u8"⚲", Color::BROWN, true));
    Tile::addSymbol(ViewId("whipping_post"), symbol(u8"}", Color::BROWN, true));
    Tile::addSymbol(ViewId("gallows"), symbol(u8"}", Color::ORANGE, true));
    Tile::addSymbol(ViewId("notice_board"), symbol(u8"|", Color::BROWN));
    Tile::addSymbol(ViewId("sokoban_hole"), symbol(u8"0", Color::DARK_BLUE));
    Tile::addSymbol(ViewId("training_wood"), symbol(u8"‡", Color::BROWN, true));
    Tile::addSymbol(ViewId("training_iron"), symbol(u8"‡", Color::LIGHT_GRAY, true));
    Tile::addSymbol(ViewId("training_ada"), symbol(u8"‡", Color::LIGHT_BLUE, true));
    Tile::addSymbol(ViewId("archery_range"), symbol(u8"⌾", Color::LIGHT_BLUE, true));
  }
  void genSymbols5() {
    Tile::addSymbol(ViewId("demon_shrine"), symbol(u8"Ω", Color::PURPLE, true));
    Tile::addSymbol(ViewId("bookcase_wood"), symbol(u8"▤", Color::BROWN, true));
    Tile::addSymbol(ViewId("bookcase_iron"), symbol(u8"▤", Color::LIGHT_GRAY, true));
    Tile::addSymbol(ViewId("bookcase_gold"), symbol(u8"▤", Color::YELLOW, true));
    Tile::addSymbol(ViewId("laboratory"), symbol(u8"ω", Color::PURPLE, true));
    Tile::addSymbol(ViewId("cauldron"), symbol(u8"ω", Color::PURPLE, true));
    Tile::addSymbol(ViewId("beast_lair"), symbol(u8".", Color::YELLOW));
    Tile::addSymbol(ViewId("beast_cage"), symbol(u8"▥", Color::LIGHT_GRAY, true));
    Tile::addSymbol(ViewId("workshop"), symbol(u8"&", Color::LIGHT_BROWN));
    Tile::addSymbol(ViewId("forge"), symbol(u8"&", Color::LIGHT_BLUE));
    Tile::addSymbol(ViewId("jeweller"), symbol(u8"&", Color::YELLOW));
    Tile::addSymbol(ViewId("furnace"), symbol(u8"&", Color::PINK));
    Tile::addSymbol(ViewId("grave"), symbol(u8"☗", Color::GRAY, true));
    Tile::addSymbol(ViewId("border_guard"), symbol(u8" ", Color::BLACK));
    Tile::addSymbol(ViewId("robe"), symbol(u8"[", Color::LIGHT_BROWN));
    Tile::addSymbol(ViewId("leather_gloves"), symbol(u8"[", Color::BROWN));
    Tile::addSymbol(ViewId("iron_gloves"), symbol(u8"[", Color::LIGHT_GRAY));
    Tile::addSymbol(ViewId("ada_gloves"), symbol(u8"[", Color::LIGHT_BLUE));
    Tile::addSymbol(ViewId("strength_gloves"), symbol(u8"[", Color::RED));
    Tile::addSymbol(ViewId("dexterity_gloves"), symbol(u8"[", Color::LIGHT_BLUE));
    Tile::addSymbol(ViewId("leather_armor"), symbol(u8"[", Color::BROWN));
    Tile::addSymbol(ViewId("leather_helm"), symbol(u8"[", Color::BROWN));
    Tile::addSymbol(ViewId("telepathy_helm"), symbol(u8"[", Color::LIGHT_GREEN));
    Tile::addSymbol(ViewId("chain_armor"), symbol(u8"[", Color::LIGHT_GRAY));
    Tile::addSymbol(ViewId("ada_armor"), symbol(u8"[", Color::LIGHT_BLUE));
    Tile::addSymbol(ViewId("iron_helm"), symbol(u8"[", Color::LIGHT_GRAY));
    Tile::addSymbol(ViewId("ada_helm"), symbol(u8"[", Color::LIGHT_BLUE));
    Tile::addSymbol(ViewId("leather_boots"), symbol(u8"[", Color::BROWN));
    Tile::addSymbol(ViewId("iron_boots"), symbol(u8"[", Color::LIGHT_GRAY));
    Tile::addSymbol(ViewId("ada_boots"), symbol(u8"[", Color::LIGHT_BLUE));
    Tile::addSymbol(ViewId("speed_boots"), symbol(u8"[", Color::LIGHT_BLUE));
    Tile::addSymbol(ViewId("levitation_boots"), symbol(u8"[", Color::LIGHT_GREEN));
    Tile::addSymbol(ViewId("wooden_shield"), symbol(u8"[", Color::LIGHT_BROWN));
    Tile::addSymbol(ViewId("fallen_tree"), symbol(u8"*", Color::GREEN));
    Tile::addSymbol(ViewId("guard_post"), symbol(u8"⚐", Color::YELLOW, true));
    Tile::addSymbol(ViewId("destroy_button"), symbol(u8"X", Color::RED));
    Tile::addSymbol(ViewId("forbid_zone"), symbol(u8"#", Color::RED));
    Tile::addSymbol(ViewId("mana"), symbol(u8"✱", Color::BLUE, true));
    Tile::addSymbol(ViewId("eyeball"), symbol(u8"e", Color::BLUE));
    Tile::addSymbol(ViewId("fetch_icon"), symbol(u8"👋", Color::LIGHT_BROWN, true));
    Tile::addSymbol(ViewId("fog_of_war"), symbol(u8" ", Color::WHITE));
    Tile::addSymbol(ViewId("pit"), symbol(u8"^", Color::WHITE));
    Tile::addSymbol(ViewId("creature_highlight"), symbol(u8" ", Color::WHITE));
    Tile::addSymbol(ViewId("square_highlight"), symbol(u8"⛶", Color::WHITE, true));
    Tile::addSymbol(ViewId("round_shadow"), symbol(u8" ", Color::WHITE));
    Tile::addSymbol(ViewId("campaign_defeated"), symbol(u8"X", Color::RED));
    Tile::addSymbol(ViewId("fog_of_war_corner"), symbol(u8" ", Color::WHITE));
    Tile::addSymbol(ViewId("accept_immigrant"), symbol(u8"✓", Color::GREEN, true));
    Tile::addSymbol(ViewId("reject_immigrant"), symbol(u8"✘", Color::RED, true));
    Tile::addSymbol(ViewId("special_blbn"), symbol(u8"B", Color::PURPLE));
    Tile::addSymbol(ViewId("special_blbw"), symbol(u8"B", Color::LIGHT_RED));
    Tile::addSymbol(ViewId("special_blgn"), symbol(u8"B", Color::LIGHT_GRAY));
    Tile::addSymbol(ViewId("special_blgw"), symbol(u8"B", Color::WHITE));
    Tile::addSymbol(ViewId("special_bmbn"), symbol(u8"B", Color::YELLOW));
    Tile::addSymbol(ViewId("special_bmbw"), symbol(u8"B", Color::ORANGE));
    Tile::addSymbol(ViewId("special_bmgn"), symbol(u8"B", Color::GREEN));
    Tile::addSymbol(ViewId("special_bmgw"), symbol(u8"B", Color::LIGHT_GREEN));
    Tile::addSymbol(ViewId("special_hlbn"), symbol(u8"H", Color::PURPLE));
    Tile::addSymbol(ViewId("special_hlbw"), symbol(u8"H", Color::LIGHT_RED));
    Tile::addSymbol(ViewId("special_hlgn"), symbol(u8"H", Color::LIGHT_GRAY));
    Tile::addSymbol(ViewId("special_hlgw"), symbol(u8"H", Color::WHITE));
    Tile::addSymbol(ViewId("special_hmbn"), symbol(u8"H", Color::YELLOW));
    Tile::addSymbol(ViewId("special_hmbw"), symbol(u8"H", Color::ORANGE));
    Tile::addSymbol(ViewId("special_hmgn"), symbol(u8"H", Color::GREEN));
    Tile::addSymbol(ViewId("special_hmgw"), symbol(u8"H", Color::LIGHT_GREEN));
    Tile::addSymbol(ViewId("halloween_kid1"), symbol(u8"@", Color::PINK));
    Tile::addSymbol(ViewId("halloween_kid2"), symbol(u8"@", Color::PURPLE));
    Tile::addSymbol(ViewId("halloween_kid3"), symbol(u8"@", Color::BLUE));
    Tile::addSymbol(ViewId("halloween_kid4"), symbol(u8"@", Color::YELLOW));
    Tile::addSymbol(ViewId("halloween_costume"), symbol(u8"[", Color::PINK));
    Tile::addSymbol(ViewId("bag_of_candy"), symbol(u8"*", Color::BLUE));
    Tile::addSymbol(ViewId("tutorial_entrance"), symbol(u8" ", Color::LIGHT_GREEN));
    Tile::addSymbol(ViewId("horn_attack"), symbol(u8" ", Color::PINK));
    Tile::addSymbol(ViewId("beak_attack"), symbol(u8" ", Color::YELLOW));
    Tile::addSymbol(ViewId("touch_attack"), symbol(u8" ", Color::WHITE));
    Tile::addSymbol(ViewId("bite_attack"), symbol(u8" ", Color::RED));
    Tile::addSymbol(ViewId("claws_attack"), symbol(u8" ", Color::BROWN));
    Tile::addSymbol(ViewId("leg_attack"), symbol(u8" ", Color::GRAY));
    Tile::addSymbol(ViewId("fist_attack"), symbol(u8" ", Color::ORANGE));
    Tile::addSymbol(ViewId("item_aura"), symbol(u8" ", Color::ORANGE));
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
  if (sprite && tiles.count(id))
    return tiles.at(id);
  else
    return symbols.at(id);
}

Color Tile::getColor(const ViewObject& object) {
  if (object.hasModifier(ViewObject::Modifier::INVISIBLE))
    return Color::DARK_GRAY;
  if (object.hasModifier(ViewObject::Modifier::HIDDEN))
    return Color::LIGHT_GRAY;
  Color color = symbols.at(object.id()).color;
  if (object.hasModifier(ViewObject::Modifier::PLANNED))
    return Color(color.r / 2, color.g / 2, color.b / 2);
  return color;
}

Tile Tile::addCorner(DirSet corner, DirSet borders, vector<TileCoord> coord) {
  anyCorners = true;
  for (DirSet dirs : Range(0, 255))
    if ((dirs & corner) == borders)
      corners[dirs].push_back(coord.getOnlyElement());
  return *this;
}

bool Tile::hasAnyCorners() const {
  return anyCorners;
}

const vector<Tile::TileCoord>& Tile::getCornerCoords(DirSet c) const {
  return corners[c];
}

const optional<FXVariantName> Tile::getFX() const {
  return fx;
}

