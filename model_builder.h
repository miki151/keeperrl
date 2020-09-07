#pragma once

#include "village_control.h"
#include "stair_key.h"
#include "avatar_info.h"
#include "resource_counts.h"

class Level;
class Model;
class ProgressMeter;
class Options;
struct SettlementInfo;
struct EnemyInfo;
class EnemyFactory;
class SokobanInput;
class ExternalEnemies;
struct EnemyEvent;
class Tutorial;
class FilePath;
class CreatureList;
class GameConfig;
class ContentFactory;
struct LevelConnection;
struct BiomeInfo;
struct BiomeEnemyInfo;

class ModelBuilder {
  public:
  ModelBuilder(ProgressMeter*, RandomGen&, Options*, SokobanInput*, ContentFactory*, EnemyFactory);
  ModelBuilder(ModelBuilder&&) = default;
  ModelBuilder(const ModelBuilder&) = delete;
  PModel singleMapModel(TribeId keeperTribe, TribeAlignment);
  PModel campaignBaseModel(TribeId keeperTribe, TribeAlignment, BiomeId, optional<ExternalEnemiesType>);
  PModel campaignSiteModel(EnemyId, VillainType, TribeAlignment);
  PModel tutorialModel();

  void measureSiteGen(int numTries, vector<string> types, vector<BiomeId> biomes);

  PModel splashModel(const FilePath& splashPath);
  PModel battleModel(const FilePath& levelPath, vector<PCreature> allies, vector<CreatureList> enemies);

  ~ModelBuilder();

  private:
  void measureModelGen(const std::string& name, int numTries, function<void()> genFun);
  PModel trySingleMapModel(TribeId keeperTribe, TribeAlignment);
  PModel tryCampaignBaseModel(TribeId keeperTribe, TribeAlignment, BiomeId, optional<ExternalEnemiesType>);
  PModel tryTutorialModel();
  PModel tryCampaignSiteModel(EnemyId, VillainType, TribeAlignment);
  PModel tryModel(int width, vector<EnemyInfo>, optional<TribeId> keeperTribe, BiomeId, optional<ExternalEnemies>);
  void makeExtraLevel(WModel model, LevelConnection& connection, SettlementInfo& mainSettlement, StairKey upLink,
      vector<EnemyInfo>& extraEnemies, int depth);
  PModel tryBuilding(int numTries, function<PModel()> buildFun, const string& name);
  void addMapVillains(vector<EnemyInfo>&, const vector<BiomeEnemyInfo>&);
  RandomGen& random;
  ProgressMeter* meter = nullptr;
  HeapAllocated<EnemyFactory> enemyFactory;
  SokobanInput* sokobanInput = nullptr;
  vector<EnemyInfo> getSingleMapEnemiesForEvilKeeper(TribeId keeperTribe);
  vector<EnemyInfo> getSingleMapEnemiesForLawfulKeeper(TribeId keeperTribe);
  ContentFactory* contentFactory = nullptr;
  using LevelMakerMethod = function<PLevelMaker(RandomGen&, SettlementInfo, Vec2 size)>;
  LevelMakerMethod getMaker(LevelType);
  SettlementInfo& processLevelConnection(Model*, EnemyInfo&, vector<EnemyInfo>& extraEnemies, int depth);
};
