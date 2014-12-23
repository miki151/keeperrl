
Tile sprite(int x, int y, int tex) {
  return sprite(byCoord(x, y, tex));
}

Tile sprite(int x, int y) {
  return sprite(byCoord(x, y, 0));
}

Tile sprite(const string& in s) {
  return sprite(byName(s));
}

Tile empty() {
  return sprite("empty");
}

Tile getRoadTile(int pathSet) {
  int tex = 5;
  return sprite(0, pathSet, tex)
    .addConnection(dirs(E, W), byCoord(2, pathSet, tex))
    .addConnection(dirs(W), byCoord(3, pathSet, tex))
    .addConnection(dirs(E), byCoord(1, pathSet, tex))
    .addConnection(dirs(S), byCoord(4, pathSet, tex))
    .addConnection(dirs(N, S), byCoord(5, pathSet, tex))
    .addConnection(dirs(N), byCoord(6, pathSet, tex))
    .addConnection(dirs(S, E), byCoord(7, pathSet, tex))
    .addConnection(dirs(S, W), byCoord(8, pathSet, tex))
    .addConnection(dirs(N, E), byCoord(9, pathSet, tex))
    .addConnection(dirs(N, W), byCoord(10, pathSet, tex))
    .addConnection(dirs(N, E, S, W), byCoord(11, pathSet, tex))
    .addConnection(dirs(E, S, W), byCoord(12, pathSet, tex))
    .addConnection(dirs(N, S, W), byCoord(13, pathSet, tex))
    .addConnection(dirs(N, E, S), byCoord(14, pathSet, tex))
    .addConnection(dirs(N, E, W), byCoord(15, pathSet, tex));
}

Tile getWallTile(int wallSet) {
  int tex = 1;
  return sprite(9, wallSet, tex).setNoShadow()
    .addConnection(dirs(E, W), byCoord(11, wallSet, tex))
    .addConnection(dirs(W), byCoord(12, wallSet, tex))
    .addConnection(dirs(E), byCoord(10, wallSet, tex))
    .addConnection(dirs(S), byCoord(13, wallSet, tex))
    .addConnection(dirs(N, S), byCoord(14, wallSet, tex))
    .addConnection(dirs(N), byCoord(15, wallSet, tex))
    .addConnection(dirs(E, S), byCoord(16, wallSet, tex))
    .addConnection(dirs(S, W), byCoord(17, wallSet, tex))
    .addConnection(dirs(N, E), byCoord(18, wallSet, tex))
    .addConnection(dirs(N, W), byCoord(19, wallSet, tex))
    .addConnection(dirs(N, E, S, W), byCoord(20, wallSet, tex))
    .addConnection(dirs(E, S, W), byCoord(21, wallSet, tex))
    .addConnection(dirs(N, S, W), byCoord(22, wallSet, tex))
    .addConnection(dirs(N, E, S), byCoord(23, wallSet, tex))
    .addConnection(dirs(N, E, W), byCoord(24, wallSet, tex));
}

Tile getMountainTile(int numT, int set) {
  int tex = 1;
  return sprite(numT, set, tex).setNoShadow()
    .addCorner(dirs(S, W), dirs(W), byCoord(10, set, tex))
    .addCorner(dirs(N, W), dirs(W), byCoord(11, set, tex))
    .addCorner(dirs(S, E), dirs(E), byCoord(12, set, tex))
    .addCorner(dirs(N, E), dirs(E), byCoord(13, set, tex))
    .addCorner(dirs(N, W), dirs(N), byCoord(14, set, tex))
    .addCorner(dirs(N, E), dirs(N), byCoord(15, set, tex))
    .addCorner(dirs(S, W), dirs(S), byCoord(16, set, tex))
    .addCorner(dirs(S, E), dirs(S), byCoord(17, set, tex))
    .addCorner(dirs(N, W), dirs(), byCoord(18, set, tex))
    .addCorner(dirs(N, E), dirs(), byCoord(19, set, tex))
    .addCorner(dirs(S, W), dirs(), byCoord(20, set, tex))
    .addCorner(dirs(S, E), dirs(), byCoord(21, set, tex))
    .addCorner(dirs(S, E, SE), dirs(S, E), byCoord(22, set, tex))
    .addCorner(dirs(S, W, SW), dirs(S, W), byCoord(23, set, tex))
    .addCorner(dirs(N, E, NE), dirs(N, E), byCoord(24, set, tex))
    .addCorner(dirs(N, W, NW), dirs(N, W), byCoord(25, set, tex));
}

Tile getWaterTile(int leftX) {
  int tex = 4;
  return sprite(leftX, 5, tex)
    .addConnection(dirs(N, E, S, W), byCoord(leftX - 1, 7, tex))
    .addConnection(dirs(E, S, W), byCoord(leftX, 4, tex))
    .addConnection(dirs(N, E, W), byCoord(leftX, 6, tex))
    .addConnection(dirs(N, S, W), byCoord(leftX + 1, 5, tex))
    .addConnection(dirs(N, E, S), byCoord(leftX - 1, 5, tex))
    .addConnection(dirs(N, E), byCoord(leftX - 1, 6, tex))
    .addConnection(dirs(E, S), byCoord(leftX - 1, 4, tex))
    .addConnection(dirs(S, W), byCoord(leftX + 1, 4, tex))
    .addConnection(dirs(N, W), byCoord(leftX + 1, 6, tex))
    .addConnection(dirs(S), byCoord(leftX, 7, tex))
    .addConnection(dirs(N), byCoord(leftX, 8, tex))
    .addConnection(dirs(W), byCoord(leftX + 1, 7, tex))
    .addConnection(dirs(E), byCoord(leftX + 1, 8, tex))
    .addConnection(dirs(N, S), byCoord(leftX + 1, 12, tex))
    .addConnection(dirs(E, W), byCoord(leftX + 1, 11, tex));
}

