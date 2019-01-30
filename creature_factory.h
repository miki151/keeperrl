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

#pragma once

#include "util.h"
#include "tribe.h"
#include "experience_type.h"

class Creature;
class MonsterAIFactory;
class Tribe;
class ItemType;
class CreatureAttributes;
class ControllerFactory;
class Gender;
class NameGenerator;

RICH_ENUM(CreatureId,
    KEEPER_MAGE,
    KEEPER_MAGE_F,
    KEEPER_KNIGHT,
    KEEPER_KNIGHT_F,
    ADVENTURER,
    ADVENTURER_F,

    GOBLIN, 
    ORC,
    ORC_SHAMAN,
    HARPY,
    SUCCUBUS,
    DOPPLEGANGER,
    BANDIT,

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

    GHOST,
    UNICORN,    
    SPIRIT,
    LOST_SOUL,
    GREEN_DRAGON,
    RED_DRAGON,
    CYCLOPS,
    DEMON_DWELLER,  
    DEMON_LORD, 
    MINOTAUR,
    HYDRA,
    SHELOB,
    SPIDER_FOOD,
    WITCH,
    WITCHMAN,

    CLAY_GOLEM,
    STONE_GOLEM,
    IRON_GOLEM,
    LAVA_GOLEM,
    ADA_GOLEM,
    AUTOMATON,

    FIRE_ELEMENTAL,
    WATER_ELEMENTAL,
    EARTH_ELEMENTAL,
    AIR_ELEMENTAL,
    ENT,
    ANGEL,

    ZOMBIE,
    VAMPIRE,
    VAMPIRE_LORD,
    MUMMY,
    SKELETON,
    
    GNOME,
    GNOME_CHIEF,
    DWARF,
    DWARF_FEMALE,
    DWARF_BARON,

    KOBOLD,

    IMP,
    OGRE,
    CHICKEN,

    PRIEST_PLAYER,
    KNIGHT_PLAYER,
    JESTER_PLAYER,
    KEEPER_KNIGHT_WHITE,
    KEEPER_KNIGHT_WHITE_F,
    ARCHER_PLAYER,
    GNOME_PLAYER,
    PESEANT_PLAYER,

    PRIEST,
    KNIGHT,
    JESTER,
    DUKE,
    ARCHER,
    PESEANT,
    PESEANT_PRISONER,
    CHILD,
    SHAMAN,
    WARRIOR,

    LIZARDMAN,
    LIZARDLORD,

    ELEMENTALIST,
    
    ELF,
    ELF_ARCHER,
    ELF_CHILD,
    ELF_LORD,
    DARK_ELF,
    DARK_ELF_WARRIOR,
    DARK_ELF_CHILD,
    DARK_ELF_LORD,
    DRIAD,
    HORSE,
    COW,
    PIG,
    GOAT,
    DONKEY,
    
    JACKAL,
    DEER,
    BOAR,
    FOX,
    RAVEN,
    VULTURE,
    WOLF,
    WEREWOLF,
    DOG,

    DEATH,
    FIRE_SPHERE,
    KRAKEN,
    BAT,
    SNAKE,
    CAVE_BEAR,
    SPIDER,
    FLY,
    RAT,
    SOFT_MONSTER,

    ANT_WORKER,
    ANT_SOLDIER,
    ANT_QUEEN,

    SOKOBAN_BOULDER,
    HALLOWEEN_KID
);

class CreatureFactory {
  public:
  PCreature fromId(CreatureId, TribeId, const MonsterAIFactory&) const;
  PCreature fromId(CreatureId, TribeId, const MonsterAIFactory&, const vector<ItemType>& inventory) const;
  PCreature fromId(CreatureId, TribeId) const;
  PCreature getShopkeeper(Rectangle shopArea, TribeId) const;
  static PCreature getRollingBoulder(TribeId, Vec2 direction);
  static PCreature getHumanForTests();
  PCreature getGhost(WCreature) const;
  static PCreature getIllusion(WCreature);

  static void addInventory(WCreature, const vector<ItemType>& items);
  static CreatureAttributes getKrakenAttributes(ViewId, const char* name);
  ViewId getViewId(CreatureId) const;
  const Gender& getGender(CreatureId);

  CreatureFactory(NameGenerator*);

  private:
  void initSplash(TribeId);
  static PCreature getSokobanBoulder(TribeId);
  PCreature getSpecial(TribeId, bool humanoid, bool large, bool living, bool wings, const ControllerFactory&) const;
  PCreature get(CreatureId, TribeId, MonsterAIFactory) const;
  static PCreature get(CreatureAttributes, TribeId, const ControllerFactory&);
  CreatureAttributes getAttributesFromId(CreatureId) const;
  CreatureAttributes getAttributes(CreatureId) const;
  EnumMap<CreatureId, ViewId> idMap;
  NameGenerator* nameGenerator;
};
