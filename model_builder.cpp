#include <SFML/Graphics.hpp>

#include "stdafx.h"
#include "model_builder.h"
#include "level.h"
#include "tribe.h"
#include "technology.h"
#include "item_type.h"
#include "inventory.h"
#include "collective_builder.h"
#include "options.h"
#include "player_control.h"
#include "spectator.h"
#include "creature.h"
#include "square.h"
#include "location.h"
#include "progress_meter.h"
#include "collective.h"
#include "name_generator.h"
#include "level_maker.h"
#include "model.h"
#include "level_builder.h"
#include "monster_ai.h"

static Location* getVillageLocation(bool markSurprise = false) {
  return new Location(NameGenerator::get(NameGeneratorId::TOWN)->getNext(), "", markSurprise);
}

typedef VillageControl::Villain VillainInfo;

enum class ExtraLevelId {
  CRYPT,
  GNOMISH_MINES,
  TOWER,
  MAZE,
  SOKOBAN,
};

struct LevelInfo {
  ExtraLevelId levelId;
  StairKey stairKey;
};

struct EnemyInfo {
  SettlementInfo settlement;
  CollectiveConfig config;
  vector<VillainInfo> villains;
  optional<LevelInfo> extraLevel;
  optional<VillainType> villainType;
};

static EnemyInfo mainVillain(SettlementInfo settlement, CollectiveConfig config, vector<VillainInfo> villains = {},
    optional<LevelInfo> extraLevel = none) {
  return EnemyInfo{settlement, config, villains, extraLevel, VillainType::MAIN};
}

static EnemyInfo lesserVillain(SettlementInfo settlement, CollectiveConfig config,
    vector<VillainInfo> villains = {}, optional<LevelInfo> extraLevel = none) {
  return EnemyInfo{settlement, config, villains, extraLevel, VillainType::LESSER};
}

static EnemyInfo noVillain(SettlementInfo settlement, CollectiveConfig config,
    vector<VillainInfo> villains = {}, optional<LevelInfo> extraLevel = none) {
  return EnemyInfo{settlement, config, villains, extraLevel, none};
}

static EnemyInfo getVault(SettlementType type, CreatureFactory factory, Tribe* tribe, int num,
    optional<ItemFactory> itemFactory = none, vector<VillainInfo> villains = {}) {
  return noVillain(CONSTRUCT(SettlementInfo,
      c.type = type;
      c.creatures = factory;
      c.numCreatures = num;
      c.location = new Location(true);
      c.tribe = tribe;
      c.buildingId = BuildingId::DUNGEON;
      c.shopFactory = itemFactory;), CollectiveConfig::noImmigrants(),
    villains);
}

static EnemyInfo getVault(SettlementType type, CreatureId id, Tribe* tribe, int num,
    optional<ItemFactory> itemFactory = none, vector<VillainInfo> villains = {}) {
  return getVault(type, CreatureFactory::singleType(tribe, id), tribe, num, itemFactory, villains);
}

struct FriendlyVault {
  CreatureId id;
  int min;
  int max;
};

static vector<FriendlyVault> friendlyVaults {
 // {CreatureId::SPECIAL_HUMANOID, 1, 2},
  {CreatureId::ORC, 3, 8},
  {CreatureId::OGRE, 2, 5},
  {CreatureId::VAMPIRE, 2, 5},
};

static vector<EnemyInfo> getVaults(RandomGen& random, TribeSet& tribeSet) {
  vector<EnemyInfo> ret {
 /*   getVault(SettlementType::VAULT, CreatureFactory::insects(tribeSet.monster.get()),
        tribeSet.monster.get(), random.get(6, 12)),*/
    getVault(SettlementType::VAULT, CreatureId::RAT, tribeSet.pest.get(), random.get(3, 8),
        ItemFactory::armory()),
  };
  for (int i : Range(random.get(1, 3))) {
    FriendlyVault v = random.choose(friendlyVaults);
    ret.push_back(getVault(SettlementType::VAULT, v.id, tribeSet.keeper.get(), random.get(v.min, v.max)));
  }
  return ret;
}

static vector<EnemyInfo> getGnomishMines(RandomGen& random, TribeSet& tribeSet) {
  StairKey gnomeKey = StairKey::getNew();
  return {
    lesserVillain(CONSTRUCT(SettlementInfo,
      c.type = SettlementType::MINETOWN;
      c.creatures = CreatureFactory::gnomeVillage(tribeSet.gnomish.get());
      c.numCreatures = random.get(12, 24);
      c.location = getVillageLocation();
      c.tribe = tribeSet.gnomish.get();
      c.buildingId = BuildingId::DUNGEON;
      c.shopFactory = ItemFactory::gnomeShop();
      c.outsideFeatures = SquareFactory::dungeonOutside();
      c.furniture = SquareFactory::roomFurniture(tribeSet.pest.get());),
      CollectiveConfig::noImmigrants(), {}, LevelInfo{ExtraLevelId::GNOMISH_MINES, gnomeKey}),
    noVillain(CONSTRUCT(SettlementInfo,
      c.type = SettlementType::SMALL_MINETOWN;
      c.creatures = CreatureFactory::gnomeEntrance(tribeSet.gnomish.get());
      c.numCreatures = random.get(3, 7);
      c.location = new Location(true);
      c.tribe = tribeSet.gnomish.get();
      c.buildingId = BuildingId::DUNGEON;
      c.downStairs = {gnomeKey};
      c.outsideFeatures = SquareFactory::dungeonOutside();
      c.furniture = SquareFactory::roomFurniture(tribeSet.pest.get());),
      CollectiveConfig::noImmigrants(), {})
  };
}

static vector<EnemyInfo> getDarkElvenMines(RandomGen& random, TribeSet& tribeSet) {
  StairKey gnomeKey = StairKey::getNew();
  return {
    lesserVillain(CONSTRUCT(SettlementInfo,
      c.type = SettlementType::MINETOWN;
      c.creatures = CreatureFactory::darkElfVillage(tribeSet.darkElven.get());
      c.numCreatures = random.get(14, 16);
      c.location = getVillageLocation();
      c.tribe = tribeSet.darkElven.get();
      c.buildingId = BuildingId::DUNGEON;
      c.outsideFeatures = SquareFactory::dungeonOutside();
      c.furniture = SquareFactory::roomFurniture(tribeSet.pest.get());),
      CollectiveConfig::withImmigrants(0.002, 15, {
          CONSTRUCT(ImmigrantInfo,
            c.id = CreatureId::DARK_ELF_WARRIOR;
            c.frequency = 3;
            c.traits = LIST(MinionTrait::FIGHTER);),
          }).allowRecruiting(4), {}, LevelInfo{ExtraLevelId::GNOMISH_MINES, gnomeKey}),
    noVillain(CONSTRUCT(SettlementInfo,
      c.type = SettlementType::SMALL_MINETOWN;
      c.creatures = CreatureFactory::darkElfEntrance(tribeSet.darkElven.get());
      c.numCreatures = random.get(3, 7);
      c.location = new Location(true);
      c.tribe = tribeSet.darkElven.get();
      c.buildingId = BuildingId::DUNGEON;
      c.downStairs = {gnomeKey};
      c.outsideFeatures = SquareFactory::dungeonOutside();
      c.furniture = SquareFactory::roomFurniture(tribeSet.pest.get());),
      CollectiveConfig::noImmigrants(), {})
  };
}

