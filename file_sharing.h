#ifndef _FILE_SHARING_H
#define _FILE_SHARING_H

#include "util.h"
#include "highscores.h"
#include "save_file_info.h"
#include "saved_game_info.h"

class ProgressMeter;

class FileSharing {
  public:
  FileSharing(const string& uploadUrl);

  optional<string> uploadRetired(const string& path, ProgressMeter&);
  optional<string> uploadSite(const string& path, ProgressMeter&);

  struct GameInfo {
    string displayName;
    string filename;
    time_t time;
    int totalGames;
    int wonGames;
    int version;
  };
  vector<GameInfo> listGames();
  struct SiteInfo {
    SavedGameInfo gameInfo;
    SaveFileInfo fileInfo;
    int version;
  };
  vector<SiteInfo> listSites();
  optional<string> download(const string& filename, const string& dir, ProgressMeter&);
  void uploadHighscores(const string& path);
  string downloadHighscores();

  void cancel();
  
  private:
  string uploadUrl;
};

#endif
