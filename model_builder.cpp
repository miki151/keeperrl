#include "stdafx.h"
#include "model_builder.h"
#include "level.h"
#include "tribe.h"
#include "inventory.h"
#include "collective_builder.h"
#include "options.h"
#include "player_control.h"
#include "spectator.h"
#include "creature.h"
#include "square.h"
#include "progress_meter.h"
#include "collective.h"
#include "level_maker.h"
#include "model.h"
#include "level_builder.h"
#include "monster_ai.h"
#include "game.h"
#include "campaign.h"
#include "creature_name.h"
#include "villain_type.h"
#include "enemy_factory.h"
#include "creature_group.h"
#include "view_object.h"
#include "item.h"
#include "furniture.h"
#include "sokoban_input.h"
#include "external_enemies.h"
#include "immigration.h"
#include "technology.h"
#include "keybinding.h"
#include "tutorial.h"
#include "settlement_info.h"
#include "enemy_info.h"
#include "game_time.h"
#include "lasting_effect.h"
#include "skill.h"
#include "game_config.h"
#include "build_info.h"
#include "tribe_alignment.h"
#include "resource_counts.h"
#include "content_factory.h"
#include "enemy_id.h"

using namespace std::chrono;

ModelBuilder::ModelBuilder(ProgressMeter* m, RandomGen& r, Options* o,
    SokobanInput* sok, ContentFactory* contentFactory, EnemyFactory enemyFactory)
    : random(r), meter(m), options(o), enemyFactory(std::move(enemyFactory)), sokobanInput(sok), contentFactory(contentFactory) {
}

ModelBuilder::~ModelBuilder() {
}

SettlementInfo& ModelBuilder::makeExtraLevel(WModel model, EnemyInfo& enemy) {
  const int towerHeight = random.get(7, 12);
  const int gnomeHeight = random.get(3, 5);
  SettlementInfo& mainSettlement = enemy.settlement;
  SettlementInfo& extraSettlement = enemy.levelConnection->otherEnemy->settlement;
  switch (enemy.levelConnection->type) {
    case LevelConnection::Type::TOWER: {
      StairKey downLink = StairKey::getNew();
      extraSettlement.upStairs = {downLink};
      for (int i : Range(towerHeight - 1)) {
        StairKey upLink = StairKey::getNew();
        model->buildLevel(
            LevelBuilder(meter, random, contentFactory, 4, 4, "Tower floor" + toString(i + 2)),
            LevelMaker::towerLevel(random,
                CONSTRUCT(SettlementInfo,
                  c.type = SettlementType::TOWER;
                  c.inhabitants.fighters = CreatureList(
                      random.get(1, 3),
                      random.choose(CreatureId("WATER_ELEMENTAL"), CreatureId("AIR_ELEMENTAL"),
                          CreatureId("FIRE_ELEMENTAL"), CreatureId("EARTH_ELEMENTAL")));
                  c.tribe = enemy.settlement.tribe;
                  c.collective = new CollectiveBuilder(CollectiveConfig::noImmigrants(), c.tribe);
                  c.upStairs = {upLink};
                  c.downStairs = {downLink};
                  c.furniture = FurnitureListId("towerInside");
                  if (enemy.levelConnection->deadInhabitants) {
                    c.corpses = c.inhabitants;
                    c.inhabitants = InhabitantsInfo{};
                  }
                  c.buildingId = extraSettlement.buildingId;)));
        downLink = upLink;
      }
      mainSettlement.downStairs = {downLink};
      model->buildLevel(
         LevelBuilder(meter, random, contentFactory, 5, 5, "Tower top"),
         LevelMaker::towerLevel(random, mainSettlement));
      return extraSettlement;
    }
    case LevelConnection::Type::CRYPT: {
      StairKey key = StairKey::getNew();
      extraSettlement.downStairs = {key};
      mainSettlement.upStairs = {key};
      model->buildLevel(
         LevelBuilder(meter, random, contentFactory, 40, 40, "Crypt"),
         LevelMaker::cryptLevel(random, mainSettlement));
      return extraSettlement;
    }
    case LevelConnection::Type::MAZE: {
      StairKey key = StairKey::getNew();
      extraSettlement.upStairs = {key};
      mainSettlement.downStairs = {key};
      model->buildLevel(
         LevelBuilder(meter, random, contentFactory, 40, 40, "Maze"),
         LevelMaker::mazeLevel(random, extraSettlement));
      return mainSettlement;
    }
    case LevelConnection::Type::GNOMISH_MINES: {
      StairKey upLink = StairKey::getNew();
      extraSettlement.downStairs = {upLink};
      for (int i : Range(gnomeHeight - 1)) {
        StairKey downLink = StairKey::getNew();
        model->buildLevel(
            LevelBuilder(meter, random, contentFactory, 60, 40, "Mines lvl " + toString(i + 1)),
            LevelMaker::roomLevel(random, CreatureGroup::gnomishMines(
                mainSettlement.tribe, TribeId::getMonster(), 0),
                CreatureGroup::waterCreatures(TribeId::getMonster()),
                CreatureGroup::lavaCreatures(TribeId::getMonster()), {upLink}, {downLink},
                FurnitureListId("roomFurniture")));
        upLink = downLink;
      }
      mainSettlement.upStairs = {upLink};
      model->buildLevel(
         LevelBuilder(meter, random, contentFactory, 60, 40, "Mine Town"),
         LevelMaker::mineTownLevel(random, mainSettlement));
      return extraSettlement;
    }
    case LevelConnection::Type::SOKOBAN:
      StairKey key = StairKey::getNew();
      extraSettlement.upStairs = {key};
      mainSettlement.downStairs = {key};
      Table<char> sokoLevel = sokobanInput->getNext();
      model->buildLevel(
          LevelBuilder(meter, random, contentFactory, sokoLevel.getBounds().width(), sokoLevel.getBounds().height(), "Sokoban"),
          LevelMaker::sokobanFromFile(random, mainSettlement, sokoLevel));
      return extraSettlement;
  }
}