static vector<EnemyInfo> getTower(RandomGen& random, TribeSet& tribeSet) {
  StairKey towerKey = StairKey::getNew();
  return {
    noVillain(CONSTRUCT(SettlementInfo,
      c.type = SettlementType::TOWER;
      c.location = new Location();
      c.tribe = tribeSet.human.get();
      c.upStairs = {towerKey};
      c.buildingId = BuildingId::BRICK;),
      CollectiveConfig::noImmigrants(), {}),
    mainVillain(CONSTRUCT(SettlementInfo,
      c.type = SettlementType::TOWER;
      c.creatures = CreatureFactory::singleType(tribeSet.human.get(), CreatureId::ELEMENTALIST);
      c.numCreatures = 1;
      c.location = new Location(true);
      c.tribe = tribeSet.human.get();
      c.buildingId = BuildingId::BRICK;
      c.furniture = SquareFactory::roomFurniture(tribeSet.pest.get());),
      CollectiveConfig::noImmigrants().setLeaderAsFighter(),
      {CONSTRUCT(VillainInfo,
        c.minPopulation = 0;
        c.minTeamSize = 1;
        c.triggers = LIST({AttackTriggerId::ROOM_BUILT, SquareId::THRONE},
            {AttackTriggerId::ROOM_BUILT, SquareId::IMPALED_HEAD});
        c.behaviour = VillageBehaviour(VillageBehaviourId::CAMP_AND_SPAWN,
          CreatureFactory::elementals(tribeSet.human.get()));
        c.ransom = make_pair(0.5, random.get(200, 400));)}, LevelInfo{ExtraLevelId::TOWER, towerKey})
  };
}

static vector<EnemyInfo> getOrcTown(RandomGen& random, TribeSet& tribeSet) {
  return {
    lesserVillain(CONSTRUCT(SettlementInfo,
      c.type = SettlementType::VILLAGE;
      c.creatures = CreatureFactory::orcTown(tribeSet.greenskins.get());
      c.numCreatures = random.get(12, 16);
      c.location = getVillageLocation();
      c.tribe = tribeSet.greenskins.get();
      c.buildingId = BuildingId::BRICK;
      c.furniture = SquareFactory::roomFurniture(tribeSet.pest.get());
      c.outsideFeatures = SquareFactory::villageOutside();),
      CollectiveConfig::withImmigrants(0.003, 16, {
          CONSTRUCT(ImmigrantInfo,
            c.id = CreatureId::ORC;
            c.frequency = 3;
            c.traits = LIST(MinionTrait::FIGHTER);),          
          CONSTRUCT(ImmigrantInfo,
            c.id = CreatureId::OGRE;
            c.frequency = 1;
            c.traits = LIST(MinionTrait::FIGHTER);),
          }).allowRecruiting(9), {})
  };
}

static vector<EnemyInfo> getFriendlyCave(RandomGen& random, TribeSet& tribeSet, CreatureId creature) {
  return {
    lesserVillain(CONSTRUCT(SettlementInfo,
      c.type = SettlementType::CAVE;
      c.creatures = CreatureFactory::singleType(tribeSet.greenskins.get(), creature);
      c.numCreatures = random.get(4, 8);
      c.location = new Location(true);
      c.tribe = tribeSet.greenskins.get();
      c.buildingId = BuildingId::DUNGEON;
      c.closeToPlayer = true;
      c.furniture = SquareFactory::roomFurniture(tribeSet.pest.get());
      c.outsideFeatures = SquareFactory::villageOutside();),
      CollectiveConfig::withImmigrants(0.003, 10, {
          CONSTRUCT(ImmigrantInfo,
            c.id = creature;
            c.frequency = 3;
            c.ignoreSpawnType = true;
            c.traits = LIST(MinionTrait::FIGHTER);), 
          }).allowRecruiting(4), {})
  };
}

static vector<EnemyInfo> getWarriorCastle(RandomGen& random, TribeSet& tribeSet) {
  return {
    mainVillain(CONSTRUCT(SettlementInfo,
      c.type = SettlementType::CASTLE2;
      c.creatures = CreatureFactory::vikingTown(tribeSet.human.get());
      c.numCreatures = random.get(12, 16);
      c.location = getVillageLocation();
      c.tribe = tribeSet.human.get();
      c.buildingId = BuildingId::WOOD_CASTLE;
      c.stockpiles = LIST({StockpileInfo::GOLD, 800});
      c.guardId = CreatureId::WARRIOR;
      c.elderLoot = ItemType(ItemId::TECH_BOOK, TechId::BEAST_MUT);
      c.furniture = SquareFactory::roomFurniture(tribeSet.pest.get());
      c.outsideFeatures = SquareFactory::castleOutside();),
      CollectiveConfig::withImmigrants(0.003, 16, {
          CONSTRUCT(ImmigrantInfo,
            c.id = CreatureId::WARRIOR;
            c.frequency = 1;
            c.traits = LIST(MinionTrait::FIGHTER);),
          }).setGhostSpawns(0.1, 6),
      {CONSTRUCT(VillainInfo,
          c.minPopulation = 6;
          c.minTeamSize = 5;
          c.triggers = LIST({AttackTriggerId::ROOM_BUILT, SquareId::THRONE}, {AttackTriggerId::SELF_VICTIMS},
            AttackTriggerId::STOLEN_ITEMS, {AttackTriggerId::ROOM_BUILT, SquareId::IMPALED_HEAD});
          c.behaviour = VillageBehaviour(VillageBehaviourId::KILL_LEADER);
          c.ransom = make_pair(0.8, random.get(500, 700));)})
  };
}

static vector<EnemyInfo> getHumanVillage(RandomGen& random, TribeSet& tribeSet, const string& boardText) {
  return {
    mainVillain(CONSTRUCT(SettlementInfo,
      c.type = SettlementType::VILLAGE;
      c.creatures = CreatureFactory::humanVillage(tribeSet.human.get());
      c.numCreatures = random.get(12, 20);
      c.location = getVillageLocation();
      c.tribe = tribeSet.human.get();
      c.buildingId = BuildingId::WOOD;
      c.shopFactory = ItemFactory::armory();
      c.furniture = SquareFactory::roomFurniture(tribeSet.pest.get());
      c.outsideFeatures = SquareFactory::villageOutside(boardText);
      ), CollectiveConfig::noImmigrants().setGhostSpawns(0.1, 4), {})
  };
}

