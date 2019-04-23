#include "tileset.h"
#include "view_id.h"
#include "tile.h"
#include "view_object.h"

void TileSet::addTile(ViewId id, Tile tile) {
  tiles.insert(make_pair(id, std::move(tile)));
}

void TileSet::addSymbol(ViewId id, Tile tile) {
  symbols.insert(make_pair(std::move(id), std::move(tile)));
}

Color TileSet::getColor(const ViewObject& object) const {
  if (object.hasModifier(ViewObject::Modifier::INVISIBLE))
    return Color::DARK_GRAY;
  if (object.hasModifier(ViewObject::Modifier::HIDDEN))
    return Color::LIGHT_GRAY;
  Color color = symbols.at(object.id()).color;
  if (object.hasModifier(ViewObject::Modifier::PLANNED))
    return Color(color.r / 2, color.g / 2, color.b / 2);
  return color;
}

class TileCoordLookup {
  public:
  TileCoordLookup(Renderer& r) : renderer(r) {}

  void loadTiles(TileSet& tileSet) {
    genTiles1(tileSet);
    genTiles2(tileSet);
    genTiles3(tileSet);
    genTiles4(tileSet);
    genTiles5(tileSet);
  }

  void loadUnicode(TileSet& tileSet) {
    genSymbols1(tileSet);
    genSymbols2(tileSet);
    genSymbols3(tileSet);
    genSymbols4(tileSet);
    genSymbols5(tileSet);
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

  void genTiles1(TileSet& tileSet) {
    tileSet.addTile(ViewId("unknown_monster"), sprite("unknown"));
    tileSet.addTile(ViewId("dig_mark"), sprite("dig_mark"));
    tileSet.addTile(ViewId("dig_mark2"), sprite("dig_mark2"));
    tileSet.addTile(ViewId("forbid_zone"), sprite("dig_mark").setColor(Color::RED));
    tileSet.addTile(ViewId("destroy_button"), sprite("remove"));
    tileSet.addTile(ViewId("empty"), empty());
    tileSet.addTile(ViewId("border_guard"), empty());
    tileSet.addTile(ViewId("demon_dweller"), sprite("demon_dweller"));
    tileSet.addTile(ViewId("demon_lord"), sprite("demon_lord"));
    tileSet.addTile(ViewId("vampire"), sprite("vampire"));
    tileSet.addTile(ViewId("fallen_tree"), sprite("treecut"));
    tileSet.addTile(ViewId("decid_tree"), sprite("tree2").setRoundShadow());
    tileSet.addTile(ViewId("canif_tree"), sprite("tree1").setRoundShadow());
    tileSet.addTile(ViewId("tree_trunk"), sprite("treecut"));
    tileSet.addTile(ViewId("unicorn"), sprite("unicorn"));
    tileSet.addTile(ViewId("burnt_tree"), sprite("treeburnt")
        .setRoundShadow());
    tileSet.addTile(ViewId("player"), sprite("adventurer"));
    tileSet.addTile(ViewId("player_f"), sprite("adventurer_female"));
    tileSet.addTile(ViewId("keeper1"), sprite("keeper1"));
    tileSet.addTile(ViewId("keeper2"), sprite("keeper2"));
    tileSet.addTile(ViewId("keeper3"), sprite("keeper3"));
    tileSet.addTile(ViewId("keeper4"), sprite("keeper4"));
    tileSet.addTile(ViewId("keeper_f1"), sprite("keeper_female1"));
    tileSet.addTile(ViewId("keeper_f2"), sprite("keeper_female2"));
    tileSet.addTile(ViewId("keeper_f3"), sprite("keeper_female3"));
    tileSet.addTile(ViewId("keeper_f4"), sprite("keeper_female4"));
    tileSet.addTile(ViewId("keeper_knight1"), sprite("keeper_knight1"));
    tileSet.addTile(ViewId("keeper_knight2"), sprite("keeper_knight2"));
    tileSet.addTile(ViewId("keeper_knight3"), sprite("keeper_knight3"));
    tileSet.addTile(ViewId("keeper_knight4"), sprite("keeper_knight4"));
    tileSet.addTile(ViewId("keeper_knight_f1"), sprite("keeper_knight_female1"));
    tileSet.addTile(ViewId("keeper_knight_f2"), sprite("keeper_knight_female2"));
    tileSet.addTile(ViewId("keeper_knight_f3"), sprite("keeper_knight_female3"));
    tileSet.addTile(ViewId("keeper_knight_f4"), sprite("keeper_knight_female4"));
    tileSet.addTile(ViewId("elf"), sprite("elf male"));
    tileSet.addTile(ViewId("elf_woman"), sprite("elf female"));
    tileSet.addTile(ViewId("elf_archer"), sprite("elf archer"));
    tileSet.addTile(ViewId("elf_child"), sprite("elf kid"));
    tileSet.addTile(ViewId("elf_lord"), sprite("elf lord"));
    tileSet.addTile(ViewId("dark_elf"), sprite("dark_elf"));
    tileSet.addTile(ViewId("dark_elf_woman"), sprite("dark_elf_female"));
    tileSet.addTile(ViewId("dark_elf_warrior"), sprite("dark_elf_warrior"));
    tileSet.addTile(ViewId("dark_elf_child"), sprite("dark_elf_kid"));
    tileSet.addTile(ViewId("dark_elf_lord"), sprite("dark_elf_lord"));
    tileSet.addTile(ViewId("driad"), sprite("driad"));
    tileSet.addTile(ViewId("kobold"), sprite("kobold"));
    tileSet.addTile(ViewId("lizardman"), sprite("lizardman"));
    tileSet.addTile(ViewId("lizardlord"), sprite("lizardlord"));
    tileSet.addTile(ViewId("imp"), sprite("imp"));
    tileSet.addTile(ViewId("prisoner"), sprite("prisoner"));
    tileSet.addTile(ViewId("ogre"), sprite("troll"));
    tileSet.addTile(ViewId("chicken"), sprite("hen"));
  }
  void genTiles2(TileSet& tileSet) {
    tileSet.addTile(ViewId("gnome"), sprite("gnome"));
    tileSet.addTile(ViewId("gnome_boss"), sprite("gnomeboss"));
    tileSet.addTile(ViewId("dwarf"), sprite("dwarf"));
    tileSet.addTile(ViewId("dwarf_baron"), sprite("dwarfboss"));
    tileSet.addTile(ViewId("dwarf_female"), sprite("dwarf_f"));
    tileSet.addTile(ViewId("shopkeeper"), sprite("salesman"));
    tileSet.addTile(ViewId("bridge"), sprite("bridge").addOption(Dir::S, byName("bridge2")));
    tileSet.addTile(ViewId("road"), getRoadTile("road"));
    tileSet.addTile(ViewId("floor"), sprite("floor"));
    tileSet.addTile(ViewId("keeper_floor"), sprite("floor_keeper"));
    tileSet.addTile(ViewId("wood_floor1"), sprite("floor_wood1"));
    tileSet.addTile(ViewId("wood_floor2"), sprite("floor_wood2"));
    tileSet.addTile(ViewId("wood_floor3"), sprite("floor_wood3"));
    tileSet.addTile(ViewId("wood_floor4"), sprite("floor_wood4"));
    tileSet.addTile(ViewId("wood_floor5"), sprite("floor_wood5"));
    tileSet.addTile(ViewId("stone_floor1"), sprite("floor_stone1"));
    tileSet.addTile(ViewId("stone_floor2"), sprite("floor_stone2"));
    tileSet.addTile(ViewId("stone_floor3"), sprite("floor_stone3"));
    tileSet.addTile(ViewId("stone_floor4"), sprite("floor_stone4"));
    tileSet.addTile(ViewId("stone_floor5"), sprite("floor_stone5"));
    tileSet.addTile(ViewId("carpet_floor1"), sprite("floor_carpet1"));
    tileSet.addTile(ViewId("carpet_floor2"), sprite("floor_carpet2"));
    tileSet.addTile(ViewId("carpet_floor3"), sprite("floor_carpet3"));
    tileSet.addTile(ViewId("carpet_floor4"), sprite("floor_carpet4"));
    tileSet.addTile(ViewId("carpet_floor5"), sprite("floor_carpet5"));
    tileSet.addTile(ViewId("buff_floor1"), sprite("floor_buff1"));
    tileSet.addTile(ViewId("buff_floor2"), sprite("floor_buff2"));
    tileSet.addTile(ViewId("buff_floor3"), sprite("floor_buff3"));
    tileSet.addTile(ViewId("sand"), getExtraBorderTile("sand")
        .addExtraBorderId(ViewId("water")));
    tileSet.addTile(ViewId("mud"), getExtraBorderTile("mud")
        .addExtraBorderId(ViewId("water"))
        .addExtraBorderId(ViewId("hill"))
        .addExtraBorderId(ViewId("sand")));
    tileSet.addTile(ViewId("grass"), getExtraBorderTile("grass")
        .addExtraBorderId(ViewId("sand"))
        .addExtraBorderId(ViewId("hill"))
        .addExtraBorderId(ViewId("mud"))
        .addExtraBorderId(ViewId("water")));
    tileSet.addTile(ViewId("crops"), sprite("wheatfield1"));
    tileSet.addTile(ViewId("crops2"), sprite("wheatfield2"));
    tileSet.addTile(ViewId("mountain"), getMountainTile(sprite("mountain_ted"), "mountain").setWallShadow());
    tileSet.addTile(ViewId("mountain2"), getMountainTile(sprite("mountain_ted2"), "mountain").setWallShadow());
    tileSet.addTile(ViewId("dungeon_wall"), getMountainTile(sprite("mountain_ted"), "dungeonwall").setWallShadow());
    tileSet.addTile(ViewId("dungeon_wall2"), getMountainTile(sprite("mountain_ted2"), "dungeonwall").setWallShadow());
    tileSet.addTile(ViewId("wall"), getWallTile("wall"));
    tileSet.addTile(ViewId("map_mountain1"), sprite("map_mountain1"));
    tileSet.addTile(ViewId("map_mountain2"), sprite("map_mountain2"));
    tileSet.addTile(ViewId("map_mountain3"), sprite("map_mountain3"));
  }

  void genTiles3(TileSet& tileSet) {
    tileSet.addTile(ViewId("gold_ore"), getMountainTile(sprite("gold_ore")
          .addBackground(byName("mountain_ted2")).setWallShadow(), "mountain"));
    tileSet.addTile(ViewId("iron_ore"), getMountainTile(sprite("iron_ore")
          .addBackground(byName("mountain_ted2")).setWallShadow(), "mountain"));
    tileSet.addTile(ViewId("stone"), getMountainTile(sprite("stone_ore")
          .addBackground(byName("mountain_ted2")).setWallShadow(), "mountain"));
    tileSet.addTile(ViewId("adamantium_ore"), getMountainTile(sprite("adamantium")
          .addBackground(byName("mountain_ted2")).setWallShadow(), "mountain"));
    tileSet.addTile(ViewId("hill"), getExtraBorderTile("hill")
        .addExtraBorderId(ViewId("sand"))
        .addExtraBorderId(ViewId("floor"))
        .addExtraBorderId(ViewId("keeper_floor"))
        .addExtraBorderId(ViewId("water")));
    tileSet.addTile(ViewId("wood_wall"), getWallTile("wood_wall").setWallShadow());
    tileSet.addTile(ViewId("castle_wall"), getWallTile("castle_wall").setWallShadow());
    tileSet.addTile(ViewId("mud_wall"), getWallTile("mud_wall").setWallShadow());
    tileSet.addTile(ViewId("ruin_wall"), getWallTile("ruin_wall"));
    tileSet.addTile(ViewId("down_staircase"), sprite("down_stairs"));
    tileSet.addTile(ViewId("up_staircase"), sprite("up_stairs"));
    tileSet.addTile(ViewId("well"), sprite("well")
        .setRoundShadow());
    tileSet.addTile(ViewId("minion_statue"), sprite("statue")
        .setRoundShadow());
    tileSet.addTile(ViewId("stone_minion_statue"), sprite("statue_stone")
        .setRoundShadow());
    tileSet.addTile(ViewId("throne"), sprite("throne").setRoundShadow());
    tileSet.addTile(ViewId("orc"), sprite("orc"));
    tileSet.addTile(ViewId("orc_captain"), sprite("orc_captain"));
    tileSet.addTile(ViewId("orc_shaman"), sprite("orcshaman"));
    tileSet.addTile(ViewId("harpy"), sprite("harpy"));
    tileSet.addTile(ViewId("doppleganger"), sprite("dopple"));
    tileSet.addTile(ViewId("succubus"), sprite("succubussmall"));
    tileSet.addTile(ViewId("bandit"), sprite("bandit"));
    tileSet.addTile(ViewId("ghost"), sprite("ghost4"));
    tileSet.addTile(ViewId("spirit"), sprite("spirit"));
    tileSet.addTile(ViewId("green_dragon"), sprite("greendragon"));
    tileSet.addTile(ViewId("red_dragon"), sprite("reddragon"));
    tileSet.addTile(ViewId("cyclops"), sprite("cyclops"));
    tileSet.addTile(ViewId("minotaur"), sprite("mino"));
    tileSet.addTile(ViewId("soft_monster"), sprite("softmonster"));
    tileSet.addTile(ViewId("hydra"), sprite("hydra"));
    tileSet.addTile(ViewId("shelob"), sprite("szelob"));
    tileSet.addTile(ViewId("witch"), sprite("witch"));
    tileSet.addTile(ViewId("witchman"), sprite("witchman"));
    tileSet.addTile(ViewId("knight"), sprite("knight"));
    tileSet.addTile(ViewId("jester"), sprite("jester"));
    tileSet.addTile(ViewId("priest"), sprite("priest"));
    tileSet.addTile(ViewId("warrior"), sprite("warrior"));
    tileSet.addTile(ViewId("shaman"), sprite("shaman"));
    tileSet.addTile(ViewId("duke1"), sprite("knightboss1"));
    tileSet.addTile(ViewId("duke2"), sprite("knightboss2"));
    tileSet.addTile(ViewId("duke3"), sprite("knightboss3"));
    tileSet.addTile(ViewId("duke4"), sprite("knightboss4"));
    tileSet.addTile(ViewId("duke_f1"), sprite("knightboss_f1"));
    tileSet.addTile(ViewId("duke_f2"), sprite("knightboss_f2"));
    tileSet.addTile(ViewId("duke_f3"), sprite("knightboss_f3"));
    tileSet.addTile(ViewId("duke_f4"), sprite("knightboss_f4"));
    tileSet.addTile(ViewId("archer"), sprite("archer"));
    tileSet.addTile(ViewId("peseant"), sprite("peasant"));
    tileSet.addTile(ViewId("peseant_woman"), sprite("peasantgirl"));
    tileSet.addTile(ViewId("child"), sprite("peasantkid"));
    tileSet.addTile(ViewId("clay_golem"), sprite("clay_golem"));
    tileSet.addTile(ViewId("stone_golem"), sprite("stone_golem"));
    tileSet.addTile(ViewId("iron_golem"), sprite("iron_golem"));
    tileSet.addTile(ViewId("lava_golem"), sprite("lava_golem"));
    tileSet.addTile(ViewId("ada_golem"), sprite("ada_golem"));
    tileSet.addTile(ViewId("automaton"), sprite("automaton"));
    tileSet.addTile(ViewId("elementalist"), sprite("elementalist"));
    tileSet.addTile(ViewId("air_elemental"), sprite("airelement"));
    tileSet.addTile(ViewId("fire_elemental"), sprite("fireelement"));
    tileSet.addTile(ViewId("water_elemental"), sprite("waterelement"));
    tileSet.addTile(ViewId("earth_elemental"), sprite("earthelement"));
    tileSet.addTile(ViewId("ent"), sprite("ent"));
    tileSet.addTile(ViewId("angel"), sprite("angel"));
    tileSet.addTile(ViewId("zombie"), sprite("zombie"));
    tileSet.addTile(ViewId("skeleton"), sprite("skeleton"));
    tileSet.addTile(ViewId("vampire_lord"), sprite("vampirelord"));
    tileSet.addTile(ViewId("mummy"), sprite("mummy"));
    tileSet.addTile(ViewId("jackal"), sprite("jackal"));
    tileSet.addTile(ViewId("deer"), sprite("deer"));
    tileSet.addTile(ViewId("horse"), sprite("horse"));
    tileSet.addTile(ViewId("cow"), sprite("cow"));
    tileSet.addTile(ViewId("pig"), sprite("pig"));
    tileSet.addTile(ViewId("donkey"), sprite("donkey"));
    tileSet.addTile(ViewId("goat"), sprite("goat"));
    tileSet.addTile(ViewId("boar"), sprite("boar"));
    tileSet.addTile(ViewId("fox"), sprite("fox"));
    tileSet.addTile(ViewId("wolf"), sprite("wolf"));
    tileSet.addTile(ViewId("werewolf"), sprite("werewolf2"));
    tileSet.addTile(ViewId("dog"), sprite("dog"));
    tileSet.addTile(ViewId("kraken_head"), sprite("krakenhead"));
    tileSet.addTile(ViewId("kraken_land"), sprite("krakenland1"));
    tileSet.addTile(ViewId("kraken_water"), sprite("krakenwater2"));
    tileSet.addTile(ViewId("death"), sprite("death"));
    tileSet.addTile(ViewId("fire_sphere"), sprite("fire_sphere").setFX(FXVariantName::FIRE_SPHERE));
    tileSet.addTile(ViewId("bear"), sprite("bear"));
    tileSet.addTile(ViewId("bat"), sprite("bat"));
    tileSet.addTile(ViewId("goblin"), sprite("goblin"));
    tileSet.addTile(ViewId("rat"), sprite("rat"));
    tileSet.addTile(ViewId("rat_soldier"), sprite("rat_soldier"));
    tileSet.addTile(ViewId("rat_lady"), sprite("rat_lady"));
    tileSet.addTile(ViewId("rat_king"), sprite("rat_king"));
    tileSet.addTile(ViewId("spider"), sprite("spider"));
    tileSet.addTile(ViewId("fly"), sprite("fly"));
    tileSet.addTile(ViewId("ant_worker"), sprite("antwork"));
    tileSet.addTile(ViewId("ant_soldier"), sprite("antw"));
    tileSet.addTile(ViewId("ant_queen"), sprite("antq"));
    tileSet.addTile(ViewId("snake"), sprite("snake"));
  }
  void genTiles4(TileSet& tileSet) {
    tileSet.addTile(ViewId("vulture"), sprite("vulture"));
    tileSet.addTile(ViewId("raven"), sprite("raven"));
    tileSet.addTile(ViewId("body_part"), sprite("corpse4"));
    tileSet.addTile(ViewId("bone"), sprite("bone"));
    tileSet.addTile(ViewId("bush"), sprite("bush").setRoundShadow());
    tileSet.addTile(ViewId("water"), getWaterTile("wateranim", "water"));
    tileSet.addTile(ViewId("magma"), getWaterTile("magmaanim", "magma"));
    tileSet.addTile(ViewId("wood_door"), sprite("door_wood").setWallShadow());
    tileSet.addTile(ViewId("iron_door"), sprite("door_iron").setWallShadow());
    tileSet.addTile(ViewId("ada_door"), sprite("door_steel").setWallShadow());
    tileSet.addTile(ViewId("barricade"), sprite("barricade").setRoundShadow());
    tileSet.addTile(ViewId("dig_icon"), sprite("dig_icon"));
    tileSet.addTile(ViewId("sword"), sprite("sword"));
    tileSet.addTile(ViewId("ada_sword"), sprite("steel_sword"));
    tileSet.addTile(ViewId("spear"), sprite("spear"));
    tileSet.addTile(ViewId("special_sword"), sprite("special_sword"));
    tileSet.addTile(ViewId("elven_sword"), sprite("elven_sword"));
    tileSet.addTile(ViewId("knife"), sprite("knife"));
    tileSet.addTile(ViewId("war_hammer"), sprite("warhammer"));
    tileSet.addTile(ViewId("ada_war_hammer"), sprite("ada_warhammer"));
    tileSet.addTile(ViewId("special_war_hammer"), sprite("special_warhammer"));
    tileSet.addTile(ViewId("battle_axe"), sprite("battle_axe"));
    tileSet.addTile(ViewId("ada_battle_axe"), sprite("steel_battle_axe"));
    tileSet.addTile(ViewId("special_battle_axe"), sprite("special_battle_axe"));
    tileSet.addTile(ViewId("bow"), sprite("bow"));
    tileSet.addTile(ViewId("elven_bow"), sprite("bow")); //For now
    tileSet.addTile(ViewId("arrow"), sprite("arrow_e"));
    tileSet.addTile(ViewId("wooden_staff"), sprite("staff_wooden"));
    tileSet.addTile(ViewId("iron_staff"), sprite("staff_iron"));
    tileSet.addTile(ViewId("golden_staff"), sprite("staff_gold"));
    tileSet.addTile(ViewId("force_bolt"), sprite("force_bolt"));
    tileSet.addTile(ViewId("fireball"), sprite("fireball"));
    tileSet.addTile(ViewId("air_blast"), sprite("air_blast"));
    tileSet.addTile(ViewId("stun_ray"), sprite("stun_ray"));
    tileSet.addTile(ViewId("club"), sprite("club"));
    tileSet.addTile(ViewId("heavy_club"), sprite("heavy club"));
    tileSet.addTile(ViewId("scroll"), sprite("scroll"));
    tileSet.addTile(ViewId("amulet1"), sprite("amulet1"));
    tileSet.addTile(ViewId("amulet2"), sprite("amulet2"));
    tileSet.addTile(ViewId("amulet3"), sprite("amulet3"));
    tileSet.addTile(ViewId("amulet4"), sprite("amulet4"));
    tileSet.addTile(ViewId("amulet5"), sprite("amulet5"));
    tileSet.addTile(ViewId("fire_resist_ring"), sprite("ring_red"));
    tileSet.addTile(ViewId("poison_resist_ring"), sprite("ring_green"));
    tileSet.addTile(ViewId("book"), sprite("book"));
    tileSet.addTile(ViewId("hand_torch"), sprite("hand_torch"));
    tileSet.addTile(ViewId("first_aid"), sprite("medkit"));
    tileSet.addTile(ViewId("trap_item"), sprite("trapbox"));
    tileSet.addTile(ViewId("potion1"), sprite("potion1"));
    tileSet.addTile(ViewId("potion2"), sprite("potion2"));
    tileSet.addTile(ViewId("potion3"), sprite("potion3"));
    tileSet.addTile(ViewId("potion4"), sprite("potion4"));
    tileSet.addTile(ViewId("potion5"), sprite("potion5"));
    tileSet.addTile(ViewId("potion6"), sprite("potion6"));
    tileSet.addTile(ViewId("rune1"), sprite("rune1"));
    tileSet.addTile(ViewId("rune2"), sprite("rune2"));
    tileSet.addTile(ViewId("rune3"), sprite("rune3"));
    tileSet.addTile(ViewId("rune4"), sprite("rune4"));
    tileSet.addTile(ViewId("mushroom1"), sprite("mushroom1").setRoundShadow());
    tileSet.addTile(ViewId("mushroom2"), sprite("mushroom2").setRoundShadow());
    tileSet.addTile(ViewId("mushroom3"), sprite("mushroom3").setRoundShadow());
    tileSet.addTile(ViewId("mushroom4"), sprite("mushroom4").setRoundShadow());
    tileSet.addTile(ViewId("mushroom5"), sprite("mushroom5").setRoundShadow());
    tileSet.addTile(ViewId("mushroom6"), sprite("mushroom6").setRoundShadow());
    tileSet.addTile(ViewId("mushroom7"), sprite("mushroom2").setRoundShadow().setColor(Color::LIGHT_BLUE));
    tileSet.addTile(ViewId("key"), sprite("key"));
    tileSet.addTile(ViewId("fountain"), sprite("fountain").setRoundShadow());
    tileSet.addTile(ViewId("gold"), sprite("gold"));
    tileSet.addTile(ViewId("treasure_chest"), sprite("treasurydeco"));
    tileSet.addTile(ViewId("chest"), sprite("chest").setRoundShadow());
    tileSet.addTile(ViewId("opened_chest"), sprite("chest_opened").setRoundShadow());
    tileSet.addTile(ViewId("coffin1"), sprite("coffin"));
    tileSet.addTile(ViewId("coffin2"), sprite("coffin2"));
    tileSet.addTile(ViewId("coffin3"), sprite("coffin3"));
    tileSet.addTile(ViewId("loot_coffin"), sprite("coffin"));
    tileSet.addTile(ViewId("opened_coffin"), sprite("coffin_opened"));
    tileSet.addTile(ViewId("boulder"), sprite("boulder").setRoundShadow());
    tileSet.addTile(ViewId("portal"), sprite("surprise").setRoundShadow());
    tileSet.addTile(ViewId("gas_trap"), sprite("gas_trap"));
    tileSet.addTile(ViewId("alarm_trap"), sprite("alarm_trap"));
    tileSet.addTile(ViewId("web_trap"), sprite("web_trap"));
    tileSet.addTile(ViewId("surprise_trap"), sprite("surprisetrap"));
    tileSet.addTile(ViewId("terror_trap"), sprite("terror_trap"));
    tileSet.addTile(ViewId("fire_trap"), sprite("fire_trap"));
    tileSet.addTile(ViewId("rock"), sprite("stonepile"));
    tileSet.addTile(ViewId("iron_rock"), sprite("ironpile"));
    tileSet.addTile(ViewId("ada_ore"), sprite("steelpile"));
    tileSet.addTile(ViewId("wood_plank"), sprite("wood2"));
    tileSet.addTile(ViewId("storage_equipment"), sprite("dig_mark").setColor(Color::BLUE.transparency(120)));
    tileSet.addTile(ViewId("storage_resources"), sprite("dig_mark").setColor(Color::GREEN.transparency(120)));
    tileSet.addTile(ViewId("quarters1"), sprite("dig_mark2").setColor(Color::PINK.transparency(120)));
    tileSet.addTile(ViewId("quarters2"), sprite("dig_mark2").setColor(Color::SKY_BLUE.transparency(120)));
    tileSet.addTile(ViewId("quarters3"), sprite("dig_mark2").setColor(Color::ORANGE.transparency(120)));
    tileSet.addTile(ViewId("leisure"), sprite("dig_mark2").setColor(Color::DARK_BLUE.transparency(120)));
    tileSet.addTile(ViewId("prison"), sprite("prison"));
    tileSet.addTile(ViewId("bed1"), sprite("bed1").setRoundShadow());
    tileSet.addTile(ViewId("bed2"), sprite("bed2").setRoundShadow());
    tileSet.addTile(ViewId("bed3"), sprite("bed3").setRoundShadow());
    tileSet.addTile(ViewId("dorm"), sprite("sleep").setFloorBorders());
    tileSet.addTile(ViewId("torch"), sprite("torch"));
    tileSet.addTile(ViewId("candelabrum_ns"), sprite("candelabrum_ns"));
    tileSet.addTile(ViewId("candelabrum_w"), sprite("candelabrum_w"));
    tileSet.addTile(ViewId("candelabrum_e"), sprite("candelabrum_e"));
    tileSet.addTile(ViewId("standing_torch"), sprite("standing_torch").setMoveUp());
    tileSet.addTile(ViewId("altar"), sprite("altar").setRoundShadow());
  }
  void genTiles5(TileSet& tileSet) {
    tileSet.addTile(ViewId("creature_altar"), sprite("altar2").setRoundShadow());
    tileSet.addTile(ViewId("altar_des"), sprite("altar_des").setRoundShadow());
    tileSet.addTile(ViewId("torture_table"), sprite("torturedeco").setRoundShadow());
    tileSet.addTile(ViewId("impaled_head"), sprite("impaledhead").setRoundShadow());
    tileSet.addTile(ViewId("whipping_post"), sprite("whipping_post").setRoundShadow());
    tileSet.addTile(ViewId("gallows"), sprite("gallows").setRoundShadow());
    tileSet.addTile(ViewId("notice_board"), sprite("board").setRoundShadow());
    tileSet.addTile(ViewId("sokoban_hole"), sprite("hole"));
    tileSet.addTile(ViewId("training_wood"), sprite("train_wood").setRoundShadow());
    tileSet.addTile(ViewId("training_iron"), sprite("train_iron").setRoundShadow());
    tileSet.addTile(ViewId("training_ada"), sprite("train_steel").setRoundShadow());
    tileSet.addTile(ViewId("archery_range"), sprite("archery_range").setRoundShadow());
    tileSet.addTile(ViewId("demon_shrine"), sprite("demon_shrine").setRoundShadow());
    tileSet.addTile(ViewId("bookcase_wood"), sprite("library_wood").setRoundShadow());
    tileSet.addTile(ViewId("bookcase_iron"), sprite("library_iron").setRoundShadow());
    tileSet.addTile(ViewId("bookcase_gold"), sprite("library_gold").setRoundShadow());
    tileSet.addTile(ViewId("laboratory"), sprite("labdeco").setRoundShadow());
    tileSet.addTile(ViewId("cauldron"), sprite("labdeco"));
    tileSet.addTile(ViewId("beast_lair"), sprite("lair").setFloorBorders());
    tileSet.addTile(ViewId("beast_cage"), sprite("lairdeco").setRoundShadow());
    tileSet.addTile(ViewId("forge"), sprite("forgedeco").setRoundShadow());
    tileSet.addTile(ViewId("workshop"), sprite("workshopdeco").setRoundShadow());
    tileSet.addTile(ViewId("jeweller"), sprite("jewelerdeco").setRoundShadow());
    tileSet.addTile(ViewId("furnace"), sprite("steel_furnace").setRoundShadow());
    tileSet.addTile(ViewId("grave"), sprite("RIP").setRoundShadow());
    tileSet.addTile(ViewId("robe"), sprite("robe"));
    tileSet.addTile(ViewId("leather_gloves"), sprite("leather_gloves"));
    tileSet.addTile(ViewId("iron_gloves"), sprite("iron_gloves"));
    tileSet.addTile(ViewId("ada_gloves"), sprite("ada_gloves"));
    tileSet.addTile(ViewId("dexterity_gloves"), sprite("blue_gloves"));
    tileSet.addTile(ViewId("strength_gloves"), sprite("red_gloves"));
    tileSet.addTile(ViewId("leather_armor"), sprite("leather_armor"));
    tileSet.addTile(ViewId("leather_helm"), sprite("leather_helm"));
    tileSet.addTile(ViewId("telepathy_helm"), sprite("blue_helm"));
    tileSet.addTile(ViewId("chain_armor"), sprite("chain_armor"));
    tileSet.addTile(ViewId("ada_armor"), sprite("steel_armor"));
    tileSet.addTile(ViewId("iron_helm"), sprite("iron_helm"));
    tileSet.addTile(ViewId("ada_helm"), sprite("ada_helm"));
    tileSet.addTile(ViewId("leather_boots"), sprite("leather_boots"));
    tileSet.addTile(ViewId("iron_boots"), sprite("iron_boots"));
    tileSet.addTile(ViewId("ada_boots"), sprite("ada_boots"));
    tileSet.addTile(ViewId("speed_boots"), sprite("blue_boots"));
    tileSet.addTile(ViewId("levitation_boots"), sprite("green_boots"));
    tileSet.addTile(ViewId("wooden_shield"), sprite("wooden_shield"));
    tileSet.addTile(ViewId("guard_post"), sprite("guardroom").setRoundShadow());
    tileSet.addTile(ViewId("mana"), sprite("mana"));
    tileSet.addTile(ViewId("fetch_icon"), sprite("leather_gloves"));
    tileSet.addTile(ViewId("eyeball"), sprite("eyeball2").setRoundShadow());
    tileSet.addTile(ViewId("fog_of_war"), getWaterTile("empty", "fogofwar"));
    tileSet.addTile(ViewId("pit"), sprite("pit"));
    tileSet.addTile(ViewId("creature_highlight"), sprite("creature_highlight"));
    tileSet.addTile(ViewId("square_highlight"), sprite("square_highlight"));
    tileSet.addTile(ViewId("round_shadow"), sprite("round_shadow"));
    tileSet.addTile(ViewId("campaign_defeated"), sprite("campaign_defeated"));
    tileSet.addTile(ViewId("accept_immigrant"), symbol(u8"✓", Color::GREEN, true));
    tileSet.addTile(ViewId("reject_immigrant"), symbol(u8"✘", Color::RED, true));
    tileSet.addTile(ViewId("fog_of_war_corner"), sprite("fogofwar")
        .addConnection({Dir::NE}, byName("fogofwarcornne"))
        .addConnection({Dir::NW}, byName("fogofwarcornnw"))
        .addConnection({Dir::SE}, byName("fogofwarcornse"))
        .addConnection({Dir::SW}, byName("fogofwarcornsw")));
    tileSet.addTile(ViewId("special_blbn"), sprite("special_blbn"));
    tileSet.addTile(ViewId("special_blbw"), sprite("special_blbw"));
    tileSet.addTile(ViewId("special_blgn"), sprite("special_blgn"));
    tileSet.addTile(ViewId("special_blgw"), sprite("special_blgw"));
    tileSet.addTile(ViewId("special_bmbn"), sprite("special_bmbn"));
    tileSet.addTile(ViewId("special_bmbw"), sprite("special_bmbw"));
    tileSet.addTile(ViewId("special_bmgn"), sprite("special_bmgn"));
    tileSet.addTile(ViewId("special_bmgw"), sprite("special_bmgw"));
    tileSet.addTile(ViewId("special_hlbn"), sprite("special_hlbn"));
    tileSet.addTile(ViewId("special_hlbw"), sprite("special_hlbw"));
    tileSet.addTile(ViewId("special_hlgn"), sprite("special_hlgn"));
    tileSet.addTile(ViewId("special_hlgw"), sprite("special_hlgw"));
    tileSet.addTile(ViewId("special_hmbn"), sprite("special_hmbn"));
    tileSet.addTile(ViewId("special_hmbw"), sprite("special_hmbw"));
    tileSet.addTile(ViewId("special_hmgn"), sprite("special_hmgn"));
    tileSet.addTile(ViewId("special_hmgw"), sprite("special_hmgw"));
    tileSet.addTile(ViewId("halloween_kid1"), sprite("halloween_kid1"));
    tileSet.addTile(ViewId("halloween_kid2"), sprite("halloween_kid2"));
    tileSet.addTile(ViewId("halloween_kid3"), sprite("halloween_kid3"));
    tileSet.addTile(ViewId("halloween_kid4"), sprite("halloween_kid4"));
    tileSet.addTile(ViewId("halloween_costume"), sprite("halloween_costume"));
    tileSet.addTile(ViewId("bag_of_candy"), sprite("halloween_candies"));
    tileSet.addTile(ViewId("horn_attack"), sprite("horn_attack"));
    tileSet.addTile(ViewId("beak_attack"), sprite("beak_attack"));
    tileSet.addTile(ViewId("touch_attack"), sprite("touch_attack"));
    tileSet.addTile(ViewId("bite_attack"), sprite("bite_attack"));
    tileSet.addTile(ViewId("claws_attack"), sprite("claws_attack"));
    tileSet.addTile(ViewId("leg_attack"), sprite("leg_attack"));
    tileSet.addTile(ViewId("fist_attack"), sprite("fist_attack"));
    tileSet.addTile(ViewId("item_aura"), sprite("aura"));
#ifndef RELEASE
    tileSet.addTile(ViewId("tutorial_entrance"), symbol(u8"?", Color::YELLOW));
#else
    tileSet.addTile(ViewId("tutorial_entrance"), sprite("empty"));
#endif
  }

  Tile symbol(const string& s, Color id, bool symbol = false) {
    return Tile::fromString(s, id, symbol);
  }

  void genSymbols1(TileSet& tileSet) {
    tileSet.addSymbol(ViewId("demon_dweller"), symbol(u8"U", Color::PURPLE));
    tileSet.addSymbol(ViewId("demon_lord"), symbol(u8"U", Color::YELLOW));
    tileSet.addSymbol(ViewId("empty"), symbol(u8" ", Color::BLACK));
    tileSet.addSymbol(ViewId("dig_mark"), symbol(u8" ", Color::BLACK));
    tileSet.addSymbol(ViewId("dig_mark2"), symbol(u8" ", Color::BLACK));
    tileSet.addSymbol(ViewId("player"), symbol(u8"@", Color::WHITE));
    tileSet.addSymbol(ViewId("player_f"), symbol(u8"@", Color::YELLOW));
    tileSet.addSymbol(ViewId("keeper1"), symbol(u8"@", Color::PURPLE));
    tileSet.addSymbol(ViewId("keeper2"), symbol(u8"@", Color::PURPLE));
    tileSet.addSymbol(ViewId("keeper3"), symbol(u8"@", Color::PURPLE));
    tileSet.addSymbol(ViewId("keeper4"), symbol(u8"@", Color::PURPLE));
    tileSet.addSymbol(ViewId("keeper_f1"), symbol(u8"@", Color::PINK));
    tileSet.addSymbol(ViewId("keeper_f2"), symbol(u8"@", Color::PINK));
    tileSet.addSymbol(ViewId("keeper_f3"), symbol(u8"@", Color::PINK));
    tileSet.addSymbol(ViewId("keeper_f4"), symbol(u8"@", Color::PINK));
    tileSet.addSymbol(ViewId("keeper_knight1"), symbol(u8"@", Color::YELLOW));
    tileSet.addSymbol(ViewId("keeper_knight2"), symbol(u8"@", Color::YELLOW));
    tileSet.addSymbol(ViewId("keeper_knight3"), symbol(u8"@", Color::YELLOW));
    tileSet.addSymbol(ViewId("keeper_knight4"), symbol(u8"@", Color::YELLOW));
    tileSet.addSymbol(ViewId("keeper_knight_f1"), symbol(u8"@", Color::YELLOW));
    tileSet.addSymbol(ViewId("keeper_knight_f2"), symbol(u8"@", Color::YELLOW));
    tileSet.addSymbol(ViewId("keeper_knight_f3"), symbol(u8"@", Color::YELLOW));
    tileSet.addSymbol(ViewId("keeper_knight_f4"), symbol(u8"@", Color::YELLOW));
    tileSet.addSymbol(ViewId("unknown_monster"), symbol(u8"?", Color::LIGHT_GREEN));
    tileSet.addSymbol(ViewId("elf"), symbol(u8"@", Color::LIGHT_GREEN));
    tileSet.addSymbol(ViewId("elf_woman"), symbol(u8"@", Color::LIGHT_GREEN));
    tileSet.addSymbol(ViewId("elf_archer"), symbol(u8"@", Color::GREEN));
    tileSet.addSymbol(ViewId("elf_child"), symbol(u8"@", Color::LIGHT_GREEN));
    tileSet.addSymbol(ViewId("elf_lord"), symbol(u8"@", Color::DARK_GREEN));
    tileSet.addSymbol(ViewId("dark_elf"), symbol(u8"@", Color::ALMOST_DARK_GRAY));
    tileSet.addSymbol(ViewId("dark_elf_woman"), symbol(u8"@", Color::ALMOST_DARK_GRAY));
    tileSet.addSymbol(ViewId("dark_elf_warrior"), symbol(u8"@", Color::GRAY));
    tileSet.addSymbol(ViewId("dark_elf_child"), symbol(u8"@", Color::ALMOST_GRAY));
    tileSet.addSymbol(ViewId("dark_elf_lord"), symbol(u8"@", Color::LIGHT_GRAY));
    tileSet.addSymbol(ViewId("driad"), symbol(u8"@", Color::LIGHT_BROWN));
    tileSet.addSymbol(ViewId("unicorn"), symbol(u8"h", Color::WHITE));
    tileSet.addSymbol(ViewId("kobold"), symbol(u8"k", Color::LIGHT_BROWN));
    tileSet.addSymbol(ViewId("shopkeeper"), symbol(u8"@", Color::LIGHT_BLUE));
    tileSet.addSymbol(ViewId("lizardman"), symbol(u8"@", Color::LIGHT_BROWN));
    tileSet.addSymbol(ViewId("lizardlord"), symbol(u8"@", Color::BROWN));
    tileSet.addSymbol(ViewId("imp"), symbol(u8"i", Color::LIGHT_BROWN));
    tileSet.addSymbol(ViewId("prisoner"), symbol(u8"@", Color::LIGHT_BROWN));
    tileSet.addSymbol(ViewId("ogre"), symbol(u8"O", Color::GREEN));
    tileSet.addSymbol(ViewId("chicken"), symbol(u8"c", Color::YELLOW));
    tileSet.addSymbol(ViewId("gnome"), symbol(u8"g", Color::GREEN));
    tileSet.addSymbol(ViewId("gnome_boss"), symbol(u8"g", Color::DARK_BLUE));
    tileSet.addSymbol(ViewId("dwarf"), symbol(u8"h", Color::BLUE));
    tileSet.addSymbol(ViewId("dwarf_baron"), symbol(u8"h", Color::DARK_BLUE));
    tileSet.addSymbol(ViewId("dwarf_female"), symbol(u8"h", Color::LIGHT_BLUE));
    tileSet.addSymbol(ViewId("floor"), symbol(u8".", Color::LIGHT_GRAY));
    tileSet.addSymbol(ViewId("keeper_floor"), symbol(u8".", Color::WHITE));
    tileSet.addSymbol(ViewId("wood_floor1"), symbol(u8".", Color::LIGHT_BROWN));
    tileSet.addSymbol(ViewId("wood_floor2"), symbol(u8".", Color::BROWN));
    tileSet.addSymbol(ViewId("wood_floor3"), symbol(u8".", Color::LIGHT_BROWN));
    tileSet.addSymbol(ViewId("wood_floor4"), symbol(u8".", Color::BROWN));
    tileSet.addSymbol(ViewId("wood_floor5"), symbol(u8".", Color::BROWN));
    tileSet.addSymbol(ViewId("stone_floor1"), symbol(u8".", Color::LIGHT_GRAY));
    tileSet.addSymbol(ViewId("stone_floor2"), symbol(u8".", Color::GRAY));
    tileSet.addSymbol(ViewId("stone_floor3"), symbol(u8".", Color::LIGHT_GRAY));
    tileSet.addSymbol(ViewId("stone_floor4"), symbol(u8".", Color::GRAY));
    tileSet.addSymbol(ViewId("stone_floor5"), symbol(u8".", Color::GRAY));
    tileSet.addSymbol(ViewId("carpet_floor1"), symbol(u8".", Color::PURPLE));
    tileSet.addSymbol(ViewId("carpet_floor2"), symbol(u8".", Color::PINK));
    tileSet.addSymbol(ViewId("carpet_floor3"), symbol(u8".", Color::PURPLE));
    tileSet.addSymbol(ViewId("carpet_floor4"), symbol(u8".", Color::PINK));
    tileSet.addSymbol(ViewId("carpet_floor5"), symbol(u8".", Color::PINK));
    tileSet.addSymbol(ViewId("buff_floor1"), symbol(u8".", Color::PURPLE));
    tileSet.addSymbol(ViewId("buff_floor2"), symbol(u8".", Color::PINK));
    tileSet.addSymbol(ViewId("buff_floor3"), symbol(u8".", Color::PURPLE));
    tileSet.addSymbol(ViewId("bridge"), symbol(u8"_", Color::BROWN));
    tileSet.addSymbol(ViewId("road"), symbol(u8".", Color::LIGHT_GRAY));
    tileSet.addSymbol(ViewId("sand"), symbol(u8".", Color::YELLOW));
    tileSet.addSymbol(ViewId("mud"), symbol(u8"𝃰", Color::BROWN, true));
    tileSet.addSymbol(ViewId("grass"), symbol(u8"𝃰", Color::GREEN, true));
  }
  void genSymbols2(TileSet& tileSet) {
    tileSet.addSymbol(ViewId("crops"), symbol(u8"𝃰", Color::YELLOW, true));
    tileSet.addSymbol(ViewId("crops2"), symbol(u8"𝃰", Color::YELLOW, true));
    tileSet.addSymbol(ViewId("castle_wall"), symbol(u8"#", Color::LIGHT_GRAY));
    tileSet.addSymbol(ViewId("mud_wall"), symbol(u8"#", Color::LIGHT_BROWN));
    tileSet.addSymbol(ViewId("ruin_wall"), symbol(u8"#", Color::DARK_GRAY));
    tileSet.addSymbol(ViewId("wall"), symbol(u8"#", Color::LIGHT_GRAY));
    tileSet.addSymbol(ViewId("mountain"), symbol(u8"#", Color::GRAY));
    tileSet.addSymbol(ViewId("mountain2"), symbol(u8"#", Color::DARK_GRAY));
    tileSet.addSymbol(ViewId("dungeon_wall"), symbol(u8"#", Color::LIGHT_GRAY));
    tileSet.addSymbol(ViewId("dungeon_wall2"), symbol(u8"#", Color::DARK_BLUE));
    tileSet.addSymbol(ViewId("map_mountain1"), symbol(u8"^", Color::DARK_GRAY));
    tileSet.addSymbol(ViewId("map_mountain2"), symbol(u8"^", Color::GRAY));
    tileSet.addSymbol(ViewId("map_mountain3"), symbol(u8"^", Color::LIGHT_GRAY));
    tileSet.addSymbol(ViewId("gold_ore"), symbol(u8"⁂", Color::YELLOW, true));
    tileSet.addSymbol(ViewId("iron_ore"), symbol(u8"⁂", Color::DARK_BROWN, true));
    tileSet.addSymbol(ViewId("stone"), symbol(u8"⁂", Color::LIGHT_GRAY, true));
    tileSet.addSymbol(ViewId("adamantium_ore"), symbol(u8"⁂", Color::LIGHT_BLUE, true));
    tileSet.addSymbol(ViewId("hill"), symbol(u8"𝀢", Color::DARK_GREEN, true));
    tileSet.addSymbol(ViewId("wood_wall"), symbol(u8"#", Color::DARK_BROWN));
    tileSet.addSymbol(ViewId("down_staircase"), symbol(u8"➘", Color::ALMOST_WHITE, true));
    tileSet.addSymbol(ViewId("up_staircase"), symbol(u8"➚", Color::ALMOST_WHITE, true));
    tileSet.addSymbol(ViewId("well"), symbol(u8"0", Color::BLUE));
    tileSet.addSymbol(ViewId("minion_statue"), symbol(u8"&", Color::LIGHT_GRAY));
    tileSet.addSymbol(ViewId("stone_minion_statue"), symbol(u8"&", Color::ALMOST_WHITE));
    tileSet.addSymbol(ViewId("throne"), symbol(u8"Ω", Color::YELLOW, true));
    tileSet.addSymbol(ViewId("orc"), symbol(u8"o", Color::DARK_BLUE));
    tileSet.addSymbol(ViewId("orc_captain"), symbol(u8"o", Color::PURPLE));
    tileSet.addSymbol(ViewId("orc_shaman"), symbol(u8"o", Color::YELLOW));
    tileSet.addSymbol(ViewId("harpy"), symbol(u8"R", Color::YELLOW));
    tileSet.addSymbol(ViewId("doppleganger"), symbol(u8"&", Color::YELLOW));
    tileSet.addSymbol(ViewId("succubus"), symbol(u8"&", Color::RED));
    tileSet.addSymbol(ViewId("bandit"), symbol(u8"@", Color::DARK_BLUE));
    tileSet.addSymbol(ViewId("green_dragon"), symbol(u8"D", Color::GREEN));
    tileSet.addSymbol(ViewId("red_dragon"), symbol(u8"D", Color::RED));
    tileSet.addSymbol(ViewId("cyclops"), symbol(u8"C", Color::GREEN));
    tileSet.addSymbol(ViewId("minotaur"), symbol(u8"M", Color::PURPLE));
    tileSet.addSymbol(ViewId("soft_monster"), symbol(u8"M", Color::PINK));
    tileSet.addSymbol(ViewId("hydra"), symbol(u8"H", Color::PURPLE));
    tileSet.addSymbol(ViewId("shelob"), symbol(u8"S", Color::PURPLE));
    tileSet.addSymbol(ViewId("witch"), symbol(u8"@", Color::BROWN));
    tileSet.addSymbol(ViewId("witchman"), symbol(u8"@", Color::WHITE));
    tileSet.addSymbol(ViewId("ghost"), symbol(u8"&", Color::WHITE));
    tileSet.addSymbol(ViewId("spirit"), symbol(u8"&", Color::LIGHT_BLUE));
    tileSet.addSymbol(ViewId("knight"), symbol(u8"@", Color::LIGHT_GRAY));
    tileSet.addSymbol(ViewId("priest"), symbol(u8"@", Color::SKY_BLUE));
    tileSet.addSymbol(ViewId("jester"), symbol(u8"@", Color::PINK));
    tileSet.addSymbol(ViewId("warrior"), symbol(u8"@", Color::DARK_GRAY));
    tileSet.addSymbol(ViewId("shaman"), symbol(u8"@", Color::YELLOW));
    tileSet.addSymbol(ViewId("duke1"), symbol(u8"@", Color::BLUE));
    tileSet.addSymbol(ViewId("duke2"), symbol(u8"@", Color::BLUE));
    tileSet.addSymbol(ViewId("duke3"), symbol(u8"@", Color::BLUE));
    tileSet.addSymbol(ViewId("duke4"), symbol(u8"@", Color::BLUE));
    tileSet.addSymbol(ViewId("duke_f1"), symbol(u8"@", Color::BLUE));
    tileSet.addSymbol(ViewId("duke_f2"), symbol(u8"@", Color::BLUE));
    tileSet.addSymbol(ViewId("duke_f3"), symbol(u8"@", Color::BLUE));
    tileSet.addSymbol(ViewId("duke_f4"), symbol(u8"@", Color::BLUE));
    tileSet.addSymbol(ViewId("archer"), symbol(u8"@", Color::BROWN));
    tileSet.addSymbol(ViewId("peseant"), symbol(u8"@", Color::GREEN));
    tileSet.addSymbol(ViewId("peseant_woman"), symbol(u8"@", Color::GREEN));
    tileSet.addSymbol(ViewId("child"), symbol(u8"@", Color::LIGHT_GREEN));
    tileSet.addSymbol(ViewId("clay_golem"), symbol(u8"Y", Color::YELLOW));
    tileSet.addSymbol(ViewId("stone_golem"), symbol(u8"Y", Color::LIGHT_GRAY));
    tileSet.addSymbol(ViewId("iron_golem"), symbol(u8"Y", Color::ORANGE));
    tileSet.addSymbol(ViewId("lava_golem"), symbol(u8"Y", Color::PURPLE));
    tileSet.addSymbol(ViewId("ada_golem"), symbol(u8"Y", Color::LIGHT_BLUE));
    tileSet.addSymbol(ViewId("automaton"), symbol(u8"A", Color::LIGHT_GRAY));
    tileSet.addSymbol(ViewId("elementalist"), symbol(u8"@", Color::YELLOW));
    tileSet.addSymbol(ViewId("air_elemental"), symbol(u8"E", Color::LIGHT_BLUE));
    tileSet.addSymbol(ViewId("fire_elemental"), symbol(u8"E", Color::RED));
    tileSet.addSymbol(ViewId("water_elemental"), symbol(u8"E", Color::BLUE));
    tileSet.addSymbol(ViewId("earth_elemental"), symbol(u8"E", Color::GRAY));
    tileSet.addSymbol(ViewId("ent"), symbol(u8"E", Color::LIGHT_BROWN));
    tileSet.addSymbol(ViewId("angel"), symbol(u8"A", Color::LIGHT_BLUE));
    tileSet.addSymbol(ViewId("zombie"), symbol(u8"Z", Color::GREEN));
    tileSet.addSymbol(ViewId("skeleton"), symbol(u8"Z", Color::WHITE));
    tileSet.addSymbol(ViewId("vampire"), symbol(u8"V", Color::DARK_GRAY));
    tileSet.addSymbol(ViewId("vampire_lord"), symbol(u8"V", Color::PURPLE));
    tileSet.addSymbol(ViewId("mummy"), symbol(u8"Z", Color::YELLOW));
  }
  void genSymbols3(TileSet& tileSet) {
    tileSet.addSymbol(ViewId("jackal"), symbol(u8"d", Color::LIGHT_BROWN));
    tileSet.addSymbol(ViewId("deer"), symbol(u8"R", Color::DARK_BROWN));
    tileSet.addSymbol(ViewId("horse"), symbol(u8"H", Color::LIGHT_BROWN));
    tileSet.addSymbol(ViewId("cow"), symbol(u8"C", Color::WHITE));
    tileSet.addSymbol(ViewId("pig"), symbol(u8"p", Color::YELLOW));
    tileSet.addSymbol(ViewId("donkey"), symbol(u8"D", Color::GRAY));
    tileSet.addSymbol(ViewId("goat"), symbol(u8"g", Color::GRAY));
    tileSet.addSymbol(ViewId("boar"), symbol(u8"b", Color::LIGHT_BROWN));
    tileSet.addSymbol(ViewId("fox"), symbol(u8"d", Color::ORANGE_BROWN));
    tileSet.addSymbol(ViewId("wolf"), symbol(u8"d", Color::DARK_BLUE));
    tileSet.addSymbol(ViewId("werewolf"), symbol(u8"d", Color::WHITE));
    tileSet.addSymbol(ViewId("dog"), symbol(u8"d", Color::BROWN));
    tileSet.addSymbol(ViewId("kraken_head"), symbol(u8"S", Color::GREEN));
    tileSet.addSymbol(ViewId("kraken_water"), symbol(u8"S", Color::DARK_GREEN));
    tileSet.addSymbol(ViewId("kraken_land"), symbol(u8"S", Color::DARK_GREEN));
    tileSet.addSymbol(ViewId("death"), symbol(u8"D", Color::DARK_GRAY));
    tileSet.addSymbol(ViewId("fire_sphere"), symbol(u8"e", Color::RED));
    tileSet.addSymbol(ViewId("bear"), symbol(u8"N", Color::BROWN));
    tileSet.addSymbol(ViewId("bat"), symbol(u8"b", Color::DARK_GRAY));
    tileSet.addSymbol(ViewId("goblin"), symbol(u8"o", Color::GREEN));
    tileSet.addSymbol(ViewId("rat"), symbol(u8"r", Color::BROWN));
    tileSet.addSymbol(ViewId("rat_king"), symbol(u8"@", Color::LIGHT_BROWN));
    tileSet.addSymbol(ViewId("rat_soldier"), symbol(u8"@", Color::BROWN));
    tileSet.addSymbol(ViewId("rat_lady"), symbol(u8"@", Color::DARK_BROWN));
    tileSet.addSymbol(ViewId("spider"), symbol(u8"s", Color::BROWN));
    tileSet.addSymbol(ViewId("ant_worker"), symbol(u8"a", Color::BROWN));
    tileSet.addSymbol(ViewId("ant_soldier"), symbol(u8"a", Color::YELLOW));
    tileSet.addSymbol(ViewId("ant_queen"), symbol(u8"a", Color::PURPLE));
    tileSet.addSymbol(ViewId("fly"), symbol(u8"b", Color::GRAY));
    tileSet.addSymbol(ViewId("snake"), symbol(u8"S", Color::YELLOW));
    tileSet.addSymbol(ViewId("vulture"), symbol(u8"v", Color::DARK_GRAY));
    tileSet.addSymbol(ViewId("raven"), symbol(u8"v", Color::DARK_GRAY));
    tileSet.addSymbol(ViewId("body_part"), symbol(u8"%", Color::RED));
    tileSet.addSymbol(ViewId("bone"), symbol(u8"%", Color::WHITE));
    tileSet.addSymbol(ViewId("bush"), symbol(u8"&", Color::DARK_GREEN));
    tileSet.addSymbol(ViewId("decid_tree"), symbol(u8"🜍", Color::DARK_GREEN, true));
    tileSet.addSymbol(ViewId("canif_tree"), symbol(u8"♣", Color::DARK_GREEN, true));
    tileSet.addSymbol(ViewId("tree_trunk"), symbol(u8".", Color::BROWN));
    tileSet.addSymbol(ViewId("burnt_tree"), symbol(u8".", Color::DARK_GRAY));
    tileSet.addSymbol(ViewId("water"), symbol(u8"~", Color::LIGHT_BLUE));
    tileSet.addSymbol(ViewId("magma"), symbol(u8"~", Color::RED));
    tileSet.addSymbol(ViewId("wood_door"), symbol(u8"|", Color::BROWN));
    tileSet.addSymbol(ViewId("iron_door"), symbol(u8"|", Color::LIGHT_GRAY));
    tileSet.addSymbol(ViewId("ada_door"), symbol(u8"|", Color::LIGHT_BLUE));
    tileSet.addSymbol(ViewId("barricade"), symbol(u8"X", Color::BROWN));
    tileSet.addSymbol(ViewId("dig_icon"), symbol(u8"⛏", Color::LIGHT_GRAY, true));
    tileSet.addSymbol(ViewId("ada_sword"), symbol(u8")", Color::LIGHT_BLUE));
    tileSet.addSymbol(ViewId("sword"), symbol(u8")", Color::LIGHT_GRAY));
    tileSet.addSymbol(ViewId("spear"), symbol(u8"/", Color::LIGHT_GRAY));
    tileSet.addSymbol(ViewId("special_sword"), symbol(u8")", Color::YELLOW));
    tileSet.addSymbol(ViewId("elven_sword"), symbol(u8")", Color::GRAY));
    tileSet.addSymbol(ViewId("knife"), symbol(u8")", Color::WHITE));
    tileSet.addSymbol(ViewId("war_hammer"), symbol(u8")", Color::BLUE));
    tileSet.addSymbol(ViewId("ada_war_hammer"), symbol(u8")", Color::LIGHT_BLUE));
    tileSet.addSymbol(ViewId("special_war_hammer"), symbol(u8")", Color::LIGHT_BLUE));
    tileSet.addSymbol(ViewId("battle_axe"), symbol(u8")", Color::GREEN));
    tileSet.addSymbol(ViewId("ada_battle_axe"), symbol(u8")", Color::LIGHT_BLUE));
    tileSet.addSymbol(ViewId("special_battle_axe"), symbol(u8")", Color::LIGHT_GREEN));
    tileSet.addSymbol(ViewId("elven_bow"), symbol(u8")", Color::YELLOW));
    tileSet.addSymbol(ViewId("bow"), symbol(u8")", Color::BROWN));
    tileSet.addSymbol(ViewId("wooden_staff"), symbol(u8")", Color::YELLOW));
    tileSet.addSymbol(ViewId("iron_staff"), symbol(u8")", Color::ORANGE));
    tileSet.addSymbol(ViewId("golden_staff"), symbol(u8")", Color::YELLOW));
    tileSet.addSymbol(ViewId("club"), symbol(u8")", Color::BROWN));
    tileSet.addSymbol(ViewId("heavy_club"), symbol(u8")", Color::BROWN));
    tileSet.addSymbol(ViewId("arrow"), symbol(u8"/", Color::BROWN));
    tileSet.addSymbol(ViewId("force_bolt"), symbol(u8"*", Color::LIGHT_BLUE));
    tileSet.addSymbol(ViewId("fireball"), symbol(u8"*", Color::ORANGE));
    tileSet.addSymbol(ViewId("air_blast"), symbol(u8"*", Color::WHITE));
    tileSet.addSymbol(ViewId("stun_ray"), symbol(u8"*", Color::LIGHT_GREEN));
    tileSet.addSymbol(ViewId("scroll"), symbol(u8"?", Color::WHITE));
    tileSet.addSymbol(ViewId("amulet1"), symbol(u8"\"", Color::YELLOW));
  }
  void genSymbols4(TileSet& tileSet) {
    tileSet.addSymbol(ViewId("amulet2"), symbol(u8"\"", Color::YELLOW));
    tileSet.addSymbol(ViewId("amulet3"), symbol(u8"\"", Color::YELLOW));
    tileSet.addSymbol(ViewId("amulet4"), symbol(u8"\"", Color::YELLOW));
    tileSet.addSymbol(ViewId("amulet5"), symbol(u8"\"", Color::YELLOW));
    tileSet.addSymbol(ViewId("fire_resist_ring"), symbol(u8"=", Color::RED));
    tileSet.addSymbol(ViewId("poison_resist_ring"), symbol(u8"=", Color::GREEN));
    tileSet.addSymbol(ViewId("book"), symbol(u8"+", Color::YELLOW));
    tileSet.addSymbol(ViewId("hand_torch"), symbol(u8"/", Color::YELLOW));
    tileSet.addSymbol(ViewId("first_aid"), symbol(u8"+", Color::RED));
    tileSet.addSymbol(ViewId("trap_item"), symbol(u8"+", Color::YELLOW));
    tileSet.addSymbol(ViewId("potion1"), symbol(u8"!", Color::LIGHT_RED));
    tileSet.addSymbol(ViewId("potion2"), symbol(u8"!", Color::BLUE));
    tileSet.addSymbol(ViewId("potion3"), symbol(u8"!", Color::YELLOW));
    tileSet.addSymbol(ViewId("potion4"), symbol(u8"!", Color::VIOLET));
    tileSet.addSymbol(ViewId("potion5"), symbol(u8"!", Color::DARK_BROWN));
    tileSet.addSymbol(ViewId("potion6"), symbol(u8"!", Color::LIGHT_GRAY));
    tileSet.addSymbol(ViewId("rune1"), symbol(u8"~", Color::GREEN));
    tileSet.addSymbol(ViewId("rune2"), symbol(u8"~", Color::PURPLE));
    tileSet.addSymbol(ViewId("rune3"), symbol(u8"~", Color::RED));
    tileSet.addSymbol(ViewId("rune4"), symbol(u8"~", Color::BLUE));
    tileSet.addSymbol(ViewId("mushroom1"), symbol(u8"⋆", Color::PINK, true));
    tileSet.addSymbol(ViewId("mushroom2"), symbol(u8"⋆", Color::YELLOW, true));
    tileSet.addSymbol(ViewId("mushroom3"), symbol(u8"⋆", Color::PURPLE, true));
    tileSet.addSymbol(ViewId("mushroom4"), symbol(u8"⋆", Color::BROWN, true));
    tileSet.addSymbol(ViewId("mushroom5"), symbol(u8"⋆", Color::LIGHT_GRAY, true));
    tileSet.addSymbol(ViewId("mushroom6"), symbol(u8"⋆", Color::ORANGE, true));
    tileSet.addSymbol(ViewId("mushroom7"), symbol(u8"⋆", Color::LIGHT_BLUE, true));
    tileSet.addSymbol(ViewId("key"), symbol(u8"*", Color::YELLOW));
    tileSet.addSymbol(ViewId("fountain"), symbol(u8"0", Color::LIGHT_BLUE));
    tileSet.addSymbol(ViewId("gold"), symbol(u8"$", Color::YELLOW));
    tileSet.addSymbol(ViewId("treasure_chest"), symbol(u8"=", Color::BROWN));
    tileSet.addSymbol(ViewId("opened_chest"), symbol(u8"=", Color::BROWN));
    tileSet.addSymbol(ViewId("chest"), symbol(u8"=", Color::BROWN));
    tileSet.addSymbol(ViewId("opened_coffin"), symbol(u8"⚰", Color::DARK_BROWN, true));
    tileSet.addSymbol(ViewId("loot_coffin"), symbol(u8"⚰", Color::BROWN, true));
    tileSet.addSymbol(ViewId("coffin1"), symbol(u8"⚰", Color::BROWN, true));
    tileSet.addSymbol(ViewId("coffin2"), symbol(u8"⚰", Color::GRAY, true));
    tileSet.addSymbol(ViewId("coffin3"), symbol(u8"⚰", Color::YELLOW, true));
    tileSet.addSymbol(ViewId("boulder"), symbol(u8"●", Color::LIGHT_GRAY, true));
    tileSet.addSymbol(ViewId("portal"), symbol(u8"𝚯", Color::WHITE, true));
    tileSet.addSymbol(ViewId("gas_trap"), symbol(u8"☠", Color::GREEN, true));
    tileSet.addSymbol(ViewId("alarm_trap"), symbol(u8"^", Color::RED, true));
    tileSet.addSymbol(ViewId("web_trap"), symbol(u8"#", Color::WHITE, true));
    tileSet.addSymbol(ViewId("surprise_trap"), symbol(u8"^", Color::BLUE, true));
    tileSet.addSymbol(ViewId("terror_trap"), symbol(u8"^", Color::WHITE, true));
    tileSet.addSymbol(ViewId("fire_trap"), symbol(u8"^", Color::RED, true));
    tileSet.addSymbol(ViewId("rock"), symbol(u8"✱", Color::LIGHT_GRAY, true));
    tileSet.addSymbol(ViewId("iron_rock"), symbol(u8"✱", Color::ORANGE, true));
    tileSet.addSymbol(ViewId("ada_ore"), symbol(u8"✱", Color::LIGHT_BLUE, true));
    tileSet.addSymbol(ViewId("wood_plank"), symbol(u8"\\", Color::BROWN));
    tileSet.addSymbol(ViewId("storage_equipment"), symbol(u8".", Color::GREEN));
    tileSet.addSymbol(ViewId("storage_resources"), symbol(u8".", Color::BLUE));
    tileSet.addSymbol(ViewId("quarters1"), symbol(u8".", Color::PINK));
    tileSet.addSymbol(ViewId("quarters2"), symbol(u8".", Color::SKY_BLUE));
    tileSet.addSymbol(ViewId("quarters3"), symbol(u8".", Color::ORANGE));
    tileSet.addSymbol(ViewId("leisure"), symbol(u8".", Color::DARK_BLUE));
    tileSet.addSymbol(ViewId("prison"), symbol(u8".", Color::BLUE));
    tileSet.addSymbol(ViewId("dorm"), symbol(u8".", Color::BROWN));
    tileSet.addSymbol(ViewId("bed1"), symbol(u8"=", Color::WHITE));
    tileSet.addSymbol(ViewId("bed2"), symbol(u8"=", Color::YELLOW));
    tileSet.addSymbol(ViewId("bed3"), symbol(u8"=", Color::PURPLE));
    tileSet.addSymbol(ViewId("torch"), symbol(u8"*", Color::YELLOW));
    tileSet.addSymbol(ViewId("standing_torch"), symbol(u8"*", Color::YELLOW));
    tileSet.addSymbol(ViewId("candelabrum_ns"), symbol(u8"*", Color::ORANGE));
    tileSet.addSymbol(ViewId("candelabrum_w"), symbol(u8"*", Color::ORANGE));
    tileSet.addSymbol(ViewId("candelabrum_e"), symbol(u8"*", Color::ORANGE));
    tileSet.addSymbol(ViewId("altar"), symbol(u8"Ω", Color::WHITE, true));
    tileSet.addSymbol(ViewId("altar_des"), symbol(u8"Ω", Color::RED, true));
    tileSet.addSymbol(ViewId("creature_altar"), symbol(u8"Ω", Color::YELLOW, true));
    tileSet.addSymbol(ViewId("torture_table"), symbol(u8"=", Color::GRAY));
    tileSet.addSymbol(ViewId("impaled_head"), symbol(u8"⚲", Color::BROWN, true));
    tileSet.addSymbol(ViewId("whipping_post"), symbol(u8"}", Color::BROWN, true));
    tileSet.addSymbol(ViewId("gallows"), symbol(u8"}", Color::ORANGE, true));
    tileSet.addSymbol(ViewId("notice_board"), symbol(u8"|", Color::BROWN));
    tileSet.addSymbol(ViewId("sokoban_hole"), symbol(u8"0", Color::DARK_BLUE));
    tileSet.addSymbol(ViewId("training_wood"), symbol(u8"‡", Color::BROWN, true));
    tileSet.addSymbol(ViewId("training_iron"), symbol(u8"‡", Color::LIGHT_GRAY, true));
    tileSet.addSymbol(ViewId("training_ada"), symbol(u8"‡", Color::LIGHT_BLUE, true));
    tileSet.addSymbol(ViewId("archery_range"), symbol(u8"⌾", Color::LIGHT_BLUE, true));
  }
  void genSymbols5(TileSet& tileSet) {
    tileSet.addSymbol(ViewId("demon_shrine"), symbol(u8"Ω", Color::PURPLE, true));
    tileSet.addSymbol(ViewId("bookcase_wood"), symbol(u8"▤", Color::BROWN, true));
    tileSet.addSymbol(ViewId("bookcase_iron"), symbol(u8"▤", Color::LIGHT_GRAY, true));
    tileSet.addSymbol(ViewId("bookcase_gold"), symbol(u8"▤", Color::YELLOW, true));
    tileSet.addSymbol(ViewId("laboratory"), symbol(u8"ω", Color::PURPLE, true));
    tileSet.addSymbol(ViewId("cauldron"), symbol(u8"ω", Color::PURPLE, true));
    tileSet.addSymbol(ViewId("beast_lair"), symbol(u8".", Color::YELLOW));
    tileSet.addSymbol(ViewId("beast_cage"), symbol(u8"▥", Color::LIGHT_GRAY, true));
    tileSet.addSymbol(ViewId("workshop"), symbol(u8"&", Color::LIGHT_BROWN));
    tileSet.addSymbol(ViewId("forge"), symbol(u8"&", Color::LIGHT_BLUE));
    tileSet.addSymbol(ViewId("jeweller"), symbol(u8"&", Color::YELLOW));
    tileSet.addSymbol(ViewId("furnace"), symbol(u8"&", Color::PINK));
    tileSet.addSymbol(ViewId("grave"), symbol(u8"☗", Color::GRAY, true));
    tileSet.addSymbol(ViewId("border_guard"), symbol(u8" ", Color::BLACK));
    tileSet.addSymbol(ViewId("robe"), symbol(u8"[", Color::LIGHT_BROWN));
    tileSet.addSymbol(ViewId("leather_gloves"), symbol(u8"[", Color::BROWN));
    tileSet.addSymbol(ViewId("iron_gloves"), symbol(u8"[", Color::LIGHT_GRAY));
    tileSet.addSymbol(ViewId("ada_gloves"), symbol(u8"[", Color::LIGHT_BLUE));
    tileSet.addSymbol(ViewId("strength_gloves"), symbol(u8"[", Color::RED));
    tileSet.addSymbol(ViewId("dexterity_gloves"), symbol(u8"[", Color::LIGHT_BLUE));
    tileSet.addSymbol(ViewId("leather_armor"), symbol(u8"[", Color::BROWN));
    tileSet.addSymbol(ViewId("leather_helm"), symbol(u8"[", Color::BROWN));
    tileSet.addSymbol(ViewId("telepathy_helm"), symbol(u8"[", Color::LIGHT_GREEN));
    tileSet.addSymbol(ViewId("chain_armor"), symbol(u8"[", Color::LIGHT_GRAY));
    tileSet.addSymbol(ViewId("ada_armor"), symbol(u8"[", Color::LIGHT_BLUE));
    tileSet.addSymbol(ViewId("iron_helm"), symbol(u8"[", Color::LIGHT_GRAY));
    tileSet.addSymbol(ViewId("ada_helm"), symbol(u8"[", Color::LIGHT_BLUE));
    tileSet.addSymbol(ViewId("leather_boots"), symbol(u8"[", Color::BROWN));
    tileSet.addSymbol(ViewId("iron_boots"), symbol(u8"[", Color::LIGHT_GRAY));
    tileSet.addSymbol(ViewId("ada_boots"), symbol(u8"[", Color::LIGHT_BLUE));
    tileSet.addSymbol(ViewId("speed_boots"), symbol(u8"[", Color::LIGHT_BLUE));
    tileSet.addSymbol(ViewId("levitation_boots"), symbol(u8"[", Color::LIGHT_GREEN));
    tileSet.addSymbol(ViewId("wooden_shield"), symbol(u8"[", Color::LIGHT_BROWN));
    tileSet.addSymbol(ViewId("fallen_tree"), symbol(u8"*", Color::GREEN));
    tileSet.addSymbol(ViewId("guard_post"), symbol(u8"⚐", Color::YELLOW, true));
    tileSet.addSymbol(ViewId("destroy_button"), symbol(u8"X", Color::RED));
    tileSet.addSymbol(ViewId("forbid_zone"), symbol(u8"#", Color::RED));
    tileSet.addSymbol(ViewId("mana"), symbol(u8"✱", Color::BLUE, true));
    tileSet.addSymbol(ViewId("eyeball"), symbol(u8"e", Color::BLUE));
    tileSet.addSymbol(ViewId("fetch_icon"), symbol(u8"👋", Color::LIGHT_BROWN, true));
    tileSet.addSymbol(ViewId("fog_of_war"), symbol(u8" ", Color::WHITE));
    tileSet.addSymbol(ViewId("pit"), symbol(u8"^", Color::WHITE));
    tileSet.addSymbol(ViewId("creature_highlight"), symbol(u8" ", Color::WHITE));
    tileSet.addSymbol(ViewId("square_highlight"), symbol(u8"⛶", Color::WHITE, true));
    tileSet.addSymbol(ViewId("round_shadow"), symbol(u8" ", Color::WHITE));
    tileSet.addSymbol(ViewId("campaign_defeated"), symbol(u8"X", Color::RED));
    tileSet.addSymbol(ViewId("fog_of_war_corner"), symbol(u8" ", Color::WHITE));
    tileSet.addSymbol(ViewId("accept_immigrant"), symbol(u8"✓", Color::GREEN, true));
    tileSet.addSymbol(ViewId("reject_immigrant"), symbol(u8"✘", Color::RED, true));
    tileSet.addSymbol(ViewId("special_blbn"), symbol(u8"B", Color::PURPLE));
    tileSet.addSymbol(ViewId("special_blbw"), symbol(u8"B", Color::LIGHT_RED));
    tileSet.addSymbol(ViewId("special_blgn"), symbol(u8"B", Color::LIGHT_GRAY));
    tileSet.addSymbol(ViewId("special_blgw"), symbol(u8"B", Color::WHITE));
    tileSet.addSymbol(ViewId("special_bmbn"), symbol(u8"B", Color::YELLOW));
    tileSet.addSymbol(ViewId("special_bmbw"), symbol(u8"B", Color::ORANGE));
    tileSet.addSymbol(ViewId("special_bmgn"), symbol(u8"B", Color::GREEN));
    tileSet.addSymbol(ViewId("special_bmgw"), symbol(u8"B", Color::LIGHT_GREEN));
    tileSet.addSymbol(ViewId("special_hlbn"), symbol(u8"H", Color::PURPLE));
    tileSet.addSymbol(ViewId("special_hlbw"), symbol(u8"H", Color::LIGHT_RED));
    tileSet.addSymbol(ViewId("special_hlgn"), symbol(u8"H", Color::LIGHT_GRAY));
    tileSet.addSymbol(ViewId("special_hlgw"), symbol(u8"H", Color::WHITE));
    tileSet.addSymbol(ViewId("special_hmbn"), symbol(u8"H", Color::YELLOW));
    tileSet.addSymbol(ViewId("special_hmbw"), symbol(u8"H", Color::ORANGE));
    tileSet.addSymbol(ViewId("special_hmgn"), symbol(u8"H", Color::GREEN));
    tileSet.addSymbol(ViewId("special_hmgw"), symbol(u8"H", Color::LIGHT_GREEN));
    tileSet.addSymbol(ViewId("halloween_kid1"), symbol(u8"@", Color::PINK));
    tileSet.addSymbol(ViewId("halloween_kid2"), symbol(u8"@", Color::PURPLE));
    tileSet.addSymbol(ViewId("halloween_kid3"), symbol(u8"@", Color::BLUE));
    tileSet.addSymbol(ViewId("halloween_kid4"), symbol(u8"@", Color::YELLOW));
    tileSet.addSymbol(ViewId("halloween_costume"), symbol(u8"[", Color::PINK));
    tileSet.addSymbol(ViewId("bag_of_candy"), symbol(u8"*", Color::BLUE));
    tileSet.addSymbol(ViewId("tutorial_entrance"), symbol(u8" ", Color::LIGHT_GREEN));
    tileSet.addSymbol(ViewId("horn_attack"), symbol(u8" ", Color::PINK));
    tileSet.addSymbol(ViewId("beak_attack"), symbol(u8" ", Color::YELLOW));
    tileSet.addSymbol(ViewId("touch_attack"), symbol(u8" ", Color::WHITE));
    tileSet.addSymbol(ViewId("bite_attack"), symbol(u8" ", Color::RED));
    tileSet.addSymbol(ViewId("claws_attack"), symbol(u8" ", Color::BROWN));
    tileSet.addSymbol(ViewId("leg_attack"), symbol(u8" ", Color::GRAY));
    tileSet.addSymbol(ViewId("fist_attack"), symbol(u8" ", Color::ORANGE));
    tileSet.addSymbol(ViewId("item_aura"), symbol(u8" ", Color::ORANGE));
  }

  private:
  Renderer& renderer;
};


TileSet::TileSet(Renderer& renderer, GameConfig* config, bool useTiles) {
  TileCoordLookup lookup(renderer);
  lookup.loadUnicode(*this);
  if (useTiles)
    lookup.loadTiles(*this);
}

const Tile& TileSet::getTile(ViewId id, bool sprite) const {
  if (sprite && tiles.count(id))
    return tiles.at(id);
  else
    return symbols.at(id);
}

