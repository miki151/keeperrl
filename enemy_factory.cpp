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
#include "enemy_id.h"

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
      {CreatureId("ORC"), 3, 5},
      {CreatureId("OGRE"), 2, 4},
      {CreatureId("VAMPIRE"), 2, 4},
  }},
  {TribeAlignment::LAWFUL, {
      {CreatureId("IRON_GOLEM"), 3, 5},
      {CreatureId("EARTH_ELEMENTAL"), 2, 4},
  }},
};

static vector<VaultInfo> hostileVaults {
  {CreatureId("SPIDER"), 3, 8},
  {CreatureId("SNAKE"), 3, 8},
  {CreatureId("BAT"), 3, 8},
};

vector<EnemyInfo> EnemyFactory::getVaults(TribeAlignment alignment, TribeId allied) const {
  vector<EnemyInfo> ret {
 /*   getVault(SettlementType::VAULT, CreatureGroup::insects(TribeId::getMonster()),
        TribeId::getMonster(), random.get(6, 12)),*/
    getVault(SettlementType::VAULT, CreatureId("RAT"), TribeId::getPest(), random.get(3, 8),
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

vector<EnemyId> EnemyFactory::getAllIds() const {
  return getKeys(enemies);
}

EnemyInfo EnemyFactory::get(EnemyId id) const {
  CHECK(enemies.count(id)) << "Enemy not found: \"" << id.data() << "\"";
  auto ret = enemies.at(id);
  ret.setId(id);
  updateCreateOnBones(ret);
  if (ret.levelConnection)
    ret.levelConnection->otherEnemy = get(ret.levelConnection->enemyId);
  if (ret.settlement.locationNameGen)
    ret.settlement.locationName = nameGenerator->getNext(*ret.settlement.locationNameGen);
  return ret;
}

void EnemyFactory::updateCreateOnBones(EnemyInfo& info) const {
  if (info.createOnBones && random.chance(info.createOnBones->probability)) {
    EnemyInfo enemy = get(random.choose(info.createOnBones->enemies));
    info.levelConnection = enemy.levelConnection;
    if (info.levelConnection) {
      info.levelConnection->otherEnemy->settlement.buildingId = BuildingId::RUINS;
      info.levelConnection->deadInhabitants = true;
    }
    if (Random.roll(2)) {
      info.settlement.buildingId = BuildingId::RUINS;
      if (info.levelConnection)
        info.levelConnection->otherEnemy->settlement.buildingId = BuildingId::RUINS;
    } else {
      // 50% chance that the original settlement is intact
      info.settlement.buildingId = enemy.settlement.buildingId;
      info.settlement.furniture = enemy.settlement.furniture;
      info.settlement.outsideFeatures = enemy.settlement.outsideFeatures;
      info.settlement.shopItems = enemy.settlement.shopItems;
    }
    info.settlement.type = enemy.settlement.type;
    info.settlement.corpses = enemy.settlement.inhabitants;
    info.settlement.shopkeeperDead = true;
  }
}

vector<ExternalEnemy> EnemyFactory::getExternalEnemies() const {
  return {
    ExternalEnemy{
        CreatureList(1, CreatureId("BANDIT"))
            .increaseBaseLevel({{ExperienceType::MELEE, -2}}),
        KillLeader{},
        "a curious bandit",
        Range(1000, 10000),
        1
    },
    ExternalEnemy{
        CreatureList(1, CreatureId("LIZARDMAN"))
            .increaseBaseLevel({{ExperienceType::MELEE, 1}}),
        KillLeader{},
        "a lizardman scout",
        Range(1000, 10000),
        1
    },
    ExternalEnemy{
        CreatureList(random.get(3, 5), CreatureId("ANT_WORKER"))
            .increaseBaseLevel({{ExperienceType::MELEE, -2}}),
        KillLeader{},
        "an ant patrol",
        Range(1000, 5000),
        2
    },
    ExternalEnemy{
        CreatureList(random.get(4, 8), CreatureId("LIZARDMAN"))
            .increaseBaseLevel({{ExperienceType::MELEE, 2}}),
        KillLeader{},
        "a nest of lizardmen",
        Range(6000, 15000),
        5
    },
    ExternalEnemy{
        CreatureList(random.get(3, 5), CreatureId("BANDIT"))
            .increaseBaseLevel({{ExperienceType::MELEE, 9}}),
        KillLeader{},
        "a gang of bandits",
        Range(8000, 20000),
        10
    },
    ExternalEnemy{
        CreatureList(random.get(3, 5), CreatureId("ANT_SOLDIER")),
        KillLeader{},
        "an ant soldier patrol",
        Range(5000, 10000),
        1
    },
    ExternalEnemy{
        CreatureList(random.get(3, 5), CreatureId("CLAY_GOLEM"))
            .increaseBaseLevel({{ExperienceType::MELEE, 3}}),
        KillLeader{},
        "an alchemist's clay golems",
        Range(3000, 10000),
        1
    },
    ExternalEnemy{
        CreatureList(random.get(3, 5), CreatureId("STONE_GOLEM"))
            .increaseBaseLevel({{ExperienceType::MELEE, 3}}),
        KillLeader{},
        "an alchemist's stone golems",
        Range(5000, 12000),
        2
    },
    ExternalEnemy{
        CreatureList(random.get(5, 8), CreatureId("ANT_SOLDIER"))
            .increaseBaseLevel({{ExperienceType::MELEE, 5}}),
        KillLeader{},
        "an ant soldier patrol",
        Range(6000, 12000),
        2
    },
    ExternalEnemy{
        CreatureList(random.get(10, 15), CreatureId("ANT_SOLDIER")).addUnique(CreatureId("ANT_QUEEN"))
            .increaseBaseLevel({{ExperienceType::MELEE, 6}}),
        KillLeader{},
        "an army of ants",
        Range(10000, 15000),
        3
    },
    ExternalEnemy{
        CreatureList(random.get(4, 8), CreatureId("ENT"))
            .increaseBaseLevel({{ExperienceType::MELEE, 3}}),
        KillLeader{},
        "a group of ents",
        Range(7000, 12000),
        2
    },
    ExternalEnemy{
        CreatureList(random.get(4, 8), CreatureId("DWARF")),
        KillLeader{},
        "a band of dwarves",
        Range(7000, 15000),
        3
    },
    ExternalEnemy{
        CreatureList(random.get(3, 5), CreatureId("IRON_GOLEM"))
            .increaseBaseLevel({{ExperienceType::MELEE, 2}}),
        KillLeader{},
        "an alchemist's iron golems",
        Range(9000, 15000),
        3
    },
    ExternalEnemy{
        CreatureList(random.get(10, 13), CreatureId("DWARF")).addUnique(CreatureId("DWARF_BARON"))
            .increaseBaseLevel({{ExperienceType::MELEE, 12}})
            .addInventory({ItemType::Scroll{Effect::DestroyWalls{}}}),
        KillLeader{},
        "a dwarf tribe",
        Range(12000, 25000),
        10
    },
    ExternalEnemy{
        CreatureList(random.get(4, 8), CreatureId("ARCHER")),
        KillLeader{},
        "a patrol of archers",
        Range(6000, 12000),
        3
    },
    ExternalEnemy{
        CreatureList(1, CreatureId("KNIGHT")),
        KillLeader{},
        "a lonely knight",
        Range(10000, 16000),
        3
    },
    ExternalEnemy{
        CreatureList(random.get(4, 8), CreatureId("WARRIOR"))
            .increaseBaseLevel({{ExperienceType::MELEE, 6}}),
        KillLeader{},
        "a group of warriors",
        Range(6000, 12000),
        3
    },
    ExternalEnemy{
        CreatureList(random.get(12, 16), CreatureId("WARRIOR")).addUnique(CreatureId("SHAMAN"))
            .increaseBaseLevel({{ExperienceType::MELEE, 14}})
            .addInventory({ItemType::Scroll{Effect::DestroyWalls{}}}),
        KillLeader{},
        "an army of warriors",
        Range(18000, 100000),
        100
    },
    ExternalEnemy{
        CreatureList(random.get(12, 16), vector<pair<int, CreatureId>>({make_pair(2, CreatureId("KNIGHT")),
            make_pair(1, CreatureId("ARCHER"))}))
            .addUnique(CreatureId("DUKE"))
            //.increaseBaseLevel({{ExperienceType::MELEE, 4}})
            .addInventory({ItemType::Scroll{Effect::DestroyWalls{}}}),
        KillLeader{},
        "an army of knights",
        Range(20000, 100000),
        100
    },
    ExternalEnemy{
        CreatureList(1, CreatureId("CYCLOPS")),
        KillLeader{},
        "a cyclops",
        Range(6000, 20000),
        4
    },
    ExternalEnemy{
        CreatureList(1, CreatureId("WITCHMAN"))
            .increaseBaseLevel({{ExperienceType::MELEE, 20}}),
        KillLeader{},
        "a witchman",
        Range(15000, 40000),
        5
    },
    ExternalEnemy{
        CreatureList(1, CreatureId("MINOTAUR"))
            .increaseBaseLevel({{ExperienceType::MELEE, 20}}),
        KillLeader{},
        "a minotaur",
        Range(15000, 100000),
        100
    },
    ExternalEnemy{
        CreatureList(1, CreatureId("ELEMENTALIST")),
        CreatureList(5, makeVec(CreatureId("FIRE_ELEMENTAL"), CreatureId("EARTH_ELEMENTAL"), CreatureId("AIR_ELEMENTAL"), CreatureId("WATER_ELEMENTAL"))),
        "an elementalist",
        Range(8000, 14000),
        1
    },
    ExternalEnemy{
        CreatureList(1, CreatureId("GREEN_DRAGON"))
            .increaseBaseLevel({{ExperienceType::MELEE, 10}}),
        KillLeader{},
        "a green dragon",
        Range(15000, 40000),
        100
    },
    ExternalEnemy{
        CreatureList(1, CreatureId("RED_DRAGON"))
            .increaseBaseLevel({{ExperienceType::MELEE, 18}}),
        KillLeader{},
        "a red dragon",
        Range(19000, 100000),
        100
    },
    ExternalEnemy{
        CreatureList(random.get(3, 6), makeVec<CreatureId>(CreatureId("ADVENTURER_F"), CreatureId("ADVENTURER")))
            .addInventory({ItemType::Scroll{Effect::DestroyWalls{}}})
            .increaseBaseLevel({{ExperienceType::MELEE, 20}}),
        KillLeader{},
        "a group of adventurers",
        Range(10000, 25000),
        100
    },
    ExternalEnemy{
        CreatureList(random.get(3, 6), CreatureId("OGRE"))
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
        CreatureList(random.get(3, 6), CreatureId("HALLOWEEN_KID")),
        HalloweenKids{},
        "kids...?",
        Range(0, 10000),
        1
    }};
}