PModel ModelBuilder::singleMapModel(const string& worldName, TribeId keeperTribe, TribeAlignment alignment) {
  return tryBuilding(10, [&] { return trySingleMapModel(worldName, keeperTribe, alignment);}, "single map");
}

vector<EnemyInfo> ModelBuilder::getSingleMapEnemiesForEvilKeeper(TribeId keeperTribe) {
  vector<EnemyInfo> enemies;
  for (int i : Range(random.get(3, 6)))
    enemies.push_back(enemyFactory->get(EnemyId("HUMAN_COTTAGE")));
  enemies.push_back(enemyFactory->get(EnemyId("KOBOLD_CAVE")));
  for (int i : Range(random.get(2, 4)))
    enemies.push_back(enemyFactory->get(random.choose({EnemyId("BANDITS"), EnemyId("COTTAGE_BANDITS")}, {3, 1})));
  enemies.push_back(enemyFactory->get(random.choose(EnemyId("GNOMES"), EnemyId("DARK_ELVES_ALLY"))).setVillainType(VillainType::ALLY));
  append(enemies, enemyFactory->getVaults(TribeAlignment::EVIL, keeperTribe));
  enemies.push_back(enemyFactory->get(EnemyId("ANTS_CLOSED")).setVillainType(VillainType::LESSER));
  enemies.push_back(enemyFactory->get(EnemyId("DWARVES")).setVillainType(VillainType::MAIN));
  enemies.push_back(enemyFactory->get(EnemyId("KNIGHTS")).setVillainType(VillainType::MAIN));
  enemies.push_back(enemyFactory->get(EnemyId("ADA_GOLEMS")));
  enemies.push_back(enemyFactory->get(random.choose(EnemyId("OGRE_CAVE"), EnemyId("HARPY_CAVE")))
      .setVillainType(VillainType::ALLY));
  enemies.push_back(enemyFactory->get(EnemyId("TEMPLE")));
  for (auto& enemy : random.chooseN(2, {
        EnemyId("ELEMENTALIST"),
        EnemyId("WARRIORS"),
        EnemyId("ELVES"),
        EnemyId("VILLAGE")}))
    enemies.push_back(enemyFactory->get(enemy).setVillainType(VillainType::MAIN));
  for (auto& enemy : random.chooseN(2, {
        EnemyId("GREEN_DRAGON"),
        EnemyId("SHELOB"),
        EnemyId("HYDRA"),
        EnemyId("RED_DRAGON"),
        EnemyId("CYCLOPS"),
        EnemyId("DRIADS"),
        EnemyId("ENTS")}))
    enemies.push_back(enemyFactory->get(enemy).setVillainType(VillainType::LESSER));
  for (auto& enemy : random.chooseN(1, {
        EnemyId("KRAKEN"),
        EnemyId("WITCH"),
        EnemyId("CEMETERY")}))
    enemies.push_back(enemyFactory->get(enemy));
  return enemies;
}

