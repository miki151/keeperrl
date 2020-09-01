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
#include "biome_id.h"
#include "zlevel.h"

using namespace std::chrono;

ModelBuilder::ModelBuilder(ProgressMeter* m, RandomGen& r, Options* o,
    SokobanInput* sok, ContentFactory* contentFactory, EnemyFactory enemyFactory)
    : random(r), meter(m), enemyFactory(std::move(enemyFactory)), sokobanInput(sok), contentFactory(contentFactory) {
}

ModelBuilder::~ModelBuilder() {
}

ModelBuilder::LevelMakerMethod ModelBuilder::getMaker(LevelType type) {
  switch (type) {
    case LevelType::BASIC:
      return [this](RandomGen& random, SettlementInfo info, Vec2 size) {
        return LevelMaker::settlementLevel(*contentFactory, random, info, size);
      };
    case LevelType::TOWER:
      return &LevelMaker::towerLevel;
    case LevelType::MAZE:
      return &LevelMaker::mazeLevel;
    case LevelType::DUNGEON:
      return &LevelMaker::roomLevel;
    case LevelType::MINETOWN:
      return &LevelMaker::mineTownLevel;
    case LevelType::BLACK_MARKET:
      return &LevelMaker::blackMarket;
    case LevelType::OUTBACK:
      return [this] (RandomGen& random, SettlementInfo info, Vec2) {
        auto& biomeInfo = contentFactory->biomeInfo.at(BiomeId("OUTBACK"));
        auto natives = enemyFactory->get(EnemyId("NATIVE_VILLAGE"));
        natives.settlement.collective = new CollectiveBuilder(natives.config, natives.settlement.tribe);
        return LevelMaker::topLevel(random, {info, natives.settlement}, 100, none, biomeInfo, ResourceCounts{},
            *contentFactory);
      };
    case LevelType::SOKOBAN: {
      Table<char> sokoLevel = sokobanInput->getNext();
      return [sokoLevel](RandomGen& random, SettlementInfo info, Vec2) {
        return LevelMaker::sokobanFromFile(random, info, sokoLevel);
      };
    }
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
  return tryModel(304, enemyInfo, keeperTribe, BiomeId("GRASSLAND"), {});
}

void ModelBuilder::addMapVillains(vector<EnemyInfo>& enemyInfo, const vector<BiomeEnemyInfo>& info) {
  for (auto& enemy : info)
    if (random.chance(enemy.probability))
      for (int i : Range(random.get(enemy.count)))
        enemyInfo.push_back(enemyFactory->get(enemy.id));
}

PModel ModelBuilder::tryCampaignBaseModel(TribeId keeperTribe, TribeAlignment alignment, BiomeId biome,
    optional<ExternalEnemiesType> externalEnemiesType) {
  vector<EnemyInfo> enemyInfo;
  auto& biomeInfo = contentFactory->biomeInfo.at(biome);
  addMapVillains(enemyInfo, alignment == TribeAlignment::EVIL ? biomeInfo.darkKeeperBaseEnemies : biomeInfo.whiteKeeperBaseEnemies);
  append(enemyInfo, enemyFactory->getVaults(alignment, keeperTribe));
  optional<ExternalEnemies> externalEnemies;
  if (externalEnemiesType)
    externalEnemies = ExternalEnemies(random, &contentFactory->getCreatures(), enemyFactory->getExternalEnemies(), *externalEnemiesType);
  return tryModel(174, enemyInfo, keeperTribe, biome, std::move(externalEnemies));
}

PModel ModelBuilder::tryTutorialModel() {
  vector<EnemyInfo> enemyInfo;
  BiomeId biome = BiomeId("MOUNTAIN");
  //enemyInfo.push_back(enemyFactory->get(EnemyId("CORPSES")));
  //enemyInfo.push_back(enemyFactory->get(EnemyId("RED_DRAGON")));
  //enemyInfo.push_back(enemyFactory->get(EnemyId("RUINS")));
  enemyInfo.push_back(enemyFactory->get(EnemyId("TUTORIAL_VILLAGE")));
  //enemyInfo.push_back(enemyFactory->get(EnemyId("ADA_GOLEMS")));
  //enemyInfo.push_back(enemyFactory->get(EnemyId("TEMPLE")));
  //enemyInfo.push_back(enemyFactory->get(EnemyId("LIZARDMEN")));
  return tryModel(174, enemyInfo, TribeId::getDarkKeeper(), biome, {});
}

static optional<BiomeId> getBiome(const EnemyInfo& enemy, RandomGen& random) {
  if (!enemy.biomes.empty())
    return random.choose(enemy.biomes);
  return enemy.settlement.type.visit(
      [&](const MapLayoutTypes::Builtin& type) -> optional<BiomeId> {
        switch (type.id) {
          case BuiltinLayoutId::CASTLE:
          case BuiltinLayoutId::CASTLE2:
          case BuiltinLayoutId::TOWER:
          case BuiltinLayoutId::VILLAGE:
          case BuiltinLayoutId::SWAMP:
            return BiomeId("GRASSLAND");
          case BuiltinLayoutId::CAVE:
          case BuiltinLayoutId::MINETOWN:
          case BuiltinLayoutId::SMALL_MINETOWN:
          case BuiltinLayoutId::ANT_NEST:
          case BuiltinLayoutId::VAULT:
            return BiomeId("MOUNTAIN");
          case BuiltinLayoutId::FORREST_COTTAGE:
          case BuiltinLayoutId::FORREST_VILLAGE:
          case BuiltinLayoutId::ISLAND_VAULT_DOOR:
          case BuiltinLayoutId::FOREST:
            return BiomeId("FOREST");
          case BuiltinLayoutId::CEMETERY:
            return random.choose(BiomeId("GRASSLAND"), BiomeId("FOREST"));
          default: return none;
        }
      },
      [&](const auto&) -> optional<BiomeId> {
        return BiomeId("GRASSLAND");
      }
    );
}

PModel ModelBuilder::tryCampaignSiteModel(EnemyId enemyId, VillainType type, TribeAlignment alignment) {
  vector<EnemyInfo> enemyInfo { enemyFactory->get(enemyId).setVillainType(type)};
  if (auto id = enemyInfo[0].otherEnemy)
    enemyInfo.push_back(enemyFactory->get(*id));
  auto biomeId = getBiome(enemyInfo[0], random);
  CHECK(biomeId) << "Unimplemented enemy in campaign " << enemyId.data();
  auto& biomeInfo = contentFactory->biomeInfo.at(*biomeId);
  addMapVillains(enemyInfo, alignment == TribeAlignment::EVIL ? biomeInfo.darkKeeperEnemies : biomeInfo.whiteKeeperEnemies);
  return tryModel(114, enemyInfo, none, *biomeId, {});
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
  USER_FATAL << "Couldn't generate a level: " << name;
  return nullptr;
}

PModel ModelBuilder::campaignBaseModel(TribeId keeperTribe, TribeAlignment alignment, BiomeId biome,
    optional<ExternalEnemiesType> externalEnemies) {
  return tryBuilding(20, [=] { return tryCampaignBaseModel(keeperTribe, alignment, biome, externalEnemies); }, "campaign base");
}

PModel ModelBuilder::tutorialModel() {
  return tryBuilding(20, [=] { return tryTutorialModel(); }, "tutorial");
}

PModel ModelBuilder::campaignSiteModel(EnemyId enemyId, VillainType type, TribeAlignment alignment) {
  return tryBuilding(20, [&] { return tryCampaignSiteModel(enemyId, type, alignment); }, enemyId.data());
}

void ModelBuilder::measureSiteGen(int numTries, vector<string> types, vector<BiomeId> biomes) {
  if (types.empty()) {
    types = {"single_map", "campaign_base", "tutorial", "zlevels"};
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
        for (auto biome : biomes)
          tasks.push_back([=] { measureModelGen(type + " (" + EnumInfo<TribeAlignment>::getString(alignment) + ", "
              + biome.data() + ")", numTries,
              [&] { tryCampaignBaseModel(tribe, alignment, biome, none); }); });
    else if (type == "zlevels")
      for (auto alignment : ENUM_ALL(TribeAlignment))
        for (int i : Range(1, 30))
          tasks.push_back([=] { measureModelGen(type + " " + toString(i) +
              " (" + EnumInfo<TribeAlignment>::getString(alignment) + ")",
              numTries,
              [&] {
                auto model = tryCampaignBaseModel(tribe, alignment, BiomeId("GRASSLAND"), none);
                auto maker = getLevelMaker(Random, contentFactory, alignment,
                    i, TribeId::getDarkKeeper(), StairKey::getNew());
                LevelBuilder(Random, contentFactory, maker.levelWidth, maker.levelWidth, true)
                    .build(model.get(), maker.maker.get(), 123);
              }); });
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
  USER_INFO << "Testing " << name;
  for (int i : Range(numTries)) {
#ifndef OSX // this triggers some compiler errors OSX, I don't need it there anyway.
    auto time = steady_clock::now();
#endif
    try {
      genFun();
      ++numSuccess;
      //std::cout << ".";
      //std::cout.flush();
    } catch (LevelGenException) {
      //std::cout << "x";
      //std::cout.flush();
    }
#ifndef OSX
    int millis = duration_cast<milliseconds>(steady_clock::now() - time).count();
    sumT += millis;
    maxT = max(maxT, millis);
    minT = min(minT, millis);
#endif
  }
  USER_INFO << numSuccess << " / " << numTries << ". MinT: " <<
    minT << ". MaxT: " << maxT << ". AvgT: " << sumT / numTries;
}

void ModelBuilder::makeExtraLevel(WModel model, LevelConnection& connection, SettlementInfo& mainSettlement,
    StairKey upLink, vector<EnemyInfo>& extraEnemies, int depth) {
  StairKey downLink = StairKey::getNew();
  int direction = connection.direction == LevelConnectionDir::DOWN ? 1 : -1;
  for (int index : All(connection.levels)) {
    auto& level = connection.levels[index];
    auto addLevel = [&] (SettlementInfo& settlement, bool bottomLevel, int depth) {
      settlement.upStairs.push_back(upLink);
      if (!bottomLevel)
        settlement.downStairs.push_back(downLink);
      if (direction == -1)
        swap(settlement.upStairs, settlement.downStairs);
      auto res = model->buildLevel(
          LevelBuilder(meter, random, contentFactory, level.levelSize.x, level.levelSize.y, true, level.isLit ? 1.0 : 0.0),
          getMaker(level.levelType)(random, settlement, level.levelSize), depth, level.name);
      res->canTranfer = level.canTransfer;
      upLink = downLink;
      downLink = StairKey::getNew();
    };
    level.enemy.match(
        [&](LevelConnection::ExtraEnemy& e) {
          for (int i : All(e.enemyInfo)) {
            auto& set = processLevelConnection(model, e.enemyInfo[i], extraEnemies, depth + direction * (index + i));
            addLevel(set, index == connection.levels.size() - 1 && i == e.enemyInfo.size() - 1,
                depth + direction * (index + i));
          }
        },
        [&](LevelConnection::MainEnemy&) {
          addLevel(mainSettlement, index == connection.levels.size() - 1, depth + direction * index);
        }
    );
  }
}

SettlementInfo& ModelBuilder::processLevelConnection(Model* model, EnemyInfo& enemyInfo,
    vector<EnemyInfo>& extraEnemies, int depth) {
  if (!enemyInfo.levelConnection)
    return enemyInfo.settlement;
  auto processExtraEnemy = [&] (LevelConnection::EnemyLevelInfo& enemyInfo) {
    if (auto extra = enemyInfo.getReferenceMaybe<LevelConnection::ExtraEnemy>()) {
      for (auto& enemy : extra->enemyInfo) {
        enemy.settlement.collective =
            new CollectiveBuilder(enemy.config, enemy.settlement.tribe);
        extraEnemies.push_back(enemy);
      }
    }
  };
  for (auto& level : enemyInfo.levelConnection->levels)
    processExtraEnemy(level.enemy);
  processExtraEnemy(enemyInfo.levelConnection->topLevel);
  StairKey downLink = StairKey::getNew();
  bool goingDown = enemyInfo.levelConnection->direction == LevelConnectionDir::DOWN;
  makeExtraLevel(model, *enemyInfo.levelConnection, enemyInfo.settlement, downLink, extraEnemies, depth + (goingDown ? 1 : -1));
  SettlementInfo* ret = nullptr;
  if (auto extra = enemyInfo.levelConnection->topLevel.getReferenceMaybe<LevelConnection::ExtraEnemy>()) {
    for (auto& enemy : extra->enemyInfo) {
      CHECK(!ret);
      ret = &enemy.settlement;
    }
  } else
    ret = &enemyInfo.settlement;
  (goingDown ? ret->downStairs : ret->upStairs).push_back(downLink);
  return *ret;
}

PModel ModelBuilder::tryModel(int width, vector<EnemyInfo> enemyInfo, optional<TribeId> keeperTribe, BiomeId biomeId,
    optional<ExternalEnemies> externalEnemies) {
  auto& biomeInfo = contentFactory->biomeInfo.at(biomeId);
  auto model = Model::create(contentFactory, biomeInfo.overrideMusic);
  vector<SettlementInfo> topLevelSettlements;
  vector<EnemyInfo> extraEnemies;
  for (auto& elem : enemyInfo) {
    elem.settlement.collective = new CollectiveBuilder(elem.config, elem.settlement.tribe);
    topLevelSettlements.push_back(processLevelConnection(model.get(), elem, extraEnemies, 0));
  }
  append(enemyInfo, extraEnemies);
  model->buildMainLevel(
      LevelBuilder(meter, random, contentFactory, width, width, false),
      LevelMaker::topLevel(random, topLevelSettlements, width, keeperTribe, biomeInfo,
          *chooseResourceCounts(random, contentFactory->resources, 0), *contentFactory));
  model->calculateStairNavigation();
  for (auto& enemy : enemyInfo)
    model->addCollective(enemy.buildCollective(contentFactory));
  if (externalEnemies)
    model->addExternalEnemies(std::move(*externalEnemies));
  return model;
}

PModel ModelBuilder::splashModel(const FilePath& splashPath) {
  auto m = Model::create(contentFactory, none);
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

PModel ModelBuilder::battleModel(const FilePath& levelPath, vector<PCreature> allies, vector<CreatureList> enemies) {
  auto m = Model::create(contentFactory, none);
  ifstream stream(levelPath.getPath());
  Table<char> level = *SokobanInput::readTable(stream);
  WLevel l = m->buildMainLevel(
      LevelBuilder(meter, Random, contentFactory, level.getBounds().width(), level.getBounds().height(), true, 1.0),
      LevelMaker::battleLevel(level, std::move(allies), enemies));
  return m;
}
