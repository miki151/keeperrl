#ifndef _MAIN_LOOP_H
#define _MAIN_LOOP_H

#include "util.h"
#include "file_sharing.h"

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

class MainLoop {
  public:
  MainLoop(View*, Highscores*, FileSharing*, const string& dataFreePath, const string& userPath,
      Options*, Jukebox*, SokobanInput*, std::atomic<bool>& finished, bool useSingleThread,
      optional<GameTypeChoice> forceGame);

  void start(bool tilesPresent);
  void modelGenTest(int numTries, RandomGen&, Options*);

  static int getAutosaveFreq();

  private:

  RetiredGames getRetiredGames();
  int getSaveVersion(const SaveFileInfo& save);
  void uploadFile(const string& path, GameSaveType);
  void saveUI(PGame&, GameSaveType type, SplashType splashType);
  void getSaveOptions(const vector<FileSharing::GameInfo>&, const vector<pair<GameSaveType, string>>&,
      vector<ListElem>& options, vector<SaveFileInfo>& allFiles);

  void getDownloadOptions(const vector<FileSharing::GameInfo>&, vector<ListElem>& options,
      vector<SaveFileInfo>& allFiles, const string& title);

  optional<SaveFileInfo> chooseSaveFile(const vector<ListElem>& options, const vector<SaveFileInfo>& allFiles,
      string noSaveMsg, View*);

  void doWithSplash(SplashType, const string& text, int totalProgress, function<void(ProgressMeter&)> fun,
    function<void()> cancelFun = nullptr);

  void doWithSplash(SplashType, const string& text, function<void()> fun, function<void()> cancelFun = nullptr);

  PGame prepareCampaign(RandomGen&);
  PGame prepareSingleMap(RandomGen&);
  void playGame(PGame&&, bool withMusic, bool noAutoSave);
  void playGameChoice();
  void splashScreen();
  void showCredits(const string& path, View*);

  void playMenuMusic();

  Table<PModel> prepareCampaignModels(Campaign& campaign, RandomGen& random);
  PModel keeperSingleMap(RandomGen& random);
  PModel quickGame(RandomGen& random);
  PGame adventurerGame();
  PGame loadGame(string file);
  PGame loadPrevious();
  string getSavePath(PGame&, GameSaveType);
  void eraseSaveFile(PGame&, GameSaveType);

  bool downloadGame(const string& filename);
  bool eraseSave();
  static vector<SaveFileInfo> getSaveFiles(const string& path, const string& suffix);

  View* view;
  string dataFreePath;
  string userPath;
  Options* options;
  Jukebox* jukebox;
  Highscores* highscores;
  FileSharing* fileSharing;
  std::atomic<bool>& finished;
  bool useSingleThread;
  optional<GameTypeChoice> forceGame;
  SokobanInput* sokobanInput;
};



#endif

