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
class NameGenerator;
struct ModelTable;
class TileSet;
class ContentFactory;
class TilePaths;

class MainLoop {
  public:
  MainLoop(View*, Highscores*, FileSharing*, const DirectoryPath& dataFreePath, const DirectoryPath& userPath,
      Options*, Jukebox*, SokobanInput*, TileSet*, bool useSingleThread, int saveVersion);

  void start(bool tilesPresent);
  void modelGenTest(int numTries, const vector<std::string>& types, RandomGen&, Options*);
  void battleTest(int numTries, const FilePath& levelPath, const FilePath& battleInfoPath, string enemyId, RandomGen&);
  int battleTest(int numTries, const FilePath& levelPath, CreatureList ally, CreatureList enemyId, RandomGen&);
  void endlessTest(int numTries, const FilePath& levelPath, const FilePath& battleInfoPath, RandomGen&, optional<int> numEnemy);
  optional<string> verifyMod(const string& path);
  void launchQuickGame(optional<int> maxTurns);

  static TimeInterval getAutosaveFreq();

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
  ExitCondition playGame(PGame, bool withMusic, bool noAutoSave, bool splashScreen,
      function<optional<ExitCondition> (WGame)> = nullptr, milliseconds stepTimeMilli = milliseconds{3}, optional<int> maxTurns = none);
  void splashScreen();
  void showCredits(const FilePath& path);
  void showMods();
  void playMenuMusic();

  ModelTable prepareCampaignModels(CampaignSetup& campaign, const AvatarInfo&, RandomGen& random, ContentFactory*);
  PGame loadGame(const FilePath&);
  PGame loadPrevious();
  FilePath getSavePath(const PGame&, GameSaveType);
  void eraseSaveFile(const PGame&, GameSaveType);

  bool downloadGame(const string& filename);
  bool eraseSave();
  static vector<SaveFileInfo> getSaveFiles(const DirectoryPath& path, const string& suffix);
  bool isCompatible(int loadedVersion);

  GameConfig getGameConfig() const;

  View* view = nullptr;
  DirectoryPath dataFreePath;
  DirectoryPath userPath;
  Options* options = nullptr;
  Jukebox* jukebox = nullptr;
  Highscores* highscores = nullptr;
  FileSharing* fileSharing = nullptr;
  bool useSingleThread;
  SokobanInput* sokobanInput;
  TileSet* tileSet;
  PModel getBaseModel(ModelBuilder&, CampaignSetup&, const AvatarInfo&);
  void considerGameEventsPrompt();
  void considerFreeVersionText(bool tilesPresent);
  void eraseAllSavesExcept(const PGame&, optional<GameSaveType>);
  PGame prepareTutorial(const ContentFactory*);
  void bugReportSave(PGame&, FilePath);
  int saveVersion;
  void saveGame(PGame&, const FilePath&);
  void saveMainModel(PGame&, const FilePath&);
  ContentFactory createContentFactory(bool vanillaOnly) const;
  TilePaths getTilePathsForAllMods() const;

  // TODO: move to file_sharing?
  pair<int, unsigned long long> getLocalVersion(const string& mod);
  void updateLocalVersion(const string& mod, int version, unsigned long long steamId);
  void removeMod(const string &modName);
  void removeOldSteamMod(unsigned long long steamId, const string &newName);

  void registerModPlaytime(bool started);
};
