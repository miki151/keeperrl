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
#include "level_maker.h"
#include "model.h"
#include "level_builder.h"
#include "monster_ai.h"
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

ModelBuilder::LevelMakerMethod ModelBuilder::getMaker(LevelType type) {
  switch (type) {
    case LevelType::TOWER:
      return &LevelMaker::towerLevel;
    case LevelType::MAZE:
      return &LevelMaker::mazeLevel;
    case LevelType::DUNGEON:
      return &LevelMaker::roomLevel;
    case LevelType::MINETOWN:
      return &LevelMaker::mineTownLevel;
    case LevelType::SOKOBAN: {
      Table<char> sokoLevel = sokobanInput->getNext();
      return [sokoLevel](RandomGen& random, SettlementInfo info) {
        return LevelMaker::sokobanFromFile(random, info, sokoLevel);
      };
    }
  }
}

void ModelBuilder::makeExtraLevel(WModel model, LevelConnection& connection, SettlementInfo& mainSettlement, StairKey upLink) {
  StairKey downLink = StairKey::getNew();
  for (int index : All(connection.levels)) {
    auto& level = connection.levels[index];
    auto addLevel = [&] (SettlementInfo& settlement) {
      settlement.upStairs = {upLink};
      if (index < connection.levels.size() - 1)
        settlement.downStairs = {downLink};
      if (connection.direction == LevelConnectionDir::UP)
        swap(settlement.upStairs, settlement.downStairs);
      model->buildLevel(
          LevelBuilder(meter, random, contentFactory, level.levelSize.x, level.levelSize.y),
          getMaker(level.levelType)(random, settlement));
      upLink = downLink;
      downLink = StairKey::getNew();
    };
    level.enemy.match(
        [&](LevelConnection::ExtraEnemy& e) {
          for (auto& enemy : e.enemyInfo)
            addLevel(enemy.settlement);
        },
        [&](LevelConnection::MainEnemy&) {
          addLevel(mainSettlement);
        }
    );
  }
}

PModel ModelBuilder::singleMapModel(TribeId keeperTribe, TribeAlignment alignment) {
  return tryBuilding(10, [&] { return trySingleMapModel(keeperTribe, alignment);}, "single map");
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
        EnemyId("BLACK_DRAGON"),
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
        EnemyId("BLACK_DRAGON"),
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

PModel ModelBuilder::trySingleMapModel(TribeId keeperTribe, TribeAlignment alignment) {
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
  return tryModel(304, enemyInfo, keeperTribe, BiomeId::GRASSLAND, {}, true);
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
    case BiomeId::DESERT:
      for (int i : Range(random.get(3, 5)))
        enemyInfo.push_back(enemyFactory->get(EnemyId("LIZARDMEN_SMALL")));
      break;
    case BiomeId::SNOW:
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
    case BiomeId::DESERT:
      for (int i : Range(random.get(3, 5)))
        enemyInfo.push_back(enemyFactory->get(EnemyId("LIZARDMEN_SMALL")));
      break;
    case BiomeId::SNOW:
      for (int i : Range(random.get(3, 5)))
        enemyInfo.push_back(enemyFactory->get(EnemyId("ESKIMO_COTTAGE")));
      break;
  }
  if (random.chance(0.3))
    enemyInfo.push_back(enemyFactory->get(EnemyId("TEMPLE")));
}

