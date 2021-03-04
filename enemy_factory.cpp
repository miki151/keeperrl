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
#include "collective_builder.h"
#include "model.h"
#include "collective.h"
#include "village_control.h"
#include "game.h"
#include "immigration.h"

EnemyFactory::EnemyFactory(RandomGen& r, NameGenerator* n, map<EnemyId, EnemyInfo> enemies, map<BuildingId, BuildingInfo> buildingInfo,
    vector<ExternalEnemy> externalEnemies)
  : random(r), nameGenerator(n), enemies(std::move(enemies)), buildingInfo(std::move(buildingInfo)),
    externalEnemies(std::move(externalEnemies)) {
}

PCollective EnemyInfo::buildCollective(ContentFactory* contentFactory) const {
  if (settlement.locationName)
    settlement.collective->setLocationName(*settlement.locationName);
  if (auto race = settlement.race)
    settlement.collective->setRaceName(*race);
  if (discoverable)
    settlement.collective->setDiscoverable();
  PCollective collective = settlement.collective->build(contentFactory);
  collective->setImmigration(makeOwner<Immigration>(collective.get(), immigrants));
  auto control = VillageControl::create(collective.get(), behaviour);
  if (villainType)
    collective->setVillainType(*villainType);
  if (id)
    collective->setEnemyId(*id);
  collective->setControl(std::move(control));
  return collective;
}

static EnemyInfo getVault(BuiltinLayoutId id, CreatureId creature, TribeId tribe, int num,
    optional<ItemListId> itemFactory = none) {
  return EnemyInfo(CONSTRUCT(SettlementInfo,
      c.type = MapLayoutTypes::Builtin LIST(id, BuildingId("DUNGEON"), {});
      c.inhabitants.fighters = CreatureList(num, creature);
      c.tribe = tribe;
      c.closeToPlayer = true;
      c.dontConnectCave = true;
      if (itemFactory)
        c.shopItems = {SettlementInfo::ShopInfo LIST(Range(16, 20), *itemFactory)};
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

static EnumMap<TribeAlignment, vector<VaultInfo>> getFriendlyVaults() {
  return {
    {TribeAlignment::EVIL, {
        {CreatureId("GOBLIN_WARRIOR"), 3, 5},
        {CreatureId("TROLL"), 2, 4},
        {CreatureId("VAMPIRE"), 2, 4},
    }},
    {TribeAlignment::LAWFUL, {
        {CreatureId("IRON_GOLEM"), 3, 5},
        {CreatureId("EARTH_ELEMENTAL"), 2, 4},
    }},
  };
}

static vector<VaultInfo> getHostileVaults() {
  return {
    {CreatureId("SPIDER"), 3, 8},
    {CreatureId("SNAKE"), 3, 8},
    {CreatureId("BAT"), 3, 8},
  };
}

vector<EnemyInfo> EnemyFactory::getVaults(TribeAlignment alignment, TribeId allied) const {
  vector<EnemyInfo> ret {
 /*   getVault(BuiltinLayoutId::VAULT, CreatureGroup::insects(TribeId::getMonster()),
        TribeId::getMonster(), random.get(6, 12)),*/
    getVault(BuiltinLayoutId::VAULT, CreatureId("RAT"), TribeId::getPest(), random.get(3, 8),
        ItemListId("armory")),
  };
  for (int i : Range(1)) {
    VaultInfo v = random.choose(getFriendlyVaults()[alignment]);
    ret.push_back(getVault(BuiltinLayoutId::VAULT, v.id, allied, random.get(v.min, v.max)));
  }
  for (int i : Range(1)) {
    VaultInfo v = random.choose(getHostileVaults());
    ret.push_back(getVault(BuiltinLayoutId::VAULT, v.id, TribeId::getMonster(), random.get(v.min, v.max)));
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
  if (ret.levelConnection) {
    for (auto& level : ret.levelConnection->levels)
      if (auto extra = level.enemy.getReferenceMaybe<LevelConnection::ExtraEnemy>())
        extra->enemyInfo = vector<EnemyInfo>(random.get(extra->numLevels), get(extra->enemy));
    if (auto extra = ret.levelConnection->topLevel.getReferenceMaybe<LevelConnection::ExtraEnemy>())
      extra->enemyInfo = vector<EnemyInfo>(random.get(extra->numLevels), get(extra->enemy));
  }
  updateCreateOnBones(ret);
  ret.updateBuildingInfo(buildingInfo);
  if (ret.settlement.locationNameGen)
    ret.settlement.locationName = nameGenerator->getNext(*ret.settlement.locationNameGen);
  return ret;
}

void EnemyFactory::updateCreateOnBones(EnemyInfo& info) const {
  if (info.createOnBones && random.chance(info.createOnBones->probability)) {
    EnemyInfo enemy = get(random.choose(info.createOnBones->enemies));
    info.levelConnection = enemy.levelConnection;
    info.biomes = enemy.biomes;
    bool makeRuins = Random.roll(2);
    if (makeRuins) {
      if (auto builtin = info.settlement.type.getReferenceMaybe<MapLayoutTypes::Builtin>())
        builtin->buildingId = BuildingId("RUINS");
    } else {
      info.settlement.furniture = enemy.settlement.furniture;
      info.settlement.outsideFeatures = enemy.settlement.outsideFeatures;
      info.settlement.shopItems = enemy.settlement.shopItems;
    }
    if (info.levelConnection) {
      auto processLevel = [&](LevelConnection::EnemyLevelInfo& enemy) {
        if (auto extra = enemy.getReferenceMaybe<LevelConnection::ExtraEnemy>())
          for (auto& enemy : extra->enemyInfo) {
            if (makeRuins) {
              if (auto builtin = enemy.settlement.type.getReferenceMaybe<MapLayoutTypes::Builtin>())
                builtin->buildingId = BuildingId("RUINS");
              enemy.settlement.furniture.reset();
              enemy.settlement.outsideFeatures.reset();
            }
            enemy.settlement.corpses = enemy.settlement.inhabitants;
            enemy.settlement.inhabitants = InhabitantsInfo();
          }
      };
      for (auto& level : info.levelConnection->levels)
        processLevel(level.enemy);
      processLevel(info.levelConnection->topLevel);
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
