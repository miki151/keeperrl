#pragma once

#include "util.h"
#include "file_sharing.h"
#include "exit_info.h"
#include "experience_type.h"

class View;
class Highscores;
class FileSharing;
class Options;
class Jukebox;
class ListElem;
class Campaign;
class Model;
class RetiredGames;
struct SaveFileInfo;
class GameEvents;
class SokobanInput;
struct CampaignSetup;
class ModelBuilder;
class ItemType;

class MainLoop {
  public:
  MainLoop(View*, Highscores*, FileSharing*, const DirectoryPath& dataFreePath, const DirectoryPath& userPath,
      Options*, Jukebox*, SokobanInput*, bool useSingleThread);

  void start(bool tilesPresent, bool quickGame);
  void modelGenTest(int numTries, const vector<std::string>& types, RandomGen&, Options*);
  struct BattleInfo {
    CreatureId ally;
    EnumMap<ExperienceType, int> allyLevelIncrease;
    vector<ItemType> allyEquipment;
  };
  void battleTest(int numTries, const FilePath& levelPath, const FilePath& battleInfoPath, string enemyId, RandomGen&);
  void battleTest(int numTries, const FilePath& levelPath, BattleInfo, CreatureId enemyId, int maxEnemies, RandomGen&);

  static int getAutosaveFreq();

  private:

  optional<RetiredGames> getRetiredGames(CampaignType);
  int getSaveVersion(const SaveFileInfo& save);
  void uploadFile(const FilePath& path, GameSaveType);
  void saveUI(PGame&, GameSaveType type, SplashType splashType);
  void getSaveOptions(const vector<pair<GameSaveType, string>>&,
      vector<ListElem>& options, vector<SaveFileInfo>& allFiles);

  optional<SaveFileInfo> chooseSaveFile(const vector<ListElem>& options, const vector<SaveFileInfo>& allFiles,
      string noSaveMsg, View*);

  void doWithSplash(SplashType, const string& text, int totalProgress, function<void(ProgressMeter&)> fun,
    function<void()> cancelFun = nullptr);

  void doWithSplash(SplashType, const string& text, function<void()> fun, function<void()> cancelFun = nullptr);

  PGame prepareCampaign(RandomGen&);
  enum class ExitCondition;
  ExitCondition playGame(PGame&&, bool withMusic, bool noAutoSave, function<optional<ExitCondition> (WGame)> = nullptr);
  void splashScreen();
  void showCredits(const FilePath& path, View*);

  void playMenuMusic();

  Table<PModel> prepareCampaignModels(CampaignSetup& campaign, RandomGen& random);
  PGame loadGame(const FilePath&);
  PGame loadPrevious();
  FilePath getSavePath(const PGame&, GameSaveType);
  void eraseSaveFile(const PGame&, GameSaveType);

  bool downloadGame(const string& filename);
  bool eraseSave();
  static vector<SaveFileInfo> getSaveFiles(const DirectoryPath& path, const string& suffix);

  View* view;
  DirectoryPath dataFreePath;
  DirectoryPath userPath;
  Options* options;
  Jukebox* jukebox;
  Highscores* highscores;
  FileSharing* fileSharing;
  bool useSingleThread;
  SokobanInput* sokobanInput;
  PModel getBaseModel(ModelBuilder&, CampaignSetup&);
  void considerGameEventsPrompt();
  void considerFreeVersionText(bool tilesPresent);
  void eraseAllSavesExcept(const PGame&, optional<GameSaveType>);
  PGame prepareTutorial();
  void launchQuickGame();
};




