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

#ifndef _VIEW_OBJECT_H
#define _VIEW_OBJECT_H

#include "debug.h"
#include "enums.h"
#include "util.h"

class ViewObject {
  public:
  ViewObject(ViewId id, ViewLayer l, const string& description);

  enum EnemyStatus { HOSTILE, FRIENDLY, UNKNOWN };
  void setEnemyStatus(EnemyStatus);
  bool isHostile() const;
  bool isFriendly() const;

  enum class Modifier { BLIND, PLAYER, HIDDEN, INVISIBLE, ILLUSION, POISONED, CASTS_SHADOW, PLANNED, LOCKED,
    ROUND_SHADOW, MOVE_UP, TEAM_HIGHLIGHT, DARK, ENUM_END};

  ViewObject& setModifier(Modifier);
  ViewObject& removeModifier(Modifier);
  bool hasModifier(Modifier) const;

  enum class Attribute { BLEEDING, BURNING, HEIGHT, ATTACK, DEFENSE, LEVEL, WATER_DEPTH, EFFICIENCY, ENUM_END };

  static void setHallu(bool);

  ViewObject& setAttribute(Attribute, double);
  double getAttribute(Attribute) const;

  string getDescription(bool stats = false) const;
  string getBareDescription() const;

  ViewLayer layer() const;
  ViewId id() const;

  const static ViewObject& unknownMonster();
  const static ViewObject& empty();
  const static ViewObject& mana();

  SERIALIZATION_DECL(ViewObject);

  private:
  string getAttributeString(Attribute) const;
  EnemyStatus SERIAL2(enemyStatus, UNKNOWN);
  EnumSet<Modifier> SERIAL(modifiers);
  EnumMap<Attribute, double> SERIAL(attributes);
  ViewId SERIAL(resource_id);
  ViewLayer SERIAL(viewLayer);
  string SERIAL(description);
};

enum class ViewId { 
  EMPTY,
  PLAYER,
  KEEPER,
  ELF,
  ELF_ARCHER,
  ELF_CHILD,
  ELF_LORD,
  ELVEN_SHOPKEEPER,
  LIZARDMAN,
  LIZARDLORD,
  DWARF,
  DWARF_BARON,
  DWARVEN_SHOPKEEPER,
  IMP,
  PRISONER,
  BILE_DEMON,
  CHICKEN,
  DARK_KNIGHT,
  GREEN_DRAGON,
  RED_DRAGON,
  CYCLOPS,
  WITCH,
  GHOST,
  SPIRIT,
  DEVIL,
  KNIGHT,
  CASTLE_GUARD,
  AVATAR,
  ARCHER,
  PESEANT,
  CHILD,
  SHAMAN,
  WARRIOR,
  GREAT_GOBLIN,
  GOBLIN,
  BANDIT,
  CLAY_GOLEM,
  STONE_GOLEM,
  IRON_GOLEM,
  LAVA_GOLEM,
  ZOMBIE,
  SKELETON,
  VAMPIRE,
  VAMPIRE_LORD,
  MUMMY,
  MUMMY_LORD,
  HORSE,
  COW,
  PIG,
  GOAT,
  SHEEP,
  JACKAL,
  DEER,
  BOAR,
  FOX,
  BEAR,
  WOLF,
  DOG,
  BAT,
  RAT,
  SPIDER,
  FLY,
  ACID_MOUND,
  SCORPION,
  SNAKE,
  VULTURE,
  RAVEN,
  GNOME,
  VODNIK,
  LEPRECHAUN,
  KRAKEN,
  KRAKEN2,
  FIRE_SPHERE,
  NIGHTMARE,
  DEATH,
  SPECIAL_BEAST,
  SPECIAL_HUMANOID,
  UNKNOWN_MONSTER,