vector<EnemyInfo> ModelBuilder::getSingleMapEnemiesForLawfulKeeper(TribeId keeperTribe) {
  vector<EnemyInfo> enemies;
  for (int i : Range(random.get(3, 6)))
    enemies.push_back(enemyFactory->get(EnemyId("COTTAGE_BANDITS")));
  enemies.push_back(enemyFactory->get(EnemyId("DARK_ELF_CAVE")));
  for (int i : Range(random.get(2, 4)))
    enemies.push_back(enemyFactory->get(random.choose({EnemyId("BANDITS"), EnemyId("COTTAGE_BANDITS")}, {3, 1})));
  enemies.push_back(enemyFactory->get(EnemyId("GNOMES")).setVillainType(VillainType::ALLY));
  append(enemies, enemyFactory->getVaults(TribeAlignment::LAWFUL, keeperTribe));
  enemies.push_back(enemyFactory->get(EnemyId("ANTS_CLOSED")).setVillainType(VillainType::LESSER));
  enemies.push_back(enemyFactory->get(EnemyId("DARK_ELVES_ENEMY")).setVillainType(VillainType::MAIN));
  enemies.push_back(enemyFactory->get(EnemyId("DEMON_DEN")).setVillainType(VillainType::MAIN));
  enemies.push_back(enemyFactory->get(EnemyId("ADA_GOLEMS")));
  enemies.push_back(enemyFactory->get(EnemyId("EVIL_TEMPLE")));
  for (auto& enemy : random.chooseN(2, {
        EnemyId("LIZARDMEN"),
        EnemyId("MINOTAUR"),
        EnemyId("ORC_VILLAGE"),
        EnemyId("VILLAGE")}))
    enemies.push_back(enemyFactory->get(enemy).setVillainType(VillainType::MAIN));
  for (auto& enemy : random.chooseN(2, {
        EnemyId("GREEN_DRAGON"),
        EnemyId("SHELOB"),
        EnemyId("HYDRA"),
        EnemyId("RED_DRAGON"),
        EnemyId("CYCLOPS"),
        EnemyId("DRIADS"),
        EnemyId("ENTS")}))
    enemies.push_back(enemyFactory->get(enemy).setVillainType(VillainType::LESSER));
  for (auto& enemy : random.chooseN(1, {
        EnemyId("KRAKEN"),
        EnemyId("WITCH"),
        EnemyId("CEMETERY")}))
    enemies.push_back(enemyFactory->get(enemy));
  return enemies;
}

