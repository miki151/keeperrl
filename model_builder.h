#pragma once

#include "village_control.h"
#include "stair_key.h"
#include "avatar_info.h"

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

class ModelBuilder {
  public:
  ModelBuilder(ProgressMeter*, RandomGen&, Options*, SokobanInput*);
  PModel singleMapModel(const string& worldName);
  PModel campaignBaseModel(const string& siteName, bool externalEnemies);
  PModel campaignSiteModel(const string& siteName, EnemyId, VillainType);
  PModel tutorialModel(const string& siteName);

  void measureSiteGen(int numTries, vector<std::string> types);

  PModel splashModel(const FilePath& splashPath);
  PModel battleModel(const FilePath& levelPath, CreatureList allies, CreatureList enemies);

  WCollective spawnKeeper(WModel, AvatarInfo, bool regenerateMana, bool hellishMode, vector<string> introText);

  static int getPigstyPopulationIncrease();
  static int getStatuePopulationIncrease();
  static int getThronePopulationIncrease();

  ~ModelBuilder();

  private:
  void measureModelGen(const std::string& name, int numTries, function<void()> genFun);
  PModel trySingleMapModel(const string& worldName);
  PModel tryCampaignBaseModel(const string& siteName, bool externalEnemies);
  PModel tryTutorialModel(const string& siteName);
  PModel tryCampaignSiteModel(const string& siteName, EnemyId, VillainType);
  PModel tryModel(int width, const string& levelName, vector<EnemyInfo>,
      bool keeperSpawn, BiomeId, optional<ExternalEnemies>, bool wildlife);
  SettlementInfo& makeExtraLevel(WModel, EnemyInfo&);
  PModel tryBuilding(int numTries, function<PModel()> buildFun, const string& name);
  void addMapVillains(vector<EnemyInfo>&, BiomeId);
  RandomGen& random;
  ProgressMeter* meter;
  Options* options;
  HeapAllocated<EnemyFactory> enemyFactory;
  SokobanInput* sokobanInput;
};
