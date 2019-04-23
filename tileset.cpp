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

const vector<TileCoord>& TileSet::getTileCoord(const string& s) const {
  return tileCoords.at(s);
}

void TileSet::loadTiles() {
  genTiles1();
  genTiles2();
  genTiles3();
  genTiles4();
  genTiles5();
}

void TileSet::loadUnicode() {
  genSymbols1();
  genSymbols2();
  genSymbols3();
  genSymbols4();
  genSymbols5();
}

const vector<TileCoord>& TileSet::byName(const string& s) {
  return tileCoords.at(s);
}

Tile TileSet::sprite(const string& s) {
  return Tile::byCoord(byName(s));
}

Tile TileSet::empty() {
  return sprite("empty");
}

Tile TileSet::getRoadTile(const string& prefix) {
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

Tile TileSet::getWallTile(const string& prefix) {
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

Tile TileSet::getMountainTile(Tile background, const string& prefix) {
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

Tile TileSet::getWaterTile(const string& background, const string& prefix) {
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

Tile TileSet::getExtraBorderTile(const string& prefix) {
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

void TileSet::genTiles1() {
  addTile(ViewId("unknown_monster"), sprite("unknown"));
  addTile(ViewId("dig_mark"), sprite("dig_mark"));
  addTile(ViewId("dig_mark2"), sprite("dig_mark2"));
  addTile(ViewId("forbid_zone"), sprite("dig_mark").setColor(Color::RED));
  addTile(ViewId("destroy_button"), sprite("remove"));
  addTile(ViewId("empty"), empty());
  addTile(ViewId("border_guard"), empty());
  addTile(ViewId("demon_dweller"), sprite("demon_dweller"));
  addTile(ViewId("demon_lord"), sprite("demon_lord"));
  addTile(ViewId("vampire"), sprite("vampire"));
  addTile(ViewId("fallen_tree"), sprite("treecut"));
  addTile(ViewId("decid_tree"), sprite("tree2").setRoundShadow());
  addTile(ViewId("canif_tree"), sprite("tree1").setRoundShadow());
  addTile(ViewId("tree_trunk"), sprite("treecut"));
  addTile(ViewId("unicorn"), sprite("unicorn"));
  addTile(ViewId("burnt_tree"), sprite("treeburnt")
      .setRoundShadow());
  addTile(ViewId("player"), sprite("adventurer"));
  addTile(ViewId("player_f"), sprite("adventurer_female"));
  addTile(ViewId("keeper1"), sprite("keeper1"));
  addTile(ViewId("keeper2"), sprite("keeper2"));
  addTile(ViewId("keeper3"), sprite("keeper3"));
  addTile(ViewId("keeper4"), sprite("keeper4"));
  addTile(ViewId("keeper_f1"), sprite("keeper_female1"));
  addTile(ViewId("keeper_f2"), sprite("keeper_female2"));
  addTile(ViewId("keeper_f3"), sprite("keeper_female3"));
  addTile(ViewId("keeper_f4"), sprite("keeper_female4"));
  addTile(ViewId("keeper_knight1"), sprite("keeper_knight1"));
  addTile(ViewId("keeper_knight2"), sprite("keeper_knight2"));
  addTile(ViewId("keeper_knight3"), sprite("keeper_knight3"));
  addTile(ViewId("keeper_knight4"), sprite("keeper_knight4"));
  addTile(ViewId("keeper_knight_f1"), sprite("keeper_knight_female1"));
  addTile(ViewId("keeper_knight_f2"), sprite("keeper_knight_female2"));
  addTile(ViewId("keeper_knight_f3"), sprite("keeper_knight_female3"));
  addTile(ViewId("keeper_knight_f4"), sprite("keeper_knight_female4"));
  addTile(ViewId("elf"), sprite("elf male"));
  addTile(ViewId("elf_woman"), sprite("elf female"));
  addTile(ViewId("elf_archer"), sprite("elf archer"));
  addTile(ViewId("elf_child"), sprite("elf kid"));
  addTile(ViewId("elf_lord"), sprite("elf lord"));
  addTile(ViewId("dark_elf"), sprite("dark_elf"));
  addTile(ViewId("dark_elf_woman"), sprite("dark_elf_female"));
  addTile(ViewId("dark_elf_warrior"), sprite("dark_elf_warrior"));
  addTile(ViewId("dark_elf_child"), sprite("dark_elf_kid"));
  addTile(ViewId("dark_elf_lord"), sprite("dark_elf_lord"));
  addTile(ViewId("driad"), sprite("driad"));
  addTile(ViewId("kobold"), sprite("kobold"));
  addTile(ViewId("lizardman"), sprite("lizardman"));
  addTile(ViewId("lizardlord"), sprite("lizardlord"));
  addTile(ViewId("imp"), sprite("imp"));
  addTile(ViewId("prisoner"), sprite("prisoner"));
  addTile(ViewId("ogre"), sprite("troll"));
  addTile(ViewId("chicken"), sprite("hen"));
}

void TileSet::genTiles2() {
  addTile(ViewId("gnome"), sprite("gnome"));
  addTile(ViewId("gnome_boss"), sprite("gnomeboss"));
  addTile(ViewId("dwarf"), sprite("dwarf"));
  addTile(ViewId("dwarf_baron"), sprite("dwarfboss"));
  addTile(ViewId("dwarf_female"), sprite("dwarf_f"));
  addTile(ViewId("shopkeeper"), sprite("salesman"));
  addTile(ViewId("bridge"), sprite("bridge").addOption(Dir::S, byName("bridge2")));
  addTile(ViewId("road"), getRoadTile("road"));
  addTile(ViewId("floor"), sprite("floor"));
  addTile(ViewId("keeper_floor"), sprite("floor_keeper"));
  addTile(ViewId("wood_floor1"), sprite("floor_wood1"));
  addTile(ViewId("wood_floor2"), sprite("floor_wood2"));
  addTile(ViewId("wood_floor3"), sprite("floor_wood3"));
  addTile(ViewId("wood_floor4"), sprite("floor_wood4"));
  addTile(ViewId("wood_floor5"), sprite("floor_wood5"));
  addTile(ViewId("stone_floor1"), sprite("floor_stone1"));
  addTile(ViewId("stone_floor2"), sprite("floor_stone2"));
  addTile(ViewId("stone_floor3"), sprite("floor_stone3"));
  addTile(ViewId("stone_floor4"), sprite("floor_stone4"));
  addTile(ViewId("stone_floor5"), sprite("floor_stone5"));
  addTile(ViewId("carpet_floor1"), sprite("floor_carpet1"));
  addTile(ViewId("carpet_floor2"), sprite("floor_carpet2"));
  addTile(ViewId("carpet_floor3"), sprite("floor_carpet3"));
  addTile(ViewId("carpet_floor4"), sprite("floor_carpet4"));
  addTile(ViewId("carpet_floor5"), sprite("floor_carpet5"));
  addTile(ViewId("buff_floor1"), sprite("floor_buff1"));
  addTile(ViewId("buff_floor2"), sprite("floor_buff2"));
  addTile(ViewId("buff_floor3"), sprite("floor_buff3"));
  addTile(ViewId("sand"), getExtraBorderTile("sand")
      .addExtraBorderId(ViewId("water")));
  addTile(ViewId("mud"), getExtraBorderTile("mud")
      .addExtraBorderId(ViewId("water"))
      .addExtraBorderId(ViewId("hill"))
      .addExtraBorderId(ViewId("sand")));
  addTile(ViewId("grass"), getExtraBorderTile("grass")
      .addExtraBorderId(ViewId("sand"))
      .addExtraBorderId(ViewId("hill"))
      .addExtraBorderId(ViewId("mud"))
      .addExtraBorderId(ViewId("water")));
  addTile(ViewId("crops"), sprite("wheatfield1"));
  addTile(ViewId("crops2"), sprite("wheatfield2"));
  addTile(ViewId("mountain"), getMountainTile(sprite("mountain_ted"), "mountain").setWallShadow());
  addTile(ViewId("mountain2"), getMountainTile(sprite("mountain_ted2"), "mountain").setWallShadow());
  addTile(ViewId("dungeon_wall"), getMountainTile(sprite("mountain_ted"), "dungeonwall").setWallShadow());
  addTile(ViewId("dungeon_wall2"), getMountainTile(sprite("mountain_ted2"), "dungeonwall").setWallShadow());
  addTile(ViewId("wall"), getWallTile("wall"));
  addTile(ViewId("map_mountain1"), sprite("map_mountain1"));
  addTile(ViewId("map_mountain2"), sprite("map_mountain2"));
  addTile(ViewId("map_mountain3"), sprite("map_mountain3"));
}

void TileSet::genTiles3() {
  addTile(ViewId("gold_ore"), getMountainTile(sprite("gold_ore")
        .addBackground(byName("mountain_ted2")).setWallShadow(), "mountain"));
  addTile(ViewId("iron_ore"), getMountainTile(sprite("iron_ore")
        .addBackground(byName("mountain_ted2")).setWallShadow(), "mountain"));
  addTile(ViewId("stone"), getMountainTile(sprite("stone_ore")
        .addBackground(byName("mountain_ted2")).setWallShadow(), "mountain"));
  addTile(ViewId("adamantium_ore"), getMountainTile(sprite("adamantium")
        .addBackground(byName("mountain_ted2")).setWallShadow(), "mountain"));
  addTile(ViewId("hill"), getExtraBorderTile("hill")
      .addExtraBorderId(ViewId("sand"))
      .addExtraBorderId(ViewId("floor"))
      .addExtraBorderId(ViewId("keeper_floor"))
      .addExtraBorderId(ViewId("water")));
  addTile(ViewId("wood_wall"), getWallTile("wood_wall").setWallShadow());
  addTile(ViewId("castle_wall"), getWallTile("castle_wall").setWallShadow());
  addTile(ViewId("mud_wall"), getWallTile("mud_wall").setWallShadow());
  addTile(ViewId("ruin_wall"), getWallTile("ruin_wall"));
  addTile(ViewId("down_staircase"), sprite("down_stairs"));
  addTile(ViewId("up_staircase"), sprite("up_stairs"));
  addTile(ViewId("well"), sprite("well")
      .setRoundShadow());
  addTile(ViewId("minion_statue"), sprite("statue")
      .setRoundShadow());
  addTile(ViewId("stone_minion_statue"), sprite("statue_stone")
      .setRoundShadow());
  addTile(ViewId("throne"), sprite("throne").setRoundShadow());
  addTile(ViewId("orc"), sprite("orc"));
  addTile(ViewId("orc_captain"), sprite("orc_captain"));
  addTile(ViewId("orc_shaman"), sprite("orcshaman"));
  addTile(ViewId("harpy"), sprite("harpy"));
  addTile(ViewId("doppleganger"), sprite("dopple"));
  addTile(ViewId("succubus"), sprite("succubussmall"));
  addTile(ViewId("bandit"), sprite("bandit"));
  addTile(ViewId("ghost"), sprite("ghost4"));
  addTile(ViewId("spirit"), sprite("spirit"));
  addTile(ViewId("green_dragon"), sprite("greendragon"));
  addTile(ViewId("red_dragon"), sprite("reddragon"));
  addTile(ViewId("cyclops"), sprite("cyclops"));
  addTile(ViewId("minotaur"), sprite("mino"));
  addTile(ViewId("soft_monster"), sprite("softmonster"));
  addTile(ViewId("hydra"), sprite("hydra"));
  addTile(ViewId("shelob"), sprite("szelob"));
  addTile(ViewId("witch"), sprite("witch"));
  addTile(ViewId("witchman"), sprite("witchman"));
  addTile(ViewId("knight"), sprite("knight"));
  addTile(ViewId("jester"), sprite("jester"));
  addTile(ViewId("priest"), sprite("priest"));
  addTile(ViewId("warrior"), sprite("warrior"));
  addTile(ViewId("shaman"), sprite("shaman"));
  addTile(ViewId("duke1"), sprite("knightboss1"));
  addTile(ViewId("duke2"), sprite("knightboss2"));
  addTile(ViewId("duke3"), sprite("knightboss3"));
  addTile(ViewId("duke4"), sprite("knightboss4"));
  addTile(ViewId("duke_f1"), sprite("knightboss_f1"));
  addTile(ViewId("duke_f2"), sprite("knightboss_f2"));
  addTile(ViewId("duke_f3"), sprite("knightboss_f3"));
  addTile(ViewId("duke_f4"), sprite("knightboss_f4"));
  addTile(ViewId("archer"), sprite("archer"));
  addTile(ViewId("peseant"), sprite("peasant"));
  addTile(ViewId("peseant_woman"), sprite("peasantgirl"));
  addTile(ViewId("child"), sprite("peasantkid"));
  addTile(ViewId("clay_golem"), sprite("clay_golem"));
  addTile(ViewId("stone_golem"), sprite("stone_golem"));
  addTile(ViewId("iron_golem"), sprite("iron_golem"));
  addTile(ViewId("lava_golem"), sprite("lava_golem"));
  addTile(ViewId("ada_golem"), sprite("ada_golem"));
  addTile(ViewId("automaton"), sprite("automaton"));
  addTile(ViewId("elementalist"), sprite("elementalist"));
  addTile(ViewId("air_elemental"), sprite("airelement"));
  addTile(ViewId("fire_elemental"), sprite("fireelement"));
  addTile(ViewId("water_elemental"), sprite("waterelement"));
  addTile(ViewId("earth_elemental"), sprite("earthelement"));
  addTile(ViewId("ent"), sprite("ent"));
  addTile(ViewId("angel"), sprite("angel"));
  addTile(ViewId("zombie"), sprite("zombie"));
  addTile(ViewId("skeleton"), sprite("skeleton"));
  addTile(ViewId("vampire_lord"), sprite("vampirelord"));
  addTile(ViewId("mummy"), sprite("mummy"));
  addTile(ViewId("jackal"), sprite("jackal"));
  addTile(ViewId("deer"), sprite("deer"));
  addTile(ViewId("horse"), sprite("horse"));
  addTile(ViewId("cow"), sprite("cow"));
  addTile(ViewId("pig"), sprite("pig"));
  addTile(ViewId("donkey"), sprite("donkey"));
  addTile(ViewId("goat"), sprite("goat"));
  addTile(ViewId("boar"), sprite("boar"));
  addTile(ViewId("fox"), sprite("fox"));
  addTile(ViewId("wolf"), sprite("wolf"));
  addTile(ViewId("werewolf"), sprite("werewolf2"));
  addTile(ViewId("dog"), sprite("dog"));
  addTile(ViewId("kraken_head"), sprite("krakenhead"));
  addTile(ViewId("kraken_land"), sprite("krakenland1"));
  addTile(ViewId("kraken_water"), sprite("krakenwater2"));
  addTile(ViewId("death"), sprite("death"));
  addTile(ViewId("fire_sphere"), sprite("fire_sphere").setFX(FXVariantName::FIRE_SPHERE));
  addTile(ViewId("bear"), sprite("bear"));
  addTile(ViewId("bat"), sprite("bat"));
  addTile(ViewId("goblin"), sprite("goblin"));
  addTile(ViewId("rat"), sprite("rat"));
  addTile(ViewId("rat_soldier"), sprite("rat_soldier"));
  addTile(ViewId("rat_lady"), sprite("rat_lady"));
  addTile(ViewId("rat_king"), sprite("rat_king"));
  addTile(ViewId("spider"), sprite("spider"));
  addTile(ViewId("fly"), sprite("fly"));
  addTile(ViewId("ant_worker"), sprite("antwork"));
  addTile(ViewId("ant_soldier"), sprite("antw"));
  addTile(ViewId("ant_queen"), sprite("antq"));
  addTile(ViewId("snake"), sprite("snake"));
}

void TileSet::genTiles4() {
  addTile(ViewId("vulture"), sprite("vulture"));
  addTile(ViewId("raven"), sprite("raven"));
  addTile(ViewId("body_part"), sprite("corpse4"));
  addTile(ViewId("bone"), sprite("bone"));
  addTile(ViewId("bush"), sprite("bush").setRoundShadow());
  addTile(ViewId("water"), getWaterTile("wateranim", "water"));
  addTile(ViewId("magma"), getWaterTile("magmaanim", "magma"));
  addTile(ViewId("wood_door"), sprite("door_wood").setWallShadow());
  addTile(ViewId("iron_door"), sprite("door_iron").setWallShadow());
  addTile(ViewId("ada_door"), sprite("door_steel").setWallShadow());
  addTile(ViewId("barricade"), sprite("barricade").setRoundShadow());
  addTile(ViewId("dig_icon"), sprite("dig_icon"));
  addTile(ViewId("sword"), sprite("sword"));
  addTile(ViewId("ada_sword"), sprite("steel_sword"));
  addTile(ViewId("spear"), sprite("spear"));
  addTile(ViewId("special_sword"), sprite("special_sword"));
  addTile(ViewId("elven_sword"), sprite("elven_sword"));
  addTile(ViewId("knife"), sprite("knife"));
  addTile(ViewId("war_hammer"), sprite("warhammer"));
  addTile(ViewId("ada_war_hammer"), sprite("ada_warhammer"));
  addTile(ViewId("special_war_hammer"), sprite("special_warhammer"));
  addTile(ViewId("battle_axe"), sprite("battle_axe"));
  addTile(ViewId("ada_battle_axe"), sprite("steel_battle_axe"));
  addTile(ViewId("special_battle_axe"), sprite("special_battle_axe"));
  addTile(ViewId("bow"), sprite("bow"));
  addTile(ViewId("elven_bow"), sprite("bow")); //For now
  addTile(ViewId("arrow"), sprite("arrow_e"));
  addTile(ViewId("wooden_staff"), sprite("staff_wooden"));
  addTile(ViewId("iron_staff"), sprite("staff_iron"));
  addTile(ViewId("golden_staff"), sprite("staff_gold"));
  addTile(ViewId("force_bolt"), sprite("force_bolt"));
  addTile(ViewId("fireball"), sprite("fireball"));
  addTile(ViewId("air_blast"), sprite("air_blast"));
  addTile(ViewId("stun_ray"), sprite("stun_ray"));
  addTile(ViewId("club"), sprite("club"));
  addTile(ViewId("heavy_club"), sprite("heavy club"));
  addTile(ViewId("scroll"), sprite("scroll"));
  addTile(ViewId("amulet1"), sprite("amulet1"));
  addTile(ViewId("amulet2"), sprite("amulet2"));
  addTile(ViewId("amulet3"), sprite("amulet3"));
  addTile(ViewId("amulet4"), sprite("amulet4"));
  addTile(ViewId("amulet5"), sprite("amulet5"));
  addTile(ViewId("fire_resist_ring"), sprite("ring_red"));
  addTile(ViewId("poison_resist_ring"), sprite("ring_green"));
  addTile(ViewId("book"), sprite("book"));
  addTile(ViewId("hand_torch"), sprite("hand_torch"));
  addTile(ViewId("first_aid"), sprite("medkit"));
  addTile(ViewId("trap_item"), sprite("trapbox"));
  addTile(ViewId("potion1"), sprite("potion1"));
  addTile(ViewId("potion2"), sprite("potion2"));
  addTile(ViewId("potion3"), sprite("potion3"));
  addTile(ViewId("potion4"), sprite("potion4"));
  addTile(ViewId("potion5"), sprite("potion5"));
  addTile(ViewId("potion6"), sprite("potion6"));
  addTile(ViewId("rune1"), sprite("rune1"));
  addTile(ViewId("rune2"), sprite("rune2"));
  addTile(ViewId("rune3"), sprite("rune3"));
  addTile(ViewId("rune4"), sprite("rune4"));
  addTile(ViewId("mushroom1"), sprite("mushroom1").setRoundShadow());
  addTile(ViewId("mushroom2"), sprite("mushroom2").setRoundShadow());
  addTile(ViewId("mushroom3"), sprite("mushroom3").setRoundShadow());
  addTile(ViewId("mushroom4"), sprite("mushroom4").setRoundShadow());
  addTile(ViewId("mushroom5"), sprite("mushroom5").setRoundShadow());
  addTile(ViewId("mushroom6"), sprite("mushroom6").setRoundShadow());
  addTile(ViewId("mushroom7"), sprite("mushroom2").setRoundShadow().setColor(Color::LIGHT_BLUE));
  addTile(ViewId("key"), sprite("key"));
  addTile(ViewId("fountain"), sprite("fountain").setRoundShadow());
  addTile(ViewId("gold"), sprite("gold"));
  addTile(ViewId("treasure_chest"), sprite("treasurydeco"));
  addTile(ViewId("chest"), sprite("chest").setRoundShadow());
  addTile(ViewId("opened_chest"), sprite("chest_opened").setRoundShadow());
  addTile(ViewId("coffin1"), sprite("coffin"));
  addTile(ViewId("coffin2"), sprite("coffin2"));
  addTile(ViewId("coffin3"), sprite("coffin3"));
  addTile(ViewId("loot_coffin"), sprite("coffin"));
  addTile(ViewId("opened_coffin"), sprite("coffin_opened"));
  addTile(ViewId("boulder"), sprite("boulder").setRoundShadow());
  addTile(ViewId("portal"), sprite("surprise").setRoundShadow());
  addTile(ViewId("gas_trap"), sprite("gas_trap"));
  addTile(ViewId("alarm_trap"), sprite("alarm_trap"));
  addTile(ViewId("web_trap"), sprite("web_trap"));
  addTile(ViewId("surprise_trap"), sprite("surprisetrap"));
  addTile(ViewId("terror_trap"), sprite("terror_trap"));
  addTile(ViewId("fire_trap"), sprite("fire_trap"));
  addTile(ViewId("rock"), sprite("stonepile"));
  addTile(ViewId("iron_rock"), sprite("ironpile"));
  addTile(ViewId("ada_ore"), sprite("steelpile"));
  addTile(ViewId("wood_plank"), sprite("wood2"));
  addTile(ViewId("storage_equipment"), sprite("dig_mark").setColor(Color::BLUE.transparency(120)));
  addTile(ViewId("storage_resources"), sprite("dig_mark").setColor(Color::GREEN.transparency(120)));
  addTile(ViewId("quarters1"), sprite("dig_mark2").setColor(Color::PINK.transparency(120)));
  addTile(ViewId("quarters2"), sprite("dig_mark2").setColor(Color::SKY_BLUE.transparency(120)));
  addTile(ViewId("quarters3"), sprite("dig_mark2").setColor(Color::ORANGE.transparency(120)));
  addTile(ViewId("leisure"), sprite("dig_mark2").setColor(Color::DARK_BLUE.transparency(120)));
  addTile(ViewId("prison"), sprite("prison"));
  addTile(ViewId("bed1"), sprite("bed1").setRoundShadow());
  addTile(ViewId("bed2"), sprite("bed2").setRoundShadow());
  addTile(ViewId("bed3"), sprite("bed3").setRoundShadow());
  addTile(ViewId("dorm"), sprite("sleep").setFloorBorders());
  addTile(ViewId("torch"), sprite("torch"));
  addTile(ViewId("candelabrum_ns"), sprite("candelabrum_ns"));
  addTile(ViewId("candelabrum_w"), sprite("candelabrum_w"));
  addTile(ViewId("candelabrum_e"), sprite("candelabrum_e"));
  addTile(ViewId("standing_torch"), sprite("standing_torch").setMoveUp());
  addTile(ViewId("altar"), sprite("altar").setRoundShadow());
}

void TileSet::genTiles5() {
  addTile(ViewId("creature_altar"), sprite("altar2").setRoundShadow());
  addTile(ViewId("altar_des"), sprite("altar_des").setRoundShadow());
  addTile(ViewId("torture_table"), sprite("torturedeco").setRoundShadow());
  addTile(ViewId("impaled_head"), sprite("impaledhead").setRoundShadow());
  addTile(ViewId("whipping_post"), sprite("whipping_post").setRoundShadow());
  addTile(ViewId("gallows"), sprite("gallows").setRoundShadow());
  addTile(ViewId("notice_board"), sprite("board").setRoundShadow());
  addTile(ViewId("sokoban_hole"), sprite("hole"));
  addTile(ViewId("training_wood"), sprite("train_wood").setRoundShadow());
  addTile(ViewId("training_iron"), sprite("train_iron").setRoundShadow());
  addTile(ViewId("training_ada"), sprite("train_steel").setRoundShadow());
  addTile(ViewId("archery_range"), sprite("archery_range").setRoundShadow());
  addTile(ViewId("demon_shrine"), sprite("demon_shrine").setRoundShadow());
  addTile(ViewId("bookcase_wood"), sprite("library_wood").setRoundShadow());
  addTile(ViewId("bookcase_iron"), sprite("library_iron").setRoundShadow());
  addTile(ViewId("bookcase_gold"), sprite("library_gold").setRoundShadow());
  addTile(ViewId("laboratory"), sprite("labdeco").setRoundShadow());
  addTile(ViewId("cauldron"), sprite("labdeco"));
  addTile(ViewId("beast_lair"), sprite("lair").setFloorBorders());
  addTile(ViewId("beast_cage"), sprite("lairdeco").setRoundShadow());
  addTile(ViewId("forge"), sprite("forgedeco").setRoundShadow());
  addTile(ViewId("workshop"), sprite("workshopdeco").setRoundShadow());
  addTile(ViewId("jeweller"), sprite("jewelerdeco").setRoundShadow());
  addTile(ViewId("furnace"), sprite("steel_furnace").setRoundShadow());
  addTile(ViewId("grave"), sprite("RIP").setRoundShadow());
  addTile(ViewId("robe"), sprite("robe"));
  addTile(ViewId("leather_gloves"), sprite("leather_gloves"));
  addTile(ViewId("iron_gloves"), sprite("iron_gloves"));
  addTile(ViewId("ada_gloves"), sprite("ada_gloves"));
  addTile(ViewId("dexterity_gloves"), sprite("blue_gloves"));
  addTile(ViewId("strength_gloves"), sprite("red_gloves"));
  addTile(ViewId("leather_armor"), sprite("leather_armor"));
  addTile(ViewId("leather_helm"), sprite("leather_helm"));
  addTile(ViewId("telepathy_helm"), sprite("blue_helm"));
  addTile(ViewId("chain_armor"), sprite("chain_armor"));
  addTile(ViewId("ada_armor"), sprite("steel_armor"));
  addTile(ViewId("iron_helm"), sprite("iron_helm"));
  addTile(ViewId("ada_helm"), sprite("ada_helm"));
  addTile(ViewId("leather_boots"), sprite("leather_boots"));
  addTile(ViewId("iron_boots"), sprite("iron_boots"));
  addTile(ViewId("ada_boots"), sprite("ada_boots"));
  addTile(ViewId("speed_boots"), sprite("blue_boots"));
  addTile(ViewId("levitation_boots"), sprite("green_boots"));
  addTile(ViewId("wooden_shield"), sprite("wooden_shield"));
  addTile(ViewId("guard_post"), sprite("guardroom").setRoundShadow());
  addTile(ViewId("mana"), sprite("mana"));
  addTile(ViewId("fetch_icon"), sprite("leather_gloves"));
  addTile(ViewId("eyeball"), sprite("eyeball2").setRoundShadow());
  addTile(ViewId("fog_of_war"), getWaterTile("empty", "fogofwar"));
  addTile(ViewId("pit"), sprite("pit"));
  addTile(ViewId("creature_highlight"), sprite("creature_highlight"));
  addTile(ViewId("square_highlight"), sprite("square_highlight"));
  addTile(ViewId("round_shadow"), sprite("round_shadow"));
  addTile(ViewId("campaign_defeated"), sprite("campaign_defeated"));
  addTile(ViewId("accept_immigrant"), symbol(u8"✓", Color::GREEN, true));
  addTile(ViewId("reject_immigrant"), symbol(u8"✘", Color::RED, true));
  addTile(ViewId("fog_of_war_corner"), sprite("fogofwar")
      .addConnection({Dir::NE}, byName("fogofwarcornne"))
      .addConnection({Dir::NW}, byName("fogofwarcornnw"))
      .addConnection({Dir::SE}, byName("fogofwarcornse"))
      .addConnection({Dir::SW}, byName("fogofwarcornsw")));
  addTile(ViewId("special_blbn"), sprite("special_blbn"));
  addTile(ViewId("special_blbw"), sprite("special_blbw"));
  addTile(ViewId("special_blgn"), sprite("special_blgn"));
  addTile(ViewId("special_blgw"), sprite("special_blgw"));
  addTile(ViewId("special_bmbn"), sprite("special_bmbn"));
  addTile(ViewId("special_bmbw"), sprite("special_bmbw"));
  addTile(ViewId("special_bmgn"), sprite("special_bmgn"));
  addTile(ViewId("special_bmgw"), sprite("special_bmgw"));
  addTile(ViewId("special_hlbn"), sprite("special_hlbn"));
  addTile(ViewId("special_hlbw"), sprite("special_hlbw"));
  addTile(ViewId("special_hlgn"), sprite("special_hlgn"));
  addTile(ViewId("special_hlgw"), sprite("special_hlgw"));
  addTile(ViewId("special_hmbn"), sprite("special_hmbn"));
  addTile(ViewId("special_hmbw"), sprite("special_hmbw"));
  addTile(ViewId("special_hmgn"), sprite("special_hmgn"));
  addTile(ViewId("special_hmgw"), sprite("special_hmgw"));
  addTile(ViewId("halloween_kid1"), sprite("halloween_kid1"));
  addTile(ViewId("halloween_kid2"), sprite("halloween_kid2"));
  addTile(ViewId("halloween_kid3"), sprite("halloween_kid3"));
  addTile(ViewId("halloween_kid4"), sprite("halloween_kid4"));
  addTile(ViewId("halloween_costume"), sprite("halloween_costume"));
  addTile(ViewId("bag_of_candy"), sprite("halloween_candies"));
  addTile(ViewId("horn_attack"), sprite("horn_attack"));
  addTile(ViewId("beak_attack"), sprite("beak_attack"));
  addTile(ViewId("touch_attack"), sprite("touch_attack"));
  addTile(ViewId("bite_attack"), sprite("bite_attack"));
  addTile(ViewId("claws_attack"), sprite("claws_attack"));
  addTile(ViewId("leg_attack"), sprite("leg_attack"));
  addTile(ViewId("fist_attack"), sprite("fist_attack"));
  addTile(ViewId("item_aura"), sprite("aura"));
#ifndef RELEASE
  addTile(ViewId("tutorial_entrance"), symbol(u8"?", Color::YELLOW));
#else
  addTile(ViewId("tutorial_entrance"), sprite("empty"));
#endif
}

Tile TileSet::symbol(const string& s, Color id, bool symbol) {
  return Tile::fromString(s, id, symbol);
}

void TileSet::genSymbols1() {
  addSymbol(ViewId("demon_dweller"), symbol(u8"U", Color::PURPLE));
  addSymbol(ViewId("demon_lord"), symbol(u8"U", Color::YELLOW));
  addSymbol(ViewId("empty"), symbol(u8" ", Color::BLACK));
  addSymbol(ViewId("dig_mark"), symbol(u8" ", Color::BLACK));
  addSymbol(ViewId("dig_mark2"), symbol(u8" ", Color::BLACK));
  addSymbol(ViewId("player"), symbol(u8"@", Color::WHITE));
  addSymbol(ViewId("player_f"), symbol(u8"@", Color::YELLOW));
  addSymbol(ViewId("keeper1"), symbol(u8"@", Color::PURPLE));
  addSymbol(ViewId("keeper2"), symbol(u8"@", Color::PURPLE));
  addSymbol(ViewId("keeper3"), symbol(u8"@", Color::PURPLE));
  addSymbol(ViewId("keeper4"), symbol(u8"@", Color::PURPLE));
  addSymbol(ViewId("keeper_f1"), symbol(u8"@", Color::PINK));
  addSymbol(ViewId("keeper_f2"), symbol(u8"@", Color::PINK));
  addSymbol(ViewId("keeper_f3"), symbol(u8"@", Color::PINK));
  addSymbol(ViewId("keeper_f4"), symbol(u8"@", Color::PINK));
  addSymbol(ViewId("keeper_knight1"), symbol(u8"@", Color::YELLOW));
  addSymbol(ViewId("keeper_knight2"), symbol(u8"@", Color::YELLOW));
  addSymbol(ViewId("keeper_knight3"), symbol(u8"@", Color::YELLOW));
  addSymbol(ViewId("keeper_knight4"), symbol(u8"@", Color::YELLOW));
  addSymbol(ViewId("keeper_knight_f1"), symbol(u8"@", Color::YELLOW));
  addSymbol(ViewId("keeper_knight_f2"), symbol(u8"@", Color::YELLOW));
  addSymbol(ViewId("keeper_knight_f3"), symbol(u8"@", Color::YELLOW));
  addSymbol(ViewId("keeper_knight_f4"), symbol(u8"@", Color::YELLOW));
  addSymbol(ViewId("unknown_monster"), symbol(u8"?", Color::LIGHT_GREEN));
  addSymbol(ViewId("elf"), symbol(u8"@", Color::LIGHT_GREEN));
  addSymbol(ViewId("elf_woman"), symbol(u8"@", Color::LIGHT_GREEN));
  addSymbol(ViewId("elf_archer"), symbol(u8"@", Color::GREEN));
  addSymbol(ViewId("elf_child"), symbol(u8"@", Color::LIGHT_GREEN));
  addSymbol(ViewId("elf_lord"), symbol(u8"@", Color::DARK_GREEN));
  addSymbol(ViewId("dark_elf"), symbol(u8"@", Color::ALMOST_DARK_GRAY));
  addSymbol(ViewId("dark_elf_woman"), symbol(u8"@", Color::ALMOST_DARK_GRAY));
  addSymbol(ViewId("dark_elf_warrior"), symbol(u8"@", Color::GRAY));
  addSymbol(ViewId("dark_elf_child"), symbol(u8"@", Color::ALMOST_GRAY));
  addSymbol(ViewId("dark_elf_lord"), symbol(u8"@", Color::LIGHT_GRAY));
  addSymbol(ViewId("driad"), symbol(u8"@", Color::LIGHT_BROWN));
  addSymbol(ViewId("unicorn"), symbol(u8"h", Color::WHITE));
  addSymbol(ViewId("kobold"), symbol(u8"k", Color::LIGHT_BROWN));
  addSymbol(ViewId("shopkeeper"), symbol(u8"@", Color::LIGHT_BLUE));
  addSymbol(ViewId("lizardman"), symbol(u8"@", Color::LIGHT_BROWN));
  addSymbol(ViewId("lizardlord"), symbol(u8"@", Color::BROWN));
  addSymbol(ViewId("imp"), symbol(u8"i", Color::LIGHT_BROWN));
  addSymbol(ViewId("prisoner"), symbol(u8"@", Color::LIGHT_BROWN));
  addSymbol(ViewId("ogre"), symbol(u8"O", Color::GREEN));
  addSymbol(ViewId("chicken"), symbol(u8"c", Color::YELLOW));
  addSymbol(ViewId("gnome"), symbol(u8"g", Color::GREEN));
  addSymbol(ViewId("gnome_boss"), symbol(u8"g", Color::DARK_BLUE));
  addSymbol(ViewId("dwarf"), symbol(u8"h", Color::BLUE));
  addSymbol(ViewId("dwarf_baron"), symbol(u8"h", Color::DARK_BLUE));
  addSymbol(ViewId("dwarf_female"), symbol(u8"h", Color::LIGHT_BLUE));
  addSymbol(ViewId("floor"), symbol(u8".", Color::LIGHT_GRAY));
  addSymbol(ViewId("keeper_floor"), symbol(u8".", Color::WHITE));
  addSymbol(ViewId("wood_floor1"), symbol(u8".", Color::LIGHT_BROWN));
  addSymbol(ViewId("wood_floor2"), symbol(u8".", Color::BROWN));
  addSymbol(ViewId("wood_floor3"), symbol(u8".", Color::LIGHT_BROWN));
  addSymbol(ViewId("wood_floor4"), symbol(u8".", Color::BROWN));
  addSymbol(ViewId("wood_floor5"), symbol(u8".", Color::BROWN));
  addSymbol(ViewId("stone_floor1"), symbol(u8".", Color::LIGHT_GRAY));
  addSymbol(ViewId("stone_floor2"), symbol(u8".", Color::GRAY));
  addSymbol(ViewId("stone_floor3"), symbol(u8".", Color::LIGHT_GRAY));
  addSymbol(ViewId("stone_floor4"), symbol(u8".", Color::GRAY));
  addSymbol(ViewId("stone_floor5"), symbol(u8".", Color::GRAY));
  addSymbol(ViewId("carpet_floor1"), symbol(u8".", Color::PURPLE));
  addSymbol(ViewId("carpet_floor2"), symbol(u8".", Color::PINK));
  addSymbol(ViewId("carpet_floor3"), symbol(u8".", Color::PURPLE));
  addSymbol(ViewId("carpet_floor4"), symbol(u8".", Color::PINK));
  addSymbol(ViewId("carpet_floor5"), symbol(u8".", Color::PINK));
  addSymbol(ViewId("buff_floor1"), symbol(u8".", Color::PURPLE));
  addSymbol(ViewId("buff_floor2"), symbol(u8".", Color::PINK));
  addSymbol(ViewId("buff_floor3"), symbol(u8".", Color::PURPLE));
  addSymbol(ViewId("bridge"), symbol(u8"_", Color::BROWN));
  addSymbol(ViewId("road"), symbol(u8".", Color::LIGHT_GRAY));
  addSymbol(ViewId("sand"), symbol(u8".", Color::YELLOW));
  addSymbol(ViewId("mud"), symbol(u8"𝃰", Color::BROWN, true));
  addSymbol(ViewId("grass"), symbol(u8"𝃰", Color::GREEN, true));
}

void TileSet::genSymbols2() {
  addSymbol(ViewId("crops"), symbol(u8"𝃰", Color::YELLOW, true));
  addSymbol(ViewId("crops2"), symbol(u8"𝃰", Color::YELLOW, true));
  addSymbol(ViewId("castle_wall"), symbol(u8"#", Color::LIGHT_GRAY));
  addSymbol(ViewId("mud_wall"), symbol(u8"#", Color::LIGHT_BROWN));
  addSymbol(ViewId("ruin_wall"), symbol(u8"#", Color::DARK_GRAY));
  addSymbol(ViewId("wall"), symbol(u8"#", Color::LIGHT_GRAY));
  addSymbol(ViewId("mountain"), symbol(u8"#", Color::GRAY));
  addSymbol(ViewId("mountain2"), symbol(u8"#", Color::DARK_GRAY));
  addSymbol(ViewId("dungeon_wall"), symbol(u8"#", Color::LIGHT_GRAY));
  addSymbol(ViewId("dungeon_wall2"), symbol(u8"#", Color::DARK_BLUE));
  addSymbol(ViewId("map_mountain1"), symbol(u8"^", Color::DARK_GRAY));
  addSymbol(ViewId("map_mountain2"), symbol(u8"^", Color::GRAY));
  addSymbol(ViewId("map_mountain3"), symbol(u8"^", Color::LIGHT_GRAY));
  addSymbol(ViewId("gold_ore"), symbol(u8"⁂", Color::YELLOW, true));
  addSymbol(ViewId("iron_ore"), symbol(u8"⁂", Color::DARK_BROWN, true));
  addSymbol(ViewId("stone"), symbol(u8"⁂", Color::LIGHT_GRAY, true));
  addSymbol(ViewId("adamantium_ore"), symbol(u8"⁂", Color::LIGHT_BLUE, true));
  addSymbol(ViewId("hill"), symbol(u8"𝀢", Color::DARK_GREEN, true));
  addSymbol(ViewId("wood_wall"), symbol(u8"#", Color::DARK_BROWN));
  addSymbol(ViewId("down_staircase"), symbol(u8"➘", Color::ALMOST_WHITE, true));
  addSymbol(ViewId("up_staircase"), symbol(u8"➚", Color::ALMOST_WHITE, true));
  addSymbol(ViewId("well"), symbol(u8"0", Color::BLUE));
  addSymbol(ViewId("minion_statue"), symbol(u8"&", Color::LIGHT_GRAY));
  addSymbol(ViewId("stone_minion_statue"), symbol(u8"&", Color::ALMOST_WHITE));
  addSymbol(ViewId("throne"), symbol(u8"Ω", Color::YELLOW, true));
  addSymbol(ViewId("orc"), symbol(u8"o", Color::DARK_BLUE));
  addSymbol(ViewId("orc_captain"), symbol(u8"o", Color::PURPLE));
  addSymbol(ViewId("orc_shaman"), symbol(u8"o", Color::YELLOW));
  addSymbol(ViewId("harpy"), symbol(u8"R", Color::YELLOW));
  addSymbol(ViewId("doppleganger"), symbol(u8"&", Color::YELLOW));
  addSymbol(ViewId("succubus"), symbol(u8"&", Color::RED));
  addSymbol(ViewId("bandit"), symbol(u8"@", Color::DARK_BLUE));
  addSymbol(ViewId("green_dragon"), symbol(u8"D", Color::GREEN));
  addSymbol(ViewId("red_dragon"), symbol(u8"D", Color::RED));
  addSymbol(ViewId("cyclops"), symbol(u8"C", Color::GREEN));
  addSymbol(ViewId("minotaur"), symbol(u8"M", Color::PURPLE));
  addSymbol(ViewId("soft_monster"), symbol(u8"M", Color::PINK));
  addSymbol(ViewId("hydra"), symbol(u8"H", Color::PURPLE));
  addSymbol(ViewId("shelob"), symbol(u8"S", Color::PURPLE));
  addSymbol(ViewId("witch"), symbol(u8"@", Color::BROWN));
  addSymbol(ViewId("witchman"), symbol(u8"@", Color::WHITE));
  addSymbol(ViewId("ghost"), symbol(u8"&", Color::WHITE));
  addSymbol(ViewId("spirit"), symbol(u8"&", Color::LIGHT_BLUE));
  addSymbol(ViewId("knight"), symbol(u8"@", Color::LIGHT_GRAY));
  addSymbol(ViewId("priest"), symbol(u8"@", Color::SKY_BLUE));
  addSymbol(ViewId("jester"), symbol(u8"@", Color::PINK));
  addSymbol(ViewId("warrior"), symbol(u8"@", Color::DARK_GRAY));
  addSymbol(ViewId("shaman"), symbol(u8"@", Color::YELLOW));
  addSymbol(ViewId("duke1"), symbol(u8"@", Color::BLUE));
  addSymbol(ViewId("duke2"), symbol(u8"@", Color::BLUE));
  addSymbol(ViewId("duke3"), symbol(u8"@", Color::BLUE));
  addSymbol(ViewId("duke4"), symbol(u8"@", Color::BLUE));
  addSymbol(ViewId("duke_f1"), symbol(u8"@", Color::BLUE));
  addSymbol(ViewId("duke_f2"), symbol(u8"@", Color::BLUE));
  addSymbol(ViewId("duke_f3"), symbol(u8"@", Color::BLUE));
  addSymbol(ViewId("duke_f4"), symbol(u8"@", Color::BLUE));
  addSymbol(ViewId("archer"), symbol(u8"@", Color::BROWN));
  addSymbol(ViewId("peseant"), symbol(u8"@", Color::GREEN));
  addSymbol(ViewId("peseant_woman"), symbol(u8"@", Color::GREEN));
  addSymbol(ViewId("child"), symbol(u8"@", Color::LIGHT_GREEN));
  addSymbol(ViewId("clay_golem"), symbol(u8"Y", Color::YELLOW));
  addSymbol(ViewId("stone_golem"), symbol(u8"Y", Color::LIGHT_GRAY));
  addSymbol(ViewId("iron_golem"), symbol(u8"Y", Color::ORANGE));
  addSymbol(ViewId("lava_golem"), symbol(u8"Y", Color::PURPLE));
  addSymbol(ViewId("ada_golem"), symbol(u8"Y", Color::LIGHT_BLUE));
  addSymbol(ViewId("automaton"), symbol(u8"A", Color::LIGHT_GRAY));
  addSymbol(ViewId("elementalist"), symbol(u8"@", Color::YELLOW));
  addSymbol(ViewId("air_elemental"), symbol(u8"E", Color::LIGHT_BLUE));
  addSymbol(ViewId("fire_elemental"), symbol(u8"E", Color::RED));
  addSymbol(ViewId("water_elemental"), symbol(u8"E", Color::BLUE));
  addSymbol(ViewId("earth_elemental"), symbol(u8"E", Color::GRAY));
  addSymbol(ViewId("ent"), symbol(u8"E", Color::LIGHT_BROWN));
  addSymbol(ViewId("angel"), symbol(u8"A", Color::LIGHT_BLUE));
  addSymbol(ViewId("zombie"), symbol(u8"Z", Color::GREEN));
  addSymbol(ViewId("skeleton"), symbol(u8"Z", Color::WHITE));
  addSymbol(ViewId("vampire"), symbol(u8"V", Color::DARK_GRAY));
  addSymbol(ViewId("vampire_lord"), symbol(u8"V", Color::PURPLE));
  addSymbol(ViewId("mummy"), symbol(u8"Z", Color::YELLOW));
}
void TileSet::genSymbols3() {
  addSymbol(ViewId("jackal"), symbol(u8"d", Color::LIGHT_BROWN));
  addSymbol(ViewId("deer"), symbol(u8"R", Color::DARK_BROWN));
  addSymbol(ViewId("horse"), symbol(u8"H", Color::LIGHT_BROWN));
  addSymbol(ViewId("cow"), symbol(u8"C", Color::WHITE));
  addSymbol(ViewId("pig"), symbol(u8"p", Color::YELLOW));
  addSymbol(ViewId("donkey"), symbol(u8"D", Color::GRAY));
  addSymbol(ViewId("goat"), symbol(u8"g", Color::GRAY));
  addSymbol(ViewId("boar"), symbol(u8"b", Color::LIGHT_BROWN));
  addSymbol(ViewId("fox"), symbol(u8"d", Color::ORANGE_BROWN));
  addSymbol(ViewId("wolf"), symbol(u8"d", Color::DARK_BLUE));
  addSymbol(ViewId("werewolf"), symbol(u8"d", Color::WHITE));
  addSymbol(ViewId("dog"), symbol(u8"d", Color::BROWN));
  addSymbol(ViewId("kraken_head"), symbol(u8"S", Color::GREEN));
  addSymbol(ViewId("kraken_water"), symbol(u8"S", Color::DARK_GREEN));
  addSymbol(ViewId("kraken_land"), symbol(u8"S", Color::DARK_GREEN));
  addSymbol(ViewId("death"), symbol(u8"D", Color::DARK_GRAY));
  addSymbol(ViewId("fire_sphere"), symbol(u8"e", Color::RED));
  addSymbol(ViewId("bear"), symbol(u8"N", Color::BROWN));
  addSymbol(ViewId("bat"), symbol(u8"b", Color::DARK_GRAY));
  addSymbol(ViewId("goblin"), symbol(u8"o", Color::GREEN));
  addSymbol(ViewId("rat"), symbol(u8"r", Color::BROWN));
  addSymbol(ViewId("rat_king"), symbol(u8"@", Color::LIGHT_BROWN));
  addSymbol(ViewId("rat_soldier"), symbol(u8"@", Color::BROWN));
  addSymbol(ViewId("rat_lady"), symbol(u8"@", Color::DARK_BROWN));
  addSymbol(ViewId("spider"), symbol(u8"s", Color::BROWN));
  addSymbol(ViewId("ant_worker"), symbol(u8"a", Color::BROWN));
  addSymbol(ViewId("ant_soldier"), symbol(u8"a", Color::YELLOW));
  addSymbol(ViewId("ant_queen"), symbol(u8"a", Color::PURPLE));
  addSymbol(ViewId("fly"), symbol(u8"b", Color::GRAY));
  addSymbol(ViewId("snake"), symbol(u8"S", Color::YELLOW));
  addSymbol(ViewId("vulture"), symbol(u8"v", Color::DARK_GRAY));
  addSymbol(ViewId("raven"), symbol(u8"v", Color::DARK_GRAY));
  addSymbol(ViewId("body_part"), symbol(u8"%", Color::RED));
  addSymbol(ViewId("bone"), symbol(u8"%", Color::WHITE));
  addSymbol(ViewId("bush"), symbol(u8"&", Color::DARK_GREEN));
  addSymbol(ViewId("decid_tree"), symbol(u8"🜍", Color::DARK_GREEN, true));
  addSymbol(ViewId("canif_tree"), symbol(u8"♣", Color::DARK_GREEN, true));
  addSymbol(ViewId("tree_trunk"), symbol(u8".", Color::BROWN));
  addSymbol(ViewId("burnt_tree"), symbol(u8".", Color::DARK_GRAY));
  addSymbol(ViewId("water"), symbol(u8"~", Color::LIGHT_BLUE));
  addSymbol(ViewId("magma"), symbol(u8"~", Color::RED));
  addSymbol(ViewId("wood_door"), symbol(u8"|", Color::BROWN));
  addSymbol(ViewId("iron_door"), symbol(u8"|", Color::LIGHT_GRAY));
  addSymbol(ViewId("ada_door"), symbol(u8"|", Color::LIGHT_BLUE));
  addSymbol(ViewId("barricade"), symbol(u8"X", Color::BROWN));
  addSymbol(ViewId("dig_icon"), symbol(u8"⛏", Color::LIGHT_GRAY, true));
  addSymbol(ViewId("ada_sword"), symbol(u8")", Color::LIGHT_BLUE));
  addSymbol(ViewId("sword"), symbol(u8")", Color::LIGHT_GRAY));
  addSymbol(ViewId("spear"), symbol(u8"/", Color::LIGHT_GRAY));
  addSymbol(ViewId("special_sword"), symbol(u8")", Color::YELLOW));
  addSymbol(ViewId("elven_sword"), symbol(u8")", Color::GRAY));
  addSymbol(ViewId("knife"), symbol(u8")", Color::WHITE));
  addSymbol(ViewId("war_hammer"), symbol(u8")", Color::BLUE));
  addSymbol(ViewId("ada_war_hammer"), symbol(u8")", Color::LIGHT_BLUE));
  addSymbol(ViewId("special_war_hammer"), symbol(u8")", Color::LIGHT_BLUE));
  addSymbol(ViewId("battle_axe"), symbol(u8")", Color::GREEN));
  addSymbol(ViewId("ada_battle_axe"), symbol(u8")", Color::LIGHT_BLUE));
  addSymbol(ViewId("special_battle_axe"), symbol(u8")", Color::LIGHT_GREEN));
  addSymbol(ViewId("elven_bow"), symbol(u8")", Color::YELLOW));
  addSymbol(ViewId("bow"), symbol(u8")", Color::BROWN));
  addSymbol(ViewId("wooden_staff"), symbol(u8")", Color::YELLOW));
  addSymbol(ViewId("iron_staff"), symbol(u8")", Color::ORANGE));
  addSymbol(ViewId("golden_staff"), symbol(u8")", Color::YELLOW));
  addSymbol(ViewId("club"), symbol(u8")", Color::BROWN));
  addSymbol(ViewId("heavy_club"), symbol(u8")", Color::BROWN));
  addSymbol(ViewId("arrow"), symbol(u8"/", Color::BROWN));
  addSymbol(ViewId("force_bolt"), symbol(u8"*", Color::LIGHT_BLUE));
  addSymbol(ViewId("fireball"), symbol(u8"*", Color::ORANGE));
  addSymbol(ViewId("air_blast"), symbol(u8"*", Color::WHITE));
  addSymbol(ViewId("stun_ray"), symbol(u8"*", Color::LIGHT_GREEN));
  addSymbol(ViewId("scroll"), symbol(u8"?", Color::WHITE));
  addSymbol(ViewId("amulet1"), symbol(u8"\"", Color::YELLOW));
}
void TileSet::genSymbols4() {
  addSymbol(ViewId("amulet2"), symbol(u8"\"", Color::YELLOW));
  addSymbol(ViewId("amulet3"), symbol(u8"\"", Color::YELLOW));
  addSymbol(ViewId("amulet4"), symbol(u8"\"", Color::YELLOW));
  addSymbol(ViewId("amulet5"), symbol(u8"\"", Color::YELLOW));
  addSymbol(ViewId("fire_resist_ring"), symbol(u8"=", Color::RED));
  addSymbol(ViewId("poison_resist_ring"), symbol(u8"=", Color::GREEN));
  addSymbol(ViewId("book"), symbol(u8"+", Color::YELLOW));
  addSymbol(ViewId("hand_torch"), symbol(u8"/", Color::YELLOW));
  addSymbol(ViewId("first_aid"), symbol(u8"+", Color::RED));
  addSymbol(ViewId("trap_item"), symbol(u8"+", Color::YELLOW));
  addSymbol(ViewId("potion1"), symbol(u8"!", Color::LIGHT_RED));
  addSymbol(ViewId("potion2"), symbol(u8"!", Color::BLUE));
  addSymbol(ViewId("potion3"), symbol(u8"!", Color::YELLOW));
  addSymbol(ViewId("potion4"), symbol(u8"!", Color::VIOLET));
  addSymbol(ViewId("potion5"), symbol(u8"!", Color::DARK_BROWN));
  addSymbol(ViewId("potion6"), symbol(u8"!", Color::LIGHT_GRAY));
  addSymbol(ViewId("rune1"), symbol(u8"~", Color::GREEN));
  addSymbol(ViewId("rune2"), symbol(u8"~", Color::PURPLE));
  addSymbol(ViewId("rune3"), symbol(u8"~", Color::RED));
  addSymbol(ViewId("rune4"), symbol(u8"~", Color::BLUE));
  addSymbol(ViewId("mushroom1"), symbol(u8"⋆", Color::PINK, true));
  addSymbol(ViewId("mushroom2"), symbol(u8"⋆", Color::YELLOW, true));
  addSymbol(ViewId("mushroom3"), symbol(u8"⋆", Color::PURPLE, true));
  addSymbol(ViewId("mushroom4"), symbol(u8"⋆", Color::BROWN, true));
  addSymbol(ViewId("mushroom5"), symbol(u8"⋆", Color::LIGHT_GRAY, true));
  addSymbol(ViewId("mushroom6"), symbol(u8"⋆", Color::ORANGE, true));
  addSymbol(ViewId("mushroom7"), symbol(u8"⋆", Color::LIGHT_BLUE, true));
  addSymbol(ViewId("key"), symbol(u8"*", Color::YELLOW));
  addSymbol(ViewId("fountain"), symbol(u8"0", Color::LIGHT_BLUE));
  addSymbol(ViewId("gold"), symbol(u8"$", Color::YELLOW));
  addSymbol(ViewId("treasure_chest"), symbol(u8"=", Color::BROWN));
  addSymbol(ViewId("opened_chest"), symbol(u8"=", Color::BROWN));
  addSymbol(ViewId("chest"), symbol(u8"=", Color::BROWN));
  addSymbol(ViewId("opened_coffin"), symbol(u8"⚰", Color::DARK_BROWN, true));
  addSymbol(ViewId("loot_coffin"), symbol(u8"⚰", Color::BROWN, true));
  addSymbol(ViewId("coffin1"), symbol(u8"⚰", Color::BROWN, true));
  addSymbol(ViewId("coffin2"), symbol(u8"⚰", Color::GRAY, true));
  addSymbol(ViewId("coffin3"), symbol(u8"⚰", Color::YELLOW, true));
  addSymbol(ViewId("boulder"), symbol(u8"●", Color::LIGHT_GRAY, true));
  addSymbol(ViewId("portal"), symbol(u8"𝚯", Color::WHITE, true));
  addSymbol(ViewId("gas_trap"), symbol(u8"☠", Color::GREEN, true));
  addSymbol(ViewId("alarm_trap"), symbol(u8"^", Color::RED, true));
  addSymbol(ViewId("web_trap"), symbol(u8"#", Color::WHITE, true));
  addSymbol(ViewId("surprise_trap"), symbol(u8"^", Color::BLUE, true));
  addSymbol(ViewId("terror_trap"), symbol(u8"^", Color::WHITE, true));
  addSymbol(ViewId("fire_trap"), symbol(u8"^", Color::RED, true));
  addSymbol(ViewId("rock"), symbol(u8"✱", Color::LIGHT_GRAY, true));
  addSymbol(ViewId("iron_rock"), symbol(u8"✱", Color::ORANGE, true));
  addSymbol(ViewId("ada_ore"), symbol(u8"✱", Color::LIGHT_BLUE, true));
  addSymbol(ViewId("wood_plank"), symbol(u8"\\", Color::BROWN));
  addSymbol(ViewId("storage_equipment"), symbol(u8".", Color::GREEN));
  addSymbol(ViewId("storage_resources"), symbol(u8".", Color::BLUE));
  addSymbol(ViewId("quarters1"), symbol(u8".", Color::PINK));
  addSymbol(ViewId("quarters2"), symbol(u8".", Color::SKY_BLUE));
  addSymbol(ViewId("quarters3"), symbol(u8".", Color::ORANGE));
  addSymbol(ViewId("leisure"), symbol(u8".", Color::DARK_BLUE));
  addSymbol(ViewId("prison"), symbol(u8".", Color::BLUE));
  addSymbol(ViewId("dorm"), symbol(u8".", Color::BROWN));
  addSymbol(ViewId("bed1"), symbol(u8"=", Color::WHITE));
  addSymbol(ViewId("bed2"), symbol(u8"=", Color::YELLOW));
  addSymbol(ViewId("bed3"), symbol(u8"=", Color::PURPLE));
  addSymbol(ViewId("torch"), symbol(u8"*", Color::YELLOW));
  addSymbol(ViewId("standing_torch"), symbol(u8"*", Color::YELLOW));
  addSymbol(ViewId("candelabrum_ns"), symbol(u8"*", Color::ORANGE));
  addSymbol(ViewId("candelabrum_w"), symbol(u8"*", Color::ORANGE));
  addSymbol(ViewId("candelabrum_e"), symbol(u8"*", Color::ORANGE));
  addSymbol(ViewId("altar"), symbol(u8"Ω", Color::WHITE, true));
  addSymbol(ViewId("altar_des"), symbol(u8"Ω", Color::RED, true));
  addSymbol(ViewId("creature_altar"), symbol(u8"Ω", Color::YELLOW, true));
  addSymbol(ViewId("torture_table"), symbol(u8"=", Color::GRAY));
  addSymbol(ViewId("impaled_head"), symbol(u8"⚲", Color::BROWN, true));
  addSymbol(ViewId("whipping_post"), symbol(u8"}", Color::BROWN, true));
  addSymbol(ViewId("gallows"), symbol(u8"}", Color::ORANGE, true));
  addSymbol(ViewId("notice_board"), symbol(u8"|", Color::BROWN));
  addSymbol(ViewId("sokoban_hole"), symbol(u8"0", Color::DARK_BLUE));
  addSymbol(ViewId("training_wood"), symbol(u8"‡", Color::BROWN, true));
  addSymbol(ViewId("training_iron"), symbol(u8"‡", Color::LIGHT_GRAY, true));
  addSymbol(ViewId("training_ada"), symbol(u8"‡", Color::LIGHT_BLUE, true));
  addSymbol(ViewId("archery_range"), symbol(u8"⌾", Color::LIGHT_BLUE, true));
}

void TileSet::genSymbols5() {
  addSymbol(ViewId("demon_shrine"), symbol(u8"Ω", Color::PURPLE, true));
  addSymbol(ViewId("bookcase_wood"), symbol(u8"▤", Color::BROWN, true));
  addSymbol(ViewId("bookcase_iron"), symbol(u8"▤", Color::LIGHT_GRAY, true));
  addSymbol(ViewId("bookcase_gold"), symbol(u8"▤", Color::YELLOW, true));
  addSymbol(ViewId("laboratory"), symbol(u8"ω", Color::PURPLE, true));
  addSymbol(ViewId("cauldron"), symbol(u8"ω", Color::PURPLE, true));
  addSymbol(ViewId("beast_lair"), symbol(u8".", Color::YELLOW));
  addSymbol(ViewId("beast_cage"), symbol(u8"▥", Color::LIGHT_GRAY, true));
  addSymbol(ViewId("workshop"), symbol(u8"&", Color::LIGHT_BROWN));
  addSymbol(ViewId("forge"), symbol(u8"&", Color::LIGHT_BLUE));
  addSymbol(ViewId("jeweller"), symbol(u8"&", Color::YELLOW));
  addSymbol(ViewId("furnace"), symbol(u8"&", Color::PINK));
  addSymbol(ViewId("grave"), symbol(u8"☗", Color::GRAY, true));
  addSymbol(ViewId("border_guard"), symbol(u8" ", Color::BLACK));
  addSymbol(ViewId("robe"), symbol(u8"[", Color::LIGHT_BROWN));
  addSymbol(ViewId("leather_gloves"), symbol(u8"[", Color::BROWN));
  addSymbol(ViewId("iron_gloves"), symbol(u8"[", Color::LIGHT_GRAY));
  addSymbol(ViewId("ada_gloves"), symbol(u8"[", Color::LIGHT_BLUE));
  addSymbol(ViewId("strength_gloves"), symbol(u8"[", Color::RED));
  addSymbol(ViewId("dexterity_gloves"), symbol(u8"[", Color::LIGHT_BLUE));
  addSymbol(ViewId("leather_armor"), symbol(u8"[", Color::BROWN));
  addSymbol(ViewId("leather_helm"), symbol(u8"[", Color::BROWN));
  addSymbol(ViewId("telepathy_helm"), symbol(u8"[", Color::LIGHT_GREEN));
  addSymbol(ViewId("chain_armor"), symbol(u8"[", Color::LIGHT_GRAY));
  addSymbol(ViewId("ada_armor"), symbol(u8"[", Color::LIGHT_BLUE));
  addSymbol(ViewId("iron_helm"), symbol(u8"[", Color::LIGHT_GRAY));
  addSymbol(ViewId("ada_helm"), symbol(u8"[", Color::LIGHT_BLUE));
  addSymbol(ViewId("leather_boots"), symbol(u8"[", Color::BROWN));
  addSymbol(ViewId("iron_boots"), symbol(u8"[", Color::LIGHT_GRAY));
  addSymbol(ViewId("ada_boots"), symbol(u8"[", Color::LIGHT_BLUE));
  addSymbol(ViewId("speed_boots"), symbol(u8"[", Color::LIGHT_BLUE));
  addSymbol(ViewId("levitation_boots"), symbol(u8"[", Color::LIGHT_GREEN));
  addSymbol(ViewId("wooden_shield"), symbol(u8"[", Color::LIGHT_BROWN));
  addSymbol(ViewId("fallen_tree"), symbol(u8"*", Color::GREEN));
  addSymbol(ViewId("guard_post"), symbol(u8"⚐", Color::YELLOW, true));
  addSymbol(ViewId("destroy_button"), symbol(u8"X", Color::RED));
  addSymbol(ViewId("forbid_zone"), symbol(u8"#", Color::RED));
  addSymbol(ViewId("mana"), symbol(u8"✱", Color::BLUE, true));
  addSymbol(ViewId("eyeball"), symbol(u8"e", Color::BLUE));
  addSymbol(ViewId("fetch_icon"), symbol(u8"👋", Color::LIGHT_BROWN, true));
  addSymbol(ViewId("fog_of_war"), symbol(u8" ", Color::WHITE));
  addSymbol(ViewId("pit"), symbol(u8"^", Color::WHITE));
  addSymbol(ViewId("creature_highlight"), symbol(u8" ", Color::WHITE));
  addSymbol(ViewId("square_highlight"), symbol(u8"⛶", Color::WHITE, true));
  addSymbol(ViewId("round_shadow"), symbol(u8" ", Color::WHITE));
  addSymbol(ViewId("campaign_defeated"), symbol(u8"X", Color::RED));
  addSymbol(ViewId("fog_of_war_corner"), symbol(u8" ", Color::WHITE));
  addSymbol(ViewId("accept_immigrant"), symbol(u8"✓", Color::GREEN, true));
  addSymbol(ViewId("reject_immigrant"), symbol(u8"✘", Color::RED, true));
  addSymbol(ViewId("special_blbn"), symbol(u8"B", Color::PURPLE));
  addSymbol(ViewId("special_blbw"), symbol(u8"B", Color::LIGHT_RED));
  addSymbol(ViewId("special_blgn"), symbol(u8"B", Color::LIGHT_GRAY));
  addSymbol(ViewId("special_blgw"), symbol(u8"B", Color::WHITE));
  addSymbol(ViewId("special_bmbn"), symbol(u8"B", Color::YELLOW));
  addSymbol(ViewId("special_bmbw"), symbol(u8"B", Color::ORANGE));
  addSymbol(ViewId("special_bmgn"), symbol(u8"B", Color::GREEN));
  addSymbol(ViewId("special_bmgw"), symbol(u8"B", Color::LIGHT_GREEN));
  addSymbol(ViewId("special_hlbn"), symbol(u8"H", Color::PURPLE));
  addSymbol(ViewId("special_hlbw"), symbol(u8"H", Color::LIGHT_RED));
  addSymbol(ViewId("special_hlgn"), symbol(u8"H", Color::LIGHT_GRAY));
  addSymbol(ViewId("special_hlgw"), symbol(u8"H", Color::WHITE));
  addSymbol(ViewId("special_hmbn"), symbol(u8"H", Color::YELLOW));
  addSymbol(ViewId("special_hmbw"), symbol(u8"H", Color::ORANGE));
  addSymbol(ViewId("special_hmgn"), symbol(u8"H", Color::GREEN));
  addSymbol(ViewId("special_hmgw"), symbol(u8"H", Color::LIGHT_GREEN));
  addSymbol(ViewId("halloween_kid1"), symbol(u8"@", Color::PINK));
  addSymbol(ViewId("halloween_kid2"), symbol(u8"@", Color::PURPLE));
  addSymbol(ViewId("halloween_kid3"), symbol(u8"@", Color::BLUE));
  addSymbol(ViewId("halloween_kid4"), symbol(u8"@", Color::YELLOW));
  addSymbol(ViewId("halloween_costume"), symbol(u8"[", Color::PINK));
  addSymbol(ViewId("bag_of_candy"), symbol(u8"*", Color::BLUE));
  addSymbol(ViewId("tutorial_entrance"), symbol(u8" ", Color::LIGHT_GREEN));
  addSymbol(ViewId("horn_attack"), symbol(u8" ", Color::PINK));
  addSymbol(ViewId("beak_attack"), symbol(u8" ", Color::YELLOW));
  addSymbol(ViewId("touch_attack"), symbol(u8" ", Color::WHITE));
  addSymbol(ViewId("bite_attack"), symbol(u8" ", Color::RED));
  addSymbol(ViewId("claws_attack"), symbol(u8" ", Color::BROWN));
  addSymbol(ViewId("leg_attack"), symbol(u8" ", Color::GRAY));
  addSymbol(ViewId("fist_attack"), symbol(u8" ", Color::ORANGE));
  addSymbol(ViewId("item_aura"), symbol(u8" ", Color::ORANGE));
}

TileSet::TileSet(GameConfig* config, const DirectoryPath& path, bool useTiles) {
  loadTilesFromDir(path.subdirectory("orig16"), Vec2(16, 16));
  loadTilesFromDir(path.subdirectory("orig24"), Vec2(24, 24));
  loadTilesFromDir(path.subdirectory("orig30"), Vec2(30, 30));
  loadUnicode();
  if (useTiles)
    loadTiles();
}

const Tile& TileSet::getTile(ViewId id, bool sprite) const {
  if (sprite && tiles.count(id))
    return tiles.at(id);
  else
    return symbols.at(id);
}

constexpr int textureWidth = 720;

void TileSet::loadTilesFromDir(const DirectoryPath& path, Vec2 size) {
  const static string imageSuf = ".png";
  auto files = path.getFiles().filter([](const FilePath& f) { return f.hasSuffix(imageSuf);});
  int rowLength = textureWidth / size.x;
  SDL::SDL_Surface* image = Texture::createSurface(textureWidth, textureWidth);
  SDL::SDL_SetSurfaceBlendMode(image, SDL::SDL_BLENDMODE_NONE);
  CHECK(image) << SDL::SDL_GetError();
  int frameCount = 0;
  vector<pair<string, Vec2>> addedPositions;
  for (int i : All(files)) {
    SDL::SDL_Surface* im = SDL::IMG_Load(files[i].getPath());
    SDL::SDL_SetSurfaceBlendMode(im, SDL::SDL_BLENDMODE_NONE);
    CHECK(im) << files[i] << ": "<< SDL::IMG_GetError();
    USER_CHECK((im->w % size.x == 0) && im->h == size.y) << files[i] << " has wrong size " << im->w << " " << im->h;
    string fileName = files[i].getFileName();
    string spriteName = fileName.substr(0, fileName.size() - imageSuf.size());
    CHECK(!tileCoords.count(spriteName)) << "Duplicate name " << spriteName;
    for (int frame : Range(im->w / size.x)) {
      SDL::SDL_Rect dest;
      int posX = frameCount % rowLength;
      int posY = frameCount / rowLength;
      dest.x = size.x * posX;
      dest.y = size.y * posY;
      CHECK(dest.x < textureWidth && dest.y < textureWidth);
      SDL::SDL_Rect src;
      src.x = frame * size.x;
      src.y = 0;
      src.w = size.x;
      src.h = size.y;
      SDL_BlitSurface(im, &src, image, &dest);
      addedPositions.emplace_back(spriteName, Vec2(posX, posY));
      INFO << "Loading tile sprite " << fileName << " at " << posX << "," << posY;
      ++frameCount;
    }
    SDL::SDL_FreeSurface(im);
  }
  textures.push_back(unique<Texture>(image));
  for (auto& pos : addedPositions)
    tileCoords[pos.first].push_back({size, pos.second, textures.back().get()});
  SDL::SDL_FreeSurface(image);
}
