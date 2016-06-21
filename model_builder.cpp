#include "stdafx.h"
#include "model_builder.h"
#include "level.h"
#include "tribe.h"
#include "item_type.h"
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
#include "location.h"
#include "event_proxy.h"

ModelBuilder::ModelBuilder(ProgressMeter* m, RandomGen& r, Options* o) : random(r), meter(m), options(o),
  enemyFactory(EnemyFactory(random)) {
}

ModelBuilder::~ModelBuilder() {
}

int ModelBuilder::getPigstyPopulationIncrease() {
  return 4;
}

int ModelBuilder::getStatuePopulationIncrease() {
  return 1;
}

int ModelBuilder::getThronePopulationIncrease() {
  return 10;
}

static CollectiveConfig getKeeperConfig(bool fastImmigration) {
  return CollectiveConfig::keeper(
      fastImmigration ? 0.1 : 0.011,
      500,
      2,
      10,
      {
      CONSTRUCT(PopulationIncrease,
        c.type = SquareApplyType::PIGSTY;
        c.increasePerSquare = 0.25;
        c.maxIncrease = ModelBuilder::getPigstyPopulationIncrease();),
      CONSTRUCT(PopulationIncrease,
        c.type = SquareApplyType::STATUE;
        c.increasePerSquare = ModelBuilder::getStatuePopulationIncrease();
        c.maxIncrease = 1000;),
      CONSTRUCT(PopulationIncrease,
        c.type = SquareApplyType::THRONE;
        c.increasePerSquare = ModelBuilder::getThronePopulationIncrease();
        c.maxIncrease = c.increasePerSquare;),
      },
      {
      CONSTRUCT(ImmigrantInfo,
        c.id = CreatureId::GOBLIN;
        c.frequency = 1;
        c.attractions = LIST(
          {{AttractionId::SQUARE, SquareId::WORKSHOP}, 1.0, 12.0},
          {{AttractionId::SQUARE, SquareId::JEWELER}, 1.0, 9.0},
          {{AttractionId::SQUARE, SquareId::FORGE}, 1.0, 9.0},
          );
        c.traits = LIST(MinionTrait::FIGHTER, MinionTrait::NO_EQUIPMENT);
        c.salary = 10;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::ORC;
          c.frequency = 0.7;
          c.attractions = LIST(
            {{AttractionId::SQUARE, SquareId::TRAINING_ROOM}, 1.0, 12.0},
            );
          c.traits = {MinionTrait::FIGHTER};
          c.salary = 20;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::ORC_SHAMAN;
          c.frequency = 0.10;
          c.attractions = LIST(
            {{AttractionId::SQUARE, SquareId::LIBRARY}, 1.0, 16.0},
            {{AttractionId::SQUARE, SquareId::LABORATORY}, 1.0, 9.0},
            );
          c.traits = {MinionTrait::FIGHTER};
          c.salary = 20;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::OGRE;
          c.frequency = 0.3;
          c.attractions = LIST(
            {{AttractionId::SQUARE, SquareId::TRAINING_ROOM}, 3.0, 16.0}
            );
          c.traits = {MinionTrait::FIGHTER};
          c.salary = 40;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::HARPY;
          c.frequency = 0.3;
          c.attractions = LIST(
            {{AttractionId::SQUARE, SquareId::TRAINING_ROOM}, 3.0, 16.0},
            {{AttractionId::ITEM_INDEX, ItemIndex::RANGED_WEAPON}, 1.0, 3.0, true}
            );
          c.traits = {MinionTrait::FIGHTER};
          c.salary = 40;),
/*      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::SPECIAL_HUMANOID;
          c.frequency = 0.1;
          c.attractions = LIST(
            {{AttractionId::SQUARE, SquareId::TRAINING_ROOM}, 3.0, 16.0},
            );
          c.traits = {MinionTrait::FIGHTER};
          c.spawnAtDorm = true;
          c.techId = TechId::HUMANOID_MUT;
          c.salary = 40;),*/
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::ZOMBIE;
          c.frequency = 0.5;
          c.traits = {MinionTrait::FIGHTER};
          c.salary = 10;
          c.spawnAtDorm = true;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::VAMPIRE;
          c.frequency = 0.2;
          c.traits = {MinionTrait::FIGHTER};
          c.salary = 40;
          c.spawnAtDorm = true;
          c.attractions = LIST(
            {{AttractionId::SQUARE, SquareId::TRAINING_ROOM}, 2.0, 12.0}
            );),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::LOST_SOUL;
          c.frequency = 0.3;
          c.traits = {MinionTrait::FIGHTER};
          c.salary = 0;
          c.attractions = LIST(
            {{AttractionId::SQUARE, SquareId::RITUAL_ROOM}, 1.0, 9.0}
            );
          c.spawnAtDorm = true;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::SUCCUBUS;
          c.frequency = 0.3;
          c.traits = LIST(MinionTrait::FIGHTER, MinionTrait::NO_EQUIPMENT);
          c.salary = 0;
          c.attractions = LIST(
            {{AttractionId::SQUARE, SquareId::RITUAL_ROOM}, 2.0, 12.0}
            );
          c.spawnAtDorm = true;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::DOPPLEGANGER;
          c.frequency = 0.2;
          c.traits = {MinionTrait::FIGHTER};
          c.salary = 0;
          c.attractions = LIST(
            {{AttractionId::SQUARE, SquareId::RITUAL_ROOM}, 4.0, 12.0}
            );
          c.spawnAtDorm = true;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::RAVEN;
          c.frequency = 1.0;
          c.traits = LIST(MinionTrait::FIGHTER, MinionTrait::NO_RETURNING);
          c.limit = SunlightState::DAY;
          c.salary = 0;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::BAT;
          c.frequency = 1.0;
          c.traits = LIST(MinionTrait::FIGHTER, MinionTrait::NO_RETURNING);
          c.limit = SunlightState::NIGHT;
          c.salary = 0;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::WOLF;
          c.frequency = 0.15;
          c.traits = LIST(MinionTrait::FIGHTER, MinionTrait::NO_RETURNING);
          c.groupSize = Range(3, 9);
          c.autoTeam = true;
          c.salary = 0;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::CAVE_BEAR;
          c.frequency = 0.1;
          c.traits = LIST(MinionTrait::FIGHTER, MinionTrait::NO_RETURNING);
          c.salary = 0;),
      CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::WEREWOLF;
          c.frequency = 0.1;
          c.traits = LIST(MinionTrait::FIGHTER, MinionTrait::NO_RETURNING);
          c.attractions = LIST(
            {{AttractionId::SQUARE, SquareId::TRAINING_ROOM}, 4.0, 12.0}
            );
          c.salary = 0;),
      /*CONSTRUCT(ImmigrantInfo,
          c.id = CreatureId::SPECIAL_MONSTER_KEEPER;
          c.frequency = 0.1;
          c.traits = LIST(MinionTrait::FIGHTER, MinionTrait::NO_RETURNING);
          c.spawnAtDorm = true;
          c.techId = TechId::BEAST_MUT;
          c.salary = 0;)*/});
}

