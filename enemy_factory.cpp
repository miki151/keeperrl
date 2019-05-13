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
#include "creature_group.h"
#include "name_generator.h"

EnemyFactory::EnemyFactory(RandomGen& r, NameGenerator* n) : random(r), nameGenerator(n) {
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


EnemyInfo& EnemyInfo::setCreateOnBones(const EnemyFactory& factory, double prob, vector<EnemyId> enemies) {
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
      {"ORC", 3, 5},
      {"OGRE", 2, 4},
      {"VAMPIRE", 2, 4},
  }},
  {TribeAlignment::LAWFUL, {
      {"IRON_GOLEM", 3, 5},
      {"EARTH_ELEMENTAL", 2, 4},
  }},
};

static vector<VaultInfo> hostileVaults {
  {"SPIDER", 3, 8},
  {"SNAKE", 3, 8},
  {"BAT", 3, 8},
};

vector<EnemyInfo> EnemyFactory::getVaults(TribeAlignment alignment, TribeId allied) const {
  vector<EnemyInfo> ret {
 /*   getVault(SettlementType::VAULT, CreatureGroup::insects(TribeId::getMonster()),
        TribeId::getMonster(), random.get(6, 12)),*/
    getVault(SettlementType::VAULT, "RAT", TribeId::getPest(), random.get(3, 8),
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

string EnemyFactory::getVillageName() const {
  return nameGenerator->getNext(NameGeneratorId::TOWN);
}

EnemyInfo EnemyFactory::get(EnemyId id) const {
  return getById(id).setId(id);
}

EnemyInfo EnemyFactory::getById(EnemyId enemyId) const {
  switch (enemyId) {
     case EnemyId::UNICORN_HERD:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::FOREST;
            c.inhabitants.fighters = CreatureList(random.get(5, 8), "UNICORN");
            c.tribe = TribeId::getMonster();
            c.race = "unicorns"_s;
            c.buildingId = BuildingId::WOOD;),
          CollectiveConfig::withImmigrants(200_visible, 9))
          .setImmigrants({ImmigrantInfo("UNICORN", {MinionTrait::FIGHTER}).setFrequency(1)});
    case EnemyId::ANTS_CLOSED:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::ANT_NEST;
            c.inhabitants.leader = CreatureList("ANT_QUEEN");
            c.inhabitants.civilians = CreatureList(random.get(5, 7), "ANT_WORKER");
            c.inhabitants.fighters = CreatureList(random.get(5, 7), "ANT_SOLDIER");
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
              ImmigrantInfo("ANT_WORKER", {}).setFrequency(1),
              ImmigrantInfo("ANT_SOLDIER", {MinionTrait::FIGHTER}).setFrequency(1)});
    case EnemyId::ANTS_CLOSED_SMALL:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::SMALL_MINETOWN;
            c.inhabitants.civilians = CreatureList(random.get(2, 5), "ANT_WORKER");
            c.inhabitants.fighters = CreatureList(random.get(3, 5), "ANT_SOLDIER");
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
            c.inhabitants.fighters = CreatureList(numGolems, "ADA_GOLEM");
            c.tribe = TribeId::getAnt();
            c.race = "adamantine golems"_s;
            c.dontConnectCave = true;
            c.surroundWithResources = 3;
            c.extraResources = FurnitureType("ADAMANTIUM_ORE");
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
                makeVec<CreatureId>("ORC", "OGRE"));
            c.locationName = getVillageName();
            c.race = "greenskins"_s;
            c.buildingId = BuildingId::BRICK;
            c.furniture = FurnitureListId("roomFurniture");
            c.outsideFeatures = FurnitureListId("villageOutside");
          ),
          CollectiveConfig::withImmigrants(300_visible, 16))
        .setCreateOnBones(*this, 0.1, {EnemyId::DWARVES, EnemyId::VILLAGE})
        .setImmigrants({
          ImmigrantInfo("ORC", {MinionTrait::FIGHTER}).setFrequency(3),
          ImmigrantInfo("OGRE", {MinionTrait::FIGHTER}).setFrequency(1)
        });
    case EnemyId::DEMON_DEN_ABOVE:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::VILLAGE;
            c.tribe = TribeId::getMonster();
            c.inhabitants.fighters = CreatureList(random.get(2, 3), "LOST_SOUL");
            c.buildingId = BuildingId::DUNGEON_SURFACE;
            c.locationName = "Darkshrine Town"_s;
            c.race = "ghosts"_s;
            c.furniture = FurnitureListId("dungeonOutside");
            ),
            CollectiveConfig::noImmigrants()).setNonDiscoverable();
    case EnemyId::DEMON_DEN:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.tribe = TribeId::getMonster();
            c.inhabitants.leader = CreatureList("DEMON_LORD");
            c.inhabitants.fighters = CreatureList(random.get(5, 10), "DEMON_DWELLER");
            c.locationName = "Darkshrine"_s;
            c.race = "demons"_s;
            c.furniture = FurnitureListId("dungeonOutside");
            c.outsideFeatures = FurnitureListId("dungeonOutside");),
          CollectiveConfig::withImmigrants(300_visible, 16),
          CONSTRUCT(VillageBehaviour,
              c.minPopulation = 0;
              c.minTeamSize = 3;
              c.triggers = LIST(
                  {AttackTriggerId::ROOM_BUILT, RoomTriggerInfo{FurnitureType("DEMON_SHRINE"), 0.0001}},
              );
              c.attackBehaviour = AttackBehaviour(AttackBehaviourId::KILL_LEADER);
              c.ransom = make_pair(0.5, random.get(50, 100));),
            LevelConnection{LevelConnection::CRYPT, get(EnemyId::DEMON_DEN_ABOVE).setCreateOnBones(*this, 1.0, {EnemyId::KNIGHTS})})
          .setImmigrants({
            ImmigrantInfo("DEMON_DWELLER", {MinionTrait::FIGHTER}).setFrequency(1),
          });
    case EnemyId::VILLAGE:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::VILLAGE;
            c.tribe = TribeId::getHuman();
            c.inhabitants.leader = CreatureList("KNIGHT");
            c.inhabitants.fighters = CreatureList(
                random.get(4, 8),
                makeVec<CreatureId>("KNIGHT", "ARCHER"));
            c.inhabitants.civilians = CreatureList(
                random.get(4, 8),
                makeVec<CreatureId>("PESEANT", "CHILD", "DONKEY", "HORSE",
                    "COW", "PIG", "DOG"));
            c.locationName = getVillageName();
            c.race = "humans"_s;
            c.buildingId = BuildingId::WOOD;
            c.shopFactory = ItemFactory::armory();
            c.furniture = FurnitureListId("roomFurniture");
            ), CollectiveConfig::noImmigrants().setGhostSpawns(0.1, 4));
    case EnemyId::WARRIORS:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::CASTLE2;
            c.tribe = TribeId::getHuman();
            c.inhabitants.leader = CreatureList("SHAMAN");
            c.inhabitants.fighters = CreatureList(random.get(7, 10), "WARRIOR");
            c.locationName = getVillageName();
            c.race = "humans"_s;
            c.buildingId = BuildingId::WOOD_CASTLE;
            c.stockpiles = LIST({StockpileInfo::GOLD, 160});
            c.elderLoot = ItemType(ItemType::TechBook{"beast mutation"});
            c.furniture = FurnitureListId("roomFurniture");
            c.outsideFeatures = FurnitureListId("castleOutside");),
          CollectiveConfig::withImmigrants(300_visible, 10).setGhostSpawns(0.1, 6),
          CONSTRUCT(VillageBehaviour,
              c.minPopulation = 3;
              c.minTeamSize = 5;
              c.triggers = LIST(
                  {AttackTriggerId::ROOM_BUILT, RoomTriggerInfo{FurnitureType("THRONE"), 0.0003}},
                  {AttackTriggerId::ROOM_BUILT, RoomTriggerInfo{FurnitureType("IMPALED_HEAD"), 0.0001}},
                  AttackTriggerId::SELF_VICTIMS,
                  {AttackTriggerId::NUM_CONQUERED, 2},
                  AttackTriggerId::STOLEN_ITEMS,
                  AttackTriggerId::FINISH_OFF,
                  AttackTriggerId::PROXIMITY);
              c.attackBehaviour = AttackBehaviour(AttackBehaviourId::KILL_LEADER);
              c.ransom = make_pair(0.8, random.get(100, 140));))
           .setImmigrants({
             ImmigrantInfo("WARRIOR", {MinionTrait::FIGHTER}).setFrequency(1),
           });
    case EnemyId::KNIGHTS:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::CASTLE;
            c.tribe = TribeId::getHuman();
            c.inhabitants.leader = CreatureList("DUKE");
            c.inhabitants.fighters = CreatureList(
                random.get(12, 17),
                makeVec(make_pair<int, CreatureId>(1, "PRIEST"),
                    make_pair<int, CreatureId>(3, "KNIGHT"),
                    make_pair<int, CreatureId>(1, "ARCHER")));
            c.inhabitants.civilians = CreatureList(
                random.get(6, 9),
                makeVec<CreatureId>("PESEANT", "CHILD", "DONKEY", "HORSE",
                    "COW", "PIG", "DOG"));
            c.locationName = getVillageName();
            c.race = "humans"_s;
            c.stockpiles = LIST({StockpileInfo::GOLD, 140});
            c.buildingId = BuildingId::BRICK;
            c.shopFactory = ItemFactory::villageShop();
            c.furniture = FurnitureListId("castleFurniture");
            c.outsideFeatures = FurnitureListId("castleOutside");),
          CollectiveConfig::withImmigrants(300_visible, 26).setGhostSpawns(0.1, 6),
          CONSTRUCT(VillageBehaviour,
              c.minPopulation = 12;
              c.minTeamSize = 10;
              c.triggers = LIST(
                  {AttackTriggerId::ROOM_BUILT, RoomTriggerInfo{FurnitureType("THRONE"), 0.0003}},
                  {AttackTriggerId::ROOM_BUILT, RoomTriggerInfo{FurnitureType("IMPALED_HEAD"), 0.0001}},
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
            ImmigrantInfo("ARCHER", {MinionTrait::FIGHTER}).setFrequency(1),
            ImmigrantInfo("KNIGHT", {MinionTrait::FIGHTER}).setFrequency(1)
          });
    case EnemyId::MINOTAUR:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::CAVE;
            c.tribe = TribeId::getMonster();
            c.inhabitants.leader = CreatureList("MINOTAUR");
            c.locationName = "maze"_s;
            c.race = "monsters"_s;
            c.furniture = FurnitureListId("roomFurniture");
            c.buildingId = BuildingId::BRICK;), CollectiveConfig::noImmigrants());
    case EnemyId::RED_DRAGON:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::CAVE;
            c.inhabitants.leader = CreatureList("RED_DRAGON");
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
            c.inhabitants.leader = CreatureList("GREEN_DRAGON");
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
            c.inhabitants.leader = CreatureList("DWARF_BARON");
            c.inhabitants.fighters = CreatureList(random.get(6, 9), "DWARF").increaseBaseLevel({{ExperienceType::MELEE, 10}});
            c.inhabitants.civilians = CreatureList(
                random.get(3, 5),
                makeVec(make_pair<int, CreatureId>(2, "DWARF_FEMALE"), make_pair<int, CreatureId>(1, "RAT")));
            c.locationName = getVillageName();
            c.race = "dwarves"_s;
            c.buildingId = BuildingId::DUNGEON;
            c.stockpiles = LIST({StockpileInfo::GOLD, 200}, {StockpileInfo::MINERALS, 120});
            c.shopFactory = ItemFactory::dwarfShop();
            c.outsideFeatures = FurnitureListId("dungeonOutside");
            c.surroundWithResources = 5;
            c.furniture = FurnitureListId("roomFurniture");),
          CollectiveConfig::withImmigrants(500_visible, 15).setGhostSpawns(0.1, 4),
          CONSTRUCT(VillageBehaviour,
            c.minPopulation = 3;
            c.minTeamSize = 4;
            c.triggers = LIST(
                {AttackTriggerId::ROOM_BUILT, RoomTriggerInfo{FurnitureType("THRONE"), 0.0003}},
                {AttackTriggerId::ROOM_BUILT, RoomTriggerInfo{FurnitureType("IMPALED_HEAD"), 0.0001}},
                AttackTriggerId::SELF_VICTIMS,
                AttackTriggerId::STOLEN_ITEMS,
                {AttackTriggerId::NUM_CONQUERED, 3},
                AttackTriggerId::FINISH_OFF,
                AttackTriggerId::PROXIMITY);
            c.attackBehaviour = AttackBehaviour(AttackBehaviourId::KILL_MEMBERS, 3);
            c.ransom = make_pair(0.8, random.get(240, 320));))
          .setImmigrants({
            ImmigrantInfo("DWARF", {MinionTrait::FIGHTER}).setFrequency(1),
          });
    case EnemyId::ELVES:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::FORREST_VILLAGE;
            c.inhabitants.leader = CreatureList("ELF_LORD");
            c.inhabitants.fighters = CreatureList(random.get(6, 9), "ELF_ARCHER");
            c.inhabitants.civilians = CreatureList(
                random.get(6, 9),
                makeVec<CreatureId>("ELF", "ELF_CHILD", "HORSE"));
            c.locationName = getVillageName();
            c.tribe = TribeId::getElf();
            c.race = "elves"_s;
            c.stockpiles = LIST({StockpileInfo::GOLD, 100});
            c.buildingId = BuildingId::WOOD;
            c.elderLoot = ItemType(ItemType::TechBook{"master sorcery"});
            c.furniture = FurnitureListId("roomFurniture");
          ),
          CollectiveConfig::withImmigrants(500_visible, 18).setGhostSpawns(0.1, 4),
          CONSTRUCT(VillageBehaviour,
              c.minPopulation = 4;
              c.minTeamSize = 4;
              c.triggers = LIST(AttackTriggerId::STOLEN_ITEMS);
              c.attackBehaviour = AttackBehaviour(AttackBehaviourId::KILL_LEADER);))
          .setImmigrants({
            ImmigrantInfo("ELF_ARCHER", {MinionTrait::FIGHTER}).setFrequency(1),
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
            c.inhabitants.leader = CreatureList("ELEMENTALIST");
            c.tribe = TribeId::getHuman();
            c.buildingId = BuildingId::BRICK;
            c.furniture = FurnitureListId("roomFurniture");),
          CollectiveConfig::noImmigrants().setLeaderAsFighter(),
          CONSTRUCT(VillageBehaviour,
              c.minPopulation = 0;
              c.minTeamSize = 1;
              c.triggers = LIST(
                  {AttackTriggerId::ROOM_BUILT, RoomTriggerInfo{FurnitureType("THRONE"), 0.0003}},
                  {AttackTriggerId::ROOM_BUILT, RoomTriggerInfo{FurnitureType("IMPALED_HEAD"), 0.0001}},
                  AttackTriggerId::PROXIMITY,
                  AttackTriggerId::FINISH_OFF,
                  {AttackTriggerId::NUM_CONQUERED, 3});
              c.attackBehaviour = AttackBehaviour(AttackBehaviourId::CAMP_AND_SPAWN,
                CreatureGroup::elementals(TribeId::getHuman()));
              c.ransom = make_pair(0.5, random.get(40, 80));),
          LevelConnection{LevelConnection::TOWER, get(EnemyId::ELEMENTALIST_ENTRY)});
    case EnemyId::NO_AGGRO_BANDITS:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::CAVE;
            c.inhabitants.fighters = CreatureList(random.get(4, 9), "BANDIT");
            c.tribe = TribeId::getBandit();
            c.race = "bandits"_s;
            c.buildingId = BuildingId::DUNGEON;),
          CollectiveConfig::withImmigrants(1000_visible, 10))
         .setCreateOnBones(*this, 0.1, {EnemyId::KOBOLD_CAVE})
         .setImmigrants({ ImmigrantInfo("BANDIT", {MinionTrait::FIGHTER}).setFrequency(1) });
    case EnemyId::BANDITS:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::CAVE;
            c.inhabitants.fighters = CreatureList(random.get(4, 9), "BANDIT");
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
          .setImmigrants({ ImmigrantInfo("BANDIT", {MinionTrait::FIGHTER}).setFrequency(1) });
    case EnemyId::COTTAGE_BANDITS:
      return getById(EnemyId::NO_AGGRO_BANDITS).setCreateOnBones(*this, 1.0, {EnemyId::HUMAN_COTTAGE});
    case EnemyId::ORC_CAVE:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::CAVE;
            c.inhabitants.fighters = CreatureList(random.get(4, 9), "ORC");
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
          .setImmigrants({ ImmigrantInfo("ORC", {MinionTrait::FIGHTER}).setFrequency(1) });
    case EnemyId::LIZARDMEN:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::VILLAGE;
            c.tribe = TribeId::getLizard();
            c.inhabitants.leader = CreatureList("LIZARDLORD");
            c.inhabitants.fighters = CreatureList(random.get(7, 10), "LIZARDMAN");
            c.locationName = getVillageName();
            c.race = "lizardmen"_s;
            c.buildingId = BuildingId::MUD;
            c.elderLoot = ItemType(ItemType::TechBook{"humanoid mutation"});
            c.shopFactory = ItemFactory::mushrooms();
            c.furniture = FurnitureListId("roomFurniture");
            c.outsideFeatures = FurnitureListId("villageOutside");),
          CollectiveConfig::withImmigrants(140_visible, 11).setGhostSpawns(0.1, 4),
          CONSTRUCT(VillageBehaviour,
              c.minPopulation = 4;
              c.minTeamSize = 4;
              c.triggers = LIST(
                  AttackTriggerId::POWER,
                  AttackTriggerId::SELF_VICTIMS,
                  AttackTriggerId::STOLEN_ITEMS,
                  {AttackTriggerId::ROOM_BUILT, RoomTriggerInfo{FurnitureType("IMPALED_HEAD"), 0.0001}},
                  {AttackTriggerId::NUM_CONQUERED, 2},
                  AttackTriggerId::FINISH_OFF,
                  AttackTriggerId::PROXIMITY);
              c.attackBehaviour = AttackBehaviour(AttackBehaviourId::KILL_LEADER);))
          .setCreateOnBones(*this, 1.0, {EnemyId::VILLAGE, EnemyId::ELVES})
          .setImmigrants({ ImmigrantInfo("LIZARDMAN", {MinionTrait::FIGHTER}).setFrequency(1) });
    case EnemyId::DARK_ELVES_ALLY:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::MINETOWN;
            c.tribe = TribeId::getDarkElf();
            c.inhabitants.leader = CreatureList("DARK_ELF_LORD");
            c.inhabitants.fighters = CreatureList(random.get(6, 9), "DARK_ELF_WARRIOR");
            c.inhabitants.civilians = CreatureList(
                random.get(6, 9),
                makeVec<CreatureId>("DARK_ELF", "DARK_ELF_CHILD", "RAT"));
            c.locationName = getVillageName();
            c.race = "dark elves"_s;
            c.buildingId = BuildingId::DUNGEON;
            c.outsideFeatures = FurnitureListId("dungeonOutside");
            c.furniture = FurnitureListId("roomFurniture");),
          CollectiveConfig::withImmigrants(500_visible, 15), none,
          LevelConnection{LevelConnection::GNOMISH_MINES, get(EnemyId::DARK_ELVES_ENTRY)})
          .setImmigrants({ ImmigrantInfo("DARK_ELF_WARRIOR", {MinionTrait::FIGHTER}).setFrequency(1) });
    case EnemyId::DARK_ELVES_ENEMY:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::MINETOWN;
            c.tribe = TribeId::getDarkElf();
            c.inhabitants.leader = CreatureList("DARK_ELF_LORD");
            c.inhabitants.fighters = CreatureList(random.get(6, 9), "DARK_ELF_WARRIOR");
            c.inhabitants.civilians = CreatureList(
                random.get(6, 9),
                makeVec<CreatureId>("DARK_ELF", "DARK_ELF_CHILD", "RAT"));
            c.inhabitants.leader.increaseBaseLevel({{ExperienceType::MELEE, 10}, {ExperienceType::SPELL, 10}});
            c.inhabitants.fighters.increaseBaseLevel({{ExperienceType::MELEE, 10}});
            c.locationName = getVillageName();
            c.race = "dark elves"_s;
            c.buildingId = BuildingId::DUNGEON;
            c.outsideFeatures = FurnitureListId("dungeonOutside");
            c.furniture = FurnitureListId("roomFurniture");),
          CollectiveConfig::withImmigrants(500_visible, 15), none,
          LevelConnection{LevelConnection::GNOMISH_MINES, get(EnemyId::DARK_ELVES_ENTRY)})
          .setImmigrants({ ImmigrantInfo("DARK_ELF_WARRIOR", {MinionTrait::FIGHTER}).setFrequency(1) });
    case EnemyId::DARK_ELVES_ENTRY:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::SMALL_MINETOWN;
            c.tribe = TribeId::getDarkElf();
            c.inhabitants.fighters = CreatureList(random.get(3, 7), "DARK_ELF_WARRIOR");
            c.race = "dark elves"_s;
            c.buildingId = BuildingId::DUNGEON;
            c.outsideFeatures = FurnitureListId("dungeonOutside");
            c.furniture = FurnitureListId("roomFurniture");),
          CollectiveConfig::noImmigrants(), {})
          .setNonDiscoverable();
    case EnemyId::GNOMES:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::MINETOWN;
            c.tribe = TribeId::getGnome();
            c.inhabitants.leader = CreatureList("GNOME_CHIEF");
            c.inhabitants.fighters = CreatureList(random.get(8, 14), "GNOME");
            c.locationName = getVillageName();
            c.race = "gnomes"_s;
            c.buildingId = BuildingId::DUNGEON;
            c.shopFactory = ItemFactory::gnomeShop();
            c.outsideFeatures = FurnitureListId("dungeonOutside");
            c.furniture = FurnitureListId("roomFurniture");),
          CollectiveConfig::noImmigrants(), none,
          LevelConnection{LevelConnection::GNOMISH_MINES, get(EnemyId::GNOMES_ENTRY)});
    case EnemyId::GNOMES_ENTRY:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::SMALL_MINETOWN;
            c.tribe = TribeId::getGnome();
            c.inhabitants.fighters = CreatureList(random.get(3, 7), "GNOME");
            c.race = "gnomes"_s;
            c.buildingId = BuildingId::DUNGEON;
            c.outsideFeatures = FurnitureListId("dungeonOutside");
            c.furniture = FurnitureListId("roomFurniture");),
          CollectiveConfig::noImmigrants())
          .setNonDiscoverable();
    case EnemyId::ENTS:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::FOREST;
            c.inhabitants.fighters = CreatureList(random.get(7, 10), "ENT");
            c.tribe = TribeId::getMonster();
            c.race = "tree spirits"_s;
            c.buildingId = BuildingId::WOOD;),
          CollectiveConfig::withImmigrants(300_visible, 10))
          .setImmigrants({ ImmigrantInfo("ENT", {MinionTrait::FIGHTER}).setFrequency(1) });
    case EnemyId::DRIADS:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::FOREST;
            c.inhabitants.fighters = CreatureList(random.get(7, 10), "DRIAD");
            c.tribe = TribeId::getMonster();
            c.race = "dryads"_s;
            c.buildingId = BuildingId::WOOD;),
          CollectiveConfig::withImmigrants(300_visible, 10))
          .setImmigrants({ ImmigrantInfo("DRIAD", {MinionTrait::FIGHTER}).setFrequency(1) });
    case EnemyId::SHELOB:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::CAVE;
            c.tribe = TribeId::getShelob();
            c.inhabitants.leader = CreatureList("SHELOB");
            c.inhabitants.civilians = CreatureList(
                random.get(4, 6),
                makeVec<CreatureId>("SPIDER_FOOD","SPIDER"));
            c.race = "giant spider"_s;
            c.buildingId = BuildingId::DUNGEON;
            ), CollectiveConfig::noImmigrants().setLeaderAsFighter())
          .setCreateOnBones(*this, 0.1, {EnemyId::DWARF_CAVE, EnemyId::KOBOLD_CAVE});
    case EnemyId::CYCLOPS:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::CAVE;
            c.inhabitants.leader = CreatureList("CYCLOPS");
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
            c.inhabitants.leader = CreatureList("HYDRA");
            c.race = "hydra"_s;
            c.tribe = TribeId::getHostile();), CollectiveConfig::noImmigrants().setLeaderAsFighter()
          .setGhostSpawns(0.03, 1));
    case EnemyId::KRAKEN:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::MOUNTAIN_LAKE;
            c.inhabitants.leader = CreatureList("KRAKEN");
            c.race = "kraken"_s;
            c.tribe = TribeId::getMonster();), CollectiveConfig::noImmigrants().setLeaderAsFighter())
          .setNonDiscoverable();
    case EnemyId::CEMETERY:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::CEMETERY;
            c.inhabitants.fighters = CreatureList(random.get(8, 12), "ZOMBIE");
            c.locationName = "cemetery"_s;
            c.tribe = TribeId::getMonster();
            c.race = "undead"_s;
            c.furniture = FurnitureListId("cryptCoffins");
            c.buildingId = BuildingId::BRICK;), CollectiveConfig::noImmigrants(), {},
          LevelConnection{LevelConnection::CRYPT, get(EnemyId::CEMETERY_ENTRY)});
    case EnemyId::CEMETERY_ENTRY:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::CEMETERY;
            c.inhabitants.fighters = CreatureList(1, "ZOMBIE");
            c.locationName = "cemetery"_s;
            c.race = "undead"_s;
            c.tribe = TribeId::getMonster();
            c.furniture = FurnitureListId("graves");
            c.buildingId = BuildingId::BRICK;), CollectiveConfig::noImmigrants())
          .setNonDiscoverable();
    case EnemyId::OGRE_CAVE:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::CAVE;
            c.tribe = TribeId::getGreenskin();
            c.inhabitants.fighters = CreatureList(random.get(4, 8), "OGRE");
            c.buildingId = BuildingId::DUNGEON;
            c.closeToPlayer = true;
            c.furniture = FurnitureListId("roomFurniture");
            c.outsideFeatures = FurnitureListId("villageOutside");),
          CollectiveConfig::withImmigrants(300_visible, 10))
          .setImmigrants({ ImmigrantInfo("OGRE", {MinionTrait::FIGHTER}).setFrequency(1) });
    case EnemyId::HARPY_CAVE: {
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::CAVE;
            c.tribe = TribeId::getGreenskin();
            c.inhabitants.fighters = CreatureList(random.get(4, 8), "HARPY");
            c.buildingId = BuildingId::DUNGEON;
            c.closeToPlayer = true;
            c.furniture = FurnitureListId("roomFurniture");
            c.outsideFeatures = FurnitureListId("villageOutside");),
          CollectiveConfig::withImmigrants(300_visible, 10))
          .setImmigrants({ ImmigrantInfo("HARPY", {MinionTrait::FIGHTER}).setFrequency(1) });
      }
    case EnemyId::SOKOBAN_ENTRY:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::ISLAND_VAULT_DOOR;
            c.buildingId = BuildingId::DUNGEON;
            ), CollectiveConfig::noImmigrants());
    case EnemyId::SOKOBAN:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::CAVE;
            c.inhabitants.leader = CreatureList(random.choose(
                "SPECIAL_HLBN"_s, "SPECIAL_HLBW"_s, "SPECIAL_HLGN"_s, "SPECIAL_HLGW"_s));
            c.tribe = TribeId::getDarkKeeper();
            c.buildingId = BuildingId::DUNGEON;
            ), CollectiveConfig::noImmigrants(), none,
          LevelConnection{LevelConnection::SOKOBAN, get(EnemyId::SOKOBAN_ENTRY)})
          .setNonDiscoverable();
    case EnemyId::WITCH:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::COTTAGE;
            c.tribe = TribeId::getMonster();
            c.inhabitants.leader = CreatureList("WITCH");
            c.race = "witch"_s;
            c.buildingId = BuildingId::WOOD;
            c.elderLoot = ItemType(ItemType::TechBook{"advanced alchemy"});
            c.furniture = FurnitureListId("witchInside");
          ), CollectiveConfig::noImmigrants());
    case EnemyId::TEMPLE:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::TEMPLE;
            c.tribe = TribeId::getHuman();
            c.race = "altar"_s;
            if (random.roll(3))
              c.inhabitants.leader = CreatureList("PRIEST");
            c.buildingId = BuildingId::BRICK;
            c.furniture = FurnitureListId("templeInside");
      ), CollectiveConfig::noImmigrants()).setNonDiscoverable();
    case EnemyId::EVIL_TEMPLE:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::TEMPLE;
            c.tribe = TribeId::getDarkKeeper();
            c.race = "altar"_s;
            if (random.roll(3))
              c.inhabitants.leader = CreatureList("ORC_SHAMAN").increaseExpLevel({{ExperienceType::SPELL, 7}});
            c.buildingId = BuildingId::WOOD;
            c.furniture = FurnitureListId("templeInside");
      ), CollectiveConfig::noImmigrants()).setNonDiscoverable();
    case EnemyId::RUINS:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::COTTAGE;
            c.tribe = TribeId::getMonster();
            c.race = "ruins"_s;
            c.dontBuildRoad = true;
            c.closeToPlayer = true;
            c.buildingId = BuildingId::RUINS;
            ), CollectiveConfig::withImmigrants(700_visible, 3).setConquerCondition(ConquerCondition::DESTROY_BUILDINGS))
            .setImmigrants({ ImmigrantInfo("LOST_SOUL", {MinionTrait::FIGHTER})
                .addSpecialTrait(1.0, LastingEffect::DISAPPEAR_DURING_DAY)
                .setFrequency(1)
                .addRequirement(1.0, SunlightState::NIGHT)
                .setSpawnLocation(InsideTerritory{})});
    case EnemyId::HUMAN_COTTAGE:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::COTTAGE;
            c.tribe = TribeId::getHuman();
            c.cropsDistance = 13;
            c.inhabitants.fighters = CreatureList(random.get(2, 4), "PESEANT");
            c.inhabitants.civilians = CreatureList(
                random.get(3, 7),
                makeVec<CreatureId>("CHILD", "HORSE", "DONKEY", "COW",
                    "PIG", "DOG"));
            c.race = "humans"_s;
            c.buildingId = BuildingId::WOOD;
            c.furniture = FurnitureListId("roomFurniture");),
          CollectiveConfig::noImmigrants().setGuardian({"WITCHMAN", 0.001, 1, 2}));
    case EnemyId::TUTORIAL_VILLAGE:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::SMALL_VILLAGE;
            c.tribe = TribeId::getHuman();
            c.inhabitants.fighters = CreatureList(random.get(2, 4), "PESEANT");
            c.inhabitants.civilians = CreatureList(
                random.get(3, 7),
                makeVec<CreatureId>("CHILD", "HORSE", "DONKEY", "COW",
                    "PIG", "DOG"));
            c.race = "humans"_s;
            c.buildingId = BuildingId::WOOD;
            c.stockpiles = LIST({StockpileInfo::GOLD, 50});
            c.cropsDistance = 16;
            c.furniture = FurnitureListId("roomFurniture");),
          CollectiveConfig::noImmigrants());
    case EnemyId::ELVEN_COTTAGE:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::FORREST_COTTAGE;
            c.tribe = TribeId::getElf();
            c.inhabitants.leader = CreatureList("ELF_ARCHER");
            c.inhabitants.fighters = CreatureList(random.get(2, 3), "ELF");
            c.inhabitants.civilians = CreatureList(
                random.get(2, 5),
                makeVec<CreatureId>("ELF_CHILD", "HORSE", "COW", "DOG"));
            c.race = "elves"_s;
            c.buildingId = BuildingId::WOOD;
            c.furniture = FurnitureListId("roomFurniture");),
          CollectiveConfig::noImmigrants());
    case EnemyId::LIZARDMEN_COTTAGE:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::FORREST_COTTAGE;
            c.tribe = TribeId::getLizard();
            c.inhabitants.fighters = CreatureList(random.get(2, 3), "LIZARDMAN");
            c.race = "lizardmen"_s;
            c.buildingId = BuildingId::WOOD;
            c.furniture = FurnitureListId("roomFurniture");),
          CollectiveConfig::noImmigrants())
          .setCreateOnBones(*this, 1.0, {EnemyId::ELVEN_COTTAGE});
    case EnemyId::KOBOLD_CAVE:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::SMALL_MINETOWN;
            c.tribe = TribeId::getDwarf();
            c.inhabitants.fighters = CreatureList(random.get(3, 7), "KOBOLD");
            c.race = "kobolds"_s;
            c.buildingId = BuildingId::DUNGEON;
            c.stockpiles = LIST({StockpileInfo::MINERALS, 60});),
          CollectiveConfig::noImmigrants());
    case EnemyId::DWARF_CAVE:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::SMALL_MINETOWN;
            c.tribe = TribeId::getDwarf();
            c.inhabitants.fighters = CreatureList(random.get(2, 5), "DWARF");
            c.inhabitants.civilians = CreatureList(random.get(2, 5), "DWARF_FEMALE");
            c.race = "dwarves"_s;
            c.buildingId = BuildingId::DUNGEON;
            c.stockpiles = LIST(random.choose(StockpileInfo{StockpileInfo::MINERALS, 60},
                StockpileInfo{StockpileInfo::GOLD, 60}));
            c.outsideFeatures = FurnitureListId("dungeonOutside");
            c.furniture = FurnitureListId("roomFurniture");
            c.surroundWithResources = 6;
          ),
          CollectiveConfig::noImmigrants(),
          CONSTRUCT(VillageBehaviour,
            c.minPopulation = 0;
            c.minTeamSize = 1;
            c.triggers = LIST(AttackTriggerId::SELF_VICTIMS, AttackTriggerId::STOLEN_ITEMS, AttackTriggerId::MINING_IN_PROXIMITY);
            c.attackBehaviour = AttackBehaviour(AttackBehaviourId::KILL_LEADER);
            c.ransom = make_pair(0.5, random.get(40, 80));));
    case EnemyId::RAT_CAVE:
      return getVault(SettlementType::VAULT, "RAT", TribeId::getMonster(), 10);
    case EnemyId::RAT_PEOPLE_CAVE:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
          c.type = SettlementType::VAULT;
          c.inhabitants.fighters = CreatureList(random.get(2, 5), "RAT_SOLDIER");
          c.inhabitants.leader = CreatureList("RAT_KING");
          c.inhabitants.civilians = CreatureList(random.get(2, 5), vector<CreatureId>(LIST("RAT_LADY"_s, "RAT"_s)));
          c.tribe = TribeId::getMonster();
          c.closeToPlayer = true;
          c.dontConnectCave = true;
          c.buildingId = BuildingId::DUNGEON;
        ), CollectiveConfig::noImmigrants())
        .setNonDiscoverable();
    case EnemyId::DARK_ELF_CAVE:
      return EnemyInfo(CONSTRUCT(SettlementInfo,
            c.type = SettlementType::SMALL_MINETOWN;
            c.tribe = TribeId::getDarkElf();
            c.inhabitants.fighters = CreatureList(random.get(2, 5), "DARK_ELF_WARRIOR")
                .increaseBaseLevel({{ExperienceType::MELEE, 7}});
            c.inhabitants.civilians = CreatureList(random.get(2, 5), "DARK_ELF");
            c.race = "dark elves"_s;
            c.buildingId = BuildingId::DUNGEON;
            c.stockpiles = LIST(random.choose(StockpileInfo{StockpileInfo::MINERALS, 60},
                StockpileInfo{StockpileInfo::GOLD, 60}));
            c.outsideFeatures = FurnitureListId("dungeonOutside");
            c.furniture = FurnitureListId("roomFurniture");
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

vector<ExternalEnemy> EnemyFactory::getExternalEnemies() const {
  return {
    ExternalEnemy{
        CreatureList(1, "BANDIT")
            .increaseBaseLevel({{ExperienceType::MELEE, -2}}),
        AttackBehaviourId::KILL_LEADER,
        "a curious bandit",
        Range(1000, 10000),
        1
    },
    ExternalEnemy{
        CreatureList(1, "LIZARDMAN")
            .increaseBaseLevel({{ExperienceType::MELEE, 1}}),
        AttackBehaviourId::KILL_LEADER,
        "a lizardman scout",
        Range(1000, 10000),
        1
    },
    ExternalEnemy{
        CreatureList(random.get(3, 5), "ANT_WORKER")
            .increaseBaseLevel({{ExperienceType::MELEE, -2}}),
        AttackBehaviourId::KILL_LEADER,
        "an ant patrol",
        Range(1000, 5000),
        2
    },
    ExternalEnemy{
        CreatureList(random.get(4, 8), "LIZARDMAN")
            .increaseBaseLevel({{ExperienceType::MELEE, 2}}),
        AttackBehaviourId::KILL_LEADER,
        "a nest of lizardmen",
        Range(6000, 15000),
        5
    },
    ExternalEnemy{
        CreatureList(random.get(3, 5), "BANDIT")
            .increaseBaseLevel({{ExperienceType::MELEE, 9}}),
        AttackBehaviourId::KILL_LEADER,
        "a gang of bandits",
        Range(8000, 20000),
        10
    },
    ExternalEnemy{
        CreatureList(random.get(3, 5), "ANT_SOLDIER"),
        AttackBehaviourId::KILL_LEADER,
        "an ant soldier patrol",
        Range(5000, 10000),
        1
    },
    ExternalEnemy{
        CreatureList(random.get(3, 5), "CLAY_GOLEM")
            .increaseBaseLevel({{ExperienceType::MELEE, 3}}),
        AttackBehaviourId::KILL_LEADER,
        "an alchemist's clay golems",
        Range(3000, 10000),
        1
    },
    ExternalEnemy{
        CreatureList(random.get(3, 5), "STONE_GOLEM")
            .increaseBaseLevel({{ExperienceType::MELEE, 3}}),
        AttackBehaviourId::KILL_LEADER,
        "an alchemist's stone golems",
        Range(5000, 12000),
        2
    },
    ExternalEnemy{
        CreatureList(random.get(5, 8), "ANT_SOLDIER")
            .increaseBaseLevel({{ExperienceType::MELEE, 5}}),
        AttackBehaviourId::KILL_LEADER,
        "an ant soldier patrol",
        Range(6000, 12000),
        2
    },
    ExternalEnemy{
        CreatureList(random.get(10, 15), "ANT_SOLDIER").addUnique("ANT_QUEEN")
            .increaseBaseLevel({{ExperienceType::MELEE, 6}}),
        AttackBehaviourId::KILL_LEADER,
        "an army of ants",
        Range(10000, 15000),
        3
    },
    ExternalEnemy{
        CreatureList(random.get(4, 8), "ENT")
            .increaseBaseLevel({{ExperienceType::MELEE, 3}}),
        AttackBehaviourId::KILL_LEADER,
        "a group of ents",
        Range(7000, 12000),
        2
    },
    ExternalEnemy{
        CreatureList(random.get(4, 8), "DWARF"),
        AttackBehaviourId::KILL_LEADER,
        "a band of dwarves",
        Range(7000, 15000),
        3
    },
    ExternalEnemy{
        CreatureList(random.get(3, 5), "IRON_GOLEM")
            .increaseBaseLevel({{ExperienceType::MELEE, 2}}),
        AttackBehaviourId::KILL_LEADER,
        "an alchemist's iron golems",
        Range(9000, 15000),
        3
    },
    ExternalEnemy{
        CreatureList(random.get(10, 13), "DWARF").addUnique("DWARF_BARON")
            .increaseBaseLevel({{ExperienceType::MELEE, 12}})
            .addInventory({ItemType::Scroll{Effect::DestroyWalls{}}}),
        AttackBehaviourId::KILL_LEADER,
        "a dwarf tribe",
        Range(12000, 25000),
        10
    },
    ExternalEnemy{
        CreatureList(random.get(4, 8), "ARCHER"),
        AttackBehaviourId::KILL_LEADER,
        "a patrol of archers",
        Range(6000, 12000),
        3
    },
    ExternalEnemy{
        CreatureList(1, "KNIGHT"),
        AttackBehaviourId::KILL_LEADER,
        "a lonely knight",
        Range(10000, 16000),
        3
    },
    ExternalEnemy{
        CreatureList(random.get(4, 8), "WARRIOR")
            .increaseBaseLevel({{ExperienceType::MELEE, 6}}),
        AttackBehaviourId::KILL_LEADER,
        "a group of warriors",
        Range(6000, 12000),
        3
    },
    ExternalEnemy{
        CreatureList(random.get(12, 16), "WARRIOR").addUnique("SHAMAN")
            .increaseBaseLevel({{ExperienceType::MELEE, 14}})
            .addInventory({ItemType::Scroll{Effect::DestroyWalls{}}}),
        AttackBehaviourId::KILL_LEADER,
        "an army of warriors",
        Range(18000, 100000),
        100
    },
    ExternalEnemy{
        CreatureList(random.get(12, 16), vector<pair<int, CreatureId>>({make_pair(2, "KNIGHT"), make_pair(1, "ARCHER")}))
            .addUnique("DUKE")
            //.increaseBaseLevel({{ExperienceType::MELEE, 4}})
            .addInventory({ItemType::Scroll{Effect::DestroyWalls{}}}),
        AttackBehaviourId::KILL_LEADER,
        "an army of knights",
        Range(20000, 100000),
        100
    },
    ExternalEnemy{
        CreatureList(1, "CYCLOPS"),
        AttackBehaviourId::KILL_LEADER,
        "a cyclops",
        Range(6000, 20000),
        4
    },
    ExternalEnemy{
        CreatureList(1, "WITCHMAN")
            .increaseBaseLevel({{ExperienceType::MELEE, 20}}),
        AttackBehaviourId::KILL_LEADER,
        "a witchman",
        Range(15000, 40000),
        5
    },
    ExternalEnemy{
        CreatureList(1, "MINOTAUR")
            .increaseBaseLevel({{ExperienceType::MELEE, 20}}),
        AttackBehaviourId::KILL_LEADER,
        "a minotaur",
        Range(15000, 100000),
        100
    },
    ExternalEnemy{
        CreatureList(1, "ELEMENTALIST"),
        {AttackBehaviourId::CAMP_AND_SPAWN, CreatureGroup::elementals(TribeId::getHuman())},
        "an elementalist",
        Range(8000, 14000),
        1
    },
    ExternalEnemy{
        CreatureList(1, "GREEN_DRAGON")
            .increaseBaseLevel({{ExperienceType::MELEE, 10}}),
        AttackBehaviourId::KILL_LEADER,
        "a green dragon",
        Range(15000, 40000),
        100
    },
    ExternalEnemy{
        CreatureList(1, "RED_DRAGON")
            .increaseBaseLevel({{ExperienceType::MELEE, 18}}),
        AttackBehaviourId::KILL_LEADER,
        "a red dragon",
        Range(19000, 100000),
        100
    },
    ExternalEnemy{
        CreatureList(random.get(3, 6), makeVec<CreatureId>("ADVENTURER_F", "ADVENTURER"))
            .addInventory({ItemType::Scroll{Effect::DestroyWalls{}}})
            .increaseBaseLevel({{ExperienceType::MELEE, 20}}),
        AttackBehaviourId::KILL_LEADER,
        "a group of adventurers",
        Range(10000, 25000),
        100
    },
    ExternalEnemy{
        CreatureList(random.get(3, 6), "OGRE")
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
        CreatureList(random.get(3, 6), "HALLOWEEN_KID"),
        AttackBehaviourId::HALLOWEEN_KIDS,
        "kids...?",
        Range(0, 10000),
        1
    }};
}
