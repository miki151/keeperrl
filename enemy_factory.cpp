#include "stdafx.h"
#include "enemy_factory.h"
#include "name_generator.h"
#include "technology.h"
#include "attack_trigger.h"
#include "external_enemies.h"
#include "immigrant_info.h"
#include "settlement_info.h"
#include "enemy_info.h"
#include "tribe_alignment.h"
#include "sunlight_info.h"
#include "conquer_condition.h"
#include "lasting_effect.h"

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

EnemyInfo& EnemyInfo::setId(EnemyId i) {
  id = i;
  return *this;
}

EnemyInfo& EnemyInfo::setImmigrants(vector<ImmigrantInfo> i) {
  immigrants = std::move(i);
  return *this;
}

EnemyInfo& EnemyInfo::setNonDiscoverable() {
  discoverable = false;
  return *this;
}

EnemyInfo& EnemyInfo::setCreateOnBones(EnemyFactory& factory, double prob, vector<EnemyId> enemies) {
  if (factory.random.chance(prob)) {
    EnemyInfo enemy = factory.get(factory.random.choose(enemies));
    levelConnection = enemy.levelConnection;
    if (levelConnection) {
      levelConnection->otherEnemy->settlement.buildingId = BuildingId::RUINS;
      levelConnection->deadInhabitants = true;
    }
    if (Random.roll(2)) {
      settlement.buildingId = BuildingId::RUINS;
      if (levelConnection)
        levelConnection->otherEnemy->settlement.buildingId = BuildingId::RUINS;
    } else {
      // 50% chance that the original settlement is intact
      settlement.buildingId = enemy.settlement.buildingId;
      settlement.furniture = enemy.settlement.furniture;
      settlement.outsideFeatures = enemy.settlement.outsideFeatures;
      settlement.shopFactory = enemy.settlement.shopFactory;
    }
    settlement.type = enemy.settlement.type;
    settlement.corpses = enemy.settlement.inhabitants;
    settlement.shopkeeperDead = true;
  }
  return *this;
}


static EnemyInfo getVault(SettlementType type, CreatureId creature, TribeId tribe, int num,
    optional<ItemFactory> itemFactory = none) {
  return EnemyInfo(CONSTRUCT(SettlementInfo,
      c.type = type;
      c.inhabitants.fighters = CreatureList(num, creature);
      c.tribe = tribe;
      c.closeToPlayer = true;
      c.dontConnectCave = true;
      c.buildingId = BuildingId::DUNGEON;
      c.shopFactory = itemFactory;
    ), CollectiveConfig::noImmigrants())
    .setNonDiscoverable();
}

namespace {
struct VaultInfo {
  CreatureId id;
  int min;
  int max;
};
}

static EnumMap<TribeAlignment, vector<VaultInfo>> friendlyVaults {
  {TribeAlignment::EVIL, {
      {CreatureId::ORC, 3, 5},
      {CreatureId::OGRE, 2, 4},
      {CreatureId::VAMPIRE, 2, 4},
  }},
  {TribeAlignment::LAWFUL, {
      {CreatureId::IRON_GOLEM, 3, 5},
      {CreatureId::EARTH_ELEMENTAL, 2, 4},
  }},
};

static vector<VaultInfo> hostileVaults {
  {CreatureId::SPIDER, 3, 8},
  {CreatureId::SNAKE, 3, 8},
  {CreatureId::BAT, 3, 8},
};

vector<EnemyInfo> EnemyFactory::getVaults(TribeAlignment alignment, TribeId allied) {
  vector<EnemyInfo> ret {
 /*   getVault(SettlementType::VAULT, CreatureFactory::insects(TribeId::getMonster()),
        TribeId::getMonster(), random.get(6, 12)),*/
    getVault(SettlementType::VAULT, CreatureId::RAT, TribeId::getPest(), random.get(3, 8),
        ItemFactory::armory()),
  };
  for (int i : Range(1)) {
    VaultInfo v = random.choose(friendlyVaults[alignment]);
    ret.push_back(getVault(SettlementType::VAULT, v.id, allied, random.get(v.min, v.max)));
  }
  for (int i : Range(1)) {
    VaultInfo v = random.choose(hostileVaults);
    ret.push_back(getVault(SettlementType::VAULT, v.id, TribeId::getMonster(), random.get(v.min, v.max)));
  }
  return ret;
}

static string getVillageName() {
  return NameGenerator::get(NameGeneratorId::TOWN)->getNext();
}

EnemyInfo EnemyFactory::get(EnemyId id){
  return getById(id).setId(id);
}

