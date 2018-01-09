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

RICH_ENUM(CreatureId,
    KEEPER,
    KEEPER_F,
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
    PRISONER,
    OGRE,
    CHICKEN,

    PRIEST,
    KNIGHT,
    JESTER,
    AVATAR,
    ARCHER,
    PESEANT,
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
  static CreatureFactory singleCreature(TribeId, CreatureId);

  static PCreature fromId(CreatureId, TribeId, const MonsterAIFactory&);
  static PCreature fromId(CreatureId, TribeId, const MonsterAIFactory&, const vector<ItemType>& inventory);
  static PCreature fromId(CreatureId, TribeId);
  static CreatureFactory splashHeroes(TribeId);
  static CreatureFactory splashLeader(TribeId);
  static CreatureFactory splashMonsters(TribeId);
  static CreatureFactory forrest(TribeId);
  static CreatureFactory singleType(TribeId, CreatureId);
  static CreatureFactory lavaCreatures(TribeId tribe);
  static CreatureFactory waterCreatures(TribeId tribe);
  static CreatureFactory elementals(TribeId tribe);
  static CreatureFactory gnomishMines(TribeId peaceful, TribeId enemy, int level);
  
  PCreature random(const MonsterAIFactory&);
  PCreature random();

  CreatureFactory& increaseBaseLevel(ExperienceType, int);
  CreatureFactory& increaseLevel(ExperienceType, int);
  CreatureFactory& increaseLevel(EnumMap<ExperienceType, int>);
  CreatureFactory& addInventory(vector<ItemType>);

  static PCreature getShopkeeper(Rectangle shopArea, TribeId);
  static PCreature getRollingBoulder(TribeId, Vec2 direction);
  static PCreature getGhost(WCreature);
  static PCreature getIllusion(WCreature);

  static void addInventory(WCreature, const vector<ItemType>& items);
  static CreatureAttributes getKrakenAttributes(ViewId, const char* name);
  static ViewId getViewId(CreatureId);
  static const Gender& getGender(CreatureId);

  ~CreatureFactory();
  CreatureFactory& operator = (const CreatureFactory&);
  CreatureFactory(const CreatureFactory&);

  SERIALIZATION_DECL(CreatureFactory)

  private:
  CreatureFactory(TribeId tribe, const vector<CreatureId>& creatures, const vector<double>& weights,
      const vector<CreatureId>& unique = {}, EnumMap<CreatureId, optional<TribeId>> overrides = {});
  CreatureFactory(const vector<tuple<CreatureId, double, TribeId>>& creatures,
      const vector<CreatureId>& unique = {});
  static void initSplash(TribeId);
  static PCreature getSokobanBoulder(TribeId);
  static PCreature getSpecial(TribeId, bool humanoid, bool large, bool living, bool wings, const ControllerFactory&);
  static PCreature get(CreatureId, TribeId, MonsterAIFactory);
  static PCreature get(CreatureAttributes, TribeId, const ControllerFactory&);
  static CreatureAttributes getAttributesFromId(CreatureId);
  static CreatureAttributes getAttributes(CreatureId id);
  TribeId getTribeFor(CreatureId);
  optional<TribeId> SERIAL(tribe);
  vector<CreatureId> SERIAL(creatures);
  vector<double> SERIAL(weights);
  vector<CreatureId> SERIAL(unique);
  EnumMap<CreatureId, optional<TribeId>> SERIAL(tribeOverrides);
  EnumMap<ExperienceType, int> SERIAL(baseLevelIncrease);
  EnumMap<ExperienceType, int> SERIAL(levelIncrease);
  vector<ItemType> SERIAL(inventory);
};
