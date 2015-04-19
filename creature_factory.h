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

#ifndef _CREATURE_FACTORY
#define _CREATURE_FACTORY

#include <map>
#include <string>
#include <functional>

#include "util.h"
#include "tribe.h"
#include "monster_ai.h"
#include "item_type.h"

class Creature;

RICH_ENUM(CreatureId,
    KEEPER,

    GOBLIN, 
    ORC,
    ORC_SHAMAN,
    GREAT_ORC,
    HARPY,
    SUCCUBUS,
    DOPPLEGANGER,
    BANDIT,

    SPECIAL_MONSTER,
    SPECIAL_MONSTER_KEEPER,
    SPECIAL_HUMANOID,

    GHOST,
    SPIRIT,
    LOST_SOUL,
    DEVIL,
    DARK_KNIGHT,
    GREEN_DRAGON,
    RED_DRAGON,
    CYCLOPS,
    WITCH,

    CLAY_GOLEM,
    STONE_GOLEM,
    IRON_GOLEM,
    LAVA_GOLEM,

    FIRE_ELEMENTAL,
    WATER_ELEMENTAL,
    EARTH_ELEMENTAL,
    AIR_ELEMENTAL,
    ENT,
    ANGEL,

    ZOMBIE,
    VAMPIRE,
    VAMPIRE_LORD,
    VAMPIRE_BAT,
    MUMMY,
    MUMMY_LORD,
    SKELETON,
    
    GNOME,
    DWARF,
    DWARF_BARON,

    IMP,
    PRISONER,
    OGRE,
    CHICKEN,

    KNIGHT,
    AVATAR,
    CASTLE_GUARD,
    ARCHER,
    PESEANT,
    CHILD,
    SHAMAN,
    WARRIOR,

    LIZARDMAN,
    LIZARDLORD,
    
    ELF,
    ELF_ARCHER,
    ELF_CHILD,
    ELF_LORD,
    HORSE,
    COW,
    PIG,
    GOAT,
    
    LEPRECHAUN,
    
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
    SCORPION,
    FLY,
    RAT
);

class CreatureFactory {
  public:
  struct SingleCreature {
    SingleCreature(Tribe*, CreatureId);
    CreatureId SERIAL(id);
    Tribe* SERIAL(tribe);
    bool operator == (const SingleCreature&) const;
    SERIALIZATION_DECL(SingleCreature);
  };
  CreatureFactory(const SingleCreature&);

  static PCreature fromId(CreatureId, Tribe*, MonsterAIFactory = MonsterAIFactory::monster());
  static vector<PCreature> getFlock(int size, CreatureId, Creature* leader);
  static CreatureFactory humanVillage(Tribe*);
  static CreatureFactory splashHeroes(Tribe*);
  static CreatureFactory splashMonsters(Tribe*);
  static CreatureFactory gnomeVillage(Tribe*);
  static CreatureFactory humanCastle(Tribe*);
  static CreatureFactory elvenVillage(Tribe*);
  static CreatureFactory forrest(Tribe*);
  static CreatureFactory crypt(Tribe*);
  static SingleCreature coffins(Tribe*);
  static CreatureFactory hellLevel(Tribe*);
  static CreatureFactory dwarfTown(Tribe*);
  static CreatureFactory vikingTown(Tribe*);
  static CreatureFactory lizardTown(Tribe*);
  static CreatureFactory orcTown(Tribe*);
  static CreatureFactory splash(Tribe*);
  static CreatureFactory singleType(Tribe*, CreatureId);
  static CreatureFactory pyramid(Tribe*, int level);
  static CreatureFactory insects(Tribe* tribe);
  static CreatureFactory lavaCreatures(Tribe* tribe);
  static CreatureFactory waterCreatures(Tribe* tribe);
  
  PCreature random(MonsterAIFactory = MonsterAIFactory::monster());

  static PCreature getShopkeeper(Location* shopArea, Tribe*);
  static PCreature getRollingBoulder(Vec2 direction, Tribe*);
  static PCreature getGuardingBoulder(Tribe* tribe);

  static PCreature addInventory(PCreature c, const vector<ItemType>& items);

  static void init();

  template <class Archive>
  static void registerTypes(Archive& ar, int version);

  SERIALIZATION_DECL(CreatureFactory);

  private:
  CreatureFactory(Tribe* tribe, const vector<CreatureId>& creatures, const vector<double>& weights,
      const vector<CreatureId>& unique, EnumMap<CreatureId, Tribe*> overrides = {}, double levelIncrease = 0);
  Tribe* getTribeFor(CreatureId);
  Tribe* SERIAL(tribe);
  vector<CreatureId> SERIAL(creatures);
  vector<double> SERIAL(weights);
  vector<CreatureId> SERIAL(unique);
  EnumMap<CreatureId, Tribe*> SERIAL(tribeOverrides);
  double SERIAL(levelIncrease) = 0;
};


#endif