static map<CollectiveResourceId, int> getKeeperCredit(bool resourceBonus) {
  if (resourceBonus) {
    map<CollectiveResourceId, int> credit;
    for (auto elem : ENUM_ALL(CollectiveResourceId))
      credit[elem] = 10000;
    return credit;
  } else
    return {{CollectiveResourceId::MANA, 200}};
 
}

static EnumSet<MinionTrait> getImpTraits() {
  return {MinionTrait::WORKER, MinionTrait::NO_LIMIT, MinionTrait::NO_EQUIPMENT};
}

PModel ModelBuilder::tryQuickModel(int width) {
  Model* m = new Model();
  string keeperName = options->getStringValue(OptionId::KEEPER_NAME);
  Level* top = m->buildTopLevel(
      LevelBuilder(meter, random, width, width, "Quick", false),
      LevelMaker::quickLevel(random));
  m->calculateStairNavigation();
  m->collectives.push_back(CollectiveBuilder(
        getKeeperConfig(options->getBoolValue(OptionId::FAST_IMMIGRATION)), TribeId::getKeeper())
      .setLevel(top)
      .setCredit(getKeeperCredit(true))
      .build());
  vector<CreatureId> ids {
    CreatureId::DONKEY,
  };
  for (auto elem : ids) {
    PCreature c = CreatureFactory::fromId(elem, TribeId::getKeeper(),
        MonsterAIFactory::monster());
    top->landCreature(StairKey::keeperSpawn(), c.get());
//    m->playerCollective->addCreature(c.get(), {MinionTrait::FIGHTER});
    m->addCreature(std::move(c));
  }
  return PModel(m);
}