PModel ModelBuilder::tryCampaignBaseModel(TribeId keeperTribe, TribeAlignment alignment,
    optional<ExternalEnemiesType> externalEnemiesType) {
  vector<EnemyInfo> enemyInfo;
  BiomeId biome = BiomeId::MOUNTAIN;
  switch (alignment) {
    case TribeAlignment::EVIL:
      enemyInfo.push_back(enemyFactory->get(EnemyId("DWARF_CAVE")));
      enemyInfo.push_back(enemyFactory->get(EnemyId("BANDITS")));
      enemyInfo.push_back(enemyFactory->get(EnemyId("ANTS_CLOSED_SMALL")));
      enemyInfo.push_back(enemyFactory->get(EnemyId("TUTORIAL_VILLAGE")));
      /*if (random.chance(0.3))
        enemyInfo.push_back(enemyFactory->get(EnemyId("TEMPLE")));*/
      break;
    case TribeAlignment::LAWFUL:
      enemyInfo.push_back(enemyFactory->get(EnemyId("DARK_ELF_CAVE")));
      enemyInfo.push_back(enemyFactory->get(EnemyId("ORC_CAVE")));
      enemyInfo.push_back(enemyFactory->get(EnemyId("ANTS_CLOSED_SMALL")));
      enemyInfo.push_back(enemyFactory->get(EnemyId("COTTAGE_BANDITS")));
      /*if (random.chance(0.3))
        enemyInfo.push_back(enemyFactory->get(EnemyId("EVIL_TEMPLE")));*/
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
  return tryModel(174, enemyInfo, keeperTribe, biome, std::move(externalEnemies), true);
}

PModel ModelBuilder::tryTutorialModel() {
  vector<EnemyInfo> enemyInfo;
  BiomeId biome = BiomeId::MOUNTAIN;
  //enemyInfo.push_back(enemyFactory->get(EnemyId("RED_DRAGON")));
  //enemyInfo.push_back(enemyFactory->get(EnemyId("RUINS")));
  /*enemyInfo.push_back(enemyFactory->get(EnemyId("BANDITS")));
  enemyInfo.push_back(enemyFactory->get(EnemyId("ADA_GOLEMS")));*/
  //enemyInfo.push_back(enemyFactory->get(EnemyId("TEMPLE")));
  enemyInfo.push_back(enemyFactory->get(EnemyId("ESKIMO_COTTAGE")));
  return tryModel(174, enemyInfo, TribeId::getDarkKeeper(), biome, {}, false);
}

static optional<BiomeId> getBiome(const EnemyInfo& enemy, RandomGen& random) {
  if (!enemy.biomes.empty())
    return random.choose(enemy.biomes);
  switch (enemy.settlement.type) {
    case SettlementType::CASTLE:
    case SettlementType::CASTLE2:
    case SettlementType::TOWER:
      return BiomeId::GRASSLAND;
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

PModel ModelBuilder::tryCampaignSiteModel(EnemyId enemyId, VillainType type, TribeAlignment alignment) {
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
  return tryModel(114, enemyInfo, none, *biomeId, {}, true);
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

PModel ModelBuilder::campaignBaseModel(TribeId keeperTribe, TribeAlignment alignment,
    optional<ExternalEnemiesType> externalEnemies) {
  return tryBuilding(20, [=] { return tryCampaignBaseModel(keeperTribe, alignment, externalEnemies); }, "campaign base");
}

PModel ModelBuilder::tutorialModel() {
  return tryBuilding(20, [=] { return tryTutorialModel(); }, "tutorial");
}

PModel ModelBuilder::campaignSiteModel(EnemyId enemyId, VillainType type, TribeAlignment alignment) {
  return tryBuilding(20, [&] { return tryCampaignSiteModel(enemyId, type, alignment); }, enemyId.data());
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
        tasks.push_back([=] { measureModelGen(type, numTries, [&] { trySingleMapModel(tribe, alignment); }); });
    else if (type == "campaign_base")
      for (auto alignment : ENUM_ALL(TribeAlignment))
        tasks.push_back([=] { measureModelGen(type, numTries,
            [&] { tryCampaignBaseModel(tribe, alignment, none); }); });
    else if (type == "tutorial")
      tasks.push_back([=] { measureModelGen(type, numTries, [&] { tryTutorialModel(); }); });
    else {
      auto id = EnemyId(type.data());
      for (auto alignment : ENUM_ALL(TribeAlignment))
        tasks.push_back([=] { measureModelGen(type, numTries, [&] {
            tryCampaignSiteModel(id, VillainType::LESSER, alignment); }); });
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

static optional<CreatureGroup> getWildlife(BiomeId id) {
  switch (id) {
    case BiomeId::DESERT:
      return CreatureGroup::singleType(TribeId::getWildlife(), CreatureId("SNAKE"));
    default:
      return CreatureGroup::forrest(TribeId::getWildlife());
  }
}

PModel ModelBuilder::tryModel(int width, vector<EnemyInfo> enemyInfo, optional<TribeId> keeperTribe, BiomeId biomeId,
    optional<ExternalEnemies> externalEnemies, bool hasWildlife) {
  auto model = Model::create(contentFactory);
  vector<SettlementInfo> topLevelSettlements;
  vector<EnemyInfo> extraEnemies;
  for (auto& elem : enemyInfo) {
    elem.settlement.collective = new CollectiveBuilder(elem.config, elem.settlement.tribe);
    if (elem.levelConnection) {
      auto processExtraEnemy = [&] (LevelConnection::EnemyLevelInfo& enemyInfo) {
        if (auto extra = enemyInfo.getReferenceMaybe<LevelConnection::ExtraEnemy>()) {
          for (auto& enemy : extra->enemyInfo) {
            enemy.settlement.collective =
                new CollectiveBuilder(enemy.config, enemy.settlement.tribe);
            extraEnemies.push_back(enemy);
          }
        }
      };
      for (auto& level : elem.levelConnection->levels)
        processExtraEnemy(level.enemy);
      processExtraEnemy(elem.levelConnection->topLevel);
      StairKey downLink = StairKey::getNew();
      makeExtraLevel(model.get(), *elem.levelConnection, elem.settlement, downLink);
      if (auto extra = elem.levelConnection->topLevel.getReferenceMaybe<LevelConnection::ExtraEnemy>()) {
        for (auto& enemy : extra->enemyInfo)
          topLevelSettlements.push_back(enemy.settlement);
      } else
        topLevelSettlements.push_back(elem.settlement);
      (elem.levelConnection->direction == LevelConnectionDir::DOWN ?
            topLevelSettlements.back().downStairs : topLevelSettlements.back().upStairs) = {downLink};
    } else
      topLevelSettlements.push_back(elem.settlement);
  }
  append(enemyInfo, extraEnemies);
  optional<CreatureGroup> wildlife;
  if (hasWildlife)
    wildlife = getWildlife(biomeId);
  model->buildMainLevel(
      LevelBuilder(meter, random, contentFactory, width, width, false),
      LevelMaker::topLevel(random, wildlife, topLevelSettlements, width,
        keeperTribe, biomeId, *chooseResourceCounts(random, contentFactory->resources, 0)));
  model->calculateStairNavigation();
  for (auto& enemy : enemyInfo)
    model->addCollective(enemy.buildCollective(contentFactory));
  if (externalEnemies)
    model->addExternalEnemies(std::move(*externalEnemies));
  return model;
}

PModel ModelBuilder::splashModel(const FilePath& splashPath) {
  auto m = Model::create(contentFactory);
  WLevel l = m->buildMainLevel(
      LevelBuilder(meter, Random, contentFactory, Level::getSplashBounds().width(),
          Level::getSplashBounds().height(), true, 1.0),
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
      LevelBuilder(meter, Random, contentFactory, level.getBounds().width(), level.getBounds().height(), true, 1.0),
      LevelMaker::battleLevel(level, allies, enemies));
  return m;
}
