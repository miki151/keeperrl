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

  /** Generates levels and all game entities for a collective game. */
  static PModel singleMapModel(ProgressMeter*, RandomGen&, Options*, const string& worldName);
  static PModel campaignModel(ProgressMeter*, RandomGen&, Options*, const string& siteName);
  static void measureModelGen(int numTries, RandomGen&, Options*);

  static PModel quickModel(ProgressMeter*, RandomGen&, Options*);

  static PModel splashModel(ProgressMeter*, const string& splashPath);

  static int getPigstyPopulationIncrease();
  static int getStatuePopulationIncrease();
  static int getThronePopulationIncrease();

  private:
  static PModel trySingleMapModel(ProgressMeter*, RandomGen&, Options*, const string& worldName);
  static PModel tryCampaignModel(ProgressMeter*, RandomGen&, Options*, const string& siteName);
  static PModel tryModel(ProgressMeter*, RandomGen&, Options*, int width, const string& levelName, vector<EnemyInfo>);
  static PModel tryQuickModel(ProgressMeter*, RandomGen&, Options*);
  static Level* makeExtraLevel(ProgressMeter*, RandomGen&, Model*, const LevelInfo&, const SettlementInfo&);
  static PModel tryBuilding(ProgressMeter*, int numTries, function<PModel()> buildFun);
};


#endif