PModel ModelBuilder::quickModel() {
  return tryBuilding(5000, [=] { return tryQuickModel(40); });
}

SettlementInfo& ModelBuilder::makeExtraLevel(Model* model, EnemyInfo& enemy) {
  const int towerHeight = random.get(7, 12);
  const int gnomeHeight = random.get(3, 5);
  SettlementInfo& mainSettlement = enemy.settlement;
  SettlementInfo& extraSettlement = enemy.levelConnection->otherEnemy->settlement;
  switch (enemy.levelConnection->type) {
    case LevelConnection::TOWER: {
      StairKey downLink = StairKey::getNew();
      extraSettlement.upStairs = {downLink};
      for (int i : Range(towerHeight - 1)) {
        StairKey upLink = StairKey::getNew();
        model->buildLevel(
            LevelBuilder(meter, random, 4, 4, "Tower floor" + toString(i + 2)),
            LevelMaker::towerLevel(random,
                CONSTRUCT(SettlementInfo,
                  c.type = SettlementType::TOWER;
                  c.creatures = CreatureFactory::singleType(TribeId::getHuman(), random.choose(
                      CreatureId::WATER_ELEMENTAL, CreatureId::AIR_ELEMENTAL, CreatureId::FIRE_ELEMENTAL,
                      CreatureId::EARTH_ELEMENTAL));
                  c.numCreatures = random.get(1, 3);
                  c.location = new Location();
                  c.upStairs = {upLink};
                  c.downStairs = {downLink};
                  c.furniture = SquareFactory::single(SquareId::TORCH);
                  c.buildingId = BuildingId::BRICK;)));
        downLink = upLink;
      }
      mainSettlement.downStairs = {downLink};
      model->buildLevel(
         LevelBuilder(meter, random, 5, 5, "Tower top"),
         LevelMaker::towerLevel(random, mainSettlement));
      return extraSettlement;
    }
    case LevelConnection::CRYPT: {
      StairKey key = StairKey::getNew();
      extraSettlement.downStairs = {key};
      mainSettlement.upStairs = {key};
      model->buildLevel(
         LevelBuilder(meter, random, 40, 40, "Crypt"),
         LevelMaker::cryptLevel(random, mainSettlement));
      return extraSettlement;
    }
    case LevelConnection::MAZE: {
      StairKey key = StairKey::getNew();
      extraSettlement.upStairs = {key};
      mainSettlement.downStairs = {key};
      model->buildLevel(
         LevelBuilder(meter, random, 40, 40, "Maze"),
         LevelMaker::mazeLevel(random, extraSettlement));
      return mainSettlement;
    }
    case LevelConnection::GNOMISH_MINES: {
      StairKey upLink = StairKey::getNew();
      extraSettlement.downStairs = {upLink};
      for (int i : Range(gnomeHeight - 1)) {
        StairKey downLink = StairKey::getNew();
        model->buildLevel(
            LevelBuilder(meter, random, 60, 40, "Mines lvl " + toString(i + 1)),
            LevelMaker::roomLevel(random, CreatureFactory::gnomishMines(
                mainSettlement.tribe, TribeId::getMonster(), 0),
                CreatureFactory::waterCreatures(mainSettlement.tribe),
                CreatureFactory::lavaCreatures(mainSettlement.tribe), {upLink}, {downLink},
                SquareFactory::roomFurniture(TribeId::getPest())));
        upLink = downLink;
      }
      mainSettlement.upStairs = {upLink};
      model->buildLevel(
         LevelBuilder(meter, random, 60, 40, "Mine Town"),
         LevelMaker::mineTownLevel(random, mainSettlement));
      return extraSettlement;
    }
    case LevelConnection::SOKOBAN:
      StairKey key = StairKey::getNew();
      extraSettlement.upStairs = {key};
      mainSettlement.downStairs = {key};
      for (int i : Range(5000)) {
        try {
          model->buildLevel(
              LevelBuilder(meter, random, 28, 14, "Sokoban"),
              LevelMaker::sokobanLevel(random, mainSettlement));
          return extraSettlement;
        } catch (LevelGenException ex) {
          Debug() << "Retrying";
        }
      }
      throw LevelGenException();
  }
}

