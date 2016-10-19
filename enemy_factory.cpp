#include "stdafx.h"
#include "enemy_factory.h"
#include "location.h"
#include "name_generator.h"
#include "technology.h"
#include "attack_trigger.h"

EnemyFactory::EnemyFactory(RandomGen& r) : random(r) {
}

EnemyInfo::EnemyInfo(SettlementInfo s, CollectiveConfig c, optional<VillageBehaviour> v,
    optional<LevelConnection> l)
  : settlement(s), config(c), villain(v), levelConnection(l) {
}

EnemyInfo& EnemyInfo::setVillainType(VillainType type) {
  villainType = type;
  return *this;
}

EnemyInfo& EnemyInfo::setSurprise() {
  settlement.location->setSurprise();
  if (levelConnection)
    levelConnection->otherEnemy->setSurprise();
  return *this;
}

static EnemyInfo getVault(SettlementType type, CreatureFactory factory, TribeId tribe, int num,
    optional<ItemFactory> itemFactory = none, optional<VillageBehaviour> villain = none) {
  return EnemyInfo(CONSTRUCT(SettlementInfo,
      c.type = type;
      c.creatures = factory;
      c.numCreatures = num;
      c.location = new Location();
      c.tribe = tribe;
      c.buildingId = BuildingId::DUNGEON;
      c.shopFactory = itemFactory;), CollectiveConfig::noImmigrants(),
    villain).setSurprise();
}

