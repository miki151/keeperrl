#ifndef _MODEL_BUILDER_H

#include "village_control.h"
#include "stair_key.h"

class Level;
class Model;
class ProgressMeter;
class Options;
struct SettlementInfo;
struct LevelInfo;
struct EnemyInfo;

class ModelBuilder {
  public:

  static PModel singleMapModel(ProgressMeter*, RandomGen&, Options*, const string& worldName);
  static PModel campaignBaseModel(ProgressMeter*, RandomGen&, Options*, const string& siteName);
  static PModel campaignSiteModel(ProgressMeter*, RandomGen&, Options*, const string& siteName, EnemyId);

  static void measureModelGen(int numTries, function<void()> genFun);
  static void measureSiteGen(int numTries, RandomGen&, Options*);

  static PModel quickModel(ProgressMeter*, RandomGen&, Options*);

  static PModel splashModel(ProgressMeter*, const string& splashPath);

  static void spawnKeeper(Model*, Options*);

  static int getPigstyPopulationIncrease();
  static int getStatuePopulationIncrease();
  static int getThronePopulationIncrease();

  private:
  static PModel trySingleMapModel(ProgressMeter*, RandomGen&, Options*, const string& worldName);
  static PModel tryCampaignBaseModel(ProgressMeter*, RandomGen&, Options*, const string& siteName);
  static PModel tryCampaignSiteModel(ProgressMeter*, RandomGen&, Options*, const string& siteName, EnemyId);
  static PModel tryModel(ProgressMeter*, RandomGen&, Options*, int width, const string& levelName, vector<EnemyInfo>,
      bool keeperSpawn, BiomeId);
  static PModel tryQuickModel(ProgressMeter*, RandomGen&, Options*, int width);
  static Level* makeExtraLevel(ProgressMeter*, RandomGen&, Model*, const LevelInfo&, const SettlementInfo&);
  static PModel tryBuilding(ProgressMeter*, int numTries, function<PModel()> buildFun);
};


#endif