static string getBoardText(const string& keeperName, const string& dukeName) {
  return dukeName + " will reward a daring hero 150 florens for slaying " + keeperName + " the Keeper.";
}

PModel ModelBuilder::singleMapModel(const string& worldName) {
  vector<EnemyInfo> enemies;
  for (int i : Range(random.get(5, 9)))
    enemies.push_back(enemyFactory->get(EnemyId::HUMAN_COTTAGE));
  for (int i : Range(random.get(1, 3)))
    enemies.push_back(enemyFactory->get(EnemyId::KOBOLD_CAVE));
  for (int i : Range(random.get(1, 3)))
    enemies.push_back(enemyFactory->get(EnemyId::BANDITS).setSurprise());
  enemies.push_back(enemyFactory->get(EnemyId::SOKOBAN).setSurprise());
  enemies.push_back(enemyFactory->get(random.choose(EnemyId::GNOMES, EnemyId::DARK_ELVES)).setSurprise());
  append(enemies, enemyFactory->getVaults());
  if (random.roll(4))
    enemies.push_back(enemyFactory->get(EnemyId::ANTS_CLOSED).setSurprise());
  enemies.push_back(enemyFactory->get(EnemyId::KNIGHTS).setSurprise());
  enemies.push_back(enemyFactory->get(EnemyId::FRIENDLY_CAVE).setSurprise());
  for (auto& enemy : random.chooseN(3, {
        EnemyId::ELEMENTALIST,
        EnemyId::WARRIORS,
        EnemyId::ELVES,
        EnemyId::DWARVES,
        EnemyId::VILLAGE}))
    enemies.push_back(enemyFactory->get(enemy).setSurprise());
  for (auto& enemy : random.chooseN(3, {
        EnemyId::GREEN_DRAGON,
        EnemyId::SHELOB,
        EnemyId::HYDRA,
        EnemyId::RED_DRAGON,
        EnemyId::CYCLOPS,
        EnemyId::DRIADS,
        EnemyId::ENTS}))
    enemies.push_back(enemyFactory->get(enemy).setSurprise());
  for (auto& enemy : random.chooseN(1, {
        EnemyId::WITCH,
        EnemyId::CEMETERY}))
    enemies.push_back(enemyFactory->get(enemy));
  return tryBuilding(10, [&] { return tryModel(360, worldName, enemies, true, BiomeId::GRASSLAND);});
}

void ModelBuilder::addMapVillains(vector<EnemyInfo>& enemyInfo, BiomeId biomeId) {
  switch (biomeId) {
    case BiomeId::GRASSLAND:
      for (int i : Range(random.get(3, 5)))
        enemyInfo.push_back(enemyFactory->get(EnemyId::HUMAN_COTTAGE));
      break;
    case BiomeId::MOUNTAIN:
      for (int i : Range(random.get(1, 4)))
        enemyInfo.push_back(enemyFactory->get(random.choose(EnemyId::DWARF_CAVE, EnemyId::KOBOLD_CAVE)));
      break;
    case BiomeId::FORREST:
      for (int i : Range(random.get(3, 5)))
        enemyInfo.push_back(enemyFactory->get(EnemyId::ELVEN_COTTAGE));
      break;
  }
}

