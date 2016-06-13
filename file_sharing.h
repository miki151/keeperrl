#ifndef _FILE_SHARING_H
#define _FILE_SHARING_H

#include "util.h"
#include "highscores.h"
#include "save_file_info.h"
#include "saved_game_info.h"

class ProgressMeter;
class GameEvents;

class FileSharing {
  public:
  FileSharing(const string& uploadUrl, Options&);

  void init();

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
  optional<vector<GameInfo>> listGames();
  struct SiteInfo {
    SavedGameInfo gameInfo;
    SaveFileInfo fileInfo;
    int totalGames;
    int wonGames;
    int version;
  };
  optional<vector<SiteInfo>> listSites();
  optional<string> download(const string& filename, const string& dir, ProgressMeter&);

  typedef map<string, string> GameEvent;
  void uploadGameEvent(const GameEvent&);
  void uploadHighscores(const string& path);

  struct BoardMessage {
    string text;
    string author;
  };
  optional<vector<BoardMessage>> getBoardMessages(int boardId);

  string downloadHighscores();

  void cancel();
  ~FileSharing();

  private:
  string uploadUrl;
  Options& options;
  SyncQueue<function<void()>> uploadQueue;
  AsyncLoop uploadLoop;
  void uploadingLoop();
  void uploadGameEventImpl(const GameEvent&, int tries);
};

#endif
