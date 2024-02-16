#pragma once

#include "util.h"
#include "file_sharing.h"
#include "exit_info.h"
#include "game_time.h"

class View;
class Highscores;
class FileSharing;
class Options;
class Jukebox;
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
struct ModVersionInfo;
struct ModDetails;
class TribeId;
struct RetiredModelInfo;
class Unlocks;
class SteamAchievements;

class MainLoop {
  public:
  MainLoop(View*, Highscores*, FileSharing*, const DirectoryPath& paidDataPath, const DirectoryPath& dataFreePath,
      const DirectoryPath& userPath, const DirectoryPath& modsDir, Options*, Jukebox*, SokobanInput*, TileSet*, Unlocks*,
      SteamAchievements*, int saveVersion, string modVersion);

  void start(bool tilesPresent);
  void modelGenTest(int numTries, const vector<std::string>& types, RandomGen&, Options*);
  void battleTest(int numTries, const FilePath& levelPath, const FilePath& battleInfoPath, string enemyId);
  int battleTest(int numTries, const FilePath& levelPath, vector<CreatureList> ally, vector<CreatureList> enemies);
  void endlessTest(int numTries, const FilePath& levelPath, const FilePath& battleInfoPath, optional<int> numEnemy);
  void campaignBattleText(int numTries, const FilePath& levelPath, EnemyId keeperId, VillainGroup);
  int campaignBattleText(int numTries, const FilePath& levelPath, EnemyId keeperId, EnemyId);
  void launchQuickGame(optional<int> maxTurns, bool tryToLoad);
  void genZLevels(const string& keeperType);
  ContentFactory createContentFactory(bool vanillaOnly) const;

  private:

  optional<RetiredGames> getRetiredGames(CampaignType);
  int getSaveVersion(const SaveFileInfo& save);
  void uploadFile(const FilePath& path, const string& title, const SavedGameInfo&);
  void saveUI(PGame&, GameSaveType type);
  vector<SaveFileInfo> getSaveOptions(const vector<GameSaveType>&);

  void doWithSplash(const string& text, int totalProgress, function<void(ProgressMeter&)> fun,
    function<void()> cancelFun = nullptr);

  void doWithSplash(const string& text, function<void()> fun, function<void()> cancelFun = nullptr);

  PGame prepareCampaign(RandomGen&);
  PGame prepareWarlord(const SaveFileInfo&);
  enum class ExitCondition;
  ExitCondition playGame(PGame, bool withMusic, bool noAutoSave, function<optional<ExitCondition> (Game*)> = nullptr,
      milliseconds stepTimeMilli = milliseconds{3}, optional<int> maxTurns = none);
  void showCredits();
  void showAchievements();
  void showMods();
  void playMenuMusic();
  ModelTable prepareCampaignModels(CampaignSetup& campaign, const AvatarInfo&, RandomGen&, ContentFactory*);
  ModelTable prepareCampaignModels(CampaignSetup& campaign, const AvatarInfo&, ModelBuilder, ContentFactory*);
  PGame loadGame(const FilePath&, const string& name);
  PGame loadOrNewGame();
  FilePath getSavePath(const PGame&, GameSaveType);
  void eraseSaveFile(const PGame&, GameSaveType);

  bool downloadGame(const SaveFileInfo&);
  bool eraseSave();
  static vector<SaveFileInfo> getSaveFiles(const DirectoryPath& path, const string& suffix);
  bool isCompatible(int loadedVersion);

  View* view = nullptr;
  DirectoryPath paidDataPath;
  DirectoryPath dataFreePath;
  DirectoryPath userPath;
  DirectoryPath modsDir;
  Options* options = nullptr;
  Jukebox* jukebox = nullptr;
  Highscores* highscores = nullptr;
  FileSharing* fileSharing = nullptr;
  SokobanInput* sokobanInput;
  TileSet* tileSet;
  int saveVersion;
  string modVersion;
  PModel getBaseModel(ModelBuilder&, CampaignSetup&, const AvatarInfo&);
  void considerGameEventsPrompt();
  void considerFreeVersionText(bool tilesPresent);
  void eraseAllSavesExcept(const PGame&, optional<GameSaveType>);
  PGame prepareTutorial(const ContentFactory*);
  void bugReportSave(PGame&, FilePath);
  void saveGame(PGame&, const FilePath&);
  void saveMainModel(PGame&, const FilePath& modelPath, const FilePath& warlordPath);
  TilePaths getTilePathsForAllMods() const;
  vector<string> getCurrentMods() const;

  optional<ModVersionInfo> getLocalModVersionInfo(const string& mod) const;
  void updateLocalModVersion(const string& mod, const ModVersionInfo&);
  optional<ModDetails> getLocalModDetails(const string& mod);
  void updateLocalModDetails(const string& mod, const ModDetails&);
  void removeMod(const string &modName);
  void removeOldSteamMod(SteamId, const string &newName);

  void registerModPlaytime(bool started);
  vector<ModInfo> getAllMods(const vector<ModInfo>& onlineMods);
  void downloadMod(ModInfo&);
  void uploadMod(ModInfo&);
  void createNewMod();
  vector<ModInfo> getOnlineMods();
  GameConfig getVanillaConfig() const;
  GameConfig getGameConfig(const vector<string>& modNames) const;
  DirectoryPath getVanillaDir() const;
  template<typename T>
  optional<T> loadFromFile(const FilePath&);
  optional<RetiredModelInfo> loadRetiredModelFromFile(const FilePath&);
  bool useSingleThread();
  Unlocks* unlocks;
  SteamAchievements* steamAchievements = nullptr;
};