PModel ModelBuilder::tryCampaignBaseModel(const string& siteName) {
  vector<EnemyInfo> enemyInfo;
  BiomeId biome = BiomeId::MOUNTAIN;
  addMapVillains(enemyInfo, biome);
 // append(enemyInfo, getBanditCave(random));
  /*      append(enemyInfo, getSokobanEntry(random));
        append(enemyInfo, random.choose({
          getFriendlyCave(random, CreatureId::ORC),
          getFriendlyCave(random, CreatureId::OGRE),
          getFriendlyCave(random, CreatureId::HARPY)}));*/
  /*      append(enemyInfo, random.choose({
          getGreenDragon(random),
          getShelob(random),
          getHydra(random),
          getRedDragon(random),
          getCyclops(random),
          getDriadTown(random),
          getEntTown(random)}));*/
  PModel ret = tryModel(210, siteName, enemyInfo, true, biome);
  return ret;
}

static BiomeId getBiome(EnemyId enemyId, RandomGen& random) {
  switch (enemyId) {
    case EnemyId::KNIGHTS:
    case EnemyId::WARRIORS:
    case EnemyId::ELEMENTALIST:
    case EnemyId::LIZARDMEN:
    case EnemyId::HYDRA:
    case EnemyId::VILLAGE:
    case EnemyId::ORC_VILLAGE: return BiomeId::GRASSLAND;
    case EnemyId::RED_DRAGON:
    case EnemyId::GREEN_DRAGON:
    case EnemyId::DWARVES:
    case EnemyId::DARK_ELVES:
    case EnemyId::FRIENDLY_CAVE:
    case EnemyId::SOKOBAN:
    case EnemyId::GNOMES:
    case EnemyId::CYCLOPS:
    case EnemyId::SHELOB:
    case EnemyId::ANTS_OPEN: return BiomeId::MOUNTAIN;
    case EnemyId::ELVES:
    case EnemyId::DRIADS:
    case EnemyId::ENTS: return BiomeId::FORREST;
    case EnemyId::BANDITS: return random.choose<BiomeId>();
    case EnemyId::CEMETERY: return random.choose(BiomeId::GRASSLAND, BiomeId::FORREST);
    default: FAIL << "Unimplemented enemy in campaign " << EnumInfo<EnemyId>::getString(enemyId);
             return BiomeId::FORREST;
  }
}

PModel ModelBuilder::tryCampaignSiteModel(const string& siteName, EnemyId enemyId, VillainType type) {
  vector<EnemyInfo> enemyInfo { enemyFactory->get(enemyId).setVillainType(type).setSurprise()};
  BiomeId biomeId = getBiome(enemyId, random);
  addMapVillains(enemyInfo, biomeId);
  return tryModel(170, siteName, enemyInfo, false, biomeId);
}

PModel ModelBuilder::tryBuilding(int numTries, function<PModel()> buildFun) {
  for (int i : Range(numTries)) {
    try {
      if (meter)
        meter->reset();
      return buildFun();
    } catch (LevelGenException ex) {
      Debug() << "Retrying level gen";
    }
  }
  FAIL << "Couldn't generate a level";
  return nullptr;

}

PModel ModelBuilder::campaignBaseModel(const string& siteName) {
  return tryBuilding(20, [&] { return tryCampaignBaseModel(siteName); });
}

PModel ModelBuilder::campaignSiteModel(const string& siteName, EnemyId enemyId, VillainType type) {
  return tryBuilding(20, [&] { return tryCampaignSiteModel(siteName, enemyId, type); });
}

void ModelBuilder::measureSiteGen(int numTries) {
  for (EnemyId id : {EnemyId::SOKOBAN}) {
//  for (EnemyId id : ENUM_ALL(EnemyId)) {
    std::cout << "Measuring " << EnumInfo<EnemyId>::getString(id) << std::endl;
    measureModelGen(numTries, [&] { tryCampaignSiteModel("", id, VillainType::LESSER); });
  }
}

void ModelBuilder::measureModelGen(int numTries, function<void()> genFun) {
  int numSuccess = 0;
  int maxT = 0;
  int minT = 1000000;
  double sumT = 0;
  for (int i : Range(numTries)) {
    try {
      // FIX this
//      sf::Clock c;
      genFun();
      int millis = 0;//c.getElapsedTime().asMilliseconds();
      sumT += millis;
      maxT = max(maxT, millis);
      minT = min(minT, millis);
      ++numSuccess;
      std::cout << ".";
      std::cout.flush();
    } catch (LevelGenException ex) {
      std::cout << "x";
      std::cout.flush();
    }
  }
  std::cout << numSuccess << " / " << numTries << " gens successful.\nMinT: " << minT << "\nMaxT: " << maxT <<
      "\nAvgT: " << sumT / numSuccess << std::endl;
}

