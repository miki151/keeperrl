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
#include "game.h"
#include "campaign.h"
#include "creature_name.h"

static Location* getVillageLocation(bool markSurprise) {
  return new Location(NameGenerator::get(NameGeneratorId::TOWN)->getNext(), markSurprise);
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
  optional<VillainInfo> villain;
  optional<LevelInfo> extraLevel;
  optional<VillainType> villainType;
};

static EnemyInfo mainVillain(SettlementInfo settlement, CollectiveConfig config,
    optional<VillainInfo> villain = none, optional<LevelInfo> extraLevel = none) {
  return EnemyInfo{settlement, config, villain, extraLevel, VillainType::MAIN};
}

static EnemyInfo lesserVillain(SettlementInfo settlement, CollectiveConfig config,
    optional<VillainInfo> villain = none, optional<LevelInfo> extraLevel = none) {
  return EnemyInfo{settlement, config, villain, extraLevel, VillainType::LESSER};
}

static EnemyInfo noVillain(SettlementInfo settlement, CollectiveConfig config,
    optional<VillainInfo> villain = none, optional<LevelInfo> extraLevel = none) {
  return EnemyInfo{settlement, config, villain, extraLevel, none};
}

static EnemyInfo getVault(SettlementType type, CreatureFactory factory, TribeId tribe, int num,
    optional<ItemFactory> itemFactory = none, optional<VillainInfo> villain = none) {
  return noVillain(CONSTRUCT(SettlementInfo,
      c.type = type;
      c.creatures = factory;
      c.numCreatures = num;
      c.location = new Location(true);
      c.tribe = tribe;
      c.buildingId = BuildingId::DUNGEON;
      c.shopFactory = itemFactory;), CollectiveConfig::noImmigrants(),
    villain);
}