static EnemyInfo getVault(SettlementType type, CreatureId id, TribeId tribe, int num,
    optional<ItemFactory> itemFactory = none, optional<VillageBehaviour> villain = none) {
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

vector<EnemyInfo> EnemyFactory::getVaults() {
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

static Location* getVillageLocation() {
  return new Location(NameGenerator::get(NameGeneratorId::TOWN)->getNext());
}

EnemyInfo EnemyFactory::get(EnemyId enemyId) {
  switch (enemyId) {
    case EnemyId::ANTS_CLOSED:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::ANT_NEST;
            c.creatures = CreatureFactory::antNest(TribeId::getAnt());
            c.numCreatures = random.get(9, 14);
            c.location = new Location();
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
          CONSTRUCT(VillageBehaviour,
            c.minPopulation = 1;
            c.minTeamSize = 4;
            c.triggers = LIST(AttackTriggerId::ENTRY);
            c.attackBehaviour = AttackBehaviour(AttackBehaviourId::KILL_LEADER);));
    case EnemyId::ANTS_OPEN: {
        auto ants = get(EnemyId::ANTS_CLOSED);
        ants.settlement.type = SettlementType::MINETOWN;
        return ants;
      }
    case EnemyId::ORC_VILLAGE:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::VILLAGE;
            c.tribe = TribeId::getGreenskin();
            c.creatures = CreatureFactory::orcTown(c.tribe);
            c.numCreatures = random.get(12, 16);
            c.location = getVillageLocation();
            c.race = "greenskins";
            c.buildingId = BuildingId::BRICK;
            c.furniture = FurnitureFactory::roomFurniture(c.tribe);
            c.outsideFeatures = FurnitureFactory::villageOutside(c.tribe);),
          CollectiveConfig::withImmigrants(0.003, 16, {
            CONSTRUCT(ImmigrantInfo,
                c.id = CreatureId::ORC;
                c.frequency = 3;
                c.traits = LIST(MinionTrait::FIGHTER);),          
            CONSTRUCT(ImmigrantInfo,
                c.id = CreatureId::OGRE;
                c.frequency = 1;
                c.traits = LIST(MinionTrait::FIGHTER);),
            }).allowRecruiting(9));
    case EnemyId::VILLAGE:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::VILLAGE;
            c.tribe = TribeId::getHuman();
            c.creatures = CreatureFactory::humanVillage(c.tribe);
            c.numCreatures = random.get(12, 20);
            c.location = getVillageLocation();
            c.race = "humans";
            c.buildingId = BuildingId::WOOD;
            c.shopFactory = ItemFactory::armory();
            c.furniture = FurnitureFactory::roomFurniture(c.tribe);
            ), CollectiveConfig::noImmigrants().setGhostSpawns(0.1, 4));
    case EnemyId::WARRIORS:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::CASTLE2;
            c.tribe = TribeId::getHuman();
            c.creatures = CreatureFactory::vikingTown(c.tribe);
            c.numCreatures = random.get(12, 16);
            c.location = getVillageLocation();
            c.race = "humans";
            c.buildingId = BuildingId::WOOD_CASTLE;
            c.stockpiles = LIST({StockpileInfo::GOLD, 800});
            c.guardId = CreatureId::WARRIOR;
            c.elderLoot = ItemType(ItemId::TECH_BOOK, TechId::BEAST_MUT);
            c.furniture = FurnitureFactory::roomFurniture(c.tribe);
            c.outsideFeatures = FurnitureFactory::castleOutside(c.tribe);),
          CollectiveConfig::withImmigrants(0.003, 16, {
            CONSTRUCT(ImmigrantInfo,
                c.id = CreatureId::WARRIOR;
                c.frequency = 1;
                c.traits = LIST(MinionTrait::FIGHTER);),
            }).setGhostSpawns(0.1, 6),
          CONSTRUCT(VillageBehaviour,
              c.minPopulation = 6;
              c.minTeamSize = 5;
              c.triggers = LIST(
                  {AttackTriggerId::ROOM_BUILT, FurnitureType::THRONE},
                  AttackTriggerId::SELF_VICTIMS,
                  {AttackTriggerId::TIMER, 7000},
                  AttackTriggerId::STOLEN_ITEMS,
                  {AttackTriggerId::ROOM_BUILT, FurnitureType::IMPALED_HEAD},
                  AttackTriggerId::FINISH_OFF,
                  AttackTriggerId::PROXIMITY);
              c.attackBehaviour = AttackBehaviour(AttackBehaviourId::KILL_LEADER);
              c.ransom = make_pair(0.8, random.get(500, 700));));
    case EnemyId::KNIGHTS:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::CASTLE;
            c.tribe = TribeId::getHuman();
            c.creatures = CreatureFactory::humanCastle(c.tribe);
            c.numCreatures = random.get(20, 26);
            c.location = getVillageLocation();
            c.race = "humans";
            c.stockpiles = LIST({StockpileInfo::GOLD, 700});
            c.buildingId = BuildingId::BRICK;
            c.guardId = CreatureId::CASTLE_GUARD;
            c.shopFactory = ItemFactory::villageShop();
            c.furniture = FurnitureFactory::castleFurniture(c.tribe);
            c.outsideFeatures = FurnitureFactory::castleOutside(c.tribe);),
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
          CONSTRUCT(VillageBehaviour,
              c.minPopulation = 12;
              c.minTeamSize = 10;
              c.triggers = LIST(
                  {AttackTriggerId::ROOM_BUILT, FurnitureType::THRONE},
                  AttackTriggerId::SELF_VICTIMS,
                  {AttackTriggerId::TIMER, 7000},
                  AttackTriggerId::STOLEN_ITEMS,
                  {AttackTriggerId::ROOM_BUILT, FurnitureType::IMPALED_HEAD},
                  AttackTriggerId::FINISH_OFF,
                  AttackTriggerId::PROXIMITY);
              c.attackBehaviour = AttackBehaviour(AttackBehaviourId::KILL_LEADER);
              c.ransom = make_pair(0.9, random.get(1400, 2000));),
          random.roll(4) ? LevelConnection{LevelConnection::MAZE, get(EnemyId::MINOTAUR)}
              : optional<LevelConnection>(none));
    case EnemyId::MINOTAUR:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.tribe = TribeId::getMonster();
            c.creatures = CreatureFactory::singleType(c.tribe, CreatureId::MINOTAUR);
            c.numCreatures = 1;
            c.location = new Location("maze");
            c.race = "monsters";
            c.furniture = FurnitureFactory::roomFurniture(c.tribe);
            c.buildingId = BuildingId::BRICK;), CollectiveConfig::noImmigrants());
    case EnemyId::RED_DRAGON:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::CAVE;
            c.creatures = CreatureFactory::singleType(TribeId::getHostile(), CreatureId::RED_DRAGON);
            c.numCreatures = 1;
            c.location = new Location();
            c.race = "dragon";
            c.tribe = TribeId::getHostile();
            c.buildingId = BuildingId::DUNGEON;
            c.shopFactory = ItemFactory::dragonCave();), CollectiveConfig::noImmigrants().setLeaderAsFighter()
          .setGhostSpawns(0.03, 1),
          CONSTRUCT(VillageBehaviour,
              c.minPopulation = 0;
              c.minTeamSize = 1;
              c.triggers = LIST(
                  {AttackTriggerId::ENEMY_POPULATION, 22},
                  AttackTriggerId::STOLEN_ITEMS,
                  {AttackTriggerId::TIMER, 7000},
                  AttackTriggerId::FINISH_OFF,
                  AttackTriggerId::PROXIMITY);
              c.attackBehaviour = AttackBehaviour(AttackBehaviourId::KILL_MEMBERS, 12);
              c.welcomeMessage = VillageBehaviour::DRAGON_WELCOME;));
    case EnemyId::GREEN_DRAGON:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::CAVE;
            c.creatures = CreatureFactory::singleType(TribeId::getHostile(), CreatureId::GREEN_DRAGON);
            c.numCreatures = 1;
            c.location = new Location();
            c.tribe = TribeId::getHostile();
            c.race = "dragon";
            c.buildingId = BuildingId::DUNGEON;
            c.shopFactory = ItemFactory::dragonCave();), CollectiveConfig::noImmigrants().setLeaderAsFighter()
          .setGhostSpawns(0.03, 1),
          CONSTRUCT(VillageBehaviour,
              c.minPopulation = 0;
              c.minTeamSize = 1;
              c.triggers = LIST(
                  {AttackTriggerId::ENEMY_POPULATION, 18},
                  AttackTriggerId::STOLEN_ITEMS,
                  {AttackTriggerId::TIMER, 7000},
                  AttackTriggerId::FINISH_OFF,
                  AttackTriggerId::PROXIMITY);
              c.attackBehaviour = AttackBehaviour(AttackBehaviourId::KILL_MEMBERS, 7);
              c.welcomeMessage = VillageBehaviour::DRAGON_WELCOME;));
    case EnemyId::DWARVES:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::MINETOWN;
            c.tribe = TribeId::getDwarf();
            c.creatures = CreatureFactory::dwarfTown(c.tribe);
            c.numCreatures = random.get(9, 14);
            c.location = getVillageLocation();
            c.race = "dwarves";
            c.buildingId = BuildingId::DUNGEON;
            c.stockpiles = LIST({StockpileInfo::GOLD, 1000}, {StockpileInfo::MINERALS, 600});
            c.shopFactory = ItemFactory::dwarfShop();
            c.outsideFeatures = FurnitureFactory::dungeonOutside(c.tribe);
            c.furniture = FurnitureFactory::roomFurniture(c.tribe);),
          CollectiveConfig::withImmigrants(0.002, 15, {
            CONSTRUCT(ImmigrantInfo,
                c.id = CreatureId::DWARF;
                c.frequency = 1;
                c.traits = LIST(MinionTrait::FIGHTER);),          
            }).setGhostSpawns(0.1, 4),
          CONSTRUCT(VillageBehaviour,
            c.minPopulation = 3;
            c.minTeamSize = 4;
            c.triggers = LIST(
                {AttackTriggerId::ROOM_BUILT, FurnitureType::THRONE},
                AttackTriggerId::SELF_VICTIMS,
                AttackTriggerId::STOLEN_ITEMS,
                {AttackTriggerId::ROOM_BUILT, FurnitureType::IMPALED_HEAD},
                {AttackTriggerId::TIMER, 7000},
                AttackTriggerId::FINISH_OFF,
                AttackTriggerId::PROXIMITY);
            c.attackBehaviour = AttackBehaviour(AttackBehaviourId::KILL_MEMBERS, 3);
            c.ransom = make_pair(0.8, random.get(1200, 1600));));
    case EnemyId::ELVES:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::VILLAGE2;
            c.creatures = CreatureFactory::elvenVillage(TribeId::getElf());
            c.numCreatures = random.get(11, 18);
            c.location = getVillageLocation();
            c.tribe = TribeId::getElf();
            c.race = "elves";
            c.stockpiles = LIST({StockpileInfo::GOLD, 800});
            c.buildingId = BuildingId::WOOD;
            c.elderLoot = ItemType(ItemId::TECH_BOOK, TechId::SPELLS_MAS);
            c.furniture = FurnitureFactory::roomFurniture(TribeId::getPest());),
          CollectiveConfig::withImmigrants(0.002, 18, {
            CONSTRUCT(ImmigrantInfo,
                c.id = CreatureId::ELF_ARCHER;
                c.frequency = 1;
                c.traits = LIST(MinionTrait::FIGHTER);),          
            }).setGhostSpawns(0.1, 4),
          CONSTRUCT(VillageBehaviour,
              c.minPopulation = 4;
              c.minTeamSize = 4;
              c.triggers = LIST(AttackTriggerId::STOLEN_ITEMS);
              c.attackBehaviour = AttackBehaviour(AttackBehaviourId::KILL_LEADER);));
    case EnemyId::ELEMENTALIST_ENTRY:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::TOWER;
            c.location = new Location();
            c.tribe = TribeId::getHuman();
            c.buildingId = BuildingId::BRICK;),
          CollectiveConfig::noImmigrants());
    case EnemyId::ELEMENTALIST:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::TOWER;
            c.creatures = CreatureFactory::singleType(TribeId::getHuman(), CreatureId::ELEMENTALIST);
            c.numCreatures = 1;
            c.location = new Location();
            c.tribe = TribeId::getHuman();
            c.buildingId = BuildingId::BRICK;
            c.furniture = FurnitureFactory::roomFurniture(TribeId::getPest());),
          CollectiveConfig::noImmigrants().setLeaderAsFighter(),
          CONSTRUCT(VillageBehaviour,
              c.minPopulation = 0;
              c.minTeamSize = 1;
              c.triggers = LIST(
                  {AttackTriggerId::ROOM_BUILT, FurnitureType::THRONE},
                  AttackTriggerId::PROXIMITY,
                  {AttackTriggerId::ROOM_BUILT, FurnitureType::IMPALED_HEAD},
                  AttackTriggerId::FINISH_OFF,
                  {AttackTriggerId::TIMER, 7000});
              c.attackBehaviour = AttackBehaviour(AttackBehaviourId::CAMP_AND_SPAWN,
                CreatureFactory::elementals(TribeId::getHuman()));
              c.ransom = make_pair(0.5, random.get(200, 400));),
          LevelConnection{LevelConnection::TOWER, get(EnemyId::ELEMENTALIST_ENTRY)});
    case EnemyId::BANDITS:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::CAVE;
            c.creatures = CreatureFactory::singleType(TribeId::getBandit(), CreatureId::BANDIT);
            c.numCreatures = random.get(4, 9);
            c.location = new Location();
            c.tribe = TribeId::getBandit();
            c.race = "bandits";
            c.buildingId = BuildingId::DUNGEON;),
          CollectiveConfig::withImmigrants(0.001, 10, {
            CONSTRUCT(ImmigrantInfo,
                c.id = CreatureId::BANDIT;
                c.frequency = 1;
                c.traits = LIST(MinionTrait::FIGHTER);),
            }),
          CONSTRUCT(VillageBehaviour,
              c.minPopulation = 0;
              c.minTeamSize = 3;
              c.triggers = LIST({AttackTriggerId::GOLD, 500});
              c.attackBehaviour = AttackBehaviour(AttackBehaviourId::STEAL_GOLD);
              c.ransom = make_pair(0.5, random.get(200, 400));));
    case EnemyId::LIZARDMEN:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::VILLAGE;
            c.tribe = TribeId::getLizard();
            c.creatures = CreatureFactory::lizardTown(c.tribe);
            c.numCreatures = random.get(8, 14);
            c.location = getVillageLocation();
            c.race = "lizardmen";
            c.buildingId = BuildingId::MUD;
            c.elderLoot = ItemType(ItemId::TECH_BOOK, TechId::HUMANOID_MUT);
            c.shopFactory = ItemFactory::mushrooms();
            c.furniture = FurnitureFactory::roomFurniture(c.tribe);
            c.outsideFeatures = FurnitureFactory::villageOutside(c.tribe);),
          CollectiveConfig::withImmigrants(0.007, 15, {
            CONSTRUCT(ImmigrantInfo,
                c.id = CreatureId::LIZARDMAN;
                c.frequency = 1;
                c.traits = LIST(MinionTrait::FIGHTER);),          
            }).setGhostSpawns(0.1, 4),
          CONSTRUCT(VillageBehaviour,
              c.minPopulation = 4;
              c.minTeamSize = 4;
              c.triggers = LIST(
                  AttackTriggerId::POWER,
                  AttackTriggerId::SELF_VICTIMS,
                  AttackTriggerId::STOLEN_ITEMS,
                  {AttackTriggerId::ROOM_BUILT, FurnitureType::IMPALED_HEAD},
                  {AttackTriggerId::TIMER, 7000},
                  AttackTriggerId::FINISH_OFF,
                  AttackTriggerId::PROXIMITY);
              c.attackBehaviour = AttackBehaviour(AttackBehaviourId::KILL_LEADER);));
    case EnemyId::DARK_ELVES:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::MINETOWN;
            c.tribe = TribeId::getDarkElf();
            c.creatures = CreatureFactory::darkElfVillage(c.tribe);
            c.numCreatures = random.get(14, 16);
            c.location = getVillageLocation();
            c.race = "dark elves";
            c.buildingId = BuildingId::DUNGEON;
            c.outsideFeatures = FurnitureFactory::dungeonOutside(c.tribe);
            c.furniture = FurnitureFactory::roomFurniture(c.tribe);),
          CollectiveConfig::withImmigrants(0.002, 15, {
            CONSTRUCT(ImmigrantInfo,
                c.id = CreatureId::DARK_ELF_WARRIOR;
                c.frequency = 3;
                c.traits = LIST(MinionTrait::FIGHTER);),
            }).allowRecruiting(2), none, 
          LevelConnection{LevelConnection::GNOMISH_MINES, get(EnemyId::DARK_ELVES_ENTRY)});
    case EnemyId::DARK_ELVES_ENTRY:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::SMALL_MINETOWN;
            c.tribe = TribeId::getDarkElf();
            c.creatures = CreatureFactory::darkElfEntrance(c.tribe);
            c.numCreatures = random.get(3, 7);
            c.location = new Location();
            c.race = "dark elves";
            c.buildingId = BuildingId::DUNGEON;
            c.outsideFeatures = FurnitureFactory::dungeonOutside(c.tribe);
            c.furniture = FurnitureFactory::roomFurniture(c.tribe);),
          CollectiveConfig::noImmigrants(), {});
    case EnemyId::GNOMES:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::MINETOWN;
            c.tribe = TribeId::getGnome();
            c.creatures = CreatureFactory::gnomeVillage(c.tribe);
            c.numCreatures = random.get(12, 24);
            c.location = getVillageLocation();
            c.race = "gnomes";
            c.buildingId = BuildingId::DUNGEON;
            c.shopFactory = ItemFactory::gnomeShop();
            c.outsideFeatures = FurnitureFactory::dungeonOutside(c.tribe);
            c.furniture = FurnitureFactory::roomFurniture(c.tribe);),
          CollectiveConfig::noImmigrants(), none,
          LevelConnection{LevelConnection::GNOMISH_MINES, get(EnemyId::GNOMES_ENTRY)});
    case EnemyId::GNOMES_ENTRY:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::SMALL_MINETOWN;
            c.tribe = TribeId::getGnome();
            c.creatures = CreatureFactory::gnomeEntrance(c.tribe);
            c.numCreatures = random.get(3, 7);
            c.location = new Location();
            c.race = "gnomes";
            c.buildingId = BuildingId::DUNGEON;
            c.outsideFeatures = FurnitureFactory::dungeonOutside(c.tribe);
            c.furniture = FurnitureFactory::roomFurniture(c.tribe);),
          CollectiveConfig::noImmigrants());
    case EnemyId::ENTS:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::FOREST;
            c.creatures = CreatureFactory::singleType(TribeId::getMonster(), CreatureId::ENT);
            c.numCreatures = random.get(7, 13);
            c.location = new Location();
            c.tribe = TribeId::getMonster();
            c.race = "tree spirits";
            c.buildingId = BuildingId::WOOD;),
          CollectiveConfig::withImmigrants(0.003, 15, {
          CONSTRUCT(ImmigrantInfo,
            c.id = CreatureId::ENT;
            c.frequency = 1;
            c.traits = LIST(MinionTrait::FIGHTER);)}));
    case EnemyId::DRIADS:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::FOREST;
            c.creatures = CreatureFactory::singleType(TribeId::getMonster(), CreatureId::DRIAD);
            c.numCreatures = random.get(7, 13);
            c.location = new Location();
            c.tribe = TribeId::getMonster();
            c.race = "driads";
            c.buildingId = BuildingId::WOOD;),
          CollectiveConfig::withImmigrants(0.003, 15, {
            CONSTRUCT(ImmigrantInfo,
                c.id = CreatureId::DRIAD;
                c.frequency = 1;
                c.traits = LIST(MinionTrait::FIGHTER);)}));
    case EnemyId::SHELOB:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::CAVE;
            c.creatures = CreatureFactory::singleType(TribeId::getShelob(), CreatureId::SHELOB);
            c.numCreatures = 1;
            c.race = "giant spider";
            c.buildingId = BuildingId::DUNGEON;
            c.location = new Location();
            c.tribe = TribeId::getShelob();), CollectiveConfig::noImmigrants().setLeaderAsFighter());
    case EnemyId::CYCLOPS:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::CAVE;
            c.creatures = CreatureFactory::singleType(TribeId::getHostile(), CreatureId::CYCLOPS);
            c.numCreatures = 1;
            c.location = new Location();
            c.race = "cyclops";
            c.tribe = TribeId::getHostile();
            c.buildingId = BuildingId::DUNGEON;
            c.shopFactory = ItemFactory::mushrooms(true);), CollectiveConfig::noImmigrants().setLeaderAsFighter()
          .setGhostSpawns(0.03, 1),
          CONSTRUCT(VillageBehaviour,
            c.minPopulation = 0;
            c.minTeamSize = 1;
            c.triggers = LIST(
                {AttackTriggerId::ENEMY_POPULATION, 13},
                AttackTriggerId::PROXIMITY);
            c.attackBehaviour = AttackBehaviour(AttackBehaviourId::KILL_MEMBERS, 4);));
    case EnemyId::HYDRA:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::SWAMP;
            c.creatures = CreatureFactory::singleType(TribeId::getHostile(), CreatureId::HYDRA);
            c.numCreatures = 1;
            c.race = "hydra";
            c.location = new Location();
            c.tribe = TribeId::getHostile();), CollectiveConfig::noImmigrants().setLeaderAsFighter()
          .setGhostSpawns(0.03, 1));
    case EnemyId::CEMETERY:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::CEMETERY;
            c.creatures = CreatureFactory::singleType(TribeId::getMonster(), CreatureId::ZOMBIE);
            c.numCreatures = random.get(8, 12);
            c.location = new Location("cemetery");
            c.tribe = TribeId::getMonster();
            c.race = "undead";
            c.furniture = FurnitureFactory::cryptCoffins(TribeId::getKeeper());
            c.buildingId = BuildingId::BRICK;), CollectiveConfig::noImmigrants(), {},
          LevelConnection{LevelConnection::CRYPT, get(EnemyId::CEMETERY_ENTRY)});
    case EnemyId::CEMETERY_ENTRY:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
          c.type = SettlementType::CEMETERY;
          c.creatures = CreatureFactory::singleType(TribeId::getMonster(), CreatureId::ZOMBIE);
          c.numCreatures = 1;
          c.location = new Location("cemetery");
          c.race = "undead";
          c.tribe = TribeId::getMonster();
          c.buildingId = BuildingId::BRICK;), CollectiveConfig::noImmigrants());
    case EnemyId::FRIENDLY_CAVE: {
      CreatureId creature = random.choose(CreatureId::OGRE, CreatureId::HARPY, CreatureId::ORC);
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::CAVE;
            c.tribe = TribeId::getGreenskin();
            c.creatures = CreatureFactory::singleType(c.tribe, creature);
            c.numCreatures = random.get(4, 8);
            c.location = new Location();
            c.buildingId = BuildingId::DUNGEON;
            c.closeToPlayer = true;
            c.furniture = FurnitureFactory::roomFurniture(c.tribe);
            c.outsideFeatures = FurnitureFactory::villageOutside(c.tribe);),
          CollectiveConfig::withImmigrants(0.003, 10, {
            CONSTRUCT(ImmigrantInfo,
                c.id = creature;
                c.frequency = 3;
                c.ignoreSpawnType = true;
                c.traits = LIST(MinionTrait::FIGHTER);), 
            }).allowRecruiting(4));
      }
    case EnemyId::SOKOBAN_ENTRY:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::ISLAND_VAULT_DOOR;
            c.location = new Location();
            c.buildingId = BuildingId::DUNGEON;
            ), CollectiveConfig::noImmigrants());
    case EnemyId::SOKOBAN:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::ISLAND_VAULT;
            c.neutralCreatures = make_pair(
              CreatureFactory::singleType(TribeId::getHostile(), CreatureId::SOKOBAN_BOULDER), 0);
            c.creatures = CreatureFactory::singleType(TribeId::getKeeper(), CreatureId::SPECIAL_HL);
            c.numCreatures = 1;
            c.tribe = TribeId::getKeeper();
            c.location = new Location();
            c.buildingId = BuildingId::DUNGEON;
            ), CollectiveConfig::noImmigrants(), none,
          LevelConnection{LevelConnection::SOKOBAN, get(EnemyId::SOKOBAN_ENTRY)});
    case EnemyId::WITCH:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::WITCH_HOUSE;
            c.tribe = TribeId::getMonster();
            c.creatures = CreatureFactory::singleType(c.tribe, CreatureId::WITCH);
            c.numCreatures = 1;
            c.location = new Location();
            c.race = "witch";
            c.buildingId = BuildingId::WOOD;
            c.elderLoot = ItemType(ItemId::TECH_BOOK, TechId::ALCHEMY_ADV);
            c.furniture = FurnitureFactory(c.tribe, FurnitureType::LABORATORY);), CollectiveConfig::noImmigrants());
    case EnemyId::HUMAN_COTTAGE:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::COTTAGE;
            c.tribe = TribeId::getHuman();
            c.creatures = CreatureFactory::humanPeaceful(c.tribe);
            c.numCreatures = random.get(3, 7);
            c.location = new Location();
            c.race = "humans";
            c.buildingId = BuildingId::WOOD;
            c.furniture = FurnitureFactory::roomFurniture(c.tribe);),
          CollectiveConfig::noImmigrants().setGuardian({CreatureId::WITCHMAN, 0.001, 1, 2}));
    case EnemyId::ELVEN_COTTAGE:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::FORREST_COTTAGE;
            c.tribe = TribeId::getElf();
            c.creatures = CreatureFactory::elvenCottage(c.tribe);
            c.numCreatures = random.get(3, 7);
            c.location = new Location();
            c.race = "elves";
            c.buildingId = BuildingId::WOOD;
            c.furniture = FurnitureFactory::roomFurniture(c.tribe);),
          CollectiveConfig::noImmigrants());
    case EnemyId::KOBOLD_CAVE:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::SMALL_MINETOWN;
            c.tribe = TribeId::getDwarf();
            c.creatures = CreatureFactory::koboldVillage(c.tribe);
            c.numCreatures = random.get(3, 7);
            c.location = new Location();
            c.race = "kobolds";
            c.buildingId = BuildingId::DUNGEON;
            c.stockpiles = LIST({StockpileInfo::MINERALS, 300});),
          CollectiveConfig::noImmigrants());
    case EnemyId::DWARF_CAVE:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::SMALL_MINETOWN;
            c.tribe = TribeId::getDwarf();
            c.creatures = CreatureFactory::dwarfCave(c.tribe);
            c.numCreatures = random.get(2, 5);
            c.location = new Location();
            c.race = "dwarves";
            c.buildingId = BuildingId::DUNGEON;
            c.stockpiles = LIST(random.choose(StockpileInfo{StockpileInfo::MINERALS, 300},
                StockpileInfo{StockpileInfo::GOLD, 300}));
            c.outsideFeatures = FurnitureFactory::dungeonOutside(c.tribe);
            c.furniture = FurnitureFactory::roomFurniture(c.tribe);),
          CollectiveConfig::noImmigrants(),
          CONSTRUCT(VillageBehaviour,
            c.minPopulation = 0;
            c.minTeamSize = 1;
            c.triggers = LIST(AttackTriggerId::SELF_VICTIMS, AttackTriggerId::STOLEN_ITEMS);
            c.attackBehaviour = AttackBehaviour(AttackBehaviourId::KILL_LEADER);
            c.ransom = make_pair(0.5, random.get(200, 400));));
  }
}