void ModelBuilder::spawnKeeper(Model* m) {
  Level* level = m->levels[0].get();
  PCreature keeper = CreatureFactory::fromId(CreatureId::KEEPER, TribeId::getKeeper());
  string keeperName = options->getStringValue(OptionId::KEEPER_NAME);
  if (!keeperName.empty())
    keeper->getName().setFirst(keeperName);
  Creature* keeperRef = keeper.get();
  level->landCreature(StairKey::keeperSpawn(), keeperRef);
  m->addCreature(std::move(keeper));
  m->collectives.push_back(CollectiveBuilder(
        getKeeperConfig(options->getBoolValue(OptionId::FAST_IMMIGRATION)), TribeId::getKeeper())
      .setLevel(level)
      .addCreature(keeperRef)
      .setCredit(getKeeperCredit(options->getBoolValue(OptionId::STARTING_RESOURCE)))
      .build());
  Collective* playerCollective = m->collectives.back().get();
  playerCollective->setControl(PCollectiveControl(new PlayerControl(playerCollective, level)));
  playerCollective->setVillainType(VillainType::PLAYER);
  for (int i : Range(4)) {
    PCreature c = CreatureFactory::fromId(CreatureId::IMP, TribeId::getKeeper(),
        MonsterAIFactory::collective(playerCollective));
    level->landCreature(StairKey::keeperSpawn(), c.get());
    playerCollective->addCreature(c.get(), getImpTraits());
    m->addCreature(std::move(c));
  }
}

PModel ModelBuilder::tryModel(int width, const string& levelName, vector<EnemyInfo> enemyInfo, bool keeperSpawn,
    BiomeId biomeId) {
  Model* model = new Model();
  vector<SettlementInfo> topLevelSettlements;
  for (auto& elem : enemyInfo) {
    elem.settlement.collective = new CollectiveBuilder(elem.config, elem.settlement.tribe);
    if (elem.levelConnection)
      topLevelSettlements.push_back(makeExtraLevel(model, elem));
    else
      topLevelSettlements.push_back(elem.settlement);
  }
  Level* top = model->buildTopLevel(
      LevelBuilder(meter, random, width, width, levelName, false),
      LevelMaker::topLevel(random, CreatureFactory::forrest(TribeId::getWildlife()), topLevelSettlements, width,
        keeperSpawn, biomeId));
  model->calculateStairNavigation();
  for (int i : All(enemyInfo)) {
    if (!enemyInfo[i].settlement.collective->hasCreatures())
      continue;
    PVillageControl control;
    Location* location = enemyInfo[i].settlement.location;
    if (auto name = location->getName())
      enemyInfo[i].settlement.collective->setLocationName(*name);
    if (auto race = enemyInfo[i].settlement.race)
      enemyInfo[i].settlement.collective->setRaceName(*race);
    PCollective collective = enemyInfo[i].settlement.collective->addSquares(location->getAllSquares()).build();
    control.reset(new VillageControl(collective.get(), enemyInfo[i].villain));
    if (enemyInfo[i].villainType)
      collective->setVillainType(*enemyInfo[i].villainType);
    collective->setControl(std::move(control));
    model->collectives.push_back(std::move(collective));
  }
  return PModel(model);
}

PModel ModelBuilder::splashModel(const string& splashPath) {
  Model* m = new Model();
  Level* l = m->buildTopLevel(
      LevelBuilder(meter, Random, Level::getSplashBounds().width(), Level::getSplashBounds().height(), "Splash",
        true, 1.0),
      LevelMaker::splashLevel(
          CreatureFactory::splashLeader(TribeId::getHuman()),
          CreatureFactory::splashHeroes(TribeId::getHuman()),
          CreatureFactory::splashMonsters(TribeId::getKeeper()),
          CreatureFactory::singleType(TribeId::getKeeper(), CreatureId::IMP), splashPath));
  m->topLevel = l;
  return PModel(m);
}

