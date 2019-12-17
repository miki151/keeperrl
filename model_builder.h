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

class ModelBuilder {
  public:
  ModelBuilder(ProgressMeter*, RandomGen&, Options*, SokobanInput*, ContentFactory*, EnemyFactory);
  PModel singleMapModel(TribeId keeperTribe, TribeAlignment);
  PModel campaignBaseModel(TribeId keeperTribe, TribeAlignment, optional<ExternalEnemiesType>);
  PModel campaignSiteModel(EnemyId, VillainType, TribeAlignment);
  PModel tutorialModel();

  void measureSiteGen(int numTries, vector<string> types);

  PModel splashModel(const FilePath& splashPath);
  PModel battleModel(const FilePath& levelPath, CreatureList allies, CreatureList enemies);

  static WCollective spawnKeeper(WModel, AvatarInfo, bool regenerateMana, vector<string> introText);

  ~ModelBuilder();

  private:
  void measureModelGen(const std::string& name, int numTries, function<void()> genFun);
  PModel trySingleMapModel(TribeId keeperTribe, TribeAlignment);
  PModel tryCampaignBaseModel(TribeId keeperTribe, TribeAlignment, optional<ExternalEnemiesType>);
  PModel tryTutorialModel();
  PModel tryCampaignSiteModel(EnemyId, VillainType, TribeAlignment);
  PModel tryModel(int width, vector<EnemyInfo>, optional<TribeId> keeperTribe, BiomeId, optional<ExternalEnemies>, bool wildlife);
  void makeExtraLevel(WModel model, LevelConnection& connection, SettlementInfo& mainSettlement, StairKey upLink,
      vector<EnemyInfo>& extraEnemies);
  PModel tryBuilding(int numTries, function<PModel()> buildFun, const string& name);
  void addMapVillainsForEvilKeeper(vector<EnemyInfo>&, BiomeId);
  void addMapVillainsForLawfulKeeper(vector<EnemyInfo>&, BiomeId);
  RandomGen& random;
  ProgressMeter* meter = nullptr;
  Options* options = nullptr;
  HeapAllocated<EnemyFactory> enemyFactory;
  SokobanInput* sokobanInput = nullptr;
  vector<EnemyInfo> getSingleMapEnemiesForEvilKeeper(TribeId keeperTribe);
  vector<EnemyInfo> getSingleMapEnemiesForLawfulKeeper(TribeId keeperTribe);
  ContentFactory* contentFactory = nullptr;
  using LevelMakerMethod = function<PLevelMaker(RandomGen&, SettlementInfo, Vec2 size)>;
  LevelMakerMethod getMaker(LevelType);
  SettlementInfo processLevelConnection(Model*, EnemyInfo&, vector<EnemyInfo>& extraEnemies);
};
