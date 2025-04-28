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
#include "game_config.h"
#include "build_info.h"
#include "tribe_alignment.h"
#include "resource_counts.h"
#include "content_factory.h"
#include "enemy_id.h"
#include "biome_id.h"
#include "zlevel.h"
#include "avatar_info.h"
#include "keeper_base_info.h"

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
      return [this](RandomGen& random, SettlementInfo info, Vec2 size, int difficulty) {
        return LevelMaker::settlementLevel(*contentFactory, random, info, size, none, none, FurnitureType("MOUNTAIN2"),
            difficulty);
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
      return [this] (RandomGen& random, SettlementInfo info, Vec2, int difficulty) {
        auto& biomeInfo = contentFactory->biomeInfo.at(BiomeId("OUTBACK"));
        auto natives = enemyFactory->get(EnemyId("NATIVE_VILLAGE"));
        natives.settlement.collective = new CollectiveBuilder(natives.config, natives.settlement.tribe, "native village");
        return LevelMaker::topLevel(random, {info, natives.settlement}, 100, difficulty, none, none, biomeInfo, ResourceCounts{},
            *contentFactory);
      };
    case LevelType::SOKOBAN: {
      Table<char> sokoLevel = sokobanInput->getNext();
      return [sokoLevel](RandomGen& random, SettlementInfo info, Vec2, int difficulty) {
        return LevelMaker::sokobanFromFile(random, info, sokoLevel, difficulty);
      };
    }
  }
}

void ModelBuilder::addMapVillains(vector<EnemyInfo>& enemyInfo, const vector<BiomeEnemyInfo>& info) {
  for (auto& enemy : info)
    if (random.chance(enemy.probability))
      for (int i : Range(random.get(enemy.count)))
        enemyInfo.push_back(enemyFactory->get(enemy.id));
}

PModel ModelBuilder::tryCampaignBaseModel(TribeAlignment alignment, optional<KeeperBaseInfo> keeperBase, BiomeId biome,
    optional<ExternalEnemiesType> externalEnemiesType) {
  vector<EnemyInfo> enemyInfo;
  auto& biomeInfo = contentFactory->biomeInfo.at(biome);
  addMapVillains(enemyInfo, alignment == TribeAlignment::EVIL
      ? biomeInfo.darkKeeperBaseEnemies
      : biomeInfo.whiteKeeperBaseEnemies);
  optional<ExternalEnemies> externalEnemies;
  if (externalEnemiesType)
    externalEnemies = ExternalEnemies(random, &contentFactory->getCreatures(), enemyFactory->getExternalEnemies(),
        *externalEnemiesType);
  return tryModel(114, 0, enemyInfo, getPlayerTribeId(alignment), std::move(keeperBase), biome, std::move(externalEnemies));
}

PModel ModelBuilder::tryTutorialModel(optional<KeeperBaseInfo> keeperBase) {
  vector<EnemyInfo> enemyInfo;
  BiomeId biome = BiomeId("MOUNTAIN");
  //enemyInfo.push_back(enemyFactory->get(EnemyId("CORPSES")));
  //enemyInfo.push_back(enemyFactory->get(EnemyId("RED_DRAGON")));
  //enemyInfo.push_back(enemyFactory->get(EnemyId("RUINS")));
  enemyInfo.push_back(enemyFactory->get(EnemyId("TUTORIAL_VILLAGE")));
  //enemyInfo.push_back(enemyFactory->get(EnemyId("ADA_GOLEMS")));
  //enemyInfo.push_back(enemyFactory->get(EnemyId("TEMPLE")));
  //enemyInfo.push_back(enemyFactory->get(EnemyId("LIZARDMEN")));
  return tryModel(114, 0, enemyInfo, TribeId::getDarkKeeper(), keeperBase, biome, {});
}