PModel ModelBuilder::trySingleMapModel(const string& worldName, TribeId keeperTribe, TribeAlignment alignment) {
  vector<EnemyInfo> enemyInfo;
  switch (alignment) {
    case TribeAlignment::EVIL:
      enemyInfo = getSingleMapEnemiesForEvilKeeper(keeperTribe);
      break;
    case TribeAlignment::LAWFUL:
      enemyInfo = getSingleMapEnemiesForLawfulKeeper(keeperTribe);
      break;
  }
  for (int i : Range(1, random.get(4)))
    enemyInfo.push_back(enemyFactory->get(EnemyId("RUINS")));
  return tryModel(304, worldName, enemyInfo, keeperTribe, BiomeId::GRASSLAND, {}, true);
}

void ModelBuilder::addMapVillainsForLawfulKeeper(vector<EnemyInfo>& enemyInfo, BiomeId biomeId) {
  switch (biomeId) {
    case BiomeId::GRASSLAND:
      for (int i : Range(random.get(3, 5)))
        enemyInfo.push_back(enemyFactory->get(EnemyId("COTTAGE_BANDITS")));
      break;
    case BiomeId::MOUNTAIN:
      for (int i : Range(random.get(1, 4)))
        enemyInfo.push_back(enemyFactory->get(EnemyId("DARK_ELF_CAVE")));
      for (int i : Range(random.get(0, 3)))
        enemyInfo.push_back(enemyFactory->get(random.choose({EnemyId("BANDITS"), EnemyId("NO_AGGRO_BANDITS")}, {1, 4})));
      break;
    case BiomeId::FORREST:
      for (int i : Range(random.get(3, 5)))
        enemyInfo.push_back(enemyFactory->get(EnemyId("LIZARDMEN_COTTAGE")));
      break;
  }
  if (random.chance(0.3))
    enemyInfo.push_back(enemyFactory->get(EnemyId("EVIL_TEMPLE")));
}

void ModelBuilder::addMapVillainsForEvilKeeper(vector<EnemyInfo>& enemyInfo, BiomeId biomeId) {
  switch (biomeId) {
    case BiomeId::GRASSLAND:
      for (int i : Range(random.get(3, 5)))
        enemyInfo.push_back(enemyFactory->get(EnemyId("HUMAN_COTTAGE")));
      if (random.roll(2))
        enemyInfo.push_back(enemyFactory->get(EnemyId("COTTAGE_BANDITS")));
      break;
    case BiomeId::MOUNTAIN:
      for (int i : Range(random.get(1, 4)))
        enemyInfo.push_back(enemyFactory->get(random.choose(EnemyId("DWARF_CAVE"), EnemyId("KOBOLD_CAVE"))));
      for (int i : Range(random.get(0, 3)))
        enemyInfo.push_back(enemyFactory->get(random.choose({EnemyId("BANDITS"), EnemyId("NO_AGGRO_BANDITS")}, {1, 4})));
      break;
    case BiomeId::FORREST:
      for (int i : Range(random.get(3, 5)))
        enemyInfo.push_back(enemyFactory->get(EnemyId("ELVEN_COTTAGE")));
      break;
  }
  if (random.chance(0.3))
    enemyInfo.push_back(enemyFactory->get(EnemyId("TEMPLE")));
}