static vector<EnemyInfo> getLizardVillage(RandomGen& random, TribeSet& tribeSet) {
  return {
    mainVillain(CONSTRUCT(SettlementInfo,
      c.type = SettlementType::VILLAGE;
      c.creatures = CreatureFactory::lizardTown(tribeSet.lizard.get());
      c.numCreatures = random.get(8, 14);
      c.location = getVillageLocation();
      c.tribe = tribeSet.lizard.get();
      c.buildingId = BuildingId::MUD;
      c.elderLoot = ItemType(ItemId::TECH_BOOK, TechId::HUMANOID_MUT);
      c.shopFactory = ItemFactory::mushrooms();
      c.furniture = SquareFactory::roomFurniture(tribeSet.pest.get());
      c.outsideFeatures = SquareFactory::villageOutside();),
      CollectiveConfig::withImmigrants(0.007, 15, {
          CONSTRUCT(ImmigrantInfo,
            c.id = CreatureId::LIZARDMAN;
            c.frequency = 1;
            c.traits = LIST(MinionTrait::FIGHTER);),          
          }).setGhostSpawns(0.1, 4),
      {CONSTRUCT(VillainInfo,
          c.minPopulation = 4;
          c.minTeamSize = 4;
          c.triggers = LIST({AttackTriggerId::POWER}, {AttackTriggerId::SELF_VICTIMS},
            AttackTriggerId::STOLEN_ITEMS, {AttackTriggerId::ROOM_BUILT, SquareId::IMPALED_HEAD});
          c.behaviour = VillageBehaviour(VillageBehaviourId::KILL_LEADER);)})
  };
}

static vector<EnemyInfo> getElvenVillage(RandomGen& random, TribeSet& tribeSet) {
  return {
    mainVillain(CONSTRUCT(SettlementInfo,
      c.type = SettlementType::VILLAGE2;
      c.creatures = CreatureFactory::elvenVillage(tribeSet.elven.get());
      c.numCreatures = random.get(11, 18);
      c.location = getVillageLocation();
      c.tribe = tribeSet.elven.get();
      c.stockpiles = LIST({StockpileInfo::GOLD, 800});
      c.buildingId = BuildingId::WOOD;
      c.elderLoot = ItemType(ItemId::TECH_BOOK, TechId::SPELLS_MAS);
      c.furniture = SquareFactory::roomFurniture(tribeSet.pest.get());),
      CollectiveConfig::withImmigrants(0.002, 18, {
          CONSTRUCT(ImmigrantInfo,
            c.id = CreatureId::ELF_ARCHER;
            c.frequency = 1;
            c.traits = LIST(MinionTrait::FIGHTER);),          
          }).setGhostSpawns(0.1, 4),
      {CONSTRUCT(VillainInfo,
          c.minPopulation = 4;
          c.minTeamSize = 4;
          c.triggers = LIST(AttackTriggerId::STOLEN_ITEMS);
          c.behaviour = VillageBehaviour(VillageBehaviourId::KILL_LEADER);)})
  };
}

static vector<EnemyInfo> getAntNest(RandomGen& random, TribeSet& tribeSet) {
  return {
    lesserVillain(CONSTRUCT(SettlementInfo,
      c.type = SettlementType::ANT_NEST;
      c.creatures = CreatureFactory::antNest(tribeSet.ants.get());
      c.numCreatures = random.get(9, 14);
      c.location = new Location(true);
      c.tribe = tribeSet.ants.get();
      c.buildingId = BuildingId::DUNGEON;),
      CollectiveConfig::withImmigrants(0.002, 15, {
          CONSTRUCT(ImmigrantInfo,
            c.id = CreatureId::ANT_WORKER;
            c.frequency = 1;
            c.traits = LIST(MinionTrait::FIGHTER);),          
          CONSTRUCT(ImmigrantInfo,
            c.id = CreatureId::ANT_SOLDIER;
            c.frequency = 1;
            c.traits = LIST(MinionTrait::FIGHTER);)}),
      {CONSTRUCT(VillainInfo,
          c.minPopulation = 1;
          c.minTeamSize = 4;
          c.triggers = LIST(AttackTriggerId::ENTRY);
          c.behaviour = VillageBehaviour(VillageBehaviourId::KILL_LEADER);)})
  };
}

static vector<EnemyInfo> getDwarfTown(RandomGen& random, TribeSet& tribeSet) {
  return {
    mainVillain(CONSTRUCT(SettlementInfo,
      c.type = SettlementType::MINETOWN;
      c.creatures = CreatureFactory::dwarfTown(tribeSet.dwarven.get());
      c.numCreatures = random.get(9, 14);
      c.location = getVillageLocation(true);
      c.tribe = tribeSet.dwarven.get();
      c.buildingId = BuildingId::DUNGEON;
      c.stockpiles = LIST({StockpileInfo::GOLD, 1000}, {StockpileInfo::MINERALS, 600});
      c.shopFactory = ItemFactory::dwarfShop();
      c.outsideFeatures = SquareFactory::dungeonOutside();
      c.furniture = SquareFactory::roomFurniture(tribeSet.pest.get());),
      CollectiveConfig::withImmigrants(0.002, 15, {
          CONSTRUCT(ImmigrantInfo,
            c.id = CreatureId::DWARF;
            c.frequency = 1;
            c.traits = LIST(MinionTrait::FIGHTER);),          
          }).setGhostSpawns(0.1, 4),
      {CONSTRUCT(VillainInfo,
          c.minPopulation = 3;
          c.minTeamSize = 4;
          c.triggers = LIST({AttackTriggerId::ROOM_BUILT, SquareId::THRONE}, {AttackTriggerId::SELF_VICTIMS},
            AttackTriggerId::STOLEN_ITEMS, {AttackTriggerId::ROOM_BUILT, SquareId::IMPALED_HEAD});
          c.behaviour = VillageBehaviour(VillageBehaviourId::KILL_MEMBERS, 3);
          c.ransom = make_pair(0.8, random.get(1200, 1600));)})
  };
}

static vector<EnemyInfo> getHumanCastle(RandomGen& random, TribeSet& tribeSet) {
  optional<StairKey> stairKey;
  if (random.roll(4))
    stairKey = StairKey::getNew();
  vector<EnemyInfo> ret {
    mainVillain(CONSTRUCT(SettlementInfo,
      c.type = SettlementType::CASTLE;
      c.creatures = CreatureFactory::humanCastle(tribeSet.human.get());
      c.numCreatures = random.get(20, 26);
      c.location = getVillageLocation();
      c.tribe = tribeSet.human.get();
      c.stockpiles = LIST({StockpileInfo::GOLD, 700});
      c.buildingId = BuildingId::BRICK;
      c.guardId = CreatureId::CASTLE_GUARD;
      if (stairKey)
        c.downStairs = { *stairKey };
      c.shopFactory = ItemFactory::villageShop();
      c.furniture = SquareFactory::castleFurniture(tribeSet.pest.get());
      c.outsideFeatures = SquareFactory::castleOutside();),
      CollectiveConfig::withImmigrants(0.003, 26, {
          CONSTRUCT(ImmigrantInfo,
            c.id = CreatureId::KNIGHT;
            c.frequency = 1;
            c.traits = LIST(MinionTrait::FIGHTER);),
          CONSTRUCT(ImmigrantInfo,
            c.id = CreatureId::ARCHER;
            c.frequency = 1;
            c.traits = LIST(MinionTrait::FIGHTER);),          
          }).setGhostSpawns(0.1, 6),
      {CONSTRUCT(VillainInfo,
          c.minPopulation = 12;
          c.minTeamSize = 10;
          c.triggers = LIST({AttackTriggerId::ROOM_BUILT, SquareId::THRONE}, {AttackTriggerId::SELF_VICTIMS},
            AttackTriggerId::STOLEN_ITEMS, {AttackTriggerId::ROOM_BUILT, SquareId::IMPALED_HEAD});
          c.behaviour = VillageBehaviour(VillageBehaviourId::KILL_LEADER);
          c.ransom = make_pair(0.9, random.get(1400, 2000));)})};
  if (stairKey)
    ret.push_back(
        lesserVillain(CONSTRUCT(SettlementInfo,
            c.creatures = CreatureFactory::singleType(tribeSet.monster.get(), CreatureId::MINOTAUR);
            c.numCreatures = 1;
            c.location = new Location("maze");
            c.tribe = tribeSet.monster.get();
            c.furniture = SquareFactory::roomFurniture(tribeSet.pest.get());
            if (stairKey)
              c.upStairs = { *stairKey };
            c.buildingId = BuildingId::BRICK;), CollectiveConfig::noImmigrants(), {},
          LevelInfo{ExtraLevelId::MAZE, *stairKey}));
  return ret;
}

