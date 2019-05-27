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

EnemyFactory::EnemyFactory(RandomGen& r, NameGenerator* n, map<EnemyId, EnemyInfo> enemies)
    : random(r), nameGenerator(n), enemies(std::move(enemies)) {
}

EnemyInfo::EnemyInfo(SettlementInfo s, CollectiveConfig c, optional<VillageBehaviour> v,
    optional<LevelConnection> l)
  : settlement(s), config(c), behaviour(v), levelConnection(l) {
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


void EnemyInfo::updateCreateOnBones(const EnemyFactory& factory) {
  if (createOnBones && factory.random.chance(createOnBones->probability)) {
    EnemyInfo enemy = factory.get(factory.random.choose(createOnBones->enemies));
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
      settlement.shopItems = enemy.settlement.shopItems;
    }
    settlement.type = enemy.settlement.type;
    settlement.corpses = enemy.settlement.inhabitants;
    settlement.shopkeeperDead = true;
  }
}


static EnemyInfo getVault(SettlementType type, CreatureId creature, TribeId tribe, int num,
    optional<ItemListId> itemFactory = none) {
  return EnemyInfo(CONSTRUCT(SettlementInfo,
      c.type = type;
      c.inhabitants.fighters = CreatureList(num, creature);
      c.tribe = tribe;
      c.closeToPlayer = true;
      c.dontConnectCave = true;
      c.buildingId = BuildingId::DUNGEON;
      c.shopItems = itemFactory;
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
        ItemListId("armory")),
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

EnemyInfo EnemyFactory::get(EnemyId id) const {
  auto ret = enemies.at(id);
  ret.setId(id);
  ret.updateCreateOnBones(*this);
  if (ret.levelConnection)
    ret.levelConnection->otherEnemy = get(ret.levelConnection->enemyId);
  if (ret.settlement.locationNameGen)
    ret.settlement.locationName = nameGenerator->getNext(*ret.settlement.locationNameGen);
  return ret;
}

vector<ExternalEnemy> EnemyFactory::getExternalEnemies() const {
  return {
    ExternalEnemy{
        CreatureList(1, "BANDIT")
            .increaseBaseLevel({{ExperienceType::MELEE, -2}}),
        KillLeader{},
        "a curious bandit",
        Range(1000, 10000),
        1
    },
    ExternalEnemy{
        CreatureList(1, "LIZARDMAN")
            .increaseBaseLevel({{ExperienceType::MELEE, 1}}),
        KillLeader{},
        "a lizardman scout",
        Range(1000, 10000),
        1
    },
    ExternalEnemy{
        CreatureList(random.get(3, 5), "ANT_WORKER")
            .increaseBaseLevel({{ExperienceType::MELEE, -2}}),
        KillLeader{},
        "an ant patrol",
        Range(1000, 5000),
        2
    },
    ExternalEnemy{
        CreatureList(random.get(4, 8), "LIZARDMAN")
            .increaseBaseLevel({{ExperienceType::MELEE, 2}}),
        KillLeader{},
        "a nest of lizardmen",
        Range(6000, 15000),
        5
    },
    ExternalEnemy{
        CreatureList(random.get(3, 5), "BANDIT")
            .increaseBaseLevel({{ExperienceType::MELEE, 9}}),
        KillLeader{},
        "a gang of bandits",
        Range(8000, 20000),
        10
    },
    ExternalEnemy{
        CreatureList(random.get(3, 5), "ANT_SOLDIER"),
        KillLeader{},
        "an ant soldier patrol",
        Range(5000, 10000),
        1
    },
    ExternalEnemy{
        CreatureList(random.get(3, 5), "CLAY_GOLEM")
            .increaseBaseLevel({{ExperienceType::MELEE, 3}}),
        KillLeader{},
        "an alchemist's clay golems",
        Range(3000, 10000),
        1
    },
    ExternalEnemy{
        CreatureList(random.get(3, 5), "STONE_GOLEM")
            .increaseBaseLevel({{ExperienceType::MELEE, 3}}),
        KillLeader{},
        "an alchemist's stone golems",
        Range(5000, 12000),
        2
    },
    ExternalEnemy{
        CreatureList(random.get(5, 8), "ANT_SOLDIER")
            .increaseBaseLevel({{ExperienceType::MELEE, 5}}),
        KillLeader{},
        "an ant soldier patrol",
        Range(6000, 12000),
        2
    },
    ExternalEnemy{
        CreatureList(random.get(10, 15), "ANT_SOLDIER").addUnique("ANT_QUEEN")
            .increaseBaseLevel({{ExperienceType::MELEE, 6}}),
        KillLeader{},
        "an army of ants",
        Range(10000, 15000),
        3
    },
    ExternalEnemy{
        CreatureList(random.get(4, 8), "ENT")
            .increaseBaseLevel({{ExperienceType::MELEE, 3}}),
        KillLeader{},
        "a group of ents",
        Range(7000, 12000),
        2
    },
    ExternalEnemy{
        CreatureList(random.get(4, 8), "DWARF"),
        KillLeader{},
        "a band of dwarves",
        Range(7000, 15000),
        3
    },
    ExternalEnemy{
        CreatureList(random.get(3, 5), "IRON_GOLEM")
            .increaseBaseLevel({{ExperienceType::MELEE, 2}}),
        KillLeader{},
        "an alchemist's iron golems",
        Range(9000, 15000),
        3
    },
    ExternalEnemy{
        CreatureList(random.get(10, 13), "DWARF").addUnique("DWARF_BARON")
            .increaseBaseLevel({{ExperienceType::MELEE, 12}})
            .addInventory({ItemType::Scroll{Effect::DestroyWalls{}}}),
        KillLeader{},
        "a dwarf tribe",
        Range(12000, 25000),
        10
    },
    ExternalEnemy{
        CreatureList(random.get(4, 8), "ARCHER"),
        KillLeader{},
        "a patrol of archers",
        Range(6000, 12000),
        3
    },
    ExternalEnemy{
        CreatureList(1, "KNIGHT"),
        KillLeader{},
        "a lonely knight",
        Range(10000, 16000),
        3
    },
    ExternalEnemy{
        CreatureList(random.get(4, 8), "WARRIOR")
            .increaseBaseLevel({{ExperienceType::MELEE, 6}}),
        KillLeader{},
        "a group of warriors",
        Range(6000, 12000),
        3
    },
    ExternalEnemy{
        CreatureList(random.get(12, 16), "WARRIOR").addUnique("SHAMAN")
            .increaseBaseLevel({{ExperienceType::MELEE, 14}})
            .addInventory({ItemType::Scroll{Effect::DestroyWalls{}}}),
        KillLeader{},
        "an army of warriors",
        Range(18000, 100000),
        100
    },
    ExternalEnemy{
        CreatureList(random.get(12, 16), vector<pair<int, CreatureId>>({make_pair(2, "KNIGHT"), make_pair(1, "ARCHER")}))
            .addUnique("DUKE")
            //.increaseBaseLevel({{ExperienceType::MELEE, 4}})
            .addInventory({ItemType::Scroll{Effect::DestroyWalls{}}}),
        KillLeader{},
        "an army of knights",
        Range(20000, 100000),
        100
    },
    ExternalEnemy{
        CreatureList(1, "CYCLOPS"),
        KillLeader{},
        "a cyclops",
        Range(6000, 20000),
        4
    },
    ExternalEnemy{
        CreatureList(1, "WITCHMAN")
            .increaseBaseLevel({{ExperienceType::MELEE, 20}}),
        KillLeader{},
        "a witchman",
        Range(15000, 40000),
        5
    },
    ExternalEnemy{
        CreatureList(1, "MINOTAUR")
            .increaseBaseLevel({{ExperienceType::MELEE, 20}}),
        KillLeader{},
        "a minotaur",
        Range(15000, 100000),
        100
    },
    ExternalEnemy{
        CreatureList(1, "ELEMENTALIST"),
        CreatureList(5, makeVec(CreatureId("FIRE_ELEMENTAL"), CreatureId("EARTH_ELEMENTAL"), CreatureId("AIR_ELEMENTAL"), CreatureId("WATER_ELEMENTAL"))),
        "an elementalist",
        Range(8000, 14000),
        1
    },
    ExternalEnemy{
        CreatureList(1, "GREEN_DRAGON")
            .increaseBaseLevel({{ExperienceType::MELEE, 10}}),
        KillLeader{},
        "a green dragon",
        Range(15000, 40000),
        100
    },
    ExternalEnemy{
        CreatureList(1, "RED_DRAGON")
            .increaseBaseLevel({{ExperienceType::MELEE, 18}}),
        KillLeader{},
        "a red dragon",
        Range(19000, 100000),
        100
    },
    ExternalEnemy{
        CreatureList(random.get(3, 6), makeVec<CreatureId>("ADVENTURER_F", "ADVENTURER"))
            .addInventory({ItemType::Scroll{Effect::DestroyWalls{}}})
            .increaseBaseLevel({{ExperienceType::MELEE, 20}}),
        KillLeader{},
        "a group of adventurers",
        Range(10000, 25000),
        100
    },
    ExternalEnemy{
        CreatureList(random.get(3, 6), "OGRE")
            .increaseBaseLevel({{ExperienceType::MELEE, 25}}),
        KillLeader{},
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
        HalloweenKids{},
        "kids...?",
        Range(0, 10000),
        1
    }};
}
