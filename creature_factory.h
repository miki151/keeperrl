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

class Creature;


class CreatureFactory {
  public:
  static PCreature fromId(CreatureId, Tribe*, MonsterAIFactory = MonsterAIFactory::monster());
  static vector<PCreature> getFlock(int size, CreatureId, Creature* leader);
  static CreatureFactory humanVillage();
  static CreatureFactory humanCastle();
  static CreatureFactory elvenVillage();
  static CreatureFactory forrest();
  static CreatureFactory crypt();
  static CreatureFactory hellLevel();
  static CreatureFactory dwarfTown();
  static CreatureFactory vikingTown();
  static CreatureFactory castleAttackers();
  static CreatureFactory elfAttackers();
  static CreatureFactory lizardTown();
  static CreatureFactory goblinTown(int num);
  static CreatureFactory level(int num);
  static CreatureFactory splash();
  static CreatureFactory singleType(Tribe*, CreatureId);
  static CreatureFactory pyramid(int level);
  static CreatureFactory insects(Tribe* tribe);
  PCreature random(MonsterAIFactory = MonsterAIFactory::monster());

  static PCreature getShopkeeper(Location* shopArea, Tribe*);
  static PCreature getRollingBoulder(Vec2 direction);
  static PCreature getGuardingBoulder(Tribe* tribe);

  static PCreature addInventory(PCreature c, const vector<ItemId>& items);

  static void init();
 
  template <class Archive>
  static void registerTypes(Archive& ar);

  private:
  CreatureFactory(Tribe* tribe, const vector<CreatureId>& creatures, const vector<double>& weights,
      const vector<CreatureId>& unique);
  Tribe* tribe;
  vector<CreatureId> creatures;
  vector<double> weights;
  vector<CreatureId> unique;
};

enum class CreatureId {
    KEEPER,

    GOBLIN,
    GREAT_GOBLIN,
    BANDIT,

    SPECIAL_MONSTER,
    SPECIAL_MONSTER_KEEPER,
    SPECIAL_HUMANOID,

    GHOST,
    SPIRIT,
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
    SHEEP,
    PIG,
    GOAT,
    
    GNOME, 
    LEPRECHAUN,
    
    JACKAL,
    DEER,
    BOAR,
    FOX,
    RAVEN,
    VULTURE,
    WOLF,
    DOG,

    DEATH,
    NIGHTMARE,
    FIRE_SPHERE,
    KRAKEN,
    BAT,
    SNAKE,
    CAVE_BEAR,
    ACID_MOUND,
    SPIDER,
    SCORPION,
    FLY,
    RAT};

#endif
