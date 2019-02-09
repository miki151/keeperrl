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
class GameConfig;

class ModelBuilder {
  public:
  ModelBuilder(ProgressMeter*, RandomGen&, Options*, SokobanInput*, const GameConfig*, const CreatureFactory*, const EnemyFactory*);
  PModel singleMapModel(const string& worldName, TribeId keeperTribe, TribeAlignment);
  PModel campaignBaseModel(const string& siteName, TribeId keeperTribe, TribeAlignment, bool externalEnemies);
  PModel campaignSiteModel(const string& siteName, EnemyId, VillainType, TribeAlignment);
  PModel tutorialModel(const string& siteName);

  void measureSiteGen(int numTries, vector<string> types);

  PModel splashModel(const FilePath& splashPath);
  PModel battleModel(const FilePath& levelPath, CreatureList allies, CreatureList enemies);

  static WCollective spawnKeeper(WModel, AvatarInfo, bool regenerateMana, vector<string> introText);

  ~ModelBuilder();

  private:
  void measureModelGen(const std::string& name, int numTries, function<void()> genFun);
  PModel trySingleMapModel(const string& worldName, TribeId keeperTribe, TribeAlignment);
  PModel tryCampaignBaseModel(const string& siteName, TribeId keeperTribe, TribeAlignment, bool externalEnemies);
  PModel tryTutorialModel(const string& siteName);
  PModel tryCampaignSiteModel(const string& siteName, EnemyId, VillainType, TribeAlignment);
  PModel tryModel(int width, const string& levelName, vector<EnemyInfo>,
      optional<TribeId> keeperTribe, BiomeId, optional<ExternalEnemies>, bool wildlife);
  SettlementInfo& makeExtraLevel(WModel, EnemyInfo&);
  PModel tryBuilding(int numTries, function<PModel()> buildFun, const string& name);
  void addMapVillainsForEvilKeeper(vector<EnemyInfo>&, BiomeId);
  void addMapVillainsForLawfulKeeper(vector<EnemyInfo>&, BiomeId);
  RandomGen& random;
  ProgressMeter* meter = nullptr;
  Options* options = nullptr;
  const EnemyFactory* enemyFactory = nullptr;
  SokobanInput* sokobanInput = nullptr;
  const GameConfig* gameConfig = nullptr;
  vector<EnemyInfo> getSingleMapEnemiesForEvilKeeper(TribeId keeperTribe);
  vector<EnemyInfo> getSingleMapEnemiesForLawfulKeeper(TribeId keeperTribe);
  const CreatureFactory* creatureFactory = nullptr;
};