PModel ModelBuilder::tryCampaignSiteModel(EnemyId enemyId, VillainType type, TribeAlignment alignment, BiomeId biomeId,
    int difficulty) {
  vector<EnemyInfo> enemyInfo { enemyFactory->get(enemyId).setVillainType(type)};
  if (auto id = enemyInfo[0].otherEnemy)
    enemyInfo.push_back(enemyFactory->get(*id));
  auto& biomeInfo = contentFactory->biomeInfo.at(biomeId);
  if (type != VillainType::MINOR)
    addMapVillains(enemyInfo, alignment == TribeAlignment::EVIL ? biomeInfo.darkKeeperEnemies : biomeInfo.whiteKeeperEnemies);
  return tryModel(type == VillainType::MINOR ? 60 : 114, difficulty, enemyInfo, none, none, biomeId, {});
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

PModel ModelBuilder::campaignBaseModel(const AvatarInfo& avatarInfo, BiomeId biome,
    optional<ExternalEnemiesType> externalEnemies) {
  return tryBuilding(20, [&] { return tryCampaignBaseModel(avatarInfo.tribeAlignment,
      avatarInfo.creatureInfo.startingBase, biome, externalEnemies); },
      "campaign base");
}

PModel ModelBuilder::tutorialModel(optional<KeeperBaseInfo> keeperBase) {
  return tryBuilding(20, [=] { return tryTutorialModel(keeperBase); }, "tutorial");
}

PModel ModelBuilder::campaignSiteModel(EnemyId enemyId, VillainType type, TribeAlignment alignment, BiomeId biome,
    int difficulty) {
  return tryBuilding(20, [&] { return tryCampaignSiteModel(enemyId, type, alignment, biome, difficulty); },
      enemyId.data());
}

void ModelBuilder::measureSiteGen(int numTries, vector<string> types, vector<BiomeId> biomes) {
  if (types.empty()) {
    types = {"campaign_base", "tutorial", "zlevels"};
    for (auto id : enemyFactory->getAllIds()) {
      auto enemy = enemyFactory->get(id);
      if (!!enemy.getBiome())
        types.push_back(id.data());
    }
  }
  vector<function<void()>> tasks;
  for (auto& type : types) {
    if (type == "campaign_base")
      for (auto alignment : ENUM_ALL(TribeAlignment))
        for (auto biome : biomes)
          tasks.push_back([=] { measureModelGen(type + " (" + EnumInfo<TribeAlignment>::getString(alignment) + ", "
              + biome.data() + ")", numTries,
              [&] { tryCampaignBaseModel(alignment, none, biome, none); }); });
    else if (type == "zlevels") {
//      FATAL << "Fix after adding z level groups";
      for (auto alignment : ENUM_ALL(TribeAlignment))
        for (int i : Range(1, 30))
          tasks.push_back([=] { measureModelGen(type + " " + toString(i) +
              " (" + EnumInfo<TribeAlignment>::getString(alignment) + ")",
              numTries,
              [&] {
                auto model = tryCampaignBaseModel(alignment, none, BiomeId("GRASSLAND"), none);
                auto size = model->getGroundLevel()->getBounds().getSize();
                auto maker = getLevelMaker(Random, contentFactory, {"basic"}, i, TribeId::getDarkKeeper(), size,
                    EnemyAggressionLevel(0));
                LevelBuilder(Random, contentFactory, size.x, size.y, true)
                    .build(contentFactory, model.get(), maker.maker.get(), 123);
              }); });
    }
    else if (type == "tutorial")
      tasks.push_back([=] { measureModelGen(type, numTries, [&] { tryTutorialModel(none); }); });
    else {
      auto id = EnemyId(type.data());
      for (auto alignment : ENUM_ALL(TribeAlignment))
        tasks.push_back([=] { measureModelGen(type, numTries, [&] {
            tryCampaignSiteModel(id, VillainType::LESSER, alignment, Random.choose(biomes), 0); }); });
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

void ModelBuilder::makeExtraLevel(Model* model, LevelConnection& connection, SettlementInfo& mainSettlement,
    StairKey upLink, vector<EnemyInfo>& extraEnemies, int depth, bool mainDungeon, int difficulty) {
  StairKey downLink = StairKey::getNew();
  int direction = connection.direction == LevelConnectionDir::DOWN ? 1 : -1;
  for (int index : All(connection.levels)) {
    auto& level = connection.levels[index];
    auto addLevel = [&] (SettlementInfo& settlement, bool bottomLevel) {
      settlement.upStairs.push_back(upLink);
      if (!bottomLevel)
        settlement.downStairs.push_back(downLink);
      if (direction == -1)
        swap(settlement.upStairs, settlement.downStairs);
      auto res = model->buildLevel(
          contentFactory,
          LevelBuilder(meter, random, contentFactory, level.levelSize.x, level.levelSize.y, true,
              level.isLit ? 1.0 : 0.0),
          getMaker(level.levelType)(random, settlement, level.levelSize, difficulty),
          depth,
          level.name.value_or("Z-level " + toString(depth)));
      res->canTranfer = level.canTransfer;
      res->aiFollows = level.aiFollows;
      res->mainDungeon = mainDungeon;
      upLink = downLink;
      downLink = StairKey::getNew();
      depth += direction;
    };
    level.enemy.match(
        [&](LevelConnection::ExtraEnemy& e) {
          for (int i : All(e.enemyInfo)) {
            auto& set = processLevelConnection(model, e.enemyInfo[i], extraEnemies, depth, false, difficulty);
            addLevel(set, index == connection.levels.size() - 1 && i == e.enemyInfo.size() - 1);
          }
        },
        [&](LevelConnection::MainEnemy&) {
          addLevel(mainSettlement, index == connection.levels.size() - 1);
        }
    );
  }
}

SettlementInfo& ModelBuilder::processLevelConnection(Model* model, EnemyInfo& enemyInfo,
    vector<EnemyInfo>& extraEnemies, int depth, bool mainDungeon, int difficulty) {
  if (!enemyInfo.levelConnection)
    return enemyInfo.settlement;
  auto processExtraEnemy = [&] (LevelConnection::EnemyLevelInfo& enemyInfo) {
    if (auto extra = enemyInfo.getReferenceMaybe<LevelConnection::ExtraEnemy>()) {
      for (auto& enemy : extra->enemyInfo) {
        enemy.settlement.collective =
            new CollectiveBuilder(enemy.config, enemy.settlement.tribe, !!enemy.id ? enemy.id->data() : "unknown");
        extraEnemies.push_back(enemy);
      }
    }
  };
  for (auto& level : enemyInfo.levelConnection->levels)
    processExtraEnemy(level.enemy);
  processExtraEnemy(enemyInfo.levelConnection->topLevel);
  StairKey downLink = StairKey::getNew();
  bool goingDown = enemyInfo.levelConnection->direction == LevelConnectionDir::DOWN;
  makeExtraLevel(model, *enemyInfo.levelConnection, enemyInfo.settlement, downLink, extraEnemies,
      depth + (goingDown ? 1 : -1), mainDungeon, difficulty);
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

PModel ModelBuilder::tryModel(int width, int difficulty, vector<EnemyInfo> enemyInfo, optional<TribeId> keeperTribe,
    optional<KeeperBaseInfo> keeperBase, BiomeId biomeId,
    optional<ExternalEnemies> externalEnemies) {
  auto& biomeInfo = contentFactory->biomeInfo.at(biomeId);
  auto model = Model::create(contentFactory, biomeInfo.overrideMusic, biomeId);
  vector<SettlementInfo> topLevelSettlements;
  vector<EnemyInfo> extraEnemies;
  for (auto& elem : enemyInfo) {
    elem.settlement.collective = new CollectiveBuilder(elem.config, elem.settlement.tribe, !!elem.id ? elem.id->data() : "unknown");
    topLevelSettlements.push_back(processLevelConnection(model.get(), elem, extraEnemies, 0, true, difficulty));
  }
  append(enemyInfo, extraEnemies);
  model->buildMainLevel(
      contentFactory,
      LevelBuilder(meter, random, contentFactory, width, width, false),
      LevelMaker::topLevel(random, topLevelSettlements, width, difficulty, keeperTribe, std::move(keeperBase),
          biomeInfo, *chooseResourceCounts(random, contentFactory->resources, 0), *contentFactory));
  model->getGroundLevel()->sightRange = biomeInfo.sightRange;
  model->calculateStairNavigation();
  for (auto& enemy : enemyInfo)
    model->addCollective(enemy.buildCollective(contentFactory));
  if (externalEnemies)
    model->addExternalEnemies(std::move(*externalEnemies));
  return model;
}

PModel ModelBuilder::battleModel(const FilePath& levelPath, vector<PCreature> allies, vector<CreatureList> enemies) {
  auto m = Model::create(contentFactory, none, BiomeId("GRASSLAND"));
  ifstream stream(levelPath.getPath());
  Table<char> level = *SokobanInput::readTable(stream);
  Level* l = m->buildMainLevel(
      contentFactory,
      LevelBuilder(meter, Random, contentFactory, level.getBounds().width(), level.getBounds().height(), true, 1.0),
      LevelMaker::battleLevel(level, std::move(allies), enemies));
  return m;
}