static vector<EnemyInfo> getWitchHouse(RandomGen& random, TribeSet& tribeSet) {
  return {
    lesserVillain(CONSTRUCT(SettlementInfo,
      c.type = SettlementType::WITCH_HOUSE;
      c.creatures = CreatureFactory::singleType(tribeSet.monster.get(), CreatureId::WITCH);
      c.numCreatures = 1;
      c.location = new Location();
      c.tribe = tribeSet.monster.get();
      c.buildingId = BuildingId::WOOD;
      c.elderLoot = ItemType(ItemId::TECH_BOOK, TechId::ALCHEMY_ADV);
      c.furniture = SquareFactory::single(SquareId::CAULDRON);), CollectiveConfig::noImmigrants(), {})
  };
}

static vector<EnemyInfo> getEntTown(RandomGen& random, TribeSet& tribeSet) {
  return {
    lesserVillain(CONSTRUCT(SettlementInfo,
      c.type = SettlementType::FOREST;
      c.creatures = CreatureFactory::singleType(tribeSet.monster.get(), CreatureId::ENT);
      c.numCreatures = random.get(7, 13);
      c.location = new Location();
      c.tribe = tribeSet.monster.get();
      c.buildingId = BuildingId::WOOD;),
      CollectiveConfig::withImmigrants(0.003, 15, {
          CONSTRUCT(ImmigrantInfo,
            c.id = CreatureId::ENT;
            c.frequency = 1;
            c.traits = LIST(MinionTrait::FIGHTER);)}), {})
  };
}

static vector<EnemyInfo> getDriadTown(RandomGen& random, TribeSet& tribeSet) {
  return {
    lesserVillain(CONSTRUCT(SettlementInfo,
      c.type = SettlementType::FOREST;
      c.creatures = CreatureFactory::singleType(tribeSet.monster.get(), CreatureId::DRIAD);
      c.numCreatures = random.get(7, 13);
      c.location = new Location();
      c.tribe = tribeSet.monster.get();
      c.buildingId = BuildingId::WOOD;),
      CollectiveConfig::withImmigrants(0.003, 15, {
          CONSTRUCT(ImmigrantInfo,
            c.id = CreatureId::DRIAD;
            c.frequency = 1;
            c.traits = LIST(MinionTrait::FIGHTER);)}), {})
  };
}

static vector<EnemyInfo> getCemetery(RandomGen& random, TribeSet& tribeSet) {
  StairKey cryptKey = StairKey::getNew();
  return {
    lesserVillain(CONSTRUCT(SettlementInfo,
          c.type = SettlementType::CEMETERY;
          c.creatures = CreatureFactory::singleType(tribeSet.monster.get(), CreatureId::ZOMBIE);
          c.numCreatures = random.get(8, 12);
          c.location = new Location("cemetery");
          c.tribe = tribeSet.monster.get();
          c.furniture = SquareFactory::cryptCoffins(tribeSet.keeper.get());
          c.upStairs = { cryptKey };
          c.buildingId = BuildingId::BRICK;), CollectiveConfig::noImmigrants(), {},
       LevelInfo{ExtraLevelId::CRYPT, cryptKey}),
    noVillain(CONSTRUCT(SettlementInfo,
          c.type = SettlementType::CEMETERY;
          c.creatures = CreatureFactory::singleType(tribeSet.monster.get(), CreatureId::ZOMBIE);
          c.numCreatures = 1;
          c.location = new Location("cemetery");
          c.tribe = tribeSet.monster.get();
          c.downStairs = { cryptKey };
          c.buildingId = BuildingId::BRICK;), CollectiveConfig::noImmigrants(), {})
  };
}

static vector<EnemyInfo> getBanditCave(RandomGen& random, TribeSet& tribeSet) {
  return {
    lesserVillain(CONSTRUCT(SettlementInfo,
      c.type = SettlementType::CAVE;
      c.creatures = CreatureFactory::singleType(tribeSet.bandit.get(), CreatureId::BANDIT);
      c.numCreatures = random.get(4, 9);
      c.location = new Location();
      c.tribe = tribeSet.bandit.get();
      c.buildingId = BuildingId::DUNGEON;),
      CollectiveConfig::withImmigrants(0.001, 10, {
          CONSTRUCT(ImmigrantInfo,
            c.id = CreatureId::BANDIT;
            c.frequency = 1;
            c.traits = LIST(MinionTrait::FIGHTER);),
          }),
      {CONSTRUCT(VillainInfo,
          c.minPopulation = 0;
          c.minTeamSize = 3;
          c.triggers = LIST({AttackTriggerId::GOLD, 500});
          c.behaviour = VillageBehaviour(VillageBehaviourId::STEAL_GOLD);
          c.ransom = make_pair(0.5, random.get(200, 400));)})
  };
}

static vector<EnemyInfo> getShelob(RandomGen& random, TribeSet& tribeSet) {
  return {
    lesserVillain(CONSTRUCT(SettlementInfo,
      c.type = SettlementType::CAVE;
      c.creatures = CreatureFactory::singleType(tribeSet.killEveryone.get(), CreatureId::SHELOB);
      c.numCreatures = 1;
      c.buildingId = BuildingId::DUNGEON;
      c.location = new Location(true);
      c.tribe = tribeSet.killEveryone.get();), CollectiveConfig::noImmigrants().setLeaderAsFighter(), {})
  };
}

