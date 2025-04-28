#pragma once

#include "util.h"
#include "highscores.h"
#include "save_file_info.h"
#include "saved_game_info.h"

class ProgressMeter;
class ModInfo;

class FileSharing {
  public:
  FileSharing(const string& uploadUrl, const string& modVersion, int saveVersion, Options&, string installId);

  const string& getInstallId() const;

  struct CancelFlag {
    std::atomic<bool> flag = {false};
    void cancel() {
      flag = true;
    }
  };

  optional<string> uploadSite(CancelFlag&, const FilePath& path, const string& title,
      const OldSavedGameInfo&, ProgressMeter&, optional<string>& url);
  optional<string> downloadSite(CancelFlag&, const SaveFileInfo&, const DirectoryPath& targetDir, ProgressMeter&);
  struct SiteInfo {
    OldSavedGameInfo gameInfo;
    SaveFileInfo fileInfo;
    int totalGames;
    int wonGames;
    int version;
    bool subscribed;
    string author;
    bool isFriend;
  };
  expected<vector<SiteInfo>, string> listSites(CancelFlag&, ProgressMeter&);

  typedef map<string, string> GameEvent;
  bool uploadGameEvent(const GameEvent&, bool requireGameEventsPermission = true);
  optional<string> uploadBugReport(CancelFlag&, const string& text, optional<FilePath> savefile,
      optional<FilePath> screenshot, ProgressMeter&);

  struct BoardMessage {
    string text;
    string author;
  };
  expected<vector<BoardMessage>, string> getBoardMessages(CancelFlag&, int boardId);
  bool uploadBoardMessage(const string& gameId, int hash, const string& author, const string& text);

  expected<vector<ModInfo>, string> getOnlineMods(CancelFlag&, ProgressMeter&);
  optional<string> downloadMod(CancelFlag&, const string& name, SteamId, const DirectoryPath& modsDir,
      ProgressMeter&);
  optional<string> uploadMod(CancelFlag&, ModInfo& modInfo, const DirectoryPath& modsDir, ProgressMeter&);

  const string& getPersonalMessage();
  void downloadPersonalMessage();

  ~FileSharing();

  struct CallbackData;
  struct UploadedFile;

  private:
  string uploadUrl;
  string modVersion;
  int saveVersion;
  Options& options;
  SyncQueue<function<void()>> uploadQueue;
  AsyncLoop uploadLoop;
  void uploadingLoop();
  void uploadGameEventImpl(const GameEvent&, int tries);
  optional<string> downloadContent(CancelFlag&, const string& url);
  optional<string> uploadContent(vector<UploadedFile>, const char* url, const CallbackData&, int timeout);
  optional<string> uploadSiteToSteam(CancelFlag&, const FilePath&, const string& title, const OldSavedGameInfo&,
      ProgressMeter&, optional<string>& url);
  optional<vector<ModInfo>> getSteamMods(CancelFlag&, ProgressMeter&);
  optional<vector<SiteInfo>> getSteamSites(CancelFlag&,ProgressMeter&);
  optional<string> downloadSteamMod(CancelFlag&, SteamId, const string& name,
      const DirectoryPath& modsDir, ProgressMeter&);
  optional<string> downloadSteamSite(CancelFlag&, const SaveFileInfo&, const DirectoryPath& targetDir,
      ProgressMeter&);
  optional<string> download(CancelFlag&, const string& filename, const string& remoteDir, const DirectoryPath& dir, ProgressMeter&);
  string installId;
  string personalMessage;
  recursive_mutex personalMessageMutex;
};

constexpr auto retiredScreenshotFilename = "retired_screenshot.png";
