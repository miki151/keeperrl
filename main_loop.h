#ifndef _MAIN_LOOP_H
#define _MAIN_LOOP_H

#include "util.h"
#include "gzstream.h"
#include "game.h"
#include "file_sharing.h"

class View;
class Highscores;
class FileSharing;
class Options;
class Jukebox;
class ListElem;
class Campaign;
class Model;

class MainLoop {
  public:
  MainLoop(View*, Highscores*, FileSharing*, const string& dataFreePath, const string& userPath, Options* o,
      Jukebox* j, std::atomic<bool>& finished, bool useSingleThread, optional<GameTypeChoice> forceGame);

  void start(bool tilesPresent);
  void modelGenTest(int numTries, RandomGen&, Options*);

  static int getAutosaveFreq();

  private:

  struct SaveFileInfo {
    string filename;
    time_t date;
    bool download;
  };

  int getSaveVersion(const SaveFileInfo& save);
  void uploadFile(const string& path);
  void saveUI(PGame&, Game::SaveType type, SplashType splashType);
  void getSaveOptions(const vector<FileSharing::GameInfo>&, const vector<pair<Game::SaveType, string>>&,
      vector<ListElem>& options, vector<SaveFileInfo>& allFiles);

  void getDownloadOptions(const vector<FileSharing::GameInfo>&, vector<ListElem>& options,
      vector<SaveFileInfo>& allFiles, const string& title);

  optional<SaveFileInfo> chooseSaveFile(const vector<ListElem>& options, const vector<SaveFileInfo>& allFiles,
      string noSaveMsg, View*);

  void doWithSplash(SplashType, int totalProgress, function<void(ProgressMeter&)> fun,
    function<void()> cancelFun = nullptr);

  PGame prepareCampaign(RandomGen&);
  PGame prepareSingleMap(RandomGen&);
  void playGame(PGame&&, bool withMusic, bool noAutoSave);
  void playGameChoice();
  void splashScreen();
  void showCredits(const string& path, View*);

  PModel keeperCampaign(RandomGen& random);
  PModel keeperSingleMap(RandomGen& random);
  PModel quickGame(RandomGen& random);
  PGame adventurerGame();
  PGame loadGame(string file, bool erase);
  PGame loadPrevious(bool erase);
  string getSavePath(PGame&, Game::SaveType);
  void eraseAutosave(PGame&);

  bool downloadGame(const string& filename);
  static bool eraseSave(Options* options);
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
};



#endif

