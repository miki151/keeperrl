#ifndef _MODEL_BUILDER_H

#include "village_control.h"
#include "stair_key.h"

class Level;
class Model;
class ProgressMeter;
class Options;
struct SettlementInfo;
struct EnemyInfo;
class EnemyFactory;

class ModelBuilder {
  public:
  ModelBuilder(ProgressMeter*, RandomGen&, Options*);
  PModel singleMapModel(const string& worldName);
  PModel campaignBaseModel(const string& siteName);
  PModel campaignSiteModel(const string& siteName, EnemyId, VillainType);

  void measureModelGen(int numTries, function<void()> genFun);
  void measureSiteGen(int numTries);

  PModel quickModel();

  PModel splashModel(const string& splashPath);

  void spawnKeeper(Model*);

  static int getPigstyPopulationIncrease();
  static int getStatuePopulationIncrease();
  static int getThronePopulationIncrease();

  ~ModelBuilder();

  private:
  PModel trySingleMapModel(const string& worldName);
  PModel tryCampaignBaseModel(const string& siteName);
  PModel tryCampaignSiteModel(const string& siteName, EnemyId, VillainType);
  PModel tryModel(int width, const string& levelName, vector<EnemyInfo>,
      bool keeperSpawn, BiomeId);
  PModel tryQuickModel(int width);
  SettlementInfo& makeExtraLevel(Model*, EnemyInfo&);
  PModel tryBuilding(int numTries, function<PModel()> buildFun);
  void addMapVillains(vector<EnemyInfo>&, BiomeId);
  RandomGen& random;
  ProgressMeter* meter;
  Options* options;
  HeapAllocated<EnemyFactory> enemyFactory;
};


#endif
