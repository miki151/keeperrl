#pragma once

#include "util.h"
#include "highscores.h"
#include "save_file_info.h"
#include "saved_game_info.h"

class ProgressMeter;

class FileSharing {
  public:
  FileSharing(const string& uploadUrl, const string& modVersion, Options&, string installId);

  optional<string> uploadSite(const FilePath& path, ProgressMeter&);
  struct SiteInfo {
    SavedGameInfo gameInfo;
    SaveFileInfo fileInfo;
    int totalGames;
    int wonGames;
    int version;
  };
  optional<vector<SiteInfo>> listSites();
  optional<string> download(const string& filename, const string& remoteDir, const DirectoryPath& dir, ProgressMeter&);

  typedef map<string, string> GameEvent;
  bool uploadGameEvent(const GameEvent&, bool requireGameEventsPermission = true);
  void uploadHighscores(const FilePath&);
  optional<string> uploadBugReport(const string& text, optional<FilePath> savefile, optional<FilePath> screenshot,
      ProgressMeter&);

  struct BoardMessage {
    string text;
    string author;
  };
  optional<vector<BoardMessage>> getBoardMessages(int boardId);
  bool uploadBoardMessage(const string& gameId, int hash, const string& author, const string& text);

  struct OnlineModInfo {
    string name;
    string author;
    string description;
    int numGames;
    int version;
    
    // Steam mod info:
    SteamId steamId;
    bool isSubscribed = false;
  };

  optional<vector<OnlineModInfo>> getSteamMods();
  optional<vector<OnlineModInfo>> getOnlineMods(int modVersion);
  optional<string> downloadSteamMod(SteamId, const string& name, const DirectoryPath& modsDir,
                                    ProgressMeter&);
  optional<string> downloadMod(const string& name, SteamId, const DirectoryPath& modsDir, ProgressMeter&);

  string downloadHighscores(int version);

  void cancel();
  bool consumeCancelled();
  ~FileSharing();

  private:
  string uploadUrl;
  string modVersion;
  Options& options;
  SyncQueue<function<void()>> uploadQueue;
  AsyncLoop uploadLoop;
  void uploadingLoop();
  void uploadGameEventImpl(const GameEvent&, int tries);
  optional<string> downloadContent(const string& url);
  string installId;
  atomic<bool> wasCancelled;
};

