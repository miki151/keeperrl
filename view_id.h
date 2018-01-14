#pragma once

#include "util.h"

RICH_ENUM(
    ViewId,
    EMPTY,
    PLAYER,
    PLAYER_F,
    KEEPER,
    KEEPER_F,
    RETIRED_KEEPER,
    RETIRED_KEEPER_F,
    ELF,
    ELF_WOMAN,
    ELF_ARCHER,
    ELF_CHILD,
    ELF_LORD,
    DARK_ELF,
    DARK_ELF_WOMAN,
    DARK_ELF_WARRIOR,
    DARK_ELF_CHILD,
    DARK_ELF_LORD,
    DEMON_DWELLER,
    DEMON_LORD,
    DRIAD,
    LIZARDMAN,
    LIZARDLORD,
    GNOME,
    GNOME_BOSS,
    KOBOLD,
    DWARF,
    DWARF_FEMALE,
    DWARF_BARON,
    SHOPKEEPER,
    IMP,
    PRISONER,
    OGRE,
    UNICORN,
    CHICKEN,
    GREEN_DRAGON,
    RED_DRAGON,
    CYCLOPS,
    MINOTAUR,
    SOFT_MONSTER,
    HYDRA,
    SHELOB,
    WITCH,
    WITCHMAN,
    GHOST,
    SPIRIT,
    KNIGHT,
    PRIEST,
    DUKE,
    JESTER,
    ARCHER,
    PESEANT,
    PESEANT_WOMAN,
    CHILD,
    SHAMAN,
    WARRIOR,
    ORC,
    ORC_CAPTAIN,
    ORC_SHAMAN,
    HARPY,
    SUCCUBUS,
    DOPPLEGANGER,
    BANDIT,
    CLAY_GOLEM,
    STONE_GOLEM,
    IRON_GOLEM,
    LAVA_GOLEM,
    AUTOMATON,
    ELEMENTALIST,
    FIRE_ELEMENTAL,
    WATER_ELEMENTAL,
    EARTH_ELEMENTAL,
    AIR_ELEMENTAL,
    ENT,
    ANGEL,
    ZOMBIE,
    SKELETON,
    VAMPIRE,
    VAMPIRE_LORD,
    MUMMY,
    HORSE,
    COW,
    PIG,
    DONKEY,
    GOAT,
    JACKAL,
    DEER,
    BOAR,
    FOX,
    BEAR,
    WOLF,
    WEREWOLF,
    DOG,
    BAT,
    RAT,
    SPIDER,
    FLY,
    ANT_WORKER,
    ANT_SOLDIER,
    ANT_QUEEN,
    SNAKE,
    VULTURE,
    RAVEN,
    GOBLIN,
    KRAKEN_HEAD,
    KRAKEN_LAND,
    KRAKEN_WATER,
    FIRE_SPHERE,
    DEATH,
    SPECIAL_BLBN,
    SPECIAL_BLBW,
    SPECIAL_BLGN,
    SPECIAL_BLGW,
    SPECIAL_BMBN,
    SPECIAL_BMBW,
    SPECIAL_BMGN,
    SPECIAL_BMGW,
    SPECIAL_HLBN,
    SPECIAL_HLBW,
    SPECIAL_HLGN,
    SPECIAL_HLGW,
    SPECIAL_HMBN,
    SPECIAL_HMBW,
    SPECIAL_HMGN,
    SPECIAL_HMGW,

    UNKNOWN_MONSTER,

    FLOOR,
    KEEPER_FLOOR,
    WOOD_FLOOR1,
    WOOD_FLOOR2,
    WOOD_FLOOR3,
    WOOD_FLOOR4,
    WOOD_FLOOR5,
    STONE_FLOOR1,
    STONE_FLOOR2,
    STONE_FLOOR3,
    STONE_FLOOR4,
    STONE_FLOOR5,
    CARPET_FLOOR1,
    CARPET_FLOOR2,
    CARPET_FLOOR3,
    CARPET_FLOOR4,
    CARPET_FLOOR5,
    BUFF_FLOOR1,
    BUFF_FLOOR2,
    BUFF_FLOOR3,
    SAND,
    BRIDGE,
    ROAD,
    GRASS,
    CROPS,
    CROPS2,
    MUD,
    WALL,
    HILL,
    MOUNTAIN,
    DUNGEON_WALL,
    MAP_MOUNTAIN1,
    MAP_MOUNTAIN2,
    MAP_MOUNTAIN3,
    GOLD_ORE,
    IRON_ORE,
    STONE,
    WOOD_WALL,
    BLACK_WALL,
    CASTLE_WALL,
    MUD_WALL,
    WELL,
    MINION_STATUE,
    DOWN_STAIRCASE,
    UP_STAIRCASE,
    CANIF_TREE,
    DECID_TREE,
    TREE_TRUNK,
    BUSH,
    WATER,
    MAGMA,
    DOOR,
    LOCKED_DOOR,
    BARRICADE,
    DIG_ICON,
    FOUNTAIN,
    TREASURE_CHEST,
    CHEST,
    OPENED_CHEST,
    COFFIN,
    OPENED_COFFIN,
    TORCH,
    DORM,
    BED,
    STORAGE_EQUIPMENT,
    STORAGE_RESOURCES,
    QUARTERS1,
    QUARTERS2,
    QUARTERS3,
    PRISON,
    TORTURE_TABLE,
    IMPALED_HEAD,
    WHIPPING_POST,
    GALLOWS,
    TRAINING_WOOD,
    TRAINING_IRON,
    TRAINING_STEEL,
    ARCHERY_RANGE,
    BOOKCASE_WOOD,
    BOOKCASE_IRON,
    BOOKCASE_GOLD,
    FORGE,
    LABORATORY,
    JEWELER,
    STEEL_FURNACE,
    WORKSHOP,
    CAULDRON,
    DEMON_SHRINE,
    BEAST_LAIR,
    BEAST_CAGE,
    ALTAR,
    CREATURE_ALTAR,
    CEMETERY,
    GRAVE,
    BORDER_GUARD,
    FALLEN_TREE,
    BURNT_TREE,
    EYEBALL,
    THRONE,
    NOTICE_BOARD,
    SOKOBAN_HOLE,

    BODY_PART,
    BONE,
    SPEAR,
    SWORD,
    STEEL_SWORD,
    SPECIAL_SWORD,
    ELVEN_SWORD,
    KNIFE,
    WAR_HAMMER,
    SPECIAL_WAR_HAMMER,
    BATTLE_AXE,
    STEEL_BATTLE_AXE,
    SPECIAL_BATTLE_AXE,
    CLUB,
    HEAVY_CLUB,
    BOW,
    ELVEN_BOW,
    ARROW,
    WOODEN_STAFF,
    IRON_STAFF,
    FORCE_BOLT,
    AIR_BLAST,
    STUN_RAY,
    SCROLL,
    AMULET1,
    AMULET2,
    AMULET3,
    AMULET4,
    AMULET5,
    LOOTING_RING,
    FIRE_RESIST_RING,
    POISON_RESIST_RING,
    BOOK,
    FIRST_AID,
    POTION1,
    POTION2,
    POTION3,
    POTION4,
    POTION5,
    POTION6,
    GOLD,
    ROBE,
    LEATHER_GLOVES,
    DEXTERITY_GLOVES,
    STRENGTH_GLOVES,
    LEATHER_ARMOR,
    LEATHER_HELM,
    TELEPATHY_HELM,
    CHAIN_ARMOR,
    STEEL_ARMOR,
    IRON_HELM,
    LEATHER_BOOTS,
    IRON_BOOTS,
    SPEED_BOOTS,
    LEVITATION_BOOTS,
    BOULDER,
    PORTAL,
    GAS_TRAP,
    ALARM_TRAP,
    WEB_TRAP,
    SURPRISE_TRAP,
    TERROR_TRAP,
    ROCK,
    IRON_ROCK,
    STEEL_INGOT,
    WOOD_PLANK,
    MUSHROOM1,
    MUSHROOM2,
    MUSHROOM3,
    MUSHROOM4,
    MUSHROOM5,
    MUSHROOM6,
    KEY,

    HORN_ATTACK,
    BEAK_ATTACK,
    TOUCH_ATTACK,
    BITE_ATTACK,
    CLAWS_ATTACK,
    LEG_ATTACK,
    FIST_ATTACK,

    TRAP_ITEM,
    GUARD_POST,
    DESTROY_BUTTON,
    MANA,
    FETCH_ICON,

    FOG_OF_WAR,
    FOG_OF_WAR_CORNER,

    DIG_MARK,
    DIG_MARK2,
    FORBID_ZONE,
    CREATURE_HIGHLIGHT,
    SQUARE_HIGHLIGHT,
    ROUND_SHADOW,
    CAMPAIGN_DEFEATED,
    ACCEPT_IMMIGRANT,
    REJECT_IMMIGRANT,
    TUTORIAL_ENTRANCE,
    HALLOWEEN_KID1,
    HALLOWEEN_KID2,
    HALLOWEEN_KID3,
    HALLOWEEN_KID4,
    HALLOWEEN_COSTUME,
    BAG_OF_CANDY,
    STANDING_TORCH
);