EnemyInfo EnemyFactory::getById(EnemyId enemyId) {
  switch (enemyId) {
     case EnemyId::UNICORN_HERD:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::FOREST;
            c.inhabitants.fighters = CreatureList(random.get(5, 8), CreatureId::UNICORN);
            c.tribe = TribeId::getMonster();
            c.race = "unicorns"_s;
            c.buildingId = BuildingId::WOOD;),
          CollectiveConfig::withImmigrants(200_visible, 9))
          .setImmigrants({ImmigrantInfo(CreatureId::UNICORN, {MinionTrait::FIGHTER}).setFrequency(1)});
    case EnemyId::ANTS_CLOSED:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::ANT_NEST;
            c.inhabitants.leader = CreatureId::ANT_QUEEN;
            c.inhabitants.civilians = CreatureList(random.get(5, 7), CreatureId::ANT_WORKER);
            c.inhabitants.fighters = CreatureList(random.get(5, 7), CreatureId::ANT_SOLDIER);
            c.dontConnectCave = true;
            c.surroundWithResources = 5;
            c.tribe = TribeId::getAnt();
            c.race = "ants"_s;
            c.buildingId = BuildingId::DUNGEON;),
          CollectiveConfig::withImmigrants(500_visible, 15),
          CONSTRUCT(VillageBehaviour,
            c.minPopulation = 1;
            c.minTeamSize = 4;
            c.triggers = LIST(AttackTriggerId::MINING_IN_PROXIMITY);
            c.attackBehaviour = AttackBehaviour(AttackBehaviourId::KILL_LEADER);))
          .setImmigrants({
              ImmigrantInfo(CreatureId::ANT_WORKER, {}).setFrequency(1),
              ImmigrantInfo(CreatureId::ANT_SOLDIER, {MinionTrait::FIGHTER}).setFrequency(1)});
    case EnemyId::ANTS_CLOSED_SMALL:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::SMALL_MINETOWN;
            c.inhabitants.civilians = CreatureList(random.get(2, 5), CreatureId::ANT_WORKER);
            c.inhabitants.fighters = CreatureList(random.get(3, 5), CreatureId::ANT_SOLDIER);
            c.tribe = TribeId::getAnt();
            c.race = "ants"_s;
            c.dontConnectCave = true;
            c.closeToPlayer = true;
            c.surroundWithResources = 6;
            c.buildingId = BuildingId::DUNGEON;),
          CollectiveConfig::noImmigrants(),
          CONSTRUCT(VillageBehaviour,
            c.minPopulation = 0;
            c.minTeamSize = 2;
            c.triggers = LIST(AttackTriggerId::MINING_IN_PROXIMITY);
            c.attackBehaviour = AttackBehaviour(AttackBehaviourId::KILL_LEADER);));
    case EnemyId::ANTS_OPEN: {
        auto ants = get(EnemyId::ANTS_CLOSED);
        ants.settlement.dontConnectCave = false;
        ants.setCreateOnBones(*this, 0.1, {EnemyId::DWARVES});
        return ants;
      }
    case EnemyId::ADA_GOLEMS: {
      int numGolems = 8;
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::VAULT;
            c.inhabitants.fighters = CreatureList(numGolems, CreatureId::ADA_GOLEM);
            c.tribe = TribeId::getAnt();
            c.race = "adamantine golems"_s;
            c.dontConnectCave = true;
            c.surroundWithResources = 3;
            c.extraResources = FurnitureType::ADAMANTIUM_ORE;
            c.buildingId = BuildingId::DUNGEON;),
          CollectiveConfig::noImmigrants(),
          CONSTRUCT(VillageBehaviour,
            c.minPopulation = 0;
            c.minTeamSize = numGolems;
            c.triggers = LIST(AttackTriggerId::MINING_IN_PROXIMITY);
            c.attackBehaviour = AttackBehaviour(AttackBehaviourId::KILL_LEADER);));
    }
    case EnemyId::ORC_VILLAGE:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::VILLAGE;
            c.tribe = TribeId::getGreenskin();
            c.inhabitants.fighters = CreatureList(
                random.get(12, 16),
                makeVec(CreatureId::ORC, CreatureId::OGRE));
            c.locationName = getVillageName();
            c.race = "greenskins"_s;
            c.buildingId = BuildingId::BRICK;
            c.furniture = FurnitureFactory::roomFurniture(c.tribe);
            c.outsideFeatures = FurnitureFactory::villageOutside(c.tribe);),
          CollectiveConfig::withImmigrants(300_visible, 16))
        .setCreateOnBones(*this, 0.1, {EnemyId::DWARVES, EnemyId::VILLAGE})
        .setImmigrants({
          ImmigrantInfo(CreatureId::ORC, {MinionTrait::FIGHTER}).setFrequency(3),
          ImmigrantInfo(CreatureId::OGRE, {MinionTrait::FIGHTER}).setFrequency(1)
        });
    case EnemyId::DEMON_DEN_ABOVE:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::VILLAGE;
            c.tribe = TribeId::getWildlife();
            c.inhabitants.fighters = CreatureList(random.get(2, 3), CreatureId::GHOST);
            c.buildingId = BuildingId::DUNGEON_SURFACE;
            c.locationName = "Darkshrine Town"_s;
            c.race = "ghosts"_s;
            c.furniture = FurnitureFactory::dungeonOutside(c.tribe);
            ),
            CollectiveConfig::noImmigrants()).setNonDiscoverable();
    case EnemyId::DEMON_DEN:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.tribe = TribeId::getMonster();
            c.inhabitants.leader = CreatureId::DEMON_LORD;
            c.inhabitants.fighters = CreatureList(random.get(5, 10), CreatureId::DEMON_DWELLER);
            c.locationName = "Darkshrine"_s;
            c.race = "demons"_s;
            c.furniture = FurnitureFactory::dungeonOutside(c.tribe);
            c.outsideFeatures = FurnitureFactory::dungeonOutside(c.tribe);),
          CollectiveConfig::withImmigrants(300_visible, 16),
          CONSTRUCT(VillageBehaviour,
              c.minPopulation = 0;
              c.minTeamSize = 3;
              c.triggers = LIST(
                  {AttackTriggerId::ROOM_BUILT, RoomTriggerInfo{FurnitureType::DEMON_SHRINE, 0.0001}},
              );
              c.attackBehaviour = AttackBehaviour(AttackBehaviourId::KILL_LEADER);
              c.ransom = make_pair(0.5, random.get(50, 100));),
          LevelConnection{LevelConnection::CRYPT, get(EnemyId::DEMON_DEN_ABOVE)})
          .setImmigrants({
            ImmigrantInfo(CreatureId::DEMON_DWELLER, {MinionTrait::FIGHTER}).setFrequency(1),
          });
    case EnemyId::VILLAGE:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::VILLAGE;
            c.tribe = TribeId::getHuman();
            c.inhabitants.leader = CreatureId::KNIGHT;
            c.inhabitants.fighters = CreatureList(
                random.get(4, 8),
                makeVec(CreatureId::KNIGHT, CreatureId::ARCHER));
            c.inhabitants.civilians = CreatureList(
                random.get(4, 8),
                makeVec(CreatureId::PESEANT, CreatureId::CHILD, CreatureId::DONKEY, CreatureId::HORSE,
                    CreatureId::COW, CreatureId::PIG, CreatureId::DOG));
            c.locationName = getVillageName();
            c.race = "humans"_s;
            c.buildingId = BuildingId::WOOD;
            c.shopFactory = ItemFactory::armory();
            c.furniture = FurnitureFactory::roomFurniture(c.tribe);
            ), CollectiveConfig::noImmigrants().setGhostSpawns(0.1, 4));
    case EnemyId::WARRIORS:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::CASTLE2;
            c.tribe = TribeId::getHuman();
            c.inhabitants.leader = CreatureId::SHAMAN;
            c.inhabitants.fighters = CreatureList(random.get(7, 10), CreatureId::WARRIOR);
            c.locationName = getVillageName();
            c.race = "humans"_s;
            c.buildingId = BuildingId::WOOD_CASTLE;
            c.stockpiles = LIST({StockpileInfo::GOLD, 160});
            c.guardId = CreatureId::WARRIOR;
            c.elderLoot = ItemType(ItemType::TechBook{"beast mutation"});
            c.furniture = FurnitureFactory::roomFurniture(c.tribe);
            c.outsideFeatures = FurnitureFactory::castleOutside(c.tribe);),
          CollectiveConfig::withImmigrants(300_visible, 10).setGhostSpawns(0.1, 6),
          CONSTRUCT(VillageBehaviour,
              c.minPopulation = 3;
              c.minTeamSize = 5;
              c.triggers = LIST(
                  {AttackTriggerId::ROOM_BUILT, RoomTriggerInfo{FurnitureType::THRONE, 0.0003}},
                  {AttackTriggerId::ROOM_BUILT, RoomTriggerInfo{FurnitureType::IMPALED_HEAD, 0.0001}},
                  AttackTriggerId::SELF_VICTIMS,
                  {AttackTriggerId::NUM_CONQUERED, 2},
                  AttackTriggerId::STOLEN_ITEMS,
                  AttackTriggerId::FINISH_OFF,
                  AttackTriggerId::PROXIMITY);
              c.attackBehaviour = AttackBehaviour(AttackBehaviourId::KILL_LEADER);
              c.ransom = make_pair(0.8, random.get(100, 140));))
           .setImmigrants({
             ImmigrantInfo(CreatureId::WARRIOR, {MinionTrait::FIGHTER}).setFrequency(1),
           });
    case EnemyId::KNIGHTS:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::CASTLE;
            c.tribe = TribeId::getHuman();
            c.inhabitants.leader = CreatureId::DUKE;
            c.inhabitants.fighters = CreatureList(
                random.get(12, 17),
                makeVec(make_pair(1, CreatureId::PRIEST),
                    make_pair(3, CreatureId::KNIGHT),
                    make_pair(1, CreatureId::ARCHER)));
            c.inhabitants.civilians = CreatureList(
                random.get(6, 9),
                makeVec(CreatureId::PESEANT, CreatureId::CHILD, CreatureId::DONKEY, CreatureId::HORSE,
                    CreatureId::COW, CreatureId::PIG, CreatureId::DOG));
            c.locationName = getVillageName();
            c.race = "humans"_s;
            c.stockpiles = LIST({StockpileInfo::GOLD, 140});
            c.buildingId = BuildingId::BRICK;
            c.guardId = CreatureId::KNIGHT;
            c.shopFactory = ItemFactory::villageShop();
            c.furniture = FurnitureFactory::castleFurniture(c.tribe);
            c.outsideFeatures = FurnitureFactory::castleOutside(c.tribe);),
          CollectiveConfig::withImmigrants(300_visible, 26).setGhostSpawns(0.1, 6),
          CONSTRUCT(VillageBehaviour,
              c.minPopulation = 12;
              c.minTeamSize = 10;
              c.triggers = LIST(
                  {AttackTriggerId::ROOM_BUILT, RoomTriggerInfo{FurnitureType::THRONE, 0.0003}},
                  {AttackTriggerId::ROOM_BUILT, RoomTriggerInfo{FurnitureType::IMPALED_HEAD, 0.0001}},
                  AttackTriggerId::SELF_VICTIMS,
                  {AttackTriggerId::NUM_CONQUERED, 3},
                  AttackTriggerId::STOLEN_ITEMS,
                  AttackTriggerId::FINISH_OFF,
                  AttackTriggerId::PROXIMITY);
              c.attackBehaviour = AttackBehaviour(AttackBehaviourId::KILL_LEADER);
              c.ransom = make_pair(0.9, random.get(280, 400));),
          random.roll(4) ? LevelConnection{LevelConnection::MAZE, get(EnemyId::MINOTAUR)}
              : optional<LevelConnection>(none))
          .setImmigrants({
            ImmigrantInfo(CreatureId::ARCHER, {MinionTrait::FIGHTER}).setFrequency(1),
            ImmigrantInfo(CreatureId::KNIGHT, {MinionTrait::FIGHTER}).setFrequency(1)
          });
    case EnemyId::MINOTAUR:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::CAVE;
            c.tribe = TribeId::getMonster();
            c.inhabitants.leader = CreatureId::MINOTAUR;
            c.locationName = "maze"_s;
            c.race = "monsters"_s;
            c.furniture = FurnitureFactory::roomFurniture(c.tribe);
            c.buildingId = BuildingId::BRICK;), CollectiveConfig::noImmigrants());
    case EnemyId::RED_DRAGON:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::CAVE;
            c.inhabitants.leader = CreatureId::RED_DRAGON;
            c.race = "dragon"_s;
            c.tribe = TribeId::getMonster();
            c.buildingId = BuildingId::DUNGEON;
            c.shopFactory = ItemFactory::dragonCave();), CollectiveConfig::noImmigrants().setLeaderAsFighter()
          .setGhostSpawns(0.03, 1),
          CONSTRUCT(VillageBehaviour,
              c.minPopulation = 0;
              c.minTeamSize = 1;
              c.triggers = LIST(
                  {AttackTriggerId::ENEMY_POPULATION, 22},
                  AttackTriggerId::STOLEN_ITEMS,
                  {AttackTriggerId::NUM_CONQUERED, 3},
                  AttackTriggerId::FINISH_OFF,
                  AttackTriggerId::PROXIMITY);
              c.attackBehaviour = AttackBehaviour(AttackBehaviourId::KILL_MEMBERS, 12);
              c.welcomeMessage = VillageBehaviour::DRAGON_WELCOME;))
                  .setCreateOnBones(*this, 0.1, {EnemyId::KNIGHTS, EnemyId::DWARVES, EnemyId::GREEN_DRAGON,
                      EnemyId::ELEMENTALIST});
    case EnemyId::GREEN_DRAGON:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::CAVE;
            c.inhabitants.leader = CreatureId::GREEN_DRAGON;
            c.tribe = TribeId::getMonster();
            c.race = "dragon"_s;
            c.buildingId = BuildingId::DUNGEON;
            c.shopFactory = ItemFactory::dragonCave();), CollectiveConfig::noImmigrants().setLeaderAsFighter()
          .setGhostSpawns(0.03, 1),
          CONSTRUCT(VillageBehaviour,
              c.minPopulation = 0;
              c.minTeamSize = 1;
              c.triggers = LIST(
                  {AttackTriggerId::ENEMY_POPULATION, 18},
                  AttackTriggerId::STOLEN_ITEMS,
                  {AttackTriggerId::NUM_CONQUERED, 2},
                  AttackTriggerId::FINISH_OFF,
                  AttackTriggerId::PROXIMITY);
              c.attackBehaviour = AttackBehaviour(AttackBehaviourId::KILL_MEMBERS, 7);
              c.welcomeMessage = VillageBehaviour::DRAGON_WELCOME;))
          .setCreateOnBones(*this, 0.1, {EnemyId::KNIGHTS, EnemyId::DWARVES, EnemyId::ELEMENTALIST});
    case EnemyId::DWARVES:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::MINETOWN;
            c.tribe = TribeId::getDwarf();
            c.inhabitants.leader = CreatureId::DWARF_BARON;
            c.inhabitants.fighters = CreatureList(random.get(6, 9), CreatureId::DWARF);
            c.inhabitants.civilians = CreatureList(
                random.get(3, 5),
                makeVec(make_pair(2, CreatureId::DWARF_FEMALE), make_pair(1, CreatureId::RAT)));
            c.locationName = getVillageName();
            c.race = "dwarves"_s;
            c.buildingId = BuildingId::DUNGEON;
            c.stockpiles = LIST({StockpileInfo::GOLD, 200}, {StockpileInfo::MINERALS, 120});
            c.shopFactory = ItemFactory::dwarfShop();
            c.outsideFeatures = FurnitureFactory::dungeonOutside(c.tribe);
            c.surroundWithResources = 5;
            c.furniture = FurnitureFactory::roomFurniture(c.tribe);),
          CollectiveConfig::withImmigrants(500_visible, 15).setGhostSpawns(0.1, 4),
          CONSTRUCT(VillageBehaviour,
            c.minPopulation = 3;
            c.minTeamSize = 4;
            c.triggers = LIST(
                {AttackTriggerId::ROOM_BUILT, RoomTriggerInfo{FurnitureType::THRONE, 0.0003}},
                {AttackTriggerId::ROOM_BUILT, RoomTriggerInfo{FurnitureType::IMPALED_HEAD, 0.0001}},
                AttackTriggerId::SELF_VICTIMS,
                AttackTriggerId::STOLEN_ITEMS,
                {AttackTriggerId::NUM_CONQUERED, 3},
                AttackTriggerId::FINISH_OFF,
                AttackTriggerId::PROXIMITY);
            c.attackBehaviour = AttackBehaviour(AttackBehaviourId::KILL_MEMBERS, 3);
            c.ransom = make_pair(0.8, random.get(240, 320));))
          .setImmigrants({
            ImmigrantInfo(CreatureId::DWARF, {MinionTrait::FIGHTER}).setFrequency(1),
          });
    case EnemyId::ELVES:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::FORREST_VILLAGE;
            c.inhabitants.leader = CreatureId::ELF_LORD;
            c.inhabitants.fighters = CreatureList(random.get(6, 9), CreatureId::ELF_ARCHER);
            c.inhabitants.civilians = CreatureList(
                random.get(6, 9),
                makeVec(CreatureId::ELF, CreatureId::ELF_CHILD, CreatureId::HORSE));
            c.locationName = getVillageName();
            c.tribe = TribeId::getElf();
            c.race = "elves"_s;
            c.stockpiles = LIST({StockpileInfo::GOLD, 100});
            c.buildingId = BuildingId::WOOD;
            c.elderLoot = ItemType(ItemType::TechBook{"master sorcery"});
            c.furniture = FurnitureFactory::roomFurniture(TribeId::getPest());),
          CollectiveConfig::withImmigrants(500_visible, 18).setGhostSpawns(0.1, 4),
          CONSTRUCT(VillageBehaviour,
              c.minPopulation = 4;
              c.minTeamSize = 4;
              c.triggers = LIST(AttackTriggerId::STOLEN_ITEMS);
              c.attackBehaviour = AttackBehaviour(AttackBehaviourId::KILL_LEADER);))
          .setImmigrants({
            ImmigrantInfo(CreatureId::ELF_ARCHER, {MinionTrait::FIGHTER}).setFrequency(1),
          });
    case EnemyId::ELEMENTALIST_ENTRY:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::TOWER;
            c.tribe = TribeId::getHuman();
            c.buildingId = BuildingId::BRICK;),
          CollectiveConfig::noImmigrants())
          .setNonDiscoverable();
    case EnemyId::ELEMENTALIST:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::TOWER;
            c.inhabitants.leader = CreatureId::ELEMENTALIST;
            c.tribe = TribeId::getHuman();
            c.buildingId = BuildingId::BRICK;
            c.furniture = FurnitureFactory::roomFurniture(TribeId::getPest());),
          CollectiveConfig::noImmigrants().setLeaderAsFighter(),
          CONSTRUCT(VillageBehaviour,
              c.minPopulation = 0;
              c.minTeamSize = 1;
              c.triggers = LIST(
                  {AttackTriggerId::ROOM_BUILT, RoomTriggerInfo{FurnitureType::THRONE, 0.0003}},
                  {AttackTriggerId::ROOM_BUILT, RoomTriggerInfo{FurnitureType::IMPALED_HEAD, 0.0001}},
                  AttackTriggerId::PROXIMITY,
                  AttackTriggerId::FINISH_OFF,
                  {AttackTriggerId::NUM_CONQUERED, 3});
              c.attackBehaviour = AttackBehaviour(AttackBehaviourId::CAMP_AND_SPAWN,
                CreatureFactory::elementals(TribeId::getHuman()));
              c.ransom = make_pair(0.5, random.get(40, 80));),
          LevelConnection{LevelConnection::TOWER, get(EnemyId::ELEMENTALIST_ENTRY)});
    case EnemyId::NO_AGGRO_BANDITS:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::CAVE;
            c.inhabitants.fighters = CreatureList(random.get(4, 9), CreatureId::BANDIT);
            c.tribe = TribeId::getBandit();
            c.race = "bandits"_s;
            c.buildingId = BuildingId::DUNGEON;),
          CollectiveConfig::withImmigrants(1000_visible, 10))
         .setCreateOnBones(*this, 0.1, {EnemyId::KOBOLD_CAVE})
         .setImmigrants({ ImmigrantInfo(CreatureId::BANDIT, {MinionTrait::FIGHTER}).setFrequency(1) });
    case EnemyId::BANDITS:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::CAVE;
            c.inhabitants.fighters = CreatureList(random.get(4, 9), CreatureId::BANDIT);
            c.tribe = TribeId::getBandit();
            c.race = "bandits"_s;
            c.buildingId = BuildingId::DUNGEON;
          ),
          CollectiveConfig::withImmigrants(1000_visible, 10),
          CONSTRUCT(VillageBehaviour,
              c.minPopulation = 0;
              c.minTeamSize = 3;
              c.triggers = LIST({AttackTriggerId::GOLD, 100});
              c.attackBehaviour = AttackBehaviour(AttackBehaviourId::STEAL_GOLD);
              c.ransom = make_pair(0.5, random.get(40, 80));))
          .setCreateOnBones(*this, 0.1, {EnemyId::KOBOLD_CAVE})
          .setImmigrants({ ImmigrantInfo(CreatureId::BANDIT, {MinionTrait::FIGHTER}).setFrequency(1) });
    case EnemyId::COTTAGE_BANDITS:
      return getById(EnemyId::NO_AGGRO_BANDITS).setCreateOnBones(*this, 1.0, {EnemyId::HUMAN_COTTAGE});
    case EnemyId::ORC_CAVE:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::CAVE;
            c.inhabitants.fighters = CreatureList(random.get(4, 9), CreatureId::ORC);
            c.tribe = TribeId::getBandit();
            c.race = "orcs"_s;
            c.buildingId = BuildingId::DUNGEON;
          ),
          CollectiveConfig::withImmigrants(1000_visible, 10),
          CONSTRUCT(VillageBehaviour,
              c.minPopulation = 0;
              c.minTeamSize = 3;
              c.triggers = LIST({AttackTriggerId::GOLD, 100});
              c.attackBehaviour = AttackBehaviour(AttackBehaviourId::STEAL_GOLD);
              c.ransom = make_pair(0.5, random.get(40, 80));))
          .setCreateOnBones(*this, 0.1, {EnemyId::KOBOLD_CAVE})
          .setImmigrants({ ImmigrantInfo(CreatureId::ORC, {MinionTrait::FIGHTER}).setFrequency(1) });
    case EnemyId::LIZARDMEN:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::VILLAGE;
            c.tribe = TribeId::getLizard();
            c.inhabitants.leader = CreatureId::LIZARDLORD;
            c.inhabitants.fighters = CreatureList(random.get(7, 10), CreatureId::LIZARDMAN);
            c.locationName = getVillageName();
            c.race = "lizardmen"_s;
            c.buildingId = BuildingId::MUD;
            c.elderLoot = ItemType(ItemType::TechBook{"humanoid mutation"});
            c.shopFactory = ItemFactory::mushrooms();
            c.furniture = FurnitureFactory::roomFurniture(c.tribe);
            c.outsideFeatures = FurnitureFactory::villageOutside(c.tribe);),
          CollectiveConfig::withImmigrants(140_visible, 11).setGhostSpawns(0.1, 4),
          CONSTRUCT(VillageBehaviour,
              c.minPopulation = 4;
              c.minTeamSize = 4;
              c.triggers = LIST(
                  AttackTriggerId::POWER,
                  AttackTriggerId::SELF_VICTIMS,
                  AttackTriggerId::STOLEN_ITEMS,
                  {AttackTriggerId::ROOM_BUILT, RoomTriggerInfo{FurnitureType::IMPALED_HEAD, 0.0001}},
                  {AttackTriggerId::NUM_CONQUERED, 2},
                  AttackTriggerId::FINISH_OFF,
                  AttackTriggerId::PROXIMITY);
              c.attackBehaviour = AttackBehaviour(AttackBehaviourId::KILL_LEADER);))
          .setCreateOnBones(*this, 1.0, {EnemyId::VILLAGE, EnemyId::ELVES})
          .setImmigrants({ ImmigrantInfo(CreatureId::LIZARDMAN, {MinionTrait::FIGHTER}).setFrequency(1) });
    case EnemyId::DARK_ELVES_ALLY:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::MINETOWN;
            c.tribe = TribeId::getDarkElf();
            c.inhabitants.leader = CreatureId::DARK_ELF_LORD;
            c.inhabitants.fighters = CreatureList(random.get(6, 9), CreatureId::DARK_ELF_WARRIOR);
            c.inhabitants.civilians = CreatureList(
                random.get(6, 9),
                makeVec(CreatureId::DARK_ELF, CreatureId::DARK_ELF_CHILD, CreatureId::RAT));
            c.locationName = getVillageName();
            c.race = "dark elves"_s;
            c.buildingId = BuildingId::DUNGEON;
            c.outsideFeatures = FurnitureFactory::dungeonOutside(c.tribe);
            c.furniture = FurnitureFactory::roomFurniture(c.tribe);),
          CollectiveConfig::withImmigrants(500_visible, 15), none,
          LevelConnection{LevelConnection::GNOMISH_MINES, get(EnemyId::DARK_ELVES_ENTRY)})
          .setImmigrants({ ImmigrantInfo(CreatureId::DARK_ELF_WARRIOR, {MinionTrait::FIGHTER}).setFrequency(1) });
    case EnemyId::DARK_ELVES_ENEMY:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::MINETOWN;
            c.tribe = TribeId::getDarkElf();
            c.inhabitants.leader = CreatureId::DARK_ELF_LORD;
            c.inhabitants.fighters = CreatureList(random.get(6, 9), CreatureId::DARK_ELF_WARRIOR);
            c.inhabitants.civilians = CreatureList(
                random.get(6, 9),
                makeVec(CreatureId::DARK_ELF, CreatureId::DARK_ELF_CHILD, CreatureId::RAT));
            c.inhabitants.leader.increaseBaseLevel({{ExperienceType::MELEE, 10}, {ExperienceType::SPELL, 10}});
            c.inhabitants.fighters.increaseBaseLevel({{ExperienceType::MELEE, 10}});
            c.locationName = getVillageName();
            c.race = "dark elves"_s;
            c.buildingId = BuildingId::DUNGEON;
            c.outsideFeatures = FurnitureFactory::dungeonOutside(c.tribe);
            c.furniture = FurnitureFactory::roomFurniture(c.tribe);),
          CollectiveConfig::withImmigrants(500_visible, 15), none,
          LevelConnection{LevelConnection::GNOMISH_MINES, get(EnemyId::DARK_ELVES_ENTRY)})
          .setImmigrants({ ImmigrantInfo(CreatureId::DARK_ELF_WARRIOR, {MinionTrait::FIGHTER}).setFrequency(1) });
    case EnemyId::DARK_ELVES_ENTRY:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::SMALL_MINETOWN;
            c.tribe = TribeId::getDarkElf();
            c.inhabitants.fighters = CreatureList(random.get(3, 7), CreatureId::DARK_ELF_WARRIOR);
            c.race = "dark elves"_s;
            c.buildingId = BuildingId::DUNGEON;
            c.outsideFeatures = FurnitureFactory::dungeonOutside(c.tribe);
            c.furniture = FurnitureFactory::roomFurniture(c.tribe);),
          CollectiveConfig::noImmigrants(), {})
          .setNonDiscoverable();
    case EnemyId::GNOMES:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::MINETOWN;
            c.tribe = TribeId::getGnome();
            c.inhabitants.leader = CreatureId::GNOME_CHIEF;
            c.inhabitants.fighters = CreatureList(random.get(8, 14), CreatureId::GNOME);
            c.locationName = getVillageName();
            c.race = "gnomes"_s;
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
            c.inhabitants.fighters = CreatureList(random.get(3, 7), CreatureId::GNOME);
            c.race = "gnomes"_s;
            c.buildingId = BuildingId::DUNGEON;
            c.outsideFeatures = FurnitureFactory::dungeonOutside(c.tribe);
            c.furniture = FurnitureFactory::roomFurniture(c.tribe);),
          CollectiveConfig::noImmigrants())
          .setNonDiscoverable();
    case EnemyId::ENTS:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::FOREST;
            c.inhabitants.fighters = CreatureList(random.get(7, 10), CreatureId::ENT);
            c.tribe = TribeId::getMonster();
            c.race = "tree spirits"_s;
            c.buildingId = BuildingId::WOOD;),
          CollectiveConfig::withImmigrants(300_visible, 10))
          .setImmigrants({ ImmigrantInfo(CreatureId::ENT, {MinionTrait::FIGHTER}).setFrequency(1) });
    case EnemyId::DRIADS:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::FOREST;
            c.inhabitants.fighters = CreatureList(random.get(7, 10), CreatureId::DRIAD);
            c.tribe = TribeId::getMonster();
            c.race = "driads"_s;
            c.buildingId = BuildingId::WOOD;),
          CollectiveConfig::withImmigrants(300_visible, 10))
          .setImmigrants({ ImmigrantInfo(CreatureId::DRIAD, {MinionTrait::FIGHTER}).setFrequency(1) });
    case EnemyId::SHELOB:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::CAVE;
            c.tribe = TribeId::getShelob();
            c.inhabitants.leader = CreatureId::SHELOB;
            c.inhabitants.civilians = CreatureList(
                random.get(4, 6),
                makeVec(CreatureId::SPIDER_FOOD,CreatureId::SPIDER));			
            c.race = "giant spider"_s;
            c.buildingId = BuildingId::DUNGEON;
            ), CollectiveConfig::noImmigrants().setLeaderAsFighter())
          .setCreateOnBones(*this, 0.1, {EnemyId::DWARF_CAVE, EnemyId::KOBOLD_CAVE});
    case EnemyId::CYCLOPS:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::CAVE;
            c.inhabitants.leader = CreatureId::CYCLOPS;
            c.race = "cyclops"_s;
            c.tribe = TribeId::getHostile();
            c.buildingId = BuildingId::DUNGEON;
            c.shopFactory = ItemFactory::mushrooms(true);), CollectiveConfig::noImmigrants().setLeaderAsFighter()
          .setGhostSpawns(0.03, 1),
          CONSTRUCT(VillageBehaviour,
            c.minPopulation = 0;
            c.minTeamSize = 1;
            c.triggers = LIST({AttackTriggerId::ENEMY_POPULATION, 13});
            c.attackBehaviour = AttackBehaviour(AttackBehaviourId::KILL_MEMBERS, 4);));
    case EnemyId::HYDRA:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::SWAMP;
            c.inhabitants.leader = CreatureId::HYDRA;
            c.race = "hydra"_s;
            c.tribe = TribeId::getHostile();), CollectiveConfig::noImmigrants().setLeaderAsFighter()
          .setGhostSpawns(0.03, 1));
    case EnemyId::KRAKEN:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::MOUNTAIN_LAKE;
            c.inhabitants.leader = CreatureId::KRAKEN;
            c.race = "kraken"_s;
            c.tribe = TribeId::getMonster();), CollectiveConfig::noImmigrants().setLeaderAsFighter())
          .setNonDiscoverable();
    case EnemyId::CEMETERY:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::CEMETERY;
            c.inhabitants.fighters = CreatureList(random.get(8, 12), CreatureId::ZOMBIE);
            c.locationName = "cemetery"_s;
            c.tribe = TribeId::getMonster();
            c.race = "undead"_s;
            c.furniture = FurnitureFactory::cryptCoffins(TribeId::getMonster());
            c.buildingId = BuildingId::BRICK;), CollectiveConfig::noImmigrants(), {},
          LevelConnection{LevelConnection::CRYPT, get(EnemyId::CEMETERY_ENTRY)});
    case EnemyId::CEMETERY_ENTRY:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::CEMETERY;
            c.inhabitants.fighters = CreatureList(1, CreatureId::ZOMBIE);
            c.locationName = "cemetery"_s;
            c.race = "undead"_s;
            c.tribe = TribeId::getMonster();
            c.buildingId = BuildingId::BRICK;), CollectiveConfig::noImmigrants())
          .setNonDiscoverable();
    case EnemyId::OGRE_CAVE:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::CAVE;
            c.tribe = TribeId::getGreenskin();
            c.inhabitants.fighters = CreatureList(random.get(4, 8), CreatureId::OGRE);
            c.buildingId = BuildingId::DUNGEON;
            c.closeToPlayer = true;
            c.furniture = FurnitureFactory::roomFurniture(c.tribe);
            c.outsideFeatures = FurnitureFactory::villageOutside(c.tribe);),
          CollectiveConfig::withImmigrants(300_visible, 10))
          .setImmigrants({ ImmigrantInfo(CreatureId::OGRE, {MinionTrait::FIGHTER}).setFrequency(1) });
    case EnemyId::HARPY_CAVE: {
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::CAVE;
            c.tribe = TribeId::getGreenskin();
            c.inhabitants.fighters = CreatureList(random.get(4, 8), CreatureId::HARPY);
            c.buildingId = BuildingId::DUNGEON;
            c.closeToPlayer = true;
            c.furniture = FurnitureFactory::roomFurniture(c.tribe);
            c.outsideFeatures = FurnitureFactory::villageOutside(c.tribe);),
          CollectiveConfig::withImmigrants(300_visible, 10))
          .setImmigrants({ ImmigrantInfo(CreatureId::HARPY, {MinionTrait::FIGHTER}).setFrequency(1) });
      }
    case EnemyId::SOKOBAN_ENTRY:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::ISLAND_VAULT_DOOR;
            c.buildingId = BuildingId::DUNGEON;
            ), CollectiveConfig::noImmigrants());
    case EnemyId::SOKOBAN:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::CAVE;
            c.inhabitants.leader = random.choose(
                CreatureId::SPECIAL_HLBN, CreatureId::SPECIAL_HLBW, CreatureId::SPECIAL_HLGN, CreatureId::SPECIAL_HLGW);
            c.tribe = TribeId::getDarkKeeper();
            c.buildingId = BuildingId::DUNGEON;
            ), CollectiveConfig::noImmigrants(), none,
          LevelConnection{LevelConnection::SOKOBAN, get(EnemyId::SOKOBAN_ENTRY)})
          .setNonDiscoverable();
    case EnemyId::WITCH:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::WITCH_HOUSE;
            c.tribe = TribeId::getMonster();
            c.inhabitants.leader = CreatureId::WITCH;
            c.race = "witch"_s;
            c.buildingId = BuildingId::WOOD;
            c.elderLoot = ItemType(ItemType::TechBook{"advanced alchemy"});
            c.furniture = FurnitureFactory(c.tribe, FurnitureType::LABORATORY);), CollectiveConfig::noImmigrants());
    case EnemyId::RUINS:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::COTTAGE;
            c.tribe = TribeId::getMonster();
            c.race = "ruins"_s;
            c.dontBuildRoad = true;
            c.closeToPlayer = true;
            c.buildingId = BuildingId::RUINS;
            ), CollectiveConfig::withImmigrants(400_visible, 3).setConquerCondition(ConquerCondition::DESTROY_BUILDINGS))
            .setImmigrants({ ImmigrantInfo(CreatureId::LOST_SOUL, {MinionTrait::FIGHTER})
                .addSpecialTrait(1.0, LastingEffect::DISAPPEAR_DURING_DAY)
                .setFrequency(1)
                .addRequirement(1.0, SunlightState::NIGHT)
                .setSpawnLocation(InsideTerritory{})});
    case EnemyId::HUMAN_COTTAGE:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::COTTAGE;
            c.tribe = TribeId::getHuman();
            c.cropsDistance = 13;
            c.inhabitants.fighters = CreatureList(random.get(2, 4), CreatureId::PESEANT);
            c.inhabitants.civilians = CreatureList(
                random.get(3, 7),
                makeVec(CreatureId::CHILD, CreatureId::HORSE, CreatureId::DONKEY, CreatureId::COW,
                    CreatureId::PIG, CreatureId::DOG));
            c.race = "humans"_s;
            c.buildingId = BuildingId::WOOD;
            c.furniture = FurnitureFactory::roomFurniture(c.tribe);),
          CollectiveConfig::noImmigrants().setGuardian({CreatureId::WITCHMAN, 0.001, 1, 2}));
    case EnemyId::TUTORIAL_VILLAGE:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::SMALL_VILLAGE;
            c.tribe = TribeId::getHuman();
            c.inhabitants.fighters = CreatureList(random.get(2, 4), CreatureId::PESEANT);
            c.inhabitants.civilians = CreatureList(
                random.get(3, 7),
                makeVec(CreatureId::CHILD, CreatureId::HORSE, CreatureId::DONKEY, CreatureId::COW,
                    CreatureId::PIG, CreatureId::DOG));
            c.race = "humans"_s;
            c.buildingId = BuildingId::WOOD;
            c.stockpiles = LIST({StockpileInfo::GOLD, 50});
            c.cropsDistance = 16;
            c.furniture = FurnitureFactory::roomFurniture(c.tribe);),
          CollectiveConfig::noImmigrants());
    case EnemyId::ELVEN_COTTAGE:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::FORREST_COTTAGE;
            c.tribe = TribeId::getElf();
            c.inhabitants.leader = CreatureId::ELF_ARCHER;
            c.inhabitants.fighters = CreatureList(random.get(2, 3), CreatureId::ELF);
            c.inhabitants.civilians = CreatureList(
                random.get(2, 5),
                makeVec(CreatureId::ELF_CHILD, CreatureId::HORSE, CreatureId::COW, CreatureId::DOG));
            c.race = "elves"_s;
            c.buildingId = BuildingId::WOOD;
            c.furniture = FurnitureFactory::roomFurniture(c.tribe);),
          CollectiveConfig::noImmigrants());
    case EnemyId::LIZARDMEN_COTTAGE:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::FORREST_COTTAGE;
            c.tribe = TribeId::getLizard();
            c.inhabitants.fighters = CreatureList(random.get(2, 3), CreatureId::LIZARDMAN);
            c.race = "lizardmen"_s;
            c.buildingId = BuildingId::WOOD;
            c.furniture = FurnitureFactory::roomFurniture(c.tribe);),
          CollectiveConfig::noImmigrants())
          .setCreateOnBones(*this, 1.0, {EnemyId::ELVEN_COTTAGE});
    case EnemyId::KOBOLD_CAVE:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::SMALL_MINETOWN;
            c.tribe = TribeId::getDwarf();
            c.inhabitants.fighters = CreatureList(random.get(3, 7), CreatureId::KOBOLD);
            c.race = "kobolds"_s;
            c.buildingId = BuildingId::DUNGEON;
            c.stockpiles = LIST({StockpileInfo::MINERALS, 60});),
          CollectiveConfig::noImmigrants());
    case EnemyId::DWARF_CAVE:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::SMALL_MINETOWN;
            c.tribe = TribeId::getDwarf();
            c.inhabitants.fighters = CreatureList(random.get(2, 5), CreatureId::DWARF);
            c.inhabitants.civilians = CreatureList(random.get(2, 5), CreatureId::DWARF_FEMALE);
            c.race = "dwarves"_s;
            c.buildingId = BuildingId::DUNGEON;
            c.stockpiles = LIST(random.choose(StockpileInfo{StockpileInfo::MINERALS, 60},
                StockpileInfo{StockpileInfo::GOLD, 60}));
            c.outsideFeatures = FurnitureFactory::dungeonOutside(c.tribe);
            c.furniture = FurnitureFactory::roomFurniture(c.tribe);
            c.surroundWithResources = 6;
          ),
          CollectiveConfig::noImmigrants(),
          CONSTRUCT(VillageBehaviour,
            c.minPopulation = 0;
            c.minTeamSize = 1;
            c.triggers = LIST(AttackTriggerId::SELF_VICTIMS, AttackTriggerId::STOLEN_ITEMS, AttackTriggerId::MINING_IN_PROXIMITY);
            c.attackBehaviour = AttackBehaviour(AttackBehaviourId::KILL_LEADER);
            c.ransom = make_pair(0.5, random.get(40, 80));));
    case EnemyId::DARK_ELF_CAVE:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::SMALL_MINETOWN;
            c.tribe = TribeId::getDarkElf();
            c.inhabitants.fighters = CreatureList(random.get(2, 5), CreatureId::DARK_ELF_WARRIOR)
                .increaseBaseLevel({{ExperienceType::MELEE, 7}});
            c.inhabitants.civilians = CreatureList(random.get(2, 5), CreatureId::DARK_ELF);
            c.race = "dark elves"_s;
            c.buildingId = BuildingId::DUNGEON;
            c.stockpiles = LIST(random.choose(StockpileInfo{StockpileInfo::MINERALS, 60},
                StockpileInfo{StockpileInfo::GOLD, 60}));
            c.outsideFeatures = FurnitureFactory::dungeonOutside(c.tribe);
            c.furniture = FurnitureFactory::roomFurniture(c.tribe);
            c.surroundWithResources = 6;
          ),
          CollectiveConfig::noImmigrants(),
          CONSTRUCT(VillageBehaviour,
            c.minPopulation = 0;
            c.minTeamSize = 1;
            c.triggers = LIST(AttackTriggerId::SELF_VICTIMS, AttackTriggerId::STOLEN_ITEMS, AttackTriggerId::MINING_IN_PROXIMITY);
            c.attackBehaviour = AttackBehaviour(AttackBehaviourId::KILL_LEADER);
            c.ransom = make_pair(0.5, random.get(40, 80));));
  }
}