static vector<EnemyInfo> getGreenDragon(RandomGen& random, TribeSet& tribeSet) {
  return {
    mainVillain(CONSTRUCT(SettlementInfo,
      c.type = SettlementType::CAVE;
      c.creatures = CreatureFactory::singleType(tribeSet.killEveryone.get(), CreatureId::GREEN_DRAGON);
      c.numCreatures = 1;
      c.location = new Location(true);
      c.tribe = tribeSet.killEveryone.get();
      c.buildingId = BuildingId::DUNGEON;
      c.shopFactory = ItemFactory::dragonCave();), CollectiveConfig::noImmigrants().setLeaderAsFighter()
          .setGhostSpawns(0.03, 1),
    { CONSTRUCT(VillainInfo,
            c.minPopulation = 0;
            c.minTeamSize = 1;
            c.triggers = LIST({AttackTriggerId::ENEMY_POPULATION, 20}, AttackTriggerId::STOLEN_ITEMS);
            c.behaviour = VillageBehaviour(VillageBehaviourId::KILL_MEMBERS, 7);
            c.welcomeMessage = VillageControl::DRAGON_WELCOME;)})
  };
}

static vector<EnemyInfo> getHydra(RandomGen& random, TribeSet& tribeSet) {
  return {
    lesserVillain(CONSTRUCT(SettlementInfo,
      c.type = SettlementType::SWAMP;
      c.creatures = CreatureFactory::singleType(tribeSet.killEveryone.get(), CreatureId::HYDRA);
      c.numCreatures = 1;
      c.location = new Location(true);
      c.tribe = tribeSet.killEveryone.get();), CollectiveConfig::noImmigrants().setLeaderAsFighter()
          .setGhostSpawns(0.03, 1),
    { /*CONSTRUCT(VillainInfo,
            c.minPopulation = 0;
            c.minTeamSize = 1;
            c.triggers = LIST({AttackTriggerId::ENEMY_POPULATION, 22});
            c.behaviour = VillageBehaviour(VillageBehaviourId::KILL_MEMBERS, 7);)*/})
  };
}

static vector<EnemyInfo> getRedDragon(RandomGen& random, TribeSet& tribeSet) {
  return {
    mainVillain(CONSTRUCT(SettlementInfo,
      c.type = SettlementType::CAVE;
      c.creatures = CreatureFactory::singleType(tribeSet.killEveryone.get(), CreatureId::RED_DRAGON);
      c.numCreatures = 1;
      c.location = new Location(true);
      c.tribe = tribeSet.killEveryone.get();
      c.buildingId = BuildingId::DUNGEON;
      c.shopFactory = ItemFactory::dragonCave();), CollectiveConfig::noImmigrants().setLeaderAsFighter()
          .setGhostSpawns(0.03, 1),
    { CONSTRUCT(VillainInfo,
            c.minPopulation = 0;
            c.minTeamSize = 1;
            c.triggers = LIST({AttackTriggerId::ENEMY_POPULATION, 25}, AttackTriggerId::STOLEN_ITEMS);
            c.behaviour = VillageBehaviour(VillageBehaviourId::KILL_MEMBERS, 12);
            c.welcomeMessage = VillageControl::DRAGON_WELCOME;)})
  };
}

static vector<EnemyInfo> getCyclops(RandomGen& random, TribeSet& tribeSet) {
  return {
    lesserVillain(CONSTRUCT(SettlementInfo,
      c.type = SettlementType::CAVE;
      c.creatures = CreatureFactory::singleType(tribeSet.killEveryone.get(), CreatureId::CYCLOPS);
      c.numCreatures = 1;
      c.location = new Location(true);
      c.tribe = tribeSet.killEveryone.get();
      c.buildingId = BuildingId::DUNGEON;
      c.shopFactory = ItemFactory::mushrooms(true);), CollectiveConfig::noImmigrants().setLeaderAsFighter()
          .setGhostSpawns(0.03, 1),
    { CONSTRUCT(VillainInfo,
            c.minPopulation = 0;
            c.minTeamSize = 1;
            c.triggers = LIST({AttackTriggerId::ENEMY_POPULATION, 13});
            c.behaviour = VillageBehaviour(VillageBehaviourId::KILL_MEMBERS, 4);)})
  };
}

static vector<EnemyInfo> getCottage(RandomGen& random, TribeSet& tribeSet) {
  return {
    noVillain(CONSTRUCT(SettlementInfo,
      c.type = SettlementType::COTTAGE;
      c.creatures = CreatureFactory::humanPeaceful(tribeSet.human.get());
      c.numCreatures = random.get(3, 7);
      c.location = new Location();
      c.tribe = tribeSet.human.get();
      c.buildingId = BuildingId::WOOD;
      c.furniture = SquareFactory::roomFurniture(tribeSet.pest.get());),
      CollectiveConfig::noImmigrants().setGuardian({CreatureId::WITCHMAN, 0.001, 1, 2}), {})
  };
}

static vector<EnemyInfo> getKoboldCave(RandomGen& random, TribeSet& tribeSet) {
  return {
    noVillain(CONSTRUCT(SettlementInfo,
      c.type = SettlementType::SMALL_MINETOWN;
      c.creatures = CreatureFactory::koboldVillage(tribeSet.dwarven.get());
      c.numCreatures = random.get(3, 7);
      c.location = new Location(true);
      c.tribe = tribeSet.dwarven.get();
      c.buildingId = BuildingId::DUNGEON;
      c.stockpiles = LIST({StockpileInfo::MINERALS, 300});
 /*     c.outsideFeatures = SquareFactory::dungeonOutside();
      c.furniture = SquareFactory::roomFurniture(tribeSet.pest.get());*/),
      CollectiveConfig::noImmigrants(), {})
  };
}

/*static vector<EnemyInfo> getIslandVault(RandomGen& random, TribeSet& tribeSet) {
  return {
    lesserVillain(CONSTRUCT(SettlementInfo,
      c.type = SettlementType::ISLAND_VAULT;
      c.location = new Location();
      c.buildingId = BuildingId::DUNGEON;
      c.stockpiles = LIST({StockpileInfo::GOLD, 800});), CollectiveConfig::noImmigrants(), {})
  };
}*/

static vector<EnemyInfo> getSokobanEntry(RandomGen& random, TribeSet& tribeSet) {
  StairKey link = StairKey::getNew();
  return {
    noVillain(CONSTRUCT(SettlementInfo,
      c.type = SettlementType::ISLAND_VAULT;
      c.location = new Location(true);
      c.buildingId = BuildingId::DUNGEON;
      c.upStairs = {link};), CollectiveConfig::noImmigrants(), {}),
    noVillain(CONSTRUCT(SettlementInfo,
      c.type = SettlementType::ISLAND_VAULT;
      c.neutralCreatures = make_pair(
          CreatureFactory::singleType(tribeSet.killEveryone.get(), CreatureId::SOKOBAN_BOULDER), 0);
      c.creatures = CreatureFactory::singleType(tribeSet.keeper.get(), CreatureId::SPECIAL_HL);
      c.numCreatures = 1;
      c.tribe = tribeSet.keeper.get();
      c.location = new Location();
      c.buildingId = BuildingId::DUNGEON;
      c.downStairs = {link};), CollectiveConfig::noImmigrants(), {}, LevelInfo{ExtraLevelId::SOKOBAN, link}),
  };
}

