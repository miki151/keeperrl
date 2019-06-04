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

EnemyFactory::EnemyFactory(RandomGen& r, NameGenerator* n, map<EnemyId, EnemyInfo> enemies, vector<ExternalEnemy> externalEnemies)
    : random(r), nameGenerator(n), enemies(std::move(enemies)), externalEnemies(std::move(externalEnemies)) {
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
  return externalEnemies;
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