  FLOOR,
  SAND,
  BRIDGE,
  PATH,
  ROAD,
  GRASS,
  CROPS,
  MUD,
  WALL,
  HILL,
  MOUNTAIN,
  MOUNTAIN2,
  SNOW,
  GOLD_ORE,
  IRON_ORE,
  STONE,
  WOOD_WALL,
  BLACK_WALL,
  YELLOW_WALL,
  HELL_WALL,
  LOW_ROCK_WALL,
  CASTLE_WALL,
  MUD_WALL,
  WELL,
  STATUE1,
  STATUE2,
  SECRETPASS,
  DUNGEON_ENTRANCE,
  DUNGEON_ENTRANCE_MUD,
  DOWN_STAIRCASE,
  UP_STAIRCASE,
  DOWN_STAIRCASE_HELL,
  UP_STAIRCASE_HELL,
  DOWN_STAIRCASE_CELLAR,
  UP_STAIRCASE_CELLAR,
  DOWN_STAIRCASE_PYR,
  UP_STAIRCASE_PYR,
  CANIF_TREE,
  DECID_TREE,
  TREE_TRUNK,
  BUSH,
  WATER,
  MAGMA,
  ABYSS,
  DOOR,
  BARRICADE,
  DIG_ICON,
  FOUNTAIN,
  CHEST,
  OPENED_CHEST,
  COFFIN,
  OPENED_COFFIN,
  TORCH,
  DORM,
  BED,
  STOCKPILE1,
  STOCKPILE2,
  STOCKPILE3,
  PRISON,
  TORTURE_TABLE,
  IMPALED_HEAD,
  TRAINING_ROOM,
  LIBRARY,
  LABORATORY,
  BEAST_LAIR,
  BEAST_CAGE,
  WORKSHOP,
  DUNGEON_HEART,
  ALTAR,
  CEMETERY,
  GRAVE,
  BARS,
  BORDER_GUARD,
  DESTROYED_FURNITURE,
  BURNT_FURNITURE,
  FALLEN_TREE,
  BURNT_TREE,

  BODY_PART,
  BONE,
  SPEAR,
  SWORD,
  SPECIAL_SWORD,
  ELVEN_SWORD,
  KNIFE,
  WAR_HAMMER,
  SPECIAL_WAR_HAMMER,
  BATTLE_AXE,
  SPECIAL_BATTLE_AXE,
  BOW,
  ARROW,
  SCROLL,
  STEEL_AMULET,
  COPPER_AMULET,
  CRYSTAL_AMULET,
  WOODEN_AMULET,
  AMBER_AMULET,
  FIRE_RESIST_RING,
  POISON_RESIST_RING,
  BOOK,
  FIRST_AID,
  EFFERVESCENT_POTION,
  MURKY_POTION,
  SWIRLY_POTION,
  VIOLET_POTION,
  PUCE_POTION,
  SMOKY_POTION,
  FIZZY_POTION,
  MILKY_POTION,
  GOLD,
  ROBE,
  LEATHER_GLOVES,
  DEXTERITY_GLOVES,
  STRENGTH_GLOVES,
  LEATHER_ARMOR,
  LEATHER_HELM,
  TELEPATHY_HELM,
  CHAIN_ARMOR,
  IRON_HELM,
  LEATHER_BOOTS,
  IRON_BOOTS,
  SPEED_BOOTS,
  LEVITATION_BOOTS,
  BOULDER,
  PORTAL,
  TRAP,
  GAS_TRAP,
  ALARM_TRAP,
  WEB_TRAP,
  SURPRISE_TRAP,
  TERROR_TRAP,
  ROCK,
  IRON_ROCK,
  WOOD_PLANK,
  SLIMY_MUSHROOM, 
  PINK_MUSHROOM, 
  DOTTED_MUSHROOM, 
  GLOWING_MUSHROOM, 
  GREEN_MUSHROOM, 
  BLACK_MUSHROOM,

  TRAP_ITEM,
  GUARD_POST,
  DESTROY_BUTTON,
  MANA,
  DANGER,
  FETCH_ICON,
};

#endif