PModel ModelBuilder::tryCampaignBaseModel(const string& siteName, TribeId keeperTribe, TribeAlignment alignment,
    optional<ExternalEnemiesType> externalEnemiesType) {
  vector<EnemyInfo> enemyInfo;
  BiomeId biome = BiomeId::MOUNTAIN;
  switch (alignment) {
    case TribeAlignment::EVIL:
      enemyInfo.push_back(enemyFactory->get(EnemyId("DWARF_CAVE")));
      enemyInfo.push_back(enemyFactory->get(EnemyId("BANDITS")));
      enemyInfo.push_back(enemyFactory->get(EnemyId("ANTS_CLOSED_SMALL")));
      enemyInfo.push_back(enemyFactory->get(EnemyId("TUTORIAL_VILLAGE")));
      if (random.chance(0.3))
        enemyInfo.push_back(enemyFactory->get(EnemyId("TEMPLE")));
      break;
    case TribeAlignment::LAWFUL:
      enemyInfo.push_back(enemyFactory->get(EnemyId("DARK_ELF_CAVE")));
      enemyInfo.push_back(enemyFactory->get(EnemyId("ORC_CAVE")));
      enemyInfo.push_back(enemyFactory->get(EnemyId("ANTS_CLOSED_SMALL")));
      enemyInfo.push_back(enemyFactory->get(EnemyId("COTTAGE_BANDITS")));
      if (random.chance(0.3))
        enemyInfo.push_back(enemyFactory->get(EnemyId("EVIL_TEMPLE")));
      break;
  }
  append(enemyInfo, enemyFactory->getVaults(alignment, keeperTribe));
  if (random.chance(0.3))
    enemyInfo.push_back(enemyFactory->get(EnemyId("KRAKEN")));
  optional<ExternalEnemies> externalEnemies;
  if (externalEnemiesType)
    externalEnemies = ExternalEnemies(random, &contentFactory->creatures, enemyFactory->getExternalEnemies(), *externalEnemiesType);
  for (int i : Range(random.get(3)))
    enemyInfo.push_back(enemyFactory->get(EnemyId("RUINS")));
  return tryModel(174, siteName, enemyInfo, keeperTribe, biome, std::move(externalEnemies), true);
}

PModel ModelBuilder::tryTutorialModel(const string& siteName) {
  vector<EnemyInfo> enemyInfo;
  BiomeId biome = BiomeId::MOUNTAIN;
  //enemyInfo.push_back(enemyFactory->get(EnemyId("RUINS")));
  /*enemyInfo.push_back(enemyFactory->get(EnemyId("BANDITS")));
  enemyInfo.push_back(enemyFactory->get(EnemyId("ADA_GOLEMS")));*/
  //enemyInfo.push_back(enemyFactory->get(EnemyId("TEMPLE")));
  enemyInfo.push_back(enemyFactory->get(EnemyId("TUTORIAL_VILLAGE")));
  return tryModel(174, siteName, enemyInfo, TribeId::getDarkKeeper(), biome, {}, false);
}

static optional<BiomeId> getBiome(EnemyInfo& enemy, RandomGen& random) {
  switch (enemy.settlement.type) {
    case SettlementType::CASTLE:
    case SettlementType::CASTLE2:
    case SettlementType::TOWER:
    case SettlementType::VILLAGE:
    case SettlementType::SWAMP:
      return BiomeId::GRASSLAND;
    case SettlementType::CAVE:
    case SettlementType::MINETOWN:
    case SettlementType::SMALL_MINETOWN:
    case SettlementType::ANT_NEST:
    case SettlementType::VAULT:
      return BiomeId::MOUNTAIN;
    case SettlementType::FORREST_COTTAGE:
    case SettlementType::FORREST_VILLAGE:
    case SettlementType::ISLAND_VAULT_DOOR:
    case SettlementType::FOREST:
      return BiomeId::FORREST;
    case SettlementType::CEMETERY:
      return random.choose(BiomeId::GRASSLAND, BiomeId::FORREST);
    default: return none;
  }
}

PModel ModelBuilder::tryCampaignSiteModel(const string& siteName, EnemyId enemyId, VillainType type, TribeAlignment alignment) {
  vector<EnemyInfo> enemyInfo { enemyFactory->get(enemyId).setVillainType(type)};
  auto biomeId = getBiome(enemyInfo[0], random);
  CHECK(biomeId) << "Unimplemented enemy in campaign " << enemyId.data();
  switch (alignment) {
    case TribeAlignment::EVIL:
      addMapVillainsForEvilKeeper(enemyInfo, *biomeId);
      break;
    case TribeAlignment::LAWFUL:
      addMapVillainsForLawfulKeeper(enemyInfo, *biomeId);
      break;
  }
  for (int i : Range(random.get(3)))
    enemyInfo.push_back(enemyFactory->get(EnemyId("RUINS")));
  return tryModel(114, siteName, enemyInfo, none, *biomeId, {}, true);
}