static vector<EnemyInfo> getEnemyInfo(RandomGen& random, TribeSet& tribeSet, const string& boardText) {
  vector<EnemyInfo> ret;
  for (int i : Range(random.get(5, 9)))
    append(ret, getCottage(random, tribeSet));
  for (int i : Range(random.get(1, 3))) {
    append(ret, getKoboldCave(random, tribeSet));
  }
  for (int i : Range(random.get(1, 3)))
    append(ret, getBanditCave(random, tribeSet));
  append(ret, getSokobanEntry(random, tribeSet));
  append(ret, random.choose({
        getGnomishMines(random, tribeSet),
        getDarkElvenMines(random, tribeSet)}));
  append(ret, getVaults(random, tribeSet));
  if (random.roll(4))
    append(ret, getAntNest(random, tribeSet));
  append(ret, getHumanCastle(random, tribeSet));
  append(ret, random.choose({
        getFriendlyCave(random, tribeSet, CreatureId::ORC),
        getFriendlyCave(random, tribeSet, CreatureId::OGRE),
        getFriendlyCave(random, tribeSet, CreatureId::HARPY)}));
  for (auto& infos : random.chooseN(3, {
        getTower(random, tribeSet),
        getWarriorCastle(random, tribeSet),
        getLizardVillage(random, tribeSet),
        getElvenVillage(random, tribeSet),
        getDwarfTown(random, tribeSet),
        getHumanVillage(random, tribeSet, boardText)}))
    append(ret, infos);
  for (auto& infos : random.chooseN(3, {
        getGreenDragon(random, tribeSet),
        getShelob(random, tribeSet),
        getHydra(random, tribeSet),
        getRedDragon(random, tribeSet),
        getCyclops(random, tribeSet),
        getDriadTown(random, tribeSet),
        getEntTown(random, tribeSet)}))
    append(ret, infos);
  for (auto& infos : random.chooseN(1, {
        getWitchHouse(random, tribeSet),
        getCemetery(random, tribeSet)}))
    append(ret, infos);
  return ret;
}

int ModelBuilder::getPigstyPopulationIncrease() {
  return 4;
}

int ModelBuilder::getStatuePopulationIncrease() {
  return 1;
}

int ModelBuilder::getThronePopulationIncrease() {
  return 10;
}

static CollectiveConfig getKeeperConfig(bool fastImmigration) {
  return CollectiveConfig::keeper(
      fastImmigration ? 0.1 : 0.011,
      500,
      2,
      10,
      {
      CONSTRUCT(PopulationIncrease,
        c.type = SquareApplyType::PIGSTY;
        c.increasePerSquare = 0.25;
        c.maxIncrease = ModelBuilder::getPigstyPopulationIncrease();),
      CONSTRUCT(PopulationIncrease,
        c.type = SquareApplyType::STATUE;
        c.increasePerSquare = ModelBuilder::getStatuePopulationIncrease();
        c.maxIncrease = 1000;),
      CONSTRUCT(PopulationIncrease,
        c.type = SquareApplyType::THRONE;
        c.increasePerSquare = ModelBuilder::getThronePopulationIncrease();
        c.maxIncrease = c.increasePerSquare;),
      },
      {
      CONSTRUCT(ImmigrantInfo,
        c.id = CreatureId::GOBLIN;
        c.frequency = 1;
        c.attractions = LIST(
          {{AttractionId::SQUARE, SquareId::WORKSHOP}, 1.0, 12.0},
          {{AttractionId::SQUARE, SquareId::JEWELER}, 1.0, 9.0},
          {{AttractionId::SQUARE, SquareId::FORGE}, 1.0, 9.0},
          );
        c.traits = LIST(MinionTrait::FIGHTER, MinionTrait::NO_EQUIPMENT);
        c.salary = 10;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::ORC;
          c.frequency = 0.7;
          c.attractions = LIST(
            {{AttractionId::SQUARE, SquareId::TRAINING_ROOM}, 1.0, 12.0},
            );
          c.traits = {MinionTrait::FIGHTER};
          c.salary = 20;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::ORC_SHAMAN;
          c.frequency = 0.10;
          c.attractions = LIST(
            {{AttractionId::SQUARE, SquareId::LIBRARY}, 1.0, 16.0},
            {{AttractionId::SQUARE, SquareId::LABORATORY}, 1.0, 9.0},
            );
          c.traits = {MinionTrait::FIGHTER};
          c.salary = 20;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::OGRE;
          c.frequency = 0.3;
          c.attractions = LIST(
            {{AttractionId::SQUARE, SquareId::TRAINING_ROOM}, 3.0, 16.0}
            );
          c.traits = {MinionTrait::FIGHTER};
          c.salary = 40;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::HARPY;
          c.frequency = 0.3;
          c.attractions = LIST(
            {{AttractionId::SQUARE, SquareId::TRAINING_ROOM}, 3.0, 16.0},
            {{AttractionId::ITEM_INDEX, ItemIndex::RANGED_WEAPON}, 1.0, 3.0, true}
            );
          c.traits = {MinionTrait::FIGHTER};
          c.salary = 40;),
/*      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::SPECIAL_HUMANOID;
          c.frequency = 0.1;
          c.attractions = LIST(
            {{AttractionId::SQUARE, SquareId::TRAINING_ROOM}, 3.0, 16.0},
            );
          c.traits = {MinionTrait::FIGHTER};
          c.spawnAtDorm = true;
          c.techId = TechId::HUMANOID_MUT;
          c.salary = 40;),*/
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::ZOMBIE;
          c.frequency = 0.5;
          c.traits = {MinionTrait::FIGHTER};
          c.salary = 10;
          c.spawnAtDorm = true;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::VAMPIRE;
          c.frequency = 0.2;
          c.traits = {MinionTrait::FIGHTER};
          c.salary = 40;
          c.spawnAtDorm = true;
          c.attractions = LIST(
            {{AttractionId::SQUARE, SquareId::TRAINING_ROOM}, 2.0, 12.0}
            );),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::LOST_SOUL;
          c.frequency = 0.3;
          c.traits = {MinionTrait::FIGHTER};
          c.salary = 0;
          c.attractions = LIST(
            {{AttractionId::SQUARE, SquareId::RITUAL_ROOM}, 1.0, 9.0}
            );
          c.spawnAtDorm = true;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::SUCCUBUS;
          c.frequency = 0.3;
          c.traits = LIST(MinionTrait::FIGHTER, MinionTrait::NO_EQUIPMENT);
          c.salary = 0;
          c.attractions = LIST(
            {{AttractionId::SQUARE, SquareId::RITUAL_ROOM}, 2.0, 12.0}
            );
          c.spawnAtDorm = true;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::DOPPLEGANGER;
          c.frequency = 0.2;
          c.traits = {MinionTrait::FIGHTER};
          c.salary = 0;
          c.attractions = LIST(
            {{AttractionId::SQUARE, SquareId::RITUAL_ROOM}, 4.0, 12.0}
            );
          c.spawnAtDorm = true;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::RAVEN;
          c.frequency = 1.0;
          c.traits = LIST(MinionTrait::FIGHTER, MinionTrait::NO_RETURNING);
          c.limit = SunlightState::DAY;
          c.salary = 0;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::BAT;
          c.frequency = 1.0;
          c.traits = LIST(MinionTrait::FIGHTER, MinionTrait::NO_RETURNING);
          c.limit = SunlightState::NIGHT;
          c.salary = 0;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::WOLF;
          c.frequency = 0.15;
          c.traits = LIST(MinionTrait::FIGHTER, MinionTrait::NO_RETURNING);
          c.groupSize = Range(3, 9);
          c.autoTeam = true;
          c.salary = 0;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::CAVE_BEAR;
          c.frequency = 0.1;
          c.traits = LIST(MinionTrait::FIGHTER, MinionTrait::NO_RETURNING);
          c.salary = 0;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::WEREWOLF;
          c.frequency = 0.1;
          c.traits = LIST(MinionTrait::FIGHTER, MinionTrait::NO_RETURNING);
          c.attractions = LIST(
            {{AttractionId::SQUARE, SquareId::TRAINING_ROOM}, 4.0, 12.0}
            );
          c.salary = 0;),
      /*CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::SPECIAL_MONSTER_KEEPER;
          c.frequency = 0.1;
          c.traits = LIST(MinionTrait::FIGHTER, MinionTrait::NO_RETURNING);
          c.spawnAtDorm = true;
          c.techId = TechId::BEAST_MUT;
          c.salary = 0;)*/});
}