Tile getExtraBorderTile(int set) {
  int tex = 5;
  return sprite(1, set, tex)
    .addExtraBorder(dirs(W, N), byCoord(10, set, tex))
    .addExtraBorder(dirs(E, N), byCoord(11, set, tex))
    .addExtraBorder(dirs(E, S), byCoord(12, set, tex))
    .addExtraBorder(dirs(W, S), byCoord(13, set, tex))
    .addExtraBorder(dirs(W, N, E), byCoord(14, set, tex))
    .addExtraBorder(dirs(S, N, E), byCoord(15, set, tex))
    .addExtraBorder(dirs(S, W, E), byCoord(16, set, tex))
    .addExtraBorder(dirs(S, W, N), byCoord(17, set, tex))
    .addExtraBorder(dirs(S, W, N, E), byCoord(18, set, tex))
    .addExtraBorder(dirs(N), byCoord(19, set, tex))
    .addExtraBorder(dirs(E), byCoord(20, set, tex))
    .addExtraBorder(dirs(S), byCoord(21, set, tex))
    .addExtraBorder(dirs(W), byCoord(22, set, tex));
}

void genTiles() {
  setGuiBackground(5, 5, 8); //RGB 0..255
  addTile(UNKNOWN_MONSTER, symbol("?", LIGHT_GREEN));
  addTile(SHEEP, symbol("s", WHITE));
  addTile(VODNIK, symbol("f", GREEN));
  addTile(ABYSS, symbol("~", DARK_GRAY));
  addTile(TRAP, symbol("➹", YELLOW, true));
  addTile(BARS, symbol("⧻", LIGHT_BLUE));
  addTile(DESTROYED_FURNITURE, symbol("*", BROWN));
  addTile(BURNT_FURNITURE, symbol("*", DARK_GRAY));
  addTile(DESTROY_BUTTON, sprite("remove"));
  addTile(EMPTY, empty());
  addTile(BORDER_GUARD, empty());
  addTile(VAMPIRE, sprite("vampire"));
  addTile(FALLEN_TREE, sprite("treecut").setNoShadow());
  addTile(DECID_TREE, sprite("tree2").setNoShadow());
  addTile(CANIF_TREE, sprite("tree1").setNoShadow());
  addTile(TREE_TRUNK, sprite("treecut").setNoShadow());
  addTile(BURNT_TREE, sprite("treeburnt").setNoShadow());
  addTile(PLAYER, sprite(1, 0));
  addTile(KEEPER, sprite("keeper"));
  addTile(UNDEAD_KEEPER, sprite(9, 16));
  addTile(ELF, sprite("elf male"));
  addTile(ELF_ARCHER, sprite("elf archer"));
  addTile(ELF_CHILD, sprite("elf kid"));
  addTile(ELF_LORD, sprite("elf lord"));
  addTile(LIZARDMAN, sprite(8, 8));
  addTile(LIZARDLORD, sprite(11, 8));
  addTile(IMP, sprite("imp"));
  addTile(PRISONER, sprite("prisoner"));
  addTile(OGRE, sprite("troll"));
  addTile(CHICKEN, sprite("chicken"));
  addTile(GNOME, sprite(15, 8));
  addTile(DWARF, sprite(2, 6));
  addTile(DWARF_BARON, sprite(3, 6));
  addTile(SHOPKEEPER, sprite(4, 2));
  addTile(BRIDGE, sprite("bridge").addOption(S, byName("bridge2")));
  addTile(ROAD, getRoadTile(7));
  addTile(FLOOR, sprite(3, 14, 1));
  addTile(KEEPER_FLOOR, sprite(4, 18, 1));
  addTile(SAND, getExtraBorderTile(14)
    .addExtraBorderId(WATER));
  addTile(MUD, getExtraBorderTile(10)
    .addExtraBorderId(WATER)
    .addExtraBorderId(HILL)
    .addExtraBorderId(SAND));
  addTile(GRASS, getExtraBorderTile(18)
    .addExtraBorderId(SAND)
    .addExtraBorderId(HILL)
    .addExtraBorderId(MUD)
    .addExtraBorderId(WATER));
  addTile(CROPS, sprite("wheatfield1"));
  addTile(CROPS2, sprite("wheatfield2"));
  addTile(MOUNTAIN2, getMountainTile(9, 18));
  addTile(WALL, getWallTile(2));
  addTile(MOUNTAIN, sprite(17, 2, 2).setNoShadow());
  addTile(GOLD_ORE, getMountainTile(8, 18));
  addTile(IRON_ORE, getMountainTile(7, 18));
  addTile(STONE, getMountainTile(6, 18));
  addTile(SNOW, sprite(16, 2, 2).setNoShadow());
  addTile(HILL, getExtraBorderTile(8)
    .addExtraBorderId(SAND)
    .addExtraBorderId(WATER));
  addTile(WOOD_WALL, getWallTile(4));
  addTile(BLACK_WALL, getWallTile(2));
  addTile(YELLOW_WALL, getWallTile(8));
  addTile(LOW_ROCK_WALL, getWallTile(21));
  addTile(HELL_WALL, getWallTile(22));
  addTile(CASTLE_WALL, getWallTile(5));
  addTile(MUD_WALL, getWallTile(13));
  addTile(SECRETPASS, sprite(0, 15, 1));
  addTile(DUNGEON_ENTRANCE, sprite(15, 2, 2).setNoShadow());
  addTile(DUNGEON_ENTRANCE_MUD, sprite(19, 2, 2).setNoShadow());
  addTile(DOWN_STAIRCASE, sprite(8, 0, 1).setNoShadow());
  addTile(UP_STAIRCASE, sprite(7, 0, 1).setNoShadow());
  addTile(DOWN_STAIRCASE_CELLAR, sprite(8, 21, 1).setNoShadow());
  addTile(UP_STAIRCASE_CELLAR, sprite(7, 21, 1).setNoShadow());
  addTile(DOWN_STAIRCASE_HELL, sprite(8, 1, 1).setNoShadow());
  addTile(UP_STAIRCASE_HELL, sprite(7, 22, 1).setNoShadow());
  addTile(DOWN_STAIRCASE_PYR, sprite(8, 8, 1).setNoShadow());
  addTile(UP_STAIRCASE_PYR, sprite(7, 8, 1).setNoShadow());
  addTile(WELL, sprite(5, 8, 2).setNoShadow());
  addTile(STATUE1, sprite(6, 5, 2).setNoShadow());
  addTile(STATUE2, sprite(7, 5, 2).setNoShadow());
  addTile(GREAT_ORC, sprite(6, 14));
  addTile(ORC, sprite("orc"));
  addTile(ORC_SHAMAN, sprite("shaman"));
  addTile(HARPY, sprite("harpysmall"));
  addTile(DOPPLEGANGER, sprite("dopple"));
  addTile(SUCCUBUS, sprite("succubussmall"));
  addTile(BANDIT, sprite(0, 2));
  addTile(GHOST, sprite("ghost4"));
  addTile(SPIRIT, sprite(17, 14));
  addTile(DEVIL, sprite(17, 18));
  addTile(DARK_KNIGHT, sprite(12, 14));
  addTile(GREEN_DRAGON, sprite("greendragon"));
  addTile(RED_DRAGON, sprite("reddragon"));
  addTile(CYCLOPS, sprite(10, 14));
  addTile(WITCH, sprite(15, 16));
  addTile(KNIGHT, sprite(0, 0));
  addTile(WARRIOR, sprite(6, 0));
  addTile(SHAMAN, sprite(5, 0));
  addTile(CASTLE_GUARD, sprite(15, 2));
  addTile(AVATAR, sprite(9, 0));
  addTile(ARCHER, sprite(2, 0));
  addTile(PESEANT, sprite("peasant"));
  addTile(PESEANT_WOMAN, sprite("peasantgirl"));
  addTile(CHILD, sprite("peasantkid"));
  addTile(CLAY_GOLEM, sprite(12, 11));
  addTile(STONE_GOLEM, sprite(10, 10));
  addTile(IRON_GOLEM, sprite(12, 10));
  addTile(LAVA_GOLEM, sprite(13, 10));
  addTile(AIR_ELEMENTAL, sprite("airelemental"));
  addTile(FIRE_ELEMENTAL, sprite("fireelemental"));
  addTile(WATER_ELEMENTAL, sprite("waterelemental"));
  addTile(EARTH_ELEMENTAL, sprite("earthelemental"));
  addTile(ENT, sprite("ent"));
  addTile(ANGEL, sprite("angel"));
  addTile(ZOMBIE, sprite(0, 16));
  addTile(SKELETON, sprite("skeleton"));
  addTile(VAMPIRE_LORD, sprite("vampirelord"));
  addTile(MUMMY, sprite(7, 16));
  addTile(MUMMY_LORD, sprite(8, 16));
  addTile(JACKAL, sprite(12, 12));
  addTile(DEER, sprite("deer"));
  addTile(HORSE, sprite("horse"));
  addTile(COW, sprite("cow"));
  addTile(PIG, sprite("pig"));
  addTile(GOAT, sprite("goat"));
  addTile(BOAR, sprite("boar"));
  addTile(FOX, sprite("fox"));
  addTile(WOLF, sprite("wolf"));
  addTile(WEREWOLF, sprite("werewolf2"));
  addTile(DOG, sprite("dog"));
  addTile(KRAKEN_HEAD, sprite("krakenhead"));
  addTile(KRAKEN_LAND, sprite("krakenland1"));
  addTile(KRAKEN_WATER, sprite("krakenwater2"));
  addTile(DEATH, sprite(9, 16));
  addTile(NIGHTMARE, sprite(9, 16));
  addTile(FIRE_SPHERE, sprite(16, 20));
  addTile(BEAR, sprite(8, 18));
  addTile(BAT, sprite(2, 12));
  addTile(GOBLIN, sprite("goblin"));
  addTile(LEPRECHAUN, sprite("leprechaun"));
  addTile(RAT, sprite("rat"));
  addTile(SPIDER, sprite(6, 12));
  addTile(FLY, sprite(10, 12));
  addTile(SCORPION, sprite(11, 18));
  addTile(SNAKE, sprite(9, 12));
  addTile(VULTURE, sprite("vulture"));
  addTile(RAVEN, sprite(17, 12));
  addTile(BODY_PART, sprite("corpse4"));
  addTile(BONE, sprite(3, 0, 2));
  addTile(BUSH, sprite(17, 0, 2).setNoShadow());
  addTile(WATER, getWaterTile(5));
  addTile(MAGMA, getWaterTile(11));
  addTile(DOOR, sprite("door").setNoShadow());
  addTile(BARRICADE, sprite("barricade").setNoShadow());
  addTile(DIG_ICON, sprite(8, 10, 2));
  addTile(SWORD, sprite(12, 9, 3));
  addTile(SPEAR, sprite(5, 8, 3));
  addTile(SPECIAL_SWORD, sprite(13, 9, 3));
  addTile(ELVEN_SWORD, sprite(14, 9, 3));
  addTile(KNIFE, sprite(20, 9, 3));
  addTile(WAR_HAMMER, sprite(10, 7, 3));
  addTile(SPECIAL_WAR_HAMMER, sprite(11, 7, 3));
  addTile(BATTLE_AXE, sprite(13, 7, 3));
  addTile(SPECIAL_BATTLE_AXE, sprite(21, 7, 3));
  addTile(BOW, sprite(14, 8, 3));
  addTile(ARROW, sprite(5, 8, 3));
  addTile(CLUB, sprite("club"));
  addTile(HEAVY_CLUB, sprite("heavy club"));
  addTile(SCROLL, sprite(3, 6, 3));
  addTile(STEEL_AMULET, sprite(1, 1, 3));
  addTile(COPPER_AMULET, sprite(2, 1, 3));
  addTile(CRYSTAL_AMULET, sprite(4, 1, 3));
  addTile(WOODEN_AMULET, sprite(0, 1, 3));
  addTile(AMBER_AMULET, sprite(3, 1, 3));
  addTile(FIRE_RESIST_RING, sprite(11, 3, 3));
  addTile(POISON_RESIST_RING, sprite(16, 3, 3));
  addTile(BOOK, sprite(0, 3, 3));
  addTile(FIRST_AID, sprite("medkit2"));
  addTile(TRAP_ITEM, sprite("trapbox"));
  addTile(EFFERVESCENT_POTION, sprite(6, 0, 3));
  addTile(MURKY_POTION, sprite(10, 0, 3));
  addTile(SWIRLY_POTION, sprite(9, 0, 3));
  addTile(VIOLET_POTION, sprite(7, 0, 3));
  addTile(PUCE_POTION, sprite(8, 0, 3));
  addTile(SMOKY_POTION, sprite(11, 0, 3));
  addTile(FIZZY_POTION, sprite(9, 0, 3));
  addTile(MILKY_POTION, sprite(11, 0, 3));
  addTile(MUSHROOM, sprite(5, 4, 3));
  addTile(FOUNTAIN, sprite(0, 7, 2).setNoShadow());
  addTile(GOLD, sprite(8, 3, 3).setNoShadow());
  addTile(TREASURE_CHEST, sprite("treasurydeco").setNoShadow().addBackground(byName("treasury")));
  addTile(CHEST, sprite(3, 3, 2).setNoShadow());
  addTile(OPENED_CHEST, sprite(6, 3, 2).setNoShadow());
  addTile(COFFIN, sprite(7, 3, 2).setNoShadow());
  addTile(OPENED_COFFIN, sprite(8, 3, 2).setNoShadow());
  addTile(BOULDER, sprite("boulder"));
  addTile(PORTAL, sprite("surprise"));
  addTile(GAS_TRAP, sprite(2, 6, 3));
  addTile(ALARM_TRAP, sprite(16, 5, 3));
  addTile(WEB_TRAP, sprite(4, 1, 2));
  addTile(SURPRISE_TRAP, sprite("surprisetrap"));
  addTile(TERROR_TRAP, sprite(1, 6, 3));
  addTile(ROCK, sprite("stonepile"));
  addTile(IRON_ROCK, sprite("ironpile2"));
  addTile(WOOD_PLANK, sprite("wood2"));
  addTile(STOCKPILE1, sprite("storage1").setFloorBorders());
  addTile(STOCKPILE2, sprite("storage2").setFloorBorders());
  addTile(STOCKPILE3, sprite("storage3").setFloorBorders());
  addTile(PRISON, sprite(6, 2, 1));
  addTile(BED, sprite("sleepdeco").setNoShadow());
  addTile(DORM, sprite("sleep").setFloorBorders());
  addTile(TORCH, sprite(12, 0, 2).setNoShadow().setTranslucent(0.35));
  addTile(DUNGEON_HEART, sprite(6, 10, 2));
  addTile(ALTAR, sprite(2, 7, 2).setNoShadow());
  addTile(CREATURE_ALTAR, sprite(3, 7, 2).setNoShadow());
  addTile(TORTURE_TABLE, empty().addConnection(setOfAllDirs(), byName("torturedeco"))
      .addBackground(byName("torture"))
      .setFloorBorders());
  addTile(IMPALED_HEAD, sprite("impaledhead").setNoShadow());
  addTile(TRAINING_ROOM, empty().addConnection(setOfAllDirs(), byName("traindeco"))
      .addBackground(byName("train")).setFloorBorders());
  addTile(RITUAL_ROOM, empty().addConnection(setOfAllDirs(), byName("ritualroomdeco"))
      .addBackground(byName("ritualroom"))
      .setFloorBorders());
  addTile(LIBRARY, empty().addConnection(setOfAllDirs(), byName("libdeco"))
      .addBackground(byName("lib")).setFloorBorders());
  addTile(LABORATORY, empty().addConnection(setOfAllDirs(), byName("labdeco"))
      .addBackground(byName("lab")).setFloorBorders());
  addTile(CAULDRON, sprite("labdeco").setNoShadow());
  addTile(BEAST_LAIR, sprite("lair").setFloorBorders());
  addTile(BEAST_CAGE, sprite("lairdeco").setNoShadow()
      .addBackground(byName("lair")));
  addTile(FORGE, empty().addConnection(setOfAllDirs(), byName("forgedeco"))
      .addBackground(byName("forge"))
      .setFloorBorders());
  addTile(WORKSHOP, empty().addConnection(setOfAllDirs(), byName("workshopdeco"))
      .addBackground(byName("workshop"))
      .setFloorBorders());
  addTile(JEWELER, empty().addConnection(setOfAllDirs(), byName("jewelerdeco"))
      .addBackground(byName("jeweler"))
      .setFloorBorders());
  addTile(CEMETERY, sprite("graveyard").setFloorBorders());
  addTile(GRAVE, sprite("gravedeco").setNoShadow().addBackground(byName("graveyard")));
  addTile(ROBE, sprite(7, 11, 3));
  addTile(LEATHER_GLOVES, sprite(15, 11, 3));
  addTile(DEXTERITY_GLOVES, sprite(19, 11, 3));
  addTile(STRENGTH_GLOVES, sprite(20, 11, 3));
  addTile(LEATHER_ARMOR, sprite(0, 12, 3));
  addTile(LEATHER_HELM, sprite(10, 12, 3));
  addTile(TELEPATHY_HELM, sprite(17, 12, 3));
  addTile(CHAIN_ARMOR, sprite(1, 12, 3));
  addTile(IRON_HELM, sprite(14, 12, 3));
  addTile(LEATHER_BOOTS, sprite(0, 13, 3));
  addTile(IRON_BOOTS, sprite(6, 13, 3));
  addTile(SPEED_BOOTS, sprite(3, 13, 3));
  addTile(LEVITATION_BOOTS, sprite(2, 13, 3));
  addTile(GUARD_POST, sprite("guardroom"));
  addTile(MANA, sprite(5, 10, 2));
  addTile(DANGER, sprite(12, 9, 2));
  addTile(FETCH_ICON, sprite(15, 11, 3));
  addTile(EYEBALL, sprite("eyeball2").setNoShadow());
  addTile(FOG_OF_WAR, getWaterTile(14));
  addTile(FOG_OF_WAR_CORNER, sprite(14, 5, 4)
      .addConnection(dirs(NE), byCoord(14, 11, 4))
      .addConnection(dirs(NW), byCoord(13, 11, 4))
      .addConnection(dirs(SE), byCoord(14, 12, 4))
      .addConnection(dirs(SW), byCoord(13, 12, 4)));
  addTile(SPECIAL_BEAST, sprite(7, 10));
  addTile(SPECIAL_HUMANOID, sprite(2, 10));
  addTile(TEAM_BUTTON, sprite("team_button"));
  addTile(TEAM_BUTTON_HIGHLIGHT, sprite("team_button_highlight"));
}

