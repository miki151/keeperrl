#ifndef _MAIN_LOOP_H
#define _MAIN_LOOP_H

#include "util.h"
#include "gzstream.h"
#include "model.h"

class View;
class Highscores;
class FileSharing;
class Options;
class Jukebox;
class ListElem;

class MainLoop {
  public:
  MainLoop(View*, Highscores*, FileSharing*, const string& dataFreePath, const string& userPath, Options* o,
      Jukebox* j, std::atomic<bool>& finished, bool useSingleThread);

  void start(bool tilesPresent);

  static int getAutosaveFreq();

  private:

  struct SaveFileInfo {
    string filename;
    time_t date;
    bool download;
  };

  int getSaveVersion(const SaveFileInfo& save);
  void uploadFile(const string& path);
  void saveUI(PModel& model, Model::GameType type, SplashType splashType);
  void getSaveOptions(const vector<pair<Model::GameType, string>>&,
      vector<ListElem>& options, vector<SaveFileInfo>& allFiles);

  void getDownloadOptions(vector<ListElem>& options, vector<SaveFileInfo>& allFiles, const string& title);

  optional<SaveFileInfo> chooseSaveFile(const vector<ListElem>& options, const vector<SaveFileInfo>& allFiles,
      string noSaveMsg, View*);

  void doWithSplash(SplashType, int totalProgress, function<void(ProgressMeter&)> fun,
    function<void()> cancelFun = nullptr);

  void playModel(PModel, bool withMusic = true, bool noAutoSave = false);
  void playGameChoice();
  void splashScreen();
  void showCredits(const string& path, View*);
  void autosave(PModel&);

  PModel keeperGame();
  PModel adventurerGame();
  PModel loadModel(string file, bool erase);
  PModel loadPrevious(bool erase);
  string getSavePath(Model*, Model::GameType);
  void eraseAutosave(Model*);

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
};



#endif