static map<CollectiveResourceId, int> getKeeperCredit(bool resourceBonus) {
  if (resourceBonus) {
    map<CollectiveResourceId, int> credit;
    for (auto elem : ENUM_ALL(CollectiveResourceId))
      credit[elem] = 10000;
    return credit;
  } else
    return {{CollectiveResourceId::MANA, 200}};
 
}

PModel ModelBuilder::tryQuickModel(ProgressMeter* meter, RandomGen& random,
    Options* options, View* view) {
  Model* m = new Model(view, "quick", TribeSet());
  m->setOptions(options);
  string keeperName = options->getStringValue(OptionId::KEEPER_NAME);
  Level* top = m->buildLevel(
      LevelBuilder(meter, random, 28, 14, "Quick", false),
      LevelMaker::quickLevel(random));
  m->calculateStairNavigation();
  m->collectives.push_back(CollectiveBuilder(
        getKeeperConfig(options->getBoolValue(OptionId::FAST_IMMIGRATION)), m->tribeSet->keeper.get())
      .setLevel(top)
      .setCredit(getKeeperCredit(true))
      .build());
 
  m->playerCollective = m->collectives.back().get();
  m->playerControl = new PlayerControl(m->playerCollective, m, top);
  m->playerCollective->setControl(PCollectiveControl(m->playerControl));
  PCreature c = CreatureFactory::fromId(CreatureId::KEEPER, m->tribeSet->keeper.get(),
      MonsterAIFactory::collective(m->playerCollective));
  if (!keeperName.empty())
    c->setFirstName(keeperName);
  m->gameIdentifier = *c->getFirstName() + "_" + m->worldName;
  m->gameDisplayName = *c->getFirstName() + " of " + m->worldName;
  Creature* ref = c.get();
  top->landCreature(StairKey::keeperSpawn(), c.get());
  m->addCreature(std::move(c));
  m->playerControl->addKeeper(ref);
  vector<CreatureId> ids {
    CreatureId::IMP,
    CreatureId::IMP,
    CreatureId::IMP,
    CreatureId::IMP,
    CreatureId::IMP,
  };
  for (auto elem : ids) {
    PCreature c = CreatureFactory::fromId(elem, m->tribeSet->monster.get(),
        MonsterAIFactory::monster());
    top->landCreature(StairKey::keeperSpawn(), c.get());
//    m->playerCollective->addCreature(c.get(), {MinionTrait::FIGHTER});
    m->addCreature(std::move(c));
  }
  for (int i : Range(4)) {
    PCreature c = CreatureFactory::fromId(CreatureId::IMP, m->tribeSet->keeper.get(),
        MonsterAIFactory::collective(m->playerCollective));
    top->landCreature(StairKey::keeperSpawn(), c.get());
    m->playerControl->addImp(c.get());
    m->addCreature(std::move(c));
  }
  return PModel(m);
}

PModel ModelBuilder::quickModel(ProgressMeter* meter, RandomGen& random,
    Options* options, View* view) {
  for (int i : Range(5000)) {
    try {
      meter->reset();
      return tryQuickModel(meter, random, options, view);
    } catch (LevelGenException ex) {
      Debug() << "Retrying level gen";
    }
  }
  FAIL << "Couldn't generate a level";
  return nullptr;
}

void ModelBuilder::measureModelGen(int numTries, RandomGen& random, Options* options) {
  int numSuccess = 0;
  int maxT = 0;
  int minT = 1000000;
  double sumT = 0;
  for (int i : Range(numTries))
    try {
      sf::Clock c;
      tryCollectiveModel(nullptr, random, options, nullptr, "pok");
      int millis = c.getElapsedTime().asMilliseconds();
      sumT += millis;
      maxT = max(maxT, millis);
      minT = min(minT, millis);
      ++numSuccess;
    } catch (LevelGenException ex) {
    }
  std::cout << numSuccess << " / " << numTries << " gens successful.\nMinT: " << minT << "\nMaxT: " << maxT <<
      "\nAvgT: " << sumT / numSuccess << std::endl;
}

PModel ModelBuilder::collectiveModel(ProgressMeter* meter, RandomGen& random,
    Options* options, View* view, const string& worldName) {
  for (int i : Range(10)) {
    try {
      meter->reset();
      return tryCollectiveModel(meter, random, options, view, worldName);
    } catch (LevelGenException ex) {
      Debug() << "Retrying level gen";
    }
  }
  FAIL << "Couldn't generate a level";
  return nullptr;
}

static string getNewIdSuffix() {
  vector<char> chars;
  for (char c : Range(128))
    if (isalnum(c))
      chars.push_back(c);
  string ret;
  for (int i : Range(4))
    ret += Random.choose(chars);
  return ret;
}