PModel ModelBuilder::tryBuilding(int numTries, function<PModel()> buildFun, const string& name) {
  for (int i : Range(numTries)) {
    try {
      if (meter)
        meter->reset();
      return buildFun();
    } catch (LevelGenException) {
      INFO << "Retrying level gen";
    }
  }
  FATAL << "Couldn't generate a level: " << name;
  return nullptr;
}

PModel ModelBuilder::campaignBaseModel(const string& siteName, TribeId keeperTribe, TribeAlignment alignment,
    optional<ExternalEnemiesType> externalEnemies) {
  return tryBuilding(20, [=] { return tryCampaignBaseModel(siteName, keeperTribe, alignment, externalEnemies); }, "campaign base");
}

PModel ModelBuilder::tutorialModel(const string& siteName) {
  return tryBuilding(20, [=] { return tryTutorialModel(siteName); }, "tutorial");
}

PModel ModelBuilder::campaignSiteModel(const string& siteName, EnemyId enemyId, VillainType type, TribeAlignment alignment) {
  return tryBuilding(20, [&] { return tryCampaignSiteModel(siteName, enemyId, type, alignment); }, enemyId.data());
}

void ModelBuilder::measureSiteGen(int numTries, vector<string> types) {
  if (types.empty()) {
    types = {"single_map", "campaign_base", "tutorial"};
    for (auto id : enemyFactory->getAllIds()) {
      auto enemy = enemyFactory->get(id);
      if (!!getBiome(enemy, random))
        types.push_back(id.data());
    }
  }
  vector<function<void()>> tasks;
  auto tribe = TribeId::getDarkKeeper();
  for (auto& type : types) {
    if (type == "single_map")
      for (auto alignment : ENUM_ALL(TribeAlignment))
        tasks.push_back([=] { measureModelGen(type, numTries, [&] { trySingleMapModel("pok", tribe, alignment); }); });
    else if (type == "campaign_base")
      for (auto alignment : ENUM_ALL(TribeAlignment))
        tasks.push_back([=] { measureModelGen(type, numTries,
            [&] { tryCampaignBaseModel("pok", tribe, alignment, none); }); });
    else if (type == "tutorial")
      tasks.push_back([=] { measureModelGen(type, numTries, [&] { tryTutorialModel("pok"); }); });
    else {
      auto id = EnemyId(type.data());
      for (auto alignment : ENUM_ALL(TribeAlignment))
        tasks.push_back([=] { measureModelGen(type, numTries, [&] {
            tryCampaignSiteModel("", id, VillainType::LESSER, alignment); }); });
    }
  }
  for (auto& t : tasks)
    t();
}

void ModelBuilder::measureModelGen(const string& name, int numTries, function<void()> genFun) {
  int numSuccess = 0;
  int maxT = 0;
  int minT = 1000000;
  double sumT = 0;
  std::cout << name;
  for (int i : Range(numTries)) {
#ifndef OSX // this triggers some compiler errors OSX, I don't need it there anyway.
    auto time = steady_clock::now();
#endif
    try {
      genFun();
      ++numSuccess;
      std::cout << ".";
      std::cout.flush();
    } catch (LevelGenException) {
      std::cout << "x";
      std::cout.flush();
    }
#ifndef OSX
    int millis = duration_cast<milliseconds>(steady_clock::now() - time).count();
    sumT += millis;
    maxT = max(maxT, millis);
    minT = min(minT, millis);
#endif
  }
  std::cout << std::endl << numSuccess << " / " << numTries << ". MinT: " <<
    minT << ". MaxT: " << maxT << ". AvgT: " << sumT / numTries << std::endl;
}

