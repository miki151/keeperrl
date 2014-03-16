#include "stdafx.h"
#include "tile.h"

using namespace colors;

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
    case ViewId::EMPTY: return Tile(' ', black);
    case ViewId::PLAYER: return Tile(1, 0);
    case ViewId::KEEPER: return Tile(3, 0);
    case ViewId::UNKNOWN_MONSTER: return Tile('?', lightGreen);
    case ViewId::SPECIAL_BEAST: return Tile(7, 10);
    case ViewId::SPECIAL_HUMANOID: return Tile(6, 10);
    case ViewId::ELF: return Tile(10, 6);
    case ViewId::ELF_ARCHER: return Tile(12, 6);
    case ViewId::ELF_CHILD: return Tile(14, 6);
    case ViewId::ELF_LORD: return Tile(13, 6);
    case ViewId::ELVEN_SHOPKEEPER: return Tile(4, 2);
    case ViewId::LIZARDMAN: return Tile(8, 8);
    case ViewId::LIZARDLORD: return Tile(11, 8);
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
    case ViewId::STONE: return Tile(1, 17, 1);
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
    case ViewId::WITCH: return Tile(15, 16);
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
    case ViewId::BURNT_TREE: return Tile(25, 3, 2, true);
    case ViewId::WATER: return getWaterTile(5);
    case ViewId::MAGMA: return getWaterTile(11);
    case ViewId::ABYSS: return Tile('~', darkGray);
    case ViewId::DOOR: return Tile(4, 2, 2, true);
    case ViewId::PLANNED_DOOR: return Tile(4, 2, 2, true).setTranslucent(0.5);
    case ViewId::DIG_ICON: return Tile(8, 10, 2);
    case ViewId::SWORD: return Tile(12, 9, 3);
    case ViewId::SPEAR: return Tile(5, 8, 3);
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
    case ViewId::ALARM_TRAP: return Tile(16, 5, 3);
    case ViewId::UNARMED_ALARM_TRAP: return Tile(16, 5, 3).setTranslucent(0.6);
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
    case ViewId::FALLEN_TREE: return Tile(26, 3, 2, true);
    case ViewId::GUARD_POST: return Tile(L'⚐', yellow, true);
    case ViewId::DESTROY_BUTTON: return Tile('X', red);
    case ViewId::MANA: return Tile(5, 10, 2);
    case ViewId::DANGER: return Tile(12, 9, 2);
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
    case ViewId::EMPTY: return Tile(' ', black);
    case ViewId::PLAYER: return Tile('@', white);
    case ViewId::KEEPER: return Tile('@', purple);
    case ViewId::UNKNOWN_MONSTER: return Tile('?', lightGreen);
    case ViewId::SPECIAL_BEAST: return getSpecialCreature(obj, false);
    case ViewId::SPECIAL_HUMANOID: return getSpecialCreature(obj, true);
    case ViewId::ELF: return Tile('@', lightGreen);
    case ViewId::ELF_ARCHER: return Tile('@', green);
    case ViewId::ELF_CHILD: return Tile('@', lightGreen);
    case ViewId::ELF_LORD: return Tile('@', darkGreen);
    case ViewId::ELVEN_SHOPKEEPER: return Tile('@', lightBlue);
    case ViewId::LIZARDMAN: return Tile('@', lightBrown);
    case ViewId::LIZARDLORD: return Tile('@', brown);
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
    case ViewId::WITCH: return Tile('@', brown);
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
    case ViewId::BURNT_TREE: return Tile('.', darkGray);
    case ViewId::WATER: return Tile('~', lightBlue);
    case ViewId::MAGMA: return Tile('~', red);
    case ViewId::ABYSS: return Tile('~', darkGray);
    case ViewId::DOOR: return Tile('|', brown);
    case ViewId::PLANNED_DOOR: return Tile('|', darkBrown);
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
    case ViewId::ALARM_TRAP: return Tile(L'^', red, true);
    case ViewId::UNARMED_ALARM_TRAP: return Tile(L'^', lightGray, true);
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
    case ViewId::LIBRARY: return Tile(L'▤', brown, true);
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
    case ViewId::GUARD_POST: return Tile(L'⚐', yellow, true);
    case ViewId::DESTROY_BUTTON: return Tile('X', red);
    case ViewId::MANA: return Tile('*', blue);
    case ViewId::DANGER: return Tile('*', red);
  }
  FAIL << "unhandled view id " << (int)obj.id();
  return Tile(' ', white);
}

Tile Tile::getTile(const ViewObject& obj, bool sprite) {
  if (sprite)
    return getSpriteTile(obj);
  else
    return getAsciiTile(obj);
}

Color Tile::getColor(const ViewObject& object) {
  if (object.isInvisible())
    return darkGray;
  if (object.isHidden())
    return lightGray;
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