Level* ModelBuilder::makeExtraLevel(ProgressMeter* meter, RandomGen& random, Model* model,
    const LevelInfo& levelInfo, const SettlementInfo& settlement1) {
  SettlementInfo settlement(settlement1);
  const int towerHeight = random.get(7, 12);
  const int gnomeHeight = random.get(3, 5);
  switch (levelInfo.levelId) {
    case ExtraLevelId::TOWER: {
      StairKey downLink = levelInfo.stairKey;
      for (int i : Range(towerHeight - 1)) {
        StairKey upLink = StairKey::getNew();
        model->buildLevel(
            LevelBuilder(meter, random, 4, 4, "Tower floor" + toString(i + 2)),
            LevelMaker::towerLevel(random,
                CONSTRUCT(SettlementInfo,
                  c.type = SettlementType::TOWER;
                  c.creatures = CreatureFactory::singleType(model->tribeSet->human.get(), random.choose(LIST(
                      CreatureId::WATER_ELEMENTAL, CreatureId::AIR_ELEMENTAL, CreatureId::FIRE_ELEMENTAL,
                      CreatureId::EARTH_ELEMENTAL)));
                  c.numCreatures = random.get(1, 3);
                  c.location = new Location();
                  c.upStairs = {upLink};
                  c.downStairs = {downLink};
                  c.furniture = SquareFactory::single(SquareId::TORCH);
                  c.buildingId = BuildingId::BRICK;)));
        downLink = upLink;
      }
      settlement.downStairs = {downLink};
      return model->buildLevel(
         LevelBuilder(meter, random, 5, 5, "Tower top"),
         LevelMaker::towerLevel(random, settlement));
      }
    case ExtraLevelId::CRYPT: 
      settlement.upStairs = {levelInfo.stairKey};
      return model->buildLevel(
         LevelBuilder(meter, random, 40, 40, "Crypt"),
         LevelMaker::cryptLevel(random, settlement));
    case ExtraLevelId::MAZE: 
      settlement.upStairs = {levelInfo.stairKey};
      return model->buildLevel(
         LevelBuilder(meter, random, 40, 40, "Maze"),
         LevelMaker::mazeLevel(random, settlement));
    case ExtraLevelId::GNOMISH_MINES: {
      StairKey upLink = levelInfo.stairKey;
      for (int i : Range(gnomeHeight - 1)) {
        StairKey downLink = StairKey::getNew();
        model->buildLevel(
            LevelBuilder(meter, random, 60, 40, "Mines lvl " + toString(i + 1)),
            LevelMaker::roomLevel(random, CreatureFactory::gnomishMines(settlement.tribe,
                    model->tribeSet->monster.get(), 0),
                CreatureFactory::waterCreatures(settlement.tribe),
                CreatureFactory::lavaCreatures(settlement.tribe), {upLink}, {downLink},
                SquareFactory::roomFurniture(model->tribeSet->pest.get())));
        upLink = downLink;
      }
      settlement.upStairs = {upLink};
      return model->buildLevel(
         LevelBuilder(meter, random, 60, 40, "Mine Town"),
         LevelMaker::mineTownLevel(random, settlement));
      }
    case ExtraLevelId::SOKOBAN:
      settlement.downStairs = {levelInfo.stairKey};
      for (int i : Range(5000))
        try {
          return model->buildLevel(
              LevelBuilder(meter, random, 28, 14, "Sokoban"),
              LevelMaker::sokobanLevel(random, settlement));
        } catch (LevelGenException ex) {
        }
      throw LevelGenException();
  }
}

static string getBoardText(const string& keeperName, const string& dukeName) {
  return dukeName + " will reward a daring hero 150 florens for slaying " + keeperName + " the Keeper.";
}

PModel ModelBuilder::tryCollectiveModel(ProgressMeter* meter, RandomGen& random,
    Options* options, View* view, const string& worldName) {
  Model* m = new Model(view, worldName, TribeSet());
  m->setOptions(options);
  string keeperName = options->getStringValue(OptionId::KEEPER_NAME);
  vector<EnemyInfo> enemyInfo = getEnemyInfo(random, *m->tribeSet, getBoardText(keeperName,
        "Duke of " + NameGenerator::get(NameGeneratorId::WORLD)->getNext()));
  vector<SettlementInfo> settlements;
  vector<pair<LevelInfo, SettlementInfo>> extraSettlements;
  for (auto& elem : enemyInfo) {
    elem.settlement.collective = new CollectiveBuilder(elem.config, elem.settlement.tribe);
    if (!elem.extraLevel)
      settlements.push_back(elem.settlement);
    else
      extraSettlements.emplace_back(*elem.extraLevel, elem.settlement);
  }
  Level* top = m->buildLevel(
      LevelBuilder(meter, random, 360, 360, "Wilderness", false),
      LevelMaker::topLevel(random, CreatureFactory::forrest(m->tribeSet->wildlife.get()), settlements));
  for (auto& elem : extraSettlements)
    makeExtraLevel(meter, random, m, elem.first, elem.second);
  m->calculateStairNavigation();
  m->collectives.push_back(CollectiveBuilder(
        getKeeperConfig(options->getBoolValue(OptionId::FAST_IMMIGRATION)), m->tribeSet->keeper.get())
      .setLevel(top)
      .setCredit(getKeeperCredit(options->getBoolValue(OptionId::STARTING_RESOURCE)))
      .build());
 
  m->playerCollective = m->collectives.back().get();
  m->playerControl = new PlayerControl(m->playerCollective, m, top);
  m->playerCollective->setControl(PCollectiveControl(m->playerControl));
  PCreature c = CreatureFactory::fromId(CreatureId::KEEPER, m->tribeSet->keeper.get(),
      MonsterAIFactory::collective(m->playerCollective));
  if (!keeperName.empty())
    c->setFirstName(keeperName);
  m->gameIdentifier = *c->getFirstName() + "_" + m->worldName + getNewIdSuffix();
  m->gameDisplayName = *c->getFirstName() + " of " + m->worldName;
  Creature* ref = c.get();
  top->landCreature(StairKey::keeperSpawn(), c.get());
  m->addCreature(std::move(c));
  m->playerControl->addKeeper(ref);
  for (int i : Range(4)) {
    PCreature c = CreatureFactory::fromId(CreatureId::IMP, m->tribeSet->keeper.get(),
        MonsterAIFactory::collective(m->playerCollective));
    top->landCreature(StairKey::keeperSpawn(), c.get());
    m->playerControl->addImp(c.get());
    m->addCreature(std::move(c));
  }
  for (int i : All(enemyInfo)) {
    if (!enemyInfo[i].settlement.collective->hasCreatures())
      continue;
    PVillageControl control;
    Location* location = enemyInfo[i].settlement.location;
    if (auto name = location->getName())
      enemyInfo[i].settlement.collective->setName(*name);
    PCollective collective = enemyInfo[i].settlement.collective->addSquares(location->getAllSquares()).build();
    if (!enemyInfo[i].villains.empty())
      getOnlyElement(enemyInfo[i].villains).collective = m->playerCollective;
    control.reset(new VillageControl(collective.get(), enemyInfo[i].villains));
    if (enemyInfo[i].villainType)
      m->villainsByType[*enemyInfo[i].villainType].push_back(collective.get());
    m->allVillains.push_back(collective.get());
    collective->setControl(std::move(control));
    m->collectives.push_back(std::move(collective));
  }
  return PModel(m);
}

PModel ModelBuilder::splashModel(ProgressMeter* meter, View* view, const string& splashPath) {
  Model* m = new Model(view, "", TribeSet());
  Level* l = m->buildLevel(
      LevelBuilder(meter, Random, Level::getSplashBounds().getW(), Level::getSplashBounds().getH(), "Splash",
          false),
      LevelMaker::splashLevel(
          CreatureFactory::splashLeader(m->tribeSet->human.get()),
          CreatureFactory::splashHeroes(m->tribeSet->human.get()),
          CreatureFactory::splashMonsters(m->tribeSet->keeper.get()),
          CreatureFactory::singleType(m->tribeSet->keeper.get(), CreatureId::IMP), splashPath));
  m->spectator.reset(new Spectator(l));
  return PModel(m);
}

