#pragma once

#include "util.h"
#include "file_sharing.h"
#include "exit_info.h"

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

class MainLoop {
  public:
  struct ForceGameInfo {
    PlayerRole role;
    CampaignType type;
  };
  MainLoop(View*, Highscores*, FileSharing*, const DirectoryPath& dataFreePath, const DirectoryPath& userPath,
      Options*, Jukebox*, SokobanInput*, bool useSingleThread, optional<ForceGameInfo>);

  void start(bool tilesPresent);
  void modelGenTest(int numTries, const vector<std::string>& types, RandomGen&, Options*);

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

  PGame prepareCampaign(RandomGen&, const optional<ForceGameInfo>&);
  void playGame(PGame&&, bool withMusic, bool noAutoSave);
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
  optional<ForceGameInfo> forceGame;
  SokobanInput* sokobanInput;
  PModel getBaseModel(ModelBuilder&, CampaignSetup&);
  void considerGameEventsPrompt();
  void considerFreeVersionText(bool tilesPresent);
  void eraseAllSavesExcept(const PGame&, optional<GameSaveType>);
  PGame prepareTutorial();
};