vector<ExternalEnemy> EnemyFactory::getExternalEnemies() {
  return {
    ExternalEnemy{
        CreatureList(1, CreatureId::BANDIT)
            .increaseBaseLevel({{ExperienceType::MELEE, -2}}),
        AttackBehaviourId::KILL_LEADER,
        "a curious bandit",
        Range(1000, 10000),
        1
    },
    ExternalEnemy{
        CreatureList(1, CreatureId::LIZARDMAN)
            .increaseBaseLevel({{ExperienceType::MELEE, 1}}),
        AttackBehaviourId::KILL_LEADER,
        "a lizardman scout",
        Range(1000, 10000),
        1
    },
    ExternalEnemy{
        CreatureList(random.get(3, 5), CreatureId::ANT_WORKER)
            .increaseBaseLevel({{ExperienceType::MELEE, -2}}),
        AttackBehaviourId::KILL_LEADER,
        "an ant patrol",
        Range(1000, 5000),
        2
    },
    ExternalEnemy{
        CreatureList(random.get(4, 8), CreatureId::LIZARDMAN)
            .increaseBaseLevel({{ExperienceType::MELEE, 2}}),
        AttackBehaviourId::KILL_LEADER,
        "a nest of lizardmen",
        Range(6000, 15000),
        5
    },
    ExternalEnemy{
        CreatureList(random.get(3, 5), CreatureId::BANDIT)
            .increaseBaseLevel({{ExperienceType::MELEE, 9}}),
        AttackBehaviourId::KILL_LEADER,
        "a gang of bandits",
        Range(8000, 20000),
        10
    },
    ExternalEnemy{
        CreatureList(random.get(3, 5), CreatureId::ANT_SOLDIER),
        AttackBehaviourId::KILL_LEADER,
        "an ant soldier patrol",
        Range(5000, 10000),
        1
    },
    ExternalEnemy{
        CreatureList(random.get(3, 5), CreatureId::CLAY_GOLEM)
            .increaseBaseLevel({{ExperienceType::MELEE, 3}}),
        AttackBehaviourId::KILL_LEADER,
        "an alchemist's clay golems",
        Range(3000, 10000),
        1
    },
    ExternalEnemy{
        CreatureList(random.get(3, 5), CreatureId::STONE_GOLEM)
            .increaseBaseLevel({{ExperienceType::MELEE, 3}}),
        AttackBehaviourId::KILL_LEADER,
        "an alchemist's stone golems",
        Range(5000, 12000),
        2
    },
    ExternalEnemy{
        CreatureList(random.get(5, 8), CreatureId::ANT_SOLDIER)
            .increaseBaseLevel({{ExperienceType::MELEE, 5}}),
        AttackBehaviourId::KILL_LEADER,
        "an ant soldier patrol",
        Range(6000, 12000),
        2
    },
    ExternalEnemy{
        CreatureList(random.get(10, 15), CreatureId::ANT_SOLDIER).addUnique(CreatureId::ANT_QUEEN)
            .increaseBaseLevel({{ExperienceType::MELEE, 6}}),
        AttackBehaviourId::KILL_LEADER,
        "an army of ants",
        Range(10000, 15000),
        3
    },
    ExternalEnemy{
        CreatureList(random.get(4, 8), CreatureId::ENT)
            .increaseBaseLevel({{ExperienceType::MELEE, 3}}),
        AttackBehaviourId::KILL_LEADER,
        "a group of ents",
        Range(7000, 12000),
        2
    },
    ExternalEnemy{
        CreatureList(random.get(4, 8), CreatureId::DWARF),
        AttackBehaviourId::KILL_LEADER,
        "a band of dwarves",
        Range(7000, 15000),
        3
    },
    ExternalEnemy{
        CreatureList(random.get(3, 5), CreatureId::IRON_GOLEM)
            .increaseBaseLevel({{ExperienceType::MELEE, 2}}),
        AttackBehaviourId::KILL_LEADER,
        "an alchemist's iron golems",
        Range(9000, 15000),
        3
    },
    ExternalEnemy{
        CreatureList(random.get(10, 13), CreatureId::DWARF).addUnique(CreatureId::DWARF_BARON)
            .increaseBaseLevel({{ExperienceType::MELEE, 12}})
            .addInventory({ItemType::Scroll{Effect::DestroyWalls{}}}),
        AttackBehaviourId::KILL_LEADER,
        "a dwarf tribe",
        Range(12000, 25000),
        10
    },
    ExternalEnemy{
        CreatureList(random.get(4, 8), CreatureId::ARCHER),
        AttackBehaviourId::KILL_LEADER,
        "a patrol of archers",
        Range(6000, 12000),
        3
    },
    ExternalEnemy{
        CreatureList(1, CreatureId::KNIGHT),
        AttackBehaviourId::KILL_LEADER,
        "a lonely knight",
        Range(10000, 16000),
        3
    },
    ExternalEnemy{
        CreatureList(random.get(4, 8), CreatureId::WARRIOR)
            .increaseBaseLevel({{ExperienceType::MELEE, 6}}),
        AttackBehaviourId::KILL_LEADER,
        "a group of warriors",
        Range(6000, 12000),
        3
    },
    ExternalEnemy{
        CreatureList(random.get(12, 16), CreatureId::WARRIOR).addUnique(CreatureId::SHAMAN)
            .increaseBaseLevel({{ExperienceType::MELEE, 14}})
            .addInventory({ItemType::Scroll{Effect::DestroyWalls{}}}),
        AttackBehaviourId::KILL_LEADER,
        "an army of warriors",
        Range(18000, 100000),
        100
    },
    ExternalEnemy{
        CreatureList(random.get(12, 16), {make_pair(2, CreatureId::KNIGHT), make_pair(1, CreatureId::ARCHER)})
            .addUnique(CreatureId::DUKE)
            //.increaseBaseLevel({{ExperienceType::MELEE, 4}})
            .addInventory({ItemType::Scroll{Effect::DestroyWalls{}}}),
        AttackBehaviourId::KILL_LEADER,
        "an army of knights",
        Range(20000, 100000),
        100
    },
    ExternalEnemy{
        CreatureList(1, CreatureId::CYCLOPS),
        AttackBehaviourId::KILL_LEADER,
        "a cyclops",
        Range(6000, 20000),
        4
    },
    ExternalEnemy{
        CreatureList(1, CreatureId::WITCHMAN)
            .increaseBaseLevel({{ExperienceType::MELEE, 20}}),
        AttackBehaviourId::KILL_LEADER,
        "a witchman",
        Range(15000, 40000),
        5
    },
    ExternalEnemy{
        CreatureList(1, CreatureId::MINOTAUR)
            .increaseBaseLevel({{ExperienceType::MELEE, 20}}),
        AttackBehaviourId::KILL_LEADER,
        "a minotaur",
        Range(15000, 100000),
        100
    },
    ExternalEnemy{
        CreatureList(1, CreatureId::ELEMENTALIST),
        {AttackBehaviourId::CAMP_AND_SPAWN, CreatureFactory::elementals(TribeId::getHuman())},
        "an elementalist",
        Range(8000, 14000),
        1
    },
    ExternalEnemy{
        CreatureList(1, CreatureId::GREEN_DRAGON)
            .increaseBaseLevel({{ExperienceType::MELEE, 10}}),
        AttackBehaviourId::KILL_LEADER,
        "a green dragon",
        Range(15000, 40000),
        100
    },
    ExternalEnemy{
        CreatureList(1, CreatureId::RED_DRAGON)
            .increaseBaseLevel({{ExperienceType::MELEE, 18}}),
        AttackBehaviourId::KILL_LEADER,
        "a red dragon",
        Range(19000, 100000),
        100
    },
    ExternalEnemy{
        CreatureList(random.get(3, 6), {CreatureId::ADVENTURER_F, CreatureId::ADVENTURER})
            .addInventory({ItemType::Scroll{Effect::DestroyWalls{}}})
            .increaseBaseLevel({{ExperienceType::MELEE, 20}}),
        AttackBehaviourId::KILL_LEADER,
        "a group of adventurers",
        Range(10000, 25000),
        100
    },
    ExternalEnemy{
        CreatureList(random.get(3, 6), CreatureId::OGRE)
            .increaseBaseLevel({{ExperienceType::MELEE, 25}}),
        AttackBehaviourId::KILL_LEADER,
        "a pack of ogres",
        Range(15000, 50000),
        100
    },
  };
}

vector<ExternalEnemy> EnemyFactory::getHalloweenKids() {
  return {
    ExternalEnemy{
        CreatureList(random.get(3, 6), CreatureId::HALLOWEEN_KID),
        AttackBehaviourId::HALLOWEEN_KIDS,
        "kids...?",
        Range(0, 10000),
        1
    }};
}