void genSymbols() {
  addSymbol(EMPTY, symbol(" ", BLACK));
  addSymbol(PLAYER, symbol("@", WHITE));
  addSymbol(KEEPER, symbol("@", PURPLE));
  addSymbol(UNDEAD_KEEPER, symbol("@", DARK_GRAY));
  addSymbol(UNKNOWN_MONSTER, symbol("?", LIGHT_GREEN));
  addSymbol(ELF, symbol("@", LIGHT_GREEN));
  addSymbol(ELF_ARCHER, symbol("@", GREEN));
  addSymbol(ELF_CHILD, symbol("@", LIGHT_GREEN));
  addSymbol(ELF_LORD, symbol("@", DARK_GREEN));
  addSymbol(SHOPKEEPER, symbol("@", LIGHT_BLUE));
  addSymbol(LIZARDMAN, symbol("@", LIGHT_BROWN));
  addSymbol(LIZARDLORD, symbol("@", BROWN));
  addSymbol(IMP, symbol("i", LIGHT_BROWN));
  addSymbol(PRISONER, symbol("@", LIGHT_BROWN));
  addSymbol(OGRE, symbol("O", GREEN));
  addSymbol(CHICKEN, symbol("c", YELLOW));
  addSymbol(GNOME, symbol("g", GREEN));
  addSymbol(DWARF, symbol("h", BLUE));
  addSymbol(DWARF_BARON, symbol("h", DARK_BLUE));
  addSymbol(FLOOR, symbol(".", LIGHT_GRAY));
  addSymbol(KEEPER_FLOOR, symbol(".", WHITE));
  addSymbol(BRIDGE, symbol("_", BROWN));
  addSymbol(ROAD, symbol(".", LIGHT_GRAY));
  addSymbol(SAND, symbol(".", YELLOW));
  addSymbol(MUD, symbol(0x1d0f0, BROWN, true));
  addSymbol(GRASS, symbol(0x1d0f0, GREEN, true));
  addSymbol(CROPS, symbol(0x1d0f0, YELLOW, true));
  addSymbol(CROPS2, symbol(0x1d0f0, YELLOW, true));
  addSymbol(CASTLE_WALL, symbol("#", LIGHT_GRAY));
  addSymbol(MUD_WALL, symbol("#", LIGHT_BROWN));
  addSymbol(WALL, symbol("#", LIGHT_GRAY));
  addSymbol(MOUNTAIN, symbol(0x25ee, DARK_GRAY, true));
  addSymbol(MOUNTAIN2, symbol("#", DARK_GRAY));
  addSymbol(GOLD_ORE, symbol("⁂", YELLOW, true));
  addSymbol(IRON_ORE, symbol("⁂", DARK_BROWN, true));
  addSymbol(STONE, symbol("⁂", LIGHT_GRAY, true));
  addSymbol(SNOW, symbol(0x25ee, WHITE, true));
  addSymbol(HILL, symbol(0x1d022, DARK_GREEN, true));
  addSymbol(WOOD_WALL, symbol("#", DARK_BROWN));
  addSymbol(BLACK_WALL, symbol("#", LIGHT_GRAY));
  addSymbol(YELLOW_WALL, symbol("#", YELLOW));
  addSymbol(LOW_ROCK_WALL, symbol("#", DARK_GRAY));
  addSymbol(HELL_WALL, symbol("#", RED));
  addSymbol(SECRETPASS, symbol("#", LIGHT_GRAY));
  addSymbol(DUNGEON_ENTRANCE, symbol(0x2798, BROWN, true));
  addSymbol(DUNGEON_ENTRANCE_MUD, symbol(0x2798, BROWN, true));
  addSymbol(DOWN_STAIRCASE_CELLAR, symbol(0x2798, ALMOST_WHITE, true));
  addSymbol(DOWN_STAIRCASE, symbol(0x2798, ALMOST_WHITE, true));
  addSymbol(UP_STAIRCASE_CELLAR, symbol(0x279a, ALMOST_WHITE, true));
  addSymbol(UP_STAIRCASE, symbol(0x279a, ALMOST_WHITE, true));
  addSymbol(DOWN_STAIRCASE_HELL, symbol(0x2798, RED, true));
  addSymbol(UP_STAIRCASE_HELL, symbol(0x279a, RED, true));
  addSymbol(DOWN_STAIRCASE_PYR, symbol(0x2798, YELLOW, true));
  addSymbol(UP_STAIRCASE_PYR, symbol(0x279a, YELLOW, true));
  addSymbol(WELL, symbol("0", BLUE));
  addSymbol(STATUE1, symbol("&", LIGHT_GRAY));
  addSymbol(STATUE2, symbol("&", LIGHT_GRAY));
  addSymbol(GREAT_ORC, symbol("o", PURPLE));
  addSymbol(ORC, symbol("o", DARK_BLUE));
  addSymbol(ORC_SHAMAN, symbol("o", YELLOW));
  addSymbol(HARPY, symbol("R", YELLOW));
  addSymbol(DOPPLEGANGER, symbol("&", YELLOW));
  addSymbol(SUCCUBUS, symbol("&", RED));
  addSymbol(BANDIT, symbol("@", DARK_BLUE));
  addSymbol(DARK_KNIGHT, symbol("@", PURPLE));
  addSymbol(GREEN_DRAGON, symbol("D", GREEN));
  addSymbol(RED_DRAGON, symbol("D", RED));
  addSymbol(CYCLOPS, symbol("C", GREEN));
  addSymbol(WITCH, symbol("@", BROWN));
  addSymbol(GHOST, symbol("&", WHITE));
  addSymbol(SPIRIT, symbol("&", LIGHT_BLUE));
  addSymbol(DEVIL, symbol("&", PURPLE));
  addSymbol(CASTLE_GUARD, symbol("@", LIGHT_GRAY));
  addSymbol(KNIGHT, symbol("@", LIGHT_GRAY));
  addSymbol(WARRIOR, symbol("@", DARK_GRAY));
  addSymbol(SHAMAN, symbol("@", YELLOW));
  addSymbol(AVATAR, symbol("@", BLUE));
  addSymbol(ARCHER, symbol("@", BROWN));
  addSymbol(PESEANT, symbol("@", GREEN));
  addSymbol(PESEANT_WOMAN, symbol("@", GREEN));
  addSymbol(CHILD, symbol("@", LIGHT_GREEN));
  addSymbol(CLAY_GOLEM, symbol("Y", YELLOW));
  addSymbol(STONE_GOLEM, symbol("Y", LIGHT_GRAY));
  addSymbol(IRON_GOLEM, symbol("Y", ORANGE));
  addSymbol(LAVA_GOLEM, symbol("Y", PURPLE));
  addSymbol(AIR_ELEMENTAL, symbol("E", LIGHT_BLUE));
  addSymbol(FIRE_ELEMENTAL, symbol("E", RED));
  addSymbol(WATER_ELEMENTAL, symbol("E", BLUE));
  addSymbol(EARTH_ELEMENTAL, symbol("E", GRAY));
  addSymbol(ENT, symbol("E", LIGHT_BROWN));
  addSymbol(ANGEL, symbol("A", LIGHT_BLUE));
  addSymbol(ZOMBIE, symbol("Z", GREEN));
  addSymbol(SKELETON, symbol("Z", WHITE));
  addSymbol(VAMPIRE, symbol("V", DARK_GRAY));
  addSymbol(VAMPIRE_LORD, symbol("V", PURPLE));
  addSymbol(MUMMY, symbol("Z", YELLOW));
  addSymbol(MUMMY_LORD, symbol("Z", ORANGE));
  addSymbol(JACKAL, symbol("d", LIGHT_BROWN));
  addSymbol(DEER, symbol("R", DARK_BROWN));
  addSymbol(HORSE, symbol("H", LIGHT_BROWN));
  addSymbol(COW, symbol("C", WHITE));
  addSymbol(SHEEP, symbol("s", WHITE));
  addSymbol(PIG, symbol("p", YELLOW));
  addSymbol(GOAT, symbol("g", GRAY));
  addSymbol(BOAR, symbol("b", LIGHT_BROWN));
  addSymbol(FOX, symbol("d", ORANGE_BROWN));
  addSymbol(WOLF, symbol("d", DARK_BLUE));
  addSymbol(WEREWOLF, symbol("d", WHITE));
  addSymbol(DOG, symbol("d", BROWN));
  addSymbol(VODNIK, symbol("f", GREEN));
  addSymbol(KRAKEN_HEAD, symbol("S", GREEN));
  addSymbol(KRAKEN_WATER, symbol("S", DARK_GREEN));
  addSymbol(KRAKEN_LAND, symbol("S", DARK_GREEN));
  addSymbol(DEATH, symbol("D", DARK_GRAY));
  addSymbol(NIGHTMARE, symbol("n", PURPLE));
  addSymbol(FIRE_SPHERE, symbol("e", RED));
  addSymbol(BEAR, symbol("N", BROWN));
  addSymbol(BAT, symbol("b", DARK_GRAY));
  addSymbol(GOBLIN, symbol("o", GREEN));
  addSymbol(LEPRECHAUN, symbol("l", GREEN));
  addSymbol(RAT, symbol("r", BROWN));
  addSymbol(SPIDER, symbol("s", BROWN));
  addSymbol(FLY, symbol("b", GRAY));
  addSymbol(SCORPION, symbol("s", LIGHT_GRAY));
  addSymbol(SNAKE, symbol("S", YELLOW));
  addSymbol(VULTURE, symbol("v", DARK_GRAY));
  addSymbol(RAVEN, symbol("v", DARK_GRAY));
  addSymbol(BODY_PART, symbol("%", RED));
  addSymbol(BONE, symbol("%", WHITE));
  addSymbol(BUSH, symbol("&", DARK_GREEN));
  addSymbol(DECID_TREE, symbol(0x1f70d, DARK_GREEN, true));
  addSymbol(CANIF_TREE, symbol(0x2663, DARK_GREEN, true));
  addSymbol(TREE_TRUNK, symbol(".", BROWN));
  addSymbol(BURNT_TREE, symbol(".", DARK_GRAY));
  addSymbol(WATER, symbol("~", LIGHT_BLUE));
  addSymbol(MAGMA, symbol("~", RED));
  addSymbol(ABYSS, symbol("~", DARK_GRAY));
  addSymbol(DOOR, symbol("|", BROWN));
  addSymbol(BARRICADE, symbol("X", BROWN));
  addSymbol(DIG_ICON, symbol(0x26cf, LIGHT_GRAY, true));
  addSymbol(SWORD, symbol(")", LIGHT_GRAY));
  addSymbol(SPEAR, symbol("/", LIGHT_GRAY));
  addSymbol(SPECIAL_SWORD, symbol(")", YELLOW));
  addSymbol(ELVEN_SWORD, symbol(")", GRAY));
  addSymbol(KNIFE, symbol(")", WHITE));
  addSymbol(WAR_HAMMER, symbol(")", BLUE));
  addSymbol(SPECIAL_WAR_HAMMER, symbol(")", LIGHT_BLUE));
  addSymbol(BATTLE_AXE, symbol(")", GREEN));
  addSymbol(SPECIAL_BATTLE_AXE, symbol(")", LIGHT_GREEN));
  addSymbol(BOW, symbol(")", BROWN));
  addSymbol(CLUB, symbol(")", BROWN));
  addSymbol(HEAVY_CLUB, symbol(")", BROWN));
  addSymbol(ARROW, symbol("\\", BROWN));
  addSymbol(SCROLL, symbol("?", WHITE));
  addSymbol(STEEL_AMULET, symbol("\"", YELLOW));
  addSymbol(COPPER_AMULET, symbol("\"", YELLOW));
  addSymbol(CRYSTAL_AMULET, symbol("\"", YELLOW));
  addSymbol(WOODEN_AMULET, symbol("\"", YELLOW));
  addSymbol(AMBER_AMULET, symbol("\"", YELLOW));
  addSymbol(FIRE_RESIST_RING, symbol("=", RED));
  addSymbol(POISON_RESIST_RING, symbol("=", GREEN));
  addSymbol(BOOK, symbol("+", YELLOW));
  addSymbol(FIRST_AID, symbol("+", RED));
  addSymbol(TRAP_ITEM, symbol("+", YELLOW));
  addSymbol(EFFERVESCENT_POTION, symbol("!", LIGHT_RED));
  addSymbol(MURKY_POTION, symbol("!", BLUE));
  addSymbol(SWIRLY_POTION, symbol("!", YELLOW));
  addSymbol(VIOLET_POTION, symbol("!", VIOLET));
  addSymbol(PUCE_POTION, symbol("!", DARK_BROWN));
  addSymbol(SMOKY_POTION, symbol("!", LIGHT_GRAY));
  addSymbol(FIZZY_POTION, symbol("!", LIGHT_BLUE));
  addSymbol(MILKY_POTION, symbol("!", WHITE));
  addSymbol(MUSHROOM, symbol(0x22c6, PINK, true));
  addSymbol(FOUNTAIN, symbol("0", LIGHT_BLUE));
  addSymbol(GOLD, symbol("$", YELLOW));
  addSymbol(TREASURE_CHEST, symbol("=", BROWN));
  addSymbol(OPENED_CHEST, symbol("=", BROWN));
  addSymbol(CHEST, symbol("=", BROWN));
  addSymbol(OPENED_COFFIN, symbol("⚰", DARK_GRAY, true));
  addSymbol(COFFIN, symbol("⚰", DARK_GRAY, true));
  addSymbol(BOULDER, symbol("●", LIGHT_GRAY, true));
  addSymbol(PORTAL, symbol(0x1d6af, LIGHT_GREEN, true));
  addSymbol(TRAP, symbol("➹", YELLOW, true));
  addSymbol(GAS_TRAP, symbol("☠", GREEN, true));
  addSymbol(ALARM_TRAP, symbol("^", RED, true));
  addSymbol(WEB_TRAP, symbol("#", WHITE, true));
  addSymbol(SURPRISE_TRAP, symbol("^", BLUE, true));
  addSymbol(TERROR_TRAP, symbol("^", WHITE, true));
  addSymbol(ROCK, symbol("✱", LIGHT_GRAY, true));
  addSymbol(IRON_ROCK, symbol("✱", ORANGE, true));
  addSymbol(WOOD_PLANK, symbol("\\", BROWN));
  addSymbol(STOCKPILE1, symbol(".", YELLOW));
  addSymbol(STOCKPILE2, symbol(".", LIGHT_GREEN));
  addSymbol(STOCKPILE3, symbol(".", LIGHT_BLUE));
  addSymbol(PRISON, symbol(".", BLUE));
  addSymbol(DORM, symbol(".", BROWN));
  addSymbol(BED, symbol("=", WHITE));
  addSymbol(DUNGEON_HEART, symbol("♥", WHITE, true));
  addSymbol(TORCH, symbol("*", YELLOW));
  addSymbol(ALTAR, symbol("Ω", WHITE, true));
  addSymbol(CREATURE_ALTAR, symbol("Ω", YELLOW, true));
  addSymbol(TORTURE_TABLE, symbol("=", GRAY));
  addSymbol(IMPALED_HEAD, symbol("⚲", BROWN, true));
  addSymbol(TRAINING_ROOM, symbol("‡", BROWN, true));
  addSymbol(RITUAL_ROOM, symbol("Ω", PURPLE, true));
  addSymbol(LIBRARY, symbol("▤", BROWN, true));
  addSymbol(LABORATORY, symbol("ω", PURPLE, true));
  addSymbol(CAULDRON, symbol("ω", PURPLE, true));
  addSymbol(BEAST_LAIR, symbol(".", YELLOW));
  addSymbol(BEAST_CAGE, symbol("▥", LIGHT_GRAY, true));
  addSymbol(WORKSHOP, symbol("&", LIGHT_BROWN));
  addSymbol(FORGE, symbol("&", LIGHT_BLUE));
  addSymbol(JEWELER, symbol("&", YELLOW));
  addSymbol(CEMETERY, symbol(".", DARK_BLUE));
  addSymbol(GRAVE, symbol(0x2617, GRAY, true));
  addSymbol(BARS, symbol("⧻", LIGHT_BLUE));
  addSymbol(BORDER_GUARD, symbol(" ", WHITE));
  addSymbol(ROBE, symbol("[", LIGHT_BROWN));
  addSymbol(LEATHER_GLOVES, symbol("[", BROWN));
  addSymbol(STRENGTH_GLOVES, symbol("[", RED));
  addSymbol(DEXTERITY_GLOVES, symbol("[", LIGHT_BLUE));
  addSymbol(LEATHER_ARMOR, symbol("[", BROWN));
  addSymbol(LEATHER_HELM, symbol("[", BROWN));
  addSymbol(TELEPATHY_HELM, symbol("[", LIGHT_GREEN));
  addSymbol(CHAIN_ARMOR, symbol("[", LIGHT_GRAY));
  addSymbol(IRON_HELM, symbol("[", LIGHT_GRAY));
  addSymbol(LEATHER_BOOTS, symbol("[", BROWN));
  addSymbol(IRON_BOOTS, symbol("[", LIGHT_GRAY));
  addSymbol(SPEED_BOOTS, symbol("[", LIGHT_BLUE));
  addSymbol(LEVITATION_BOOTS, symbol("[", LIGHT_GREEN));
  addSymbol(DESTROYED_FURNITURE, symbol("*", BROWN));
  addSymbol(BURNT_FURNITURE, symbol("*", DARK_GRAY));
  addSymbol(FALLEN_TREE, symbol("*", GREEN));
  addSymbol(GUARD_POST, symbol("⚐", YELLOW, true));
  addSymbol(DESTROY_BUTTON, symbol("X", RED));
  addSymbol(MANA, symbol("✱", BLUE, true));
  addSymbol(EYEBALL, symbol("e", BLUE));
  addSymbol(DANGER, symbol("*", RED));
  addSymbol(FETCH_ICON, symbol(0x1f44b, LIGHT_BROWN, true));
  addSymbol(FOG_OF_WAR, symbol(' ', WHITE));
  addSymbol(FOG_OF_WAR_CORNER, symbol(' ', WHITE));
  addSymbol(SPECIAL_BEAST, symbol('B', PURPLE));
  addSymbol(SPECIAL_HUMANOID, symbol('H', PURPLE));
  addSymbol(TEAM_BUTTON, symbol("⚐", WHITE, true));
  addSymbol(TEAM_BUTTON_HIGHLIGHT, symbol("⚐", GREEN, true));
}