PModel ModelBuilder::tryModel(int width, const string& levelName, vector<EnemyInfo> enemyInfo,
    optional<TribeId> keeperTribe, BiomeId biomeId, optional<ExternalEnemies> externalEnemies, bool hasWildlife) {
  auto model = Model::create(contentFactory);
  vector<SettlementInfo> topLevelSettlements;
  vector<EnemyInfo> extraEnemies;
  for (auto& elem : enemyInfo) {
    elem.settlement.collective = new CollectiveBuilder(elem.config, elem.settlement.tribe);
    if (elem.levelConnection) {
      elem.levelConnection->otherEnemy->settlement.collective =
          new CollectiveBuilder(elem.levelConnection->otherEnemy->config,
                                elem.levelConnection->otherEnemy->settlement.tribe);
      topLevelSettlements.push_back(makeExtraLevel(model.get(), elem));
      extraEnemies.push_back(*elem.levelConnection->otherEnemy);
    } else
      topLevelSettlements.push_back(elem.settlement);
  }
  append(enemyInfo, extraEnemies);
  optional<CreatureGroup> wildlife;
  if (hasWildlife)
    wildlife = CreatureGroup::forrest(TribeId::getWildlife());
  WLevel top =  model->buildMainLevel(
      LevelBuilder(meter, random, contentFactory, width, width, levelName, false),
      LevelMaker::topLevel(random, wildlife, topLevelSettlements, width,
        keeperTribe, biomeId, *chooseResourceCounts(random, contentFactory->resources, 0)));
  model->calculateStairNavigation();
  for (auto& enemy : enemyInfo) {
    if (enemy.settlement.locationName)
      enemy.settlement.collective->setLocationName(*enemy.settlement.locationName);
    if (auto race = enemy.settlement.race)
      enemy.settlement.collective->setRaceName(*race);
    if (enemy.discoverable)
      enemy.settlement.collective->setDiscoverable();
    PCollective collective = enemy.settlement.collective->build(contentFactory);
    collective->setImmigration(makeOwner<Immigration>(collective.get(), std::move(enemy.immigrants)));
    auto control = VillageControl::create(collective.get(), enemy.behaviour);
    if (enemy.villainType)
      collective->setVillainType(*enemy.villainType);
    if (enemy.id)
      collective->setEnemyId(*enemy.id);
    collective->setControl(std::move(control));
    model->collectives.push_back(std::move(collective));
  }
  if (externalEnemies)
    model->addExternalEnemies(std::move(*externalEnemies));
  return model;
}

PModel ModelBuilder::splashModel(const FilePath& splashPath) {
  auto m = Model::create(contentFactory);
  WLevel l = m->buildMainLevel(
      LevelBuilder(meter, Random, contentFactory, Level::getSplashBounds().width(),
          Level::getSplashBounds().height(), "Splash", true, 1.0),
      LevelMaker::splashLevel(
          CreatureGroup::splashLeader(TribeId::getHuman()),
          CreatureGroup::splashHeroes(TribeId::getHuman()),
          CreatureGroup::splashMonsters(TribeId::getDarkKeeper()),
          CreatureGroup::singleType(TribeId::getDarkKeeper(), CreatureId("IMP")), splashPath));
  return m;
}

PModel ModelBuilder::battleModel(const FilePath& levelPath, CreatureList allies, CreatureList enemies) {
  auto m = Model::create(contentFactory);
  ifstream stream(levelPath.getPath());
  Table<char> level = *SokobanInput::readTable(stream);
  WLevel l = m->buildMainLevel(
      LevelBuilder(meter, Random, contentFactory, level.getBounds().width(), level.getBounds().height(), "Battle", true, 1.0),
      LevelMaker::battleLevel(level, allies, enemies));
  return m;
}