static EnemyInfo getVault(SettlementType type, CreatureId id, TribeId tribe, int num,
    optional<ItemFactory> itemFactory = none, optional<VillainInfo> villain = none) {
  return getVault(type, CreatureFactory::singleType(tribe, id), tribe, num, itemFactory, villain);
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

static vector<EnemyInfo> getVaults(RandomGen& random) {
  vector<EnemyInfo> ret {
 /*   getVault(SettlementType::VAULT, CreatureFactory::insects(TribeId::getMonster()),
        TribeId::getMonster(), random.get(6, 12)),*/
    getVault(SettlementType::VAULT, CreatureId::RAT, TribeId::getPest(), random.get(3, 8),
        ItemFactory::armory()),
  };
  for (int i : Range(random.get(1, 3))) {
    FriendlyVault v = random.choose(friendlyVaults);
    ret.push_back(getVault(SettlementType::VAULT, v.id, TribeId::getKeeper(), random.get(v.min, v.max)));
  }
  return ret;
}

static vector<EnemyInfo> getGnomishMines(RandomGen& random, bool mark) {
  StairKey gnomeKey = StairKey::getNew();
  return {
    lesserVillain(CONSTRUCT(SettlementInfo,
      c.type = SettlementType::MINETOWN;
      c.creatures = CreatureFactory::gnomeVillage(TribeId::getGnome());
      c.numCreatures = random.get(12, 24);
      c.location = getVillageLocation(false);
      c.tribe = TribeId::getGnome();
      c.race = "gnomes";
      c.buildingId = BuildingId::DUNGEON;
      c.shopFactory = ItemFactory::gnomeShop();
      c.outsideFeatures = SquareFactory::dungeonOutside();
      c.furniture = SquareFactory::roomFurniture(TribeId::getPest());),
      CollectiveConfig::noImmigrants(), {}, LevelInfo{ExtraLevelId::GNOMISH_MINES, gnomeKey}),
    noVillain(CONSTRUCT(SettlementInfo,
      c.type = SettlementType::SMALL_MINETOWN;
      c.creatures = CreatureFactory::gnomeEntrance(TribeId::getGnome());
      c.numCreatures = random.get(3, 7);
      c.location = new Location(mark);
      c.tribe = TribeId::getGnome();
      c.race = "gnomes";
      c.buildingId = BuildingId::DUNGEON;
      c.downStairs = {gnomeKey};
      c.outsideFeatures = SquareFactory::dungeonOutside();
      c.furniture = SquareFactory::roomFurniture(TribeId::getPest());),
      CollectiveConfig::noImmigrants(), {})
  };
}

static vector<EnemyInfo> getDarkElvenMines(RandomGen& random, bool mark) {
  StairKey gnomeKey = StairKey::getNew();
  return {
    lesserVillain(CONSTRUCT(SettlementInfo,
      c.type = SettlementType::MINETOWN;
      c.creatures = CreatureFactory::darkElfVillage(TribeId::getDarkElf());
      c.numCreatures = random.get(14, 16);
      c.location = getVillageLocation(false);
      c.tribe = TribeId::getDarkElf();
      c.race = "dark elves";
      c.buildingId = BuildingId::DUNGEON;
      c.outsideFeatures = SquareFactory::dungeonOutside();
      c.furniture = SquareFactory::roomFurniture(TribeId::getPest());),
      CollectiveConfig::withImmigrants(0.002, 15, {
          CONSTRUCT(ImmigrantInfo,
            c.id = CreatureId::DARK_ELF_WARRIOR;
            c.frequency = 3;
            c.traits = LIST(MinionTrait::FIGHTER);),
          }).allowRecruiting(2), {}, LevelInfo{ExtraLevelId::GNOMISH_MINES, gnomeKey}),
    noVillain(CONSTRUCT(SettlementInfo,
      c.type = SettlementType::SMALL_MINETOWN;
      c.creatures = CreatureFactory::darkElfEntrance(TribeId::getDarkElf());
      c.numCreatures = random.get(3, 7);
      c.location = new Location(mark);
      c.tribe = TribeId::getDarkElf();
      c.race = "dark elves";
      c.buildingId = BuildingId::DUNGEON;
      c.downStairs = {gnomeKey};
      c.outsideFeatures = SquareFactory::dungeonOutside();
      c.furniture = SquareFactory::roomFurniture(TribeId::getPest());),
      CollectiveConfig::noImmigrants(), {})
  };
}

static vector<EnemyInfo> getTower(RandomGen& random, bool mark) {
  StairKey towerKey = StairKey::getNew();
  return {
    noVillain(CONSTRUCT(SettlementInfo,
      c.type = SettlementType::TOWER;
      c.location = new Location(mark);
      c.tribe = TribeId::getHuman();
      c.upStairs = {towerKey};
      c.buildingId = BuildingId::BRICK;),
      CollectiveConfig::noImmigrants(), {}),
    mainVillain(CONSTRUCT(SettlementInfo,
      c.type = SettlementType::TOWER;
      c.creatures = CreatureFactory::singleType(TribeId::getHuman(), CreatureId::ELEMENTALIST);
      c.numCreatures = 1;
      c.location = new Location(false);
      c.tribe = TribeId::getHuman();
      c.buildingId = BuildingId::BRICK;
      c.furniture = SquareFactory::roomFurniture(TribeId::getPest());),
      CollectiveConfig::noImmigrants().setLeaderAsFighter(),
      {CONSTRUCT(VillainInfo,
        c.minPopulation = 0;
        c.minTeamSize = 1;
        c.triggers = LIST({AttackTriggerId::ROOM_BUILT, SquareId::THRONE}, AttackTriggerId::PROXIMITY,
            {AttackTriggerId::ROOM_BUILT, SquareId::IMPALED_HEAD}, AttackTriggerId::FINISH_OFF);
        c.behaviour = VillageBehaviour(VillageBehaviourId::CAMP_AND_SPAWN,
          CreatureFactory::elementals(TribeId::getHuman()));
        c.ransom = make_pair(0.5, random.get(200, 400));)}, LevelInfo{ExtraLevelId::TOWER, towerKey})
  };
}

static vector<EnemyInfo> getOrcTown(RandomGen& random, bool mark) {
  return {
    lesserVillain(CONSTRUCT(SettlementInfo,
      c.type = SettlementType::VILLAGE;
      c.creatures = CreatureFactory::orcTown(TribeId::getGreenskin());
      c.numCreatures = random.get(12, 16);
      c.location = getVillageLocation(mark);
      c.tribe = TribeId::getGreenskin();
      c.race = "greenskins";
      c.buildingId = BuildingId::BRICK;
      c.furniture = SquareFactory::roomFurniture(TribeId::getPest());
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

static vector<EnemyInfo> getFriendlyCave(RandomGen& random, CreatureId creature, bool mark) {
  return {
    lesserVillain(CONSTRUCT(SettlementInfo,
      c.type = SettlementType::CAVE;
      c.creatures = CreatureFactory::singleType(TribeId::getGreenskin(), creature);
      c.numCreatures = random.get(4, 8);
      c.location = new Location(mark);
      c.tribe = TribeId::getGreenskin();
      c.buildingId = BuildingId::DUNGEON;
      c.closeToPlayer = true;
      c.furniture = SquareFactory::roomFurniture(TribeId::getPest());
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

static vector<EnemyInfo> getWarriorCastle(RandomGen& random, bool mark) {
  return {
    mainVillain(CONSTRUCT(SettlementInfo,
      c.type = SettlementType::CASTLE2;
      c.creatures = CreatureFactory::vikingTown(TribeId::getHuman());
      c.numCreatures = random.get(12, 16);
      c.location = getVillageLocation(mark);
      c.tribe = TribeId::getHuman();
      c.race = "humans";
      c.buildingId = BuildingId::WOOD_CASTLE;
      c.stockpiles = LIST({StockpileInfo::GOLD, 800});
      c.guardId = CreatureId::WARRIOR;
      c.elderLoot = ItemType(ItemId::TECH_BOOK, TechId::BEAST_MUT);
      c.furniture = SquareFactory::roomFurniture(TribeId::getPest());
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
            AttackTriggerId::STOLEN_ITEMS, {AttackTriggerId::ROOM_BUILT, SquareId::IMPALED_HEAD},
            AttackTriggerId::FINISH_OFF, AttackTriggerId::PROXIMITY);
          c.behaviour = VillageBehaviour(VillageBehaviourId::KILL_LEADER);
          c.ransom = make_pair(0.8, random.get(500, 700));)})
  };
}

static vector<EnemyInfo> getHumanVillage(RandomGen& random, const string& boardText, bool mark) {
  return {
    mainVillain(CONSTRUCT(SettlementInfo,
      c.type = SettlementType::VILLAGE;
      c.creatures = CreatureFactory::humanVillage(TribeId::getHuman());
      c.numCreatures = random.get(12, 20);
      c.location = getVillageLocation(mark);
      c.tribe = TribeId::getHuman();
      c.race = "humans";
      c.buildingId = BuildingId::WOOD;
      c.shopFactory = ItemFactory::armory();
      c.furniture = SquareFactory::roomFurniture(TribeId::getPest());
      c.outsideFeatures = SquareFactory::villageOutside(boardText);
      ), CollectiveConfig::noImmigrants().setGhostSpawns(0.1, 4), {})
  };
}

static vector<EnemyInfo> getLizardVillage(RandomGen& random, bool mark) {
  return {
    mainVillain(CONSTRUCT(SettlementInfo,
      c.type = SettlementType::VILLAGE;
      c.creatures = CreatureFactory::lizardTown(TribeId::getLizard());
      c.numCreatures = random.get(8, 14);
      c.location = getVillageLocation(mark);
      c.tribe = TribeId::getLizard();
      c.race = "lizardmen";
      c.buildingId = BuildingId::MUD;
      c.elderLoot = ItemType(ItemId::TECH_BOOK, TechId::HUMANOID_MUT);
      c.shopFactory = ItemFactory::mushrooms();
      c.furniture = SquareFactory::roomFurniture(TribeId::getPest());
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
            AttackTriggerId::STOLEN_ITEMS, {AttackTriggerId::ROOM_BUILT, SquareId::IMPALED_HEAD},
            AttackTriggerId::FINISH_OFF, AttackTriggerId::PROXIMITY);
          c.behaviour = VillageBehaviour(VillageBehaviourId::KILL_LEADER);)})
  };
}

static vector<EnemyInfo> getElvenVillage(RandomGen& random, bool mark) {
  return {
    mainVillain(CONSTRUCT(SettlementInfo,
      c.type = SettlementType::VILLAGE2;
      c.creatures = CreatureFactory::elvenVillage(TribeId::getElf());
      c.numCreatures = random.get(11, 18);
      c.location = getVillageLocation(mark);
      c.tribe = TribeId::getElf();
      c.race = "elves";
      c.stockpiles = LIST({StockpileInfo::GOLD, 800});
      c.buildingId = BuildingId::WOOD;
      c.elderLoot = ItemType(ItemId::TECH_BOOK, TechId::SPELLS_MAS);
      c.furniture = SquareFactory::roomFurniture(TribeId::getPest());),
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

static vector<EnemyInfo> getAntNest(RandomGen& random, bool closedOff, bool mark) {
  return {
    lesserVillain(CONSTRUCT(SettlementInfo,
      c.type = closedOff ? SettlementType::ANT_NEST : SettlementType::MINETOWN;
      c.creatures = CreatureFactory::antNest(TribeId::getAnt());
      c.numCreatures = random.get(9, 14);
      c.location = new Location(mark);
      c.tribe = TribeId::getAnt();
      c.race = "ants";
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

static vector<EnemyInfo> getDwarfTown(RandomGen& random, bool mark) {
  return {
    mainVillain(CONSTRUCT(SettlementInfo,
      c.type = SettlementType::MINETOWN;
      c.creatures = CreatureFactory::dwarfTown(TribeId::getDwarf());
      c.numCreatures = random.get(9, 14);
      c.location = getVillageLocation(mark);
      c.tribe = TribeId::getDwarf();
      c.race = "dwarves";
      c.buildingId = BuildingId::DUNGEON;
      c.stockpiles = LIST({StockpileInfo::GOLD, 1000}, {StockpileInfo::MINERALS, 600});
      c.shopFactory = ItemFactory::dwarfShop();
      c.outsideFeatures = SquareFactory::dungeonOutside();
      c.furniture = SquareFactory::roomFurniture(TribeId::getPest());),
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
            AttackTriggerId::STOLEN_ITEMS, {AttackTriggerId::ROOM_BUILT, SquareId::IMPALED_HEAD},
            AttackTriggerId::FINISH_OFF, AttackTriggerId::PROXIMITY);
          c.behaviour = VillageBehaviour(VillageBehaviourId::KILL_MEMBERS, 3);
          c.ransom = make_pair(0.8, random.get(1200, 1600));)})
  };
}

static vector<EnemyInfo> getHumanCastle(RandomGen& random, bool mark) {
  optional<StairKey> stairKey;
  if (random.roll(4))
    stairKey = StairKey::getNew();
  vector<EnemyInfo> ret {
    mainVillain(CONSTRUCT(SettlementInfo,
      c.type = SettlementType::CASTLE;
      c.creatures = CreatureFactory::humanCastle(TribeId::getHuman());
      c.numCreatures = random.get(20, 26);
      c.location = getVillageLocation(mark);
      c.tribe = TribeId::getHuman();
      c.race = "humans";
      c.stockpiles = LIST({StockpileInfo::GOLD, 700});
      c.buildingId = BuildingId::BRICK;
      c.guardId = CreatureId::CASTLE_GUARD;
      if (stairKey)
        c.downStairs = { *stairKey };
      c.shopFactory = ItemFactory::villageShop();
      c.furniture = SquareFactory::castleFurniture(TribeId::getPest());
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
            AttackTriggerId::STOLEN_ITEMS, {AttackTriggerId::ROOM_BUILT, SquareId::IMPALED_HEAD},
            AttackTriggerId::FINISH_OFF, AttackTriggerId::PROXIMITY);
          c.behaviour = VillageBehaviour(VillageBehaviourId::KILL_LEADER);
          c.ransom = make_pair(0.9, random.get(1400, 2000));)})};
  if (stairKey)
    ret.push_back(
        lesserVillain(CONSTRUCT(SettlementInfo,
            c.creatures = CreatureFactory::singleType(TribeId::getMonster(), CreatureId::MINOTAUR);
            c.numCreatures = 1;
            c.location = new Location("maze");
            c.tribe = TribeId::getMonster();
            c.race = "monsters";
            c.furniture = SquareFactory::roomFurniture(TribeId::getPest());
            if (stairKey)
              c.upStairs = { *stairKey };
            c.buildingId = BuildingId::BRICK;), CollectiveConfig::noImmigrants(), {},
          LevelInfo{ExtraLevelId::MAZE, *stairKey}));
  return ret;
}

static vector<EnemyInfo> getWitchHouse(RandomGen& random, bool mark) {
  return {
    lesserVillain(CONSTRUCT(SettlementInfo,
      c.type = SettlementType::WITCH_HOUSE;
      c.creatures = CreatureFactory::singleType(TribeId::getMonster(), CreatureId::WITCH);
      c.numCreatures = 1;
      c.location = new Location(mark);
      c.tribe = TribeId::getMonster();
      c.race = "witch";
      c.buildingId = BuildingId::WOOD;
      c.elderLoot = ItemType(ItemId::TECH_BOOK, TechId::ALCHEMY_ADV);
      c.furniture = SquareFactory::single(SquareId::CAULDRON);), CollectiveConfig::noImmigrants(), {})
  };
}

static vector<EnemyInfo> getEntTown(RandomGen& random, bool mark) {
  return {
    lesserVillain(CONSTRUCT(SettlementInfo,
      c.type = SettlementType::FOREST;
      c.creatures = CreatureFactory::singleType(TribeId::getMonster(), CreatureId::ENT);
      c.numCreatures = random.get(7, 13);
      c.location = new Location(mark);
      c.tribe = TribeId::getMonster();
      c.race = "ents";
      c.buildingId = BuildingId::WOOD;),
      CollectiveConfig::withImmigrants(0.003, 15, {
          CONSTRUCT(ImmigrantInfo,
            c.id = CreatureId::ENT;
            c.frequency = 1;
            c.traits = LIST(MinionTrait::FIGHTER);)}), {})
  };
}

static vector<EnemyInfo> getDriadTown(RandomGen& random, bool mark) {
  return {
    lesserVillain(CONSTRUCT(SettlementInfo,
      c.type = SettlementType::FOREST;
      c.creatures = CreatureFactory::singleType(TribeId::getMonster(), CreatureId::DRIAD);
      c.numCreatures = random.get(7, 13);
      c.location = new Location(mark);
      c.tribe = TribeId::getMonster();
      c.race = "driads";
      c.buildingId = BuildingId::WOOD;),
      CollectiveConfig::withImmigrants(0.003, 15, {
          CONSTRUCT(ImmigrantInfo,
            c.id = CreatureId::DRIAD;
            c.frequency = 1;
            c.traits = LIST(MinionTrait::FIGHTER);)}), {})
  };
}

static vector<EnemyInfo> getCemetery(RandomGen& random, bool mark) {
  StairKey cryptKey = StairKey::getNew();
  return {
    lesserVillain(CONSTRUCT(SettlementInfo,
          c.type = SettlementType::CEMETERY;
          c.creatures = CreatureFactory::singleType(TribeId::getMonster(), CreatureId::ZOMBIE);
          c.numCreatures = random.get(8, 12);
          c.location = new Location("cemetery");
          c.tribe = TribeId::getMonster();
          c.race = "undead";
          c.furniture = SquareFactory::cryptCoffins(TribeId::getKeeper());
          c.upStairs = { cryptKey };
          c.buildingId = BuildingId::BRICK;), CollectiveConfig::noImmigrants(), {},
       LevelInfo{ExtraLevelId::CRYPT, cryptKey}),
    noVillain(CONSTRUCT(SettlementInfo,
          c.type = SettlementType::CEMETERY;
          c.creatures = CreatureFactory::singleType(TribeId::getMonster(), CreatureId::ZOMBIE);
          c.numCreatures = 1;
          c.location = new Location("cemetery", mark);
          c.race = "undead";
          c.tribe = TribeId::getMonster();
          c.downStairs = { cryptKey };
          c.buildingId = BuildingId::BRICK;), CollectiveConfig::noImmigrants(), {})
  };
}

static vector<EnemyInfo> getBanditCave(RandomGen& random, bool mark) {
  return {
    lesserVillain(CONSTRUCT(SettlementInfo,
      c.type = SettlementType::CAVE;
      c.creatures = CreatureFactory::singleType(TribeId::getBandit(), CreatureId::BANDIT);
      c.numCreatures = random.get(4, 9);
      c.location = new Location(mark);
      c.tribe = TribeId::getBandit();
      c.race = "bandits";
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

static vector<EnemyInfo> getShelob(RandomGen& random, bool mark) {
  return {
    lesserVillain(CONSTRUCT(SettlementInfo,
      c.type = SettlementType::CAVE;
      c.creatures = CreatureFactory::singleType(TribeId::getHostile(), CreatureId::SHELOB);
      c.numCreatures = 1;
      c.race = "giant spider";
      c.buildingId = BuildingId::DUNGEON;
      c.location = new Location(mark);
      c.tribe = TribeId::getHostile();), CollectiveConfig::noImmigrants().setLeaderAsFighter(), {})
  };
}

static vector<EnemyInfo> getGreenDragon(RandomGen& random, bool mark) {
  return {
    mainVillain(CONSTRUCT(SettlementInfo,
      c.type = SettlementType::CAVE;
      c.creatures = CreatureFactory::singleType(TribeId::getHostile(), CreatureId::GREEN_DRAGON);
      c.numCreatures = 1;
      c.location = new Location(mark);
      c.tribe = TribeId::getHostile();
      c.race = "dragon";
      c.buildingId = BuildingId::DUNGEON;
      c.shopFactory = ItemFactory::dragonCave();), CollectiveConfig::noImmigrants().setLeaderAsFighter()
          .setGhostSpawns(0.03, 1),
    { CONSTRUCT(VillainInfo,
            c.minPopulation = 0;
            c.minTeamSize = 1;
            c.triggers = LIST({AttackTriggerId::ENEMY_POPULATION, 20}, AttackTriggerId::STOLEN_ITEMS,
              AttackTriggerId::FINISH_OFF, AttackTriggerId::PROXIMITY);
            c.behaviour = VillageBehaviour(VillageBehaviourId::KILL_MEMBERS, 7);
            c.welcomeMessage = VillageControl::DRAGON_WELCOME;)})
  };
}

static vector<EnemyInfo> getHydra(RandomGen& random, bool mark) {
  return {
    lesserVillain(CONSTRUCT(SettlementInfo,
      c.type = SettlementType::SWAMP;
      c.creatures = CreatureFactory::singleType(TribeId::getHostile(), CreatureId::HYDRA);
      c.numCreatures = 1;
      c.race = "hydra";
      c.location = new Location(mark);
      c.tribe = TribeId::getHostile();), CollectiveConfig::noImmigrants().setLeaderAsFighter()
          .setGhostSpawns(0.03, 1),
    { /*CONSTRUCT(VillainInfo,
            c.minPopulation = 0;
            c.minTeamSize = 1;
            c.triggers = LIST({AttackTriggerId::ENEMY_POPULATION, 22});
            c.behaviour = VillageBehaviour(VillageBehaviourId::KILL_MEMBERS, 7);)*/})
  };
}

static vector<EnemyInfo> getRedDragon(RandomGen& random, bool mark) {
  return {
    mainVillain(CONSTRUCT(SettlementInfo,
      c.type = SettlementType::CAVE;
      c.creatures = CreatureFactory::singleType(TribeId::getHostile(), CreatureId::RED_DRAGON);
      c.numCreatures = 1;
      c.location = new Location(mark);
      c.race = "dragon";
      c.tribe = TribeId::getHostile();
      c.buildingId = BuildingId::DUNGEON;
      c.shopFactory = ItemFactory::dragonCave();), CollectiveConfig::noImmigrants().setLeaderAsFighter()
          .setGhostSpawns(0.03, 1),
    { CONSTRUCT(VillainInfo,
            c.minPopulation = 0;
            c.minTeamSize = 1;
            c.triggers = LIST({AttackTriggerId::ENEMY_POPULATION, 25}, AttackTriggerId::STOLEN_ITEMS,
              AttackTriggerId::FINISH_OFF, AttackTriggerId::PROXIMITY);
            c.behaviour = VillageBehaviour(VillageBehaviourId::KILL_MEMBERS, 12);
            c.welcomeMessage = VillageControl::DRAGON_WELCOME;)})
  };
}

static vector<EnemyInfo> getCyclops(RandomGen& random, bool mark) {
  return {
    lesserVillain(CONSTRUCT(SettlementInfo,
      c.type = SettlementType::CAVE;
      c.creatures = CreatureFactory::singleType(TribeId::getHostile(), CreatureId::CYCLOPS);
      c.numCreatures = 1;
      c.location = new Location(mark);
      c.race = "cyclops";
      c.tribe = TribeId::getHostile();
      c.buildingId = BuildingId::DUNGEON;
      c.shopFactory = ItemFactory::mushrooms(true);), CollectiveConfig::noImmigrants().setLeaderAsFighter()
          .setGhostSpawns(0.03, 1),
    { CONSTRUCT(VillainInfo,
            c.minPopulation = 0;
            c.minTeamSize = 1;
            c.triggers = LIST({AttackTriggerId::ENEMY_POPULATION, 13}, AttackTriggerId::PROXIMITY);
            c.behaviour = VillageBehaviour(VillageBehaviourId::KILL_MEMBERS, 4);)})
  };
}

static vector<EnemyInfo> getCottage(RandomGen& random, bool mark) {
  return {
    noVillain(CONSTRUCT(SettlementInfo,
      c.type = SettlementType::COTTAGE;
      c.creatures = CreatureFactory::humanPeaceful(TribeId::getHuman());
      c.numCreatures = random.get(3, 7);
      c.location = new Location(mark);
      c.tribe = TribeId::getHuman();
      c.race = "humans";
      c.buildingId = BuildingId::WOOD;
      c.furniture = SquareFactory::roomFurniture(TribeId::getPest());),
      CollectiveConfig::noImmigrants().setGuardian({CreatureId::WITCHMAN, 0.001, 1, 2}), {})
  };
}

static vector<EnemyInfo> getKoboldCave(RandomGen& random, bool mark) {
  return {
    noVillain(CONSTRUCT(SettlementInfo,
      c.type = SettlementType::SMALL_MINETOWN;
      c.creatures = CreatureFactory::koboldVillage(TribeId::getDwarf());
      c.numCreatures = random.get(3, 7);
      c.location = new Location(mark);    
      c.race = "kobolds";
      c.tribe = TribeId::getDwarf();
      c.buildingId = BuildingId::DUNGEON;
      c.stockpiles = LIST({StockpileInfo::MINERALS, 300});
 /*     c.outsideFeatures = SquareFactory::dungeonOutside();
      c.furniture = SquareFactory::roomFurniture(TribeId::getPest());*/),
      CollectiveConfig::noImmigrants(), {})
  };
}

/*static vector<EnemyInfo> getIslandVault(RandomGen& random) {
  return {
    lesserVillain(CONSTRUCT(SettlementInfo,
      c.type = SettlementType::ISLAND_VAULT;
      c.location = new Location();
      c.buildingId = BuildingId::DUNGEON;
      c.stockpiles = LIST({StockpileInfo::GOLD, 800});), CollectiveConfig::noImmigrants(), {})
  };
}*/

static vector<EnemyInfo> getSokobanEntry(RandomGen& random, SettlementType entryType, bool mark) {
  StairKey link = StairKey::getNew();
  return {
    noVillain(CONSTRUCT(SettlementInfo,
      c.type = entryType;
      c.location = new Location(mark);
      c.buildingId = BuildingId::DUNGEON;
      c.upStairs = {link};), CollectiveConfig::noImmigrants(), {}),
    noVillain(CONSTRUCT(SettlementInfo,
      c.type = SettlementType::ISLAND_VAULT;
      c.neutralCreatures = make_pair(
          CreatureFactory::singleType(TribeId::getHostile(), CreatureId::SOKOBAN_BOULDER), 0);
      c.creatures = CreatureFactory::singleType(TribeId::getKeeper(), CreatureId::SPECIAL_HL);
      c.numCreatures = 1;
      c.tribe = TribeId::getKeeper();
      c.location = new Location(false);
      c.buildingId = BuildingId::DUNGEON;
      c.downStairs = {link};), CollectiveConfig::noImmigrants(), {}, LevelInfo{ExtraLevelId::SOKOBAN, link}),
  };
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

static EnumSet<MinionTrait> getImpTraits() {
  return {MinionTrait::WORKER, MinionTrait::NO_LIMIT, MinionTrait::NO_EQUIPMENT};
}

PModel ModelBuilder::tryQuickModel(ProgressMeter* meter, RandomGen& random, Options* options, int width) {
  Model* m = new Model();
  string keeperName = options->getStringValue(OptionId::KEEPER_NAME);
  Level* top = m->buildLevel(
      LevelBuilder(meter, random, width, width, "Quick", false),
      LevelMaker::quickLevel(random));
  m->calculateStairNavigation();
  m->collectives.push_back(CollectiveBuilder(
        getKeeperConfig(options->getBoolValue(OptionId::FAST_IMMIGRATION)), TribeId::getKeeper())
      .setLevel(top)
      .setCredit(getKeeperCredit(true))
      .build());
  vector<CreatureId> ids {
    CreatureId::DONKEY,
  };
  for (auto elem : ids) {
    PCreature c = CreatureFactory::fromId(elem, TribeId::getKeeper(),
        MonsterAIFactory::monster());
    top->landCreature(StairKey::keeperSpawn(), c.get());
//    m->playerCollective->addCreature(c.get(), {MinionTrait::FIGHTER});
    m->addCreature(std::move(c));
  }
  return PModel(m);
}

PModel ModelBuilder::quickModel(ProgressMeter* meter, RandomGen& random, Options* options) {
  return tryBuilding(meter, 5000, [=, &random] { return tryQuickModel(meter, random, options, 40); });
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
                  c.creatures = CreatureFactory::singleType(TribeId::getHuman(), random.choose(LIST(
                      CreatureId::WATER_ELEMENTAL, CreatureId::AIR_ELEMENTAL, CreatureId::FIRE_ELEMENTAL,
                      CreatureId::EARTH_ELEMENTAL)));
                  c.numCreatures = random.get(1, 3);
                  c.location = new Location(false);
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
            LevelMaker::roomLevel(random, CreatureFactory::gnomishMines(settlement.tribe, TribeId::getMonster(), 0),
                CreatureFactory::waterCreatures(settlement.tribe),
                CreatureFactory::lavaCreatures(settlement.tribe), {upLink}, {downLink},
                SquareFactory::roomFurniture(TribeId::getPest())));
        upLink = downLink;
      }
      settlement.upStairs = {upLink};
      return model->buildLevel(
         LevelBuilder(meter, random, 60, 40, "Mine Town"),
         LevelMaker::mineTownLevel(random, settlement));
      }
    case ExtraLevelId::SOKOBAN:
      settlement.downStairs = {levelInfo.stairKey};
      for (int i : Range(5000)) {
        try {
          return model->buildLevel(
              LevelBuilder(meter, random, 28, 14, "Sokoban"),
              LevelMaker::sokobanLevel(random, settlement));
        } catch (LevelGenException ex) {
          Debug() << "Retrying";
        }
      }
      throw LevelGenException();
  }
}

static string getBoardText(const string& keeperName, const string& dukeName) {
  return dukeName + " will reward a daring hero 150 florens for slaying " + keeperName + " the Keeper.";
}

static vector<EnemyInfo> getEnemyInfo(RandomGen& random, const string& boardText) {
  vector<EnemyInfo> ret;
  for (int i : Range(random.get(5, 9)))
    append(ret, getCottage(random, true));
  for (int i : Range(random.get(1, 3))) {
    append(ret, getKoboldCave(random, true));
  }
  for (int i : Range(random.get(1, 3)))
    append(ret, getBanditCave(random, true));
  append(ret, getSokobanEntry(random, SettlementType::ISLAND_VAULT, true));
  append(ret, random.choose({
        getGnomishMines(random, true),
        getDarkElvenMines(random, true)}));
  append(ret, getVaults(random));
  if (random.roll(4))
    append(ret, getAntNest(random, true, true));
  append(ret, getHumanCastle(random, true));
  append(ret, random.choose({
        getFriendlyCave(random, CreatureId::ORC, true),
        getFriendlyCave(random, CreatureId::OGRE, true),
        getFriendlyCave(random, CreatureId::HARPY, true)}));
  for (auto& infos : random.chooseN(3, {
        getTower(random, true),
        getWarriorCastle(random, true),
        getLizardVillage(random, true),
        getElvenVillage(random, true),
        getDwarfTown(random, true),
        getHumanVillage(random, boardText, true)}))
    append(ret, infos);
  for (auto& infos : random.chooseN(3, {
        getGreenDragon(random, true),
        getShelob(random, true),
        getHydra(random, true),
        getRedDragon(random, true),
        getCyclops(random, true),
        getDriadTown(random, true),
        getEntTown(random, true)}))
    append(ret, infos);
  for (auto& infos : random.chooseN(1, {
        getWitchHouse(random, false),
        getCemetery(random, true)}))
    append(ret, infos);
  return ret;
}

PModel ModelBuilder::singleMapModel(ProgressMeter* meter, RandomGen& random,
    Options* options, const string& worldName) {
  PModel ret = tryBuilding(meter, 10, [=, &random] {
      return tryModel(meter, random, options, 360, worldName,
          getEnemyInfo(random, getBoardText(options->getStringValue(OptionId::KEEPER_NAME),
            "Duke of " + NameGenerator::get(NameGeneratorId::WORLD)->getNext())), true, BiomeId::GRASSLAND);});
  return ret;
}

PModel ModelBuilder::tryCampaignBaseModel(ProgressMeter* meter, RandomGen& random,
    Options* options, const string& siteName) {
  vector<EnemyInfo> enemyInfo;
 // append(enemyInfo, getBanditCave(random));
  /*      append(enemyInfo, getSokobanEntry(random));
        append(enemyInfo, random.choose({
          getFriendlyCave(random, CreatureId::ORC),
          getFriendlyCave(random, CreatureId::OGRE),
          getFriendlyCave(random, CreatureId::HARPY)}));*/
  /*      append(enemyInfo, random.choose({
          getGreenDragon(random),
          getShelob(random),
          getHydra(random),
          getRedDragon(random),
          getCyclops(random),
          getDriadTown(random),
          getEntTown(random)}));*/
  PModel ret = tryModel(meter, random, options, 210, siteName, enemyInfo, true, BiomeId::MOUNTAIN);
  return ret;
}

PModel ModelBuilder::tryCampaignSiteModel(ProgressMeter* meter, RandomGen& random,
    Options* options, const string& siteName, EnemyId enemyId) {
  vector<EnemyInfo> enemyInfo;
  BiomeId biomeId;
  switch (enemyId) {
    case EnemyId::ANTS:
      append(enemyInfo, getAntNest(random, false, true)); break;
    case EnemyId::ORC_VILLAGE:
      append(enemyInfo, getOrcTown(random, true)); break;
    case EnemyId::VILLAGE:
      append(enemyInfo, getHumanVillage(random, getBoardText(options->getStringValue(OptionId::KEEPER_NAME),
            "Duke of " + NameGenerator::get(NameGeneratorId::WORLD)->getNext()), true)); break;
    case EnemyId::WARRIORS:
      append(enemyInfo, getWarriorCastle(random, true)); break;
    case EnemyId::KNIGHTS:
      append(enemyInfo, getHumanCastle(random, true));
      for (int i : Range(random.get(3, 5)))
        append(enemyInfo, getCottage(random, false));
      break;
    case EnemyId::RED_DRAGON:
      append(enemyInfo, getRedDragon(random, true)); break;
    case EnemyId::GREEN_DRAGON:
      append(enemyInfo, getGreenDragon(random, true)); break;
    case EnemyId::DWARVES:
      append(enemyInfo, getDwarfTown(random, true));
      for (int i : Range(random.get(1, 3)))
        append(enemyInfo, getKoboldCave(random, true));
      break;
    case EnemyId::ELVES:
      append(enemyInfo, getElvenVillage(random, true)); break;
    case EnemyId::ELEMENTALIST:
      append(enemyInfo, getTower(random, true)); break;
    case EnemyId::BANDITS:
      append(enemyInfo, getBanditCave(random, true)); break;
    case EnemyId::LIZARDMEN:
      append(enemyInfo, getLizardVillage(random, true)); break;
    case EnemyId::DARK_ELVES:
      append(enemyInfo, getDarkElvenMines(random, true)); break;
    case EnemyId::GNOMES:
      append(enemyInfo, getGnomishMines(random, true)); break;
    case EnemyId::ENTS:
      append(enemyInfo, getEntTown(random, true)); break;
    case EnemyId::DRIADS:
      append(enemyInfo, getDriadTown(random, true)); break;
    case EnemyId::SHELOB:
      append(enemyInfo, getShelob(random, true)); break;
    case EnemyId::CYCLOPS:
      append(enemyInfo, getCyclops(random, true)); break;
    case EnemyId::HYDRA:
      append(enemyInfo, getHydra(random, true)); break;
    case EnemyId::CEMETERY:
      append(enemyInfo, getCemetery(random, true)); break;
    case EnemyId::FRIENDLY_CAVE:
      append(enemyInfo, getFriendlyCave(random,
            random.choose({CreatureId::ORC, CreatureId::HARPY, CreatureId::OGRE}), true));
      break;
    case EnemyId::SOKOBAN:
      append(enemyInfo, getSokobanEntry(random, SettlementType::ISLAND_VAULT_DOOR, true));
      break;
  }
  switch (enemyId) {
    case EnemyId::KNIGHTS:
    case EnemyId::WARRIORS:
    case EnemyId::ELEMENTALIST:
    case EnemyId::LIZARDMEN:
    case EnemyId::HYDRA:
    case EnemyId::VILLAGE:
    case EnemyId::ORC_VILLAGE:
      biomeId = BiomeId::GRASSLAND;
      break;
    case EnemyId::RED_DRAGON:
    case EnemyId::GREEN_DRAGON:
    case EnemyId::DWARVES:
    case EnemyId::DARK_ELVES:
    case EnemyId::FRIENDLY_CAVE:
    case EnemyId::SOKOBAN:
    case EnemyId::GNOMES:
    case EnemyId::CYCLOPS:
    case EnemyId::SHELOB:
    case EnemyId::ANTS:
      biomeId = BiomeId::MOUNTAIN;
      break;
    case EnemyId::ELVES:
    case EnemyId::DRIADS:
    case EnemyId::ENTS:
      biomeId = BiomeId::FORREST;
      break;
    case EnemyId::BANDITS:
      biomeId = random.choose<BiomeId>();
      break;
    case EnemyId::CEMETERY:
      biomeId = random.choose({BiomeId::GRASSLAND, BiomeId::FORREST});
      break;
  }
  return tryModel(meter, random, options, 170, siteName, enemyInfo, false, biomeId);
}

PModel ModelBuilder::tryBuilding(ProgressMeter* meter, int numTries, function<PModel()> buildFun) {
  for (int i : Range(numTries)) {
    try {
      if (meter)
        meter->reset();
      return buildFun();
    } catch (LevelGenException ex) {
      Debug() << "Retrying level gen";
    }
  }
  FAIL << "Couldn't generate a level";
  return nullptr;

}

PModel ModelBuilder::campaignBaseModel(ProgressMeter* meter, RandomGen& random,
    Options* options, const string& siteName) {
  return tryBuilding(meter, 20, [=, &random] {
      return tryCampaignBaseModel(meter, random, options, siteName); });
}

PModel ModelBuilder::campaignSiteModel(ProgressMeter* meter, RandomGen& random,
    Options* options, const string& siteName, EnemyId enemyId) {
  return tryBuilding(meter, 20, [=, &random] {
      return tryCampaignSiteModel(meter, random, options, siteName, enemyId); });
}

void ModelBuilder::measureSiteGen(int numTries, RandomGen& random, Options* options) {
  for (EnemyId id : {EnemyId::SOKOBAN}) {
//  for (EnemyId id : ENUM_ALL(EnemyId)) {
    std::cout << "Measuring " << EnumInfo<EnemyId>::getString(id) << std::endl;
    measureModelGen(numTries, [&] {
      tryCampaignSiteModel(nullptr, random, options, "", id); });
  }
}

void ModelBuilder::measureModelGen(int numTries, function<void()> genFun) {
  int numSuccess = 0;
  int maxT = 0;
  int minT = 1000000;
  double sumT = 0;
  for (int i : Range(numTries)) {
    try {
      // FIX this
//      sf::Clock c;
      genFun();
      int millis = 0;//c.getElapsedTime().asMilliseconds();
      sumT += millis;
      maxT = max(maxT, millis);
      minT = min(minT, millis);
      ++numSuccess;
      std::cout << ".";
      std::cout.flush();
    } catch (LevelGenException ex) {
      std::cout << "x";
      std::cout.flush();
    }
  }
  std::cout << numSuccess << " / " << numTries << " gens successful.\nMinT: " << minT << "\nMaxT: " << maxT <<
      "\nAvgT: " << sumT / numSuccess << std::endl;
}

void ModelBuilder::spawnKeeper(Model* m, Options* options) {
  Level* level = m->levels[0].get();
  PCreature keeper = CreatureFactory::fromId(CreatureId::KEEPER, TribeId::getKeeper());
  string keeperName = options->getStringValue(OptionId::KEEPER_NAME);
  if (!keeperName.empty())
    keeper->getName().setFirst(keeperName);
  Creature* keeperRef = keeper.get();
  level->landCreature(StairKey::keeperSpawn(), keeperRef);
  m->addCreature(std::move(keeper));
  m->collectives.push_back(CollectiveBuilder(
        getKeeperConfig(options->getBoolValue(OptionId::FAST_IMMIGRATION)), TribeId::getKeeper())
      .setLevel(level)
      .addCreature(keeperRef)
      .setCredit(getKeeperCredit(options->getBoolValue(OptionId::STARTING_RESOURCE)))
      .build());
  Collective* playerCollective = m->collectives.back().get();
  playerCollective->setVillainType(VillainType::PLAYER);
  playerCollective->setControl(PCollectiveControl(new PlayerControl(playerCollective, level)));
  for (int i : Range(4)) {
    PCreature c = CreatureFactory::fromId(CreatureId::IMP, TribeId::getKeeper(),
        MonsterAIFactory::collective(playerCollective));
    level->landCreature(StairKey::keeperSpawn(), c.get());
    playerCollective->addCreature(c.get(), getImpTraits());
    m->addCreature(std::move(c));
  }
}

PModel ModelBuilder::tryModel(ProgressMeter* meter, RandomGen& random, Options* options, int width,
    const string& levelName, vector<EnemyInfo> enemyInfo, bool keeperSpawn, BiomeId biomeId) {
  Model* m = new Model();
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
      LevelBuilder(meter, random, width, width, levelName, false),
      LevelMaker::topLevel(random, CreatureFactory::forrest(TribeId::getWildlife()), settlements, width, keeperSpawn,
          biomeId));
  for (auto& elem : extraSettlements)
    makeExtraLevel(meter, random, m, elem.first, elem.second);
  m->calculateStairNavigation();
  for (int i : All(enemyInfo)) {
    if (!enemyInfo[i].settlement.collective->hasCreatures())
      continue;
    PVillageControl control;
    Location* location = enemyInfo[i].settlement.location;
    if (auto name = location->getName())
      enemyInfo[i].settlement.collective->setLocationName(*name);
    if (auto race = enemyInfo[i].settlement.race)
      enemyInfo[i].settlement.collective->setRaceName(*race);
    PCollective collective = enemyInfo[i].settlement.collective->addSquares(location->getAllSquares()).build();
    control.reset(new VillageControl(collective.get(), enemyInfo[i].villain));
    if (enemyInfo[i].villainType)
      collective->setVillainType(*enemyInfo[i].villainType);
    collective->setControl(std::move(control));
    m->collectives.push_back(std::move(collective));
  }
  return PModel(m);
}

PModel ModelBuilder::splashModel(ProgressMeter* meter, const string& splashPath) {
  Model* m = new Model();
  Level* l = m->buildLevel(
      LevelBuilder(meter, Random, Level::getSplashBounds().width(), Level::getSplashBounds().height(), "Splash",
        true, 1.0),
      LevelMaker::splashLevel(
          CreatureFactory::splashLeader(TribeId::getHuman()),
          CreatureFactory::splashHeroes(TribeId::getHuman()),
          CreatureFactory::splashMonsters(TribeId::getKeeper()),
          CreatureFactory::singleType(TribeId::getKeeper(), CreatureId::IMP), splashPath));
  return PModel(m);
}

