#pragma once

#include "util.h"
#include "file_sharing.h"
#include "exit_info.h"
#include "experience_type.h"
#include "game_time.h"

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
class CreatureList;
class GameConfig;
class AvatarInfo;

class MainLoop {
  public:
  MainLoop(View*, Highscores*, FileSharing*, const DirectoryPath& dataFreePath, const DirectoryPath& userPath,
      Options*, Jukebox*, SokobanInput*, GameConfig*, bool useSingleThread, int saveVersion);

  void start(bool tilesPresent, bool quickGame);
  void modelGenTest(int numTries, const vector<std::string>& types, RandomGen&, Options*);
  void battleTest(int numTries, const FilePath& levelPath, const FilePath& battleInfoPath, string enemyId, RandomGen&);
  int battleTest(int numTries, const FilePath& levelPath, CreatureList ally, CreatureList enemyId, RandomGen&);
  void endlessTest(int numTries, const FilePath& levelPath, const FilePath& battleInfoPath, RandomGen&, optional<int> numEnemy);

  static TimeInterval getAutosaveFreq();
  static void reloadModel(const FilePath& path);

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
  ExitCondition playGame(PGame, bool withMusic, bool noAutoSave, function<optional<ExitCondition> (WGame)> = nullptr);
  void splashScreen();
  void showCredits(const FilePath& path, View*);

  void playMenuMusic();

  Table<PModel> prepareCampaignModels(CampaignSetup& campaign, const AvatarInfo&, RandomGen& random);
  PGame loadGame(const FilePath&);
  PGame loadPrevious();
  FilePath getSavePath(const PGame&, GameSaveType);
  void eraseSaveFile(const PGame&, GameSaveType);

  bool downloadGame(const string& filename);
  bool eraseSave();
  static vector<SaveFileInfo> getSaveFiles(const DirectoryPath& path, const string& suffix);
  bool isCompatible(int loadedVersion);

  View* view;
  DirectoryPath dataFreePath;
  DirectoryPath userPath;
  Options* options;
  Jukebox* jukebox;
  Highscores* highscores;
  FileSharing* fileSharing;
  GameConfig* gameConfig;
  bool useSingleThread;
  SokobanInput* sokobanInput;
  PModel getBaseModel(ModelBuilder&, CampaignSetup&, const AvatarInfo&);
  void considerGameEventsPrompt();
  void considerFreeVersionText(bool tilesPresent);
  void eraseAllSavesExcept(const PGame&, optional<GameSaveType>);
  PGame prepareTutorial();
  void launchQuickGame();
  void bugReportSave(PGame&, FilePath);
  int saveVersion;
  void saveGame(PGame&, const FilePath&);
  void saveMainModel(PGame&, const FilePath&);
};




