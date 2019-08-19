#include "stdafx.h"
#include "file_sharing.h"
#include "progress_meter.h"
#include "save_file_info.h"
#include "parse_game.h"
#include "options.h"
#include "text_serialization.h"
#include "miniunz.h"
#include "mod_info.h"

#include <curl/curl.h>

#ifdef USE_STEAMWORKS
#include "steam_client.h"
#include "steam_ugc.h"
#include "steam_user.h"
#include "steam_friends.h"
#endif

FileSharing::FileSharing(const string& url, const string& modVer, Options& o, string id)
    : uploadUrl(url), modVersion(modVer), options(o), uploadLoop(bindMethod(&FileSharing::uploadingLoop, this)),
      installId(id), wasCancelled(false) {
  curl_global_init(CURL_GLOBAL_ALL);
}

FileSharing::~FileSharing() {
  // pushing null acts as a signal for the upload loop to finish
  uploadQueue.push(nullptr);
}

struct CallbackData {
  function<void(double)> progressCallback;
  FileSharing* fileSharing;
};

static int progressFunction(void* ptr, double totalDown, double nowDown, double totalUp, double nowUp) {
  auto callbackData = static_cast<CallbackData*>(ptr);
  if (totalUp > 0)
    callbackData->progressCallback(nowUp / totalUp);
  if (totalDown > 0)
    callbackData->progressCallback(nowDown / totalDown);
  if (callbackData->fileSharing->consumeCancelled())
    return 1;
  else
    return 0;
}

size_t dataFun(void *buffer, size_t size, size_t nmemb, void *userp) {
  string& buf = *((string*) userp);
  buf += string((char*) buffer, (char*) buffer + size * nmemb);
  return size * nmemb;
}

static string escapeSpaces(string s) {
  string ret;
  for (auto c : s)
    if (c == ' ')
      ret += "%20";
    else
      ret += c;
  return ret;
}

static string escapeEverything(const string& s) {
  char* tmp = curl_easy_escape(curl_easy_init(), s.c_str(), (int) s.size());
  string ret(tmp);
  curl_free(tmp);
  return ret;
}

static string unescapeEverything(const string& s) {
  char* tmp = curl_easy_unescape(curl_easy_init(), s.c_str(), (int) s.size(), nullptr);
  string ret(tmp);
  curl_free(tmp);
  return ret;
}

static optional<string> curlUpload(const char* path, const char* url, const CallbackData& callback, int timeout) {
  curl_httppost* formpost = nullptr;
  curl_httppost* lastptr = nullptr;

  curl_formadd(&formpost,
      &lastptr,
      CURLFORM_COPYNAME, "fileToUpload",
      CURLFORM_FILE, path,
      CURLFORM_END);

  curl_formadd(&formpost,
      &lastptr,
      CURLFORM_COPYNAME, "submit",
      CURLFORM_COPYCONTENTS, "send",
      CURLFORM_END);

  if (CURL* curl = curl_easy_init()) {
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, dataFun);
    string ret;
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ret);
    /* what URL that receives this POST */
    curl_easy_setopt(curl, CURLOPT_URL, escapeSpaces(url).c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
    if (timeout > 0)
      curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, false);
    curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, &callback);
    curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progressFunction);
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK)
      ret = string("Upload failed: ") + curl_easy_strerror(res);
    curl_easy_cleanup(curl);
    curl_formfree(formpost);
    if (!ret.empty())
      return ret;
    else
      return none;
  } else
    return string("Failed to initialize libcurl");
}

static CallbackData getCallbackData(FileSharing* f) {
  return { [] (double) {}, f };
}

static CallbackData getCallbackData(FileSharing* f, ProgressMeter& meter) {
  return { [&meter] (double p) { meter.setProgress((float) p); }, f };
}

optional<string> FileSharing::uploadSite(const FilePath& path, ProgressMeter& meter) {
  if (!options.getBoolValue(OptionId::ONLINE))
    return none;
  return curlUpload(path.getPath(), (uploadUrl + "/upload_site.php").c_str(), getCallbackData(this, meter), 0);
}

void FileSharing::uploadHighscores(const FilePath& path) {
  if (options.getBoolValue(OptionId::ONLINE))
    uploadQueue.push([this, path] {
      curlUpload(path.getPath(), (uploadUrl + "/upload_scores.php").c_str(), getCallbackData(this), 5);
    });
}

optional<string> FileSharing::uploadBugReport(const string& text, optional<FilePath> savefile,
    optional<FilePath> screenshot, ProgressMeter& meter) {
  string params = "description=" + escapeEverything(text);
  curl_httppost* formpost = nullptr;
  curl_httppost* lastptr = nullptr;

  auto addFile = [&] (const char* type, const char* path) {
    curl_formadd(&formpost,
        &lastptr,
        CURLFORM_COPYNAME, type,
        CURLFORM_FILE, path,
        CURLFORM_END);
  };
  if (savefile)
    addFile("savefile", savefile->getPath());
  if (screenshot)
    addFile("screenshot", screenshot->getPath());

  curl_formadd(&formpost,
      &lastptr,
      CURLFORM_COPYNAME, "description",
      CURLFORM_COPYCONTENTS, text.data(),
      CURLFORM_END);
  curl_formadd(&formpost,
      &lastptr,
      CURLFORM_COPYNAME, "installId",
      CURLFORM_COPYCONTENTS, installId.data(),
      CURLFORM_END);
  if (CURL* curl = curl_easy_init()) {
    string ret;
    string url = uploadUrl + "/upload.php";
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
    auto callback = getCallbackData(this, meter);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, false);
    curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, &callback);
    curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progressFunction);
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    curl_formfree(formpost);
    if (res != CURLE_OK)
      return "Upload failed: "_s + curl_easy_strerror(res);
  }
  return none;
}

void FileSharing::uploadingLoop() {
  function<void()> uploadFun = uploadQueue.pop();
  if (uploadFun)
    uploadFun();
  else
    uploadLoop.setDone();
}

bool FileSharing::uploadGameEvent(const GameEvent& data1, bool requireGameEventsPermission) {
  GameEvent data(data1);
  data.emplace("installId", installId);
  if (options.getBoolValue(OptionId::ONLINE) &&
      (!requireGameEventsPermission || options.getBoolValue(OptionId::GAME_EVENTS))) {
    uploadGameEventImpl(data, 5);
    return true;
  } else
    return false;
}

void FileSharing::uploadGameEventImpl(const GameEvent& data, int tries) {
  if (tries >= 0)
    uploadQueue.push([data, this, tries] {
      string params;
      for (auto& elem : data) {
        if (!params.empty())
          params += "&";
        params += elem.first + "=" + escapeEverything(elem.second);
      }
      if (CURL* curl = curl_easy_init()) {
        string ret;
        curl_easy_setopt(curl, CURLOPT_URL, escapeSpaces(uploadUrl + "/game_event.php").c_str());
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, params.c_str());
        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        if (res != CURLE_OK)
          uploadGameEventImpl(data, tries - 1);
      }
    });
}

string FileSharing::downloadHighscores(int version) {
  string ret;
  if (options.getBoolValue(OptionId::ONLINE))
    if(CURL* curl = curl_easy_init()) {
      curl_easy_setopt(curl, CURLOPT_URL,
          escapeSpaces(uploadUrl + "/highscores.php?version=" + toString(version)).c_str());
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, dataFun);
      curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ret);
      curl_easy_setopt(curl, CURLOPT_NOPROGRESS, false);
      auto callback = getCallbackData(this);
      curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, &callback);
      curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progressFunction);
      curl_easy_perform(curl);
      curl_easy_cleanup(curl);
    }
  return ret;
}

template<typename Elem>
static vector<Elem> parseLines(const string& s, function<optional<Elem>(const vector<string>&)> parseLine) {
  std::stringstream iss(s);
  vector<Elem> ret;
  while (!!iss) {
    char buf[10000];
    iss.getline(buf, 10000);
    if (!iss)
      break;
    INFO << "Parsing " << string(buf);
    if (auto elem = parseLine(split(buf, {','})))
      ret.push_back(*elem);
  }
  return ret;

}

optional<string> FileSharing::downloadContent(const string& url) {
  if (CURL* curl = curl_easy_init()) {
    curl_easy_setopt(curl, CURLOPT_URL, escapeSpaces(url).c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, dataFun);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, false);
    auto callback = getCallbackData(this);
    curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, &callback);
    curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progressFunction);
    string ret;
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ret);
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    if (res == CURLE_OK)
      return ret;
  }
  return none;
}

static optional<FileSharing::SiteInfo> parseSite(const vector<string>& fields) {
  if (fields.size() < 6)
    return none;
  INFO << "Parsed " << fields;
  FileSharing::SiteInfo elem;
  elem.fileInfo.filename = fields[0];
  try {
    elem.fileInfo.date = fromString<int>(fields[1]);
    elem.wonGames = fromString<int>(fields[2]);
    elem.totalGames = fromString<int>(fields[3]);
    elem.version = fromString<int>(fields[5]);
    elem.fileInfo.download = true;
    TextInput input(fields[4]);
    input.getArchive() >> elem.gameInfo;
  } catch (cereal::Exception) {
    return none;
  } catch (ParsingException e) {
    return none;
  }
  return elem;
}

optional<vector<FileSharing::SiteInfo>> FileSharing::listSites() {
  if (!options.getBoolValue(OptionId::ONLINE))
    return {};
  if (auto content = downloadContent(uploadUrl + "/get_sites.php"))
    return parseLines<FileSharing::SiteInfo>(*content, parseSite);
  else
    return none;
}

static optional<FileSharing::BoardMessage> parseBoardMessage(const vector<string>& fields) {
  if (fields.size() >= 2)
    return FileSharing::BoardMessage{unescapeEverything(fields[0]), unescapeEverything(fields[1])};
  else
    return none;
}

optional<vector<FileSharing::BoardMessage>> FileSharing::getBoardMessages(int boardId) {
  if (options.getBoolValue(OptionId::ONLINE))
    if (auto content = downloadContent(uploadUrl + "/get_messages.php?boardId=" + toString(boardId)))
      return parseLines<FileSharing::BoardMessage>(*content, parseBoardMessage);
  return none;
}

bool FileSharing::uploadBoardMessage(const string& gameId, int hash, const string& author, const string& text) {
  return uploadGameEvent({
      { "gameId", gameId },
      { "eventType", "boardMessage"},
      { "boardId", toString(hash) },
      { "author", author },
      { "text", text }
  }, false);
}

static optional<ModInfo> parseModInfo(const vector<string>& fields, const string& modVersion) {
  if (fields.size() >= 8)
    if (auto numGames = fromStringSafe<int>(unescapeEverything(fields[3])))
      if (auto version = fromStringSafe<int>(unescapeEverything(fields[4])))
        if (auto steamId = fromStringSafe<SteamId>(unescapeEverything(fields[5])))
          if (auto rating = fromStringSafe<double>(unescapeEverything(fields[7])))
            if (fields[6] == modVersion)
              return ModInfo{unescapeEverything(fields[0]), unescapeEverything(fields[1]), unescapeEverything(fields[2]),
                    ModVersionInfo{*steamId, *version, modVersion}, *numGames, *rating, false, false, false, {}};
  return none;
}

static string firstLines(string text, int max_lines = 3) {
  int num_lines = 1;
  for (int n = 0; n < (int)text.size(); n++) {
    if (text[n] == '\n') {
      num_lines++;
      if (num_lines > max_lines) {
        text.resize(n);
        break;
      }
    }
  }
  while (text.back() == '\n')
    text.pop_back();
  return text;
}

optional<vector<ModInfo>> FileSharing::getSteamMods() {
#ifdef USE_STEAMWORKS
  if (!steam::Client::isAvailable()) {
#ifdef RELEASE
    USER_INFO << "Steam client not available";
#endif
    return none;
  }

  // TODO: thread safety
  // TODO: filter mods by tags early
  auto& ugc = steam::UGC::instance();
  auto& user = steam::User::instance();
  auto& friends = steam::Friends::instance();

  vector<steam::ItemId> items, subscribedItems;
  subscribedItems = ugc.subscribedItems();

  // TODO: Is this check necessary? Maybe we should try anyways?
  if (user.isLoggedOn()) {
    steam::FindItemInfo qinfo;
    qinfo.order = SteamFindOrder::playtime;
    auto qid = ugc.createFindQuery(qinfo, 1);

    // TODO: multiple pages
    // TODO: handle errors
    // TODO: czy chcemy je jakoś filtrować? Czy na razie po prostu dajemy wszystkie / najpopularniejsze?

    ugc.waitForQueries({qid}, milliseconds(2000));

    if (ugc.queryStatus(qid) == QueryStatus::completed) {
      items = ugc.finishFindQuery(qid);
    } else {
      INFO << "STEAM: FindQuery failed: " << ugc.queryError(qid, "timeout (2 sec)");
      ugc.finishQuery(qid);
    }
  }

  for (auto id : subscribedItems)
    if (!items.contains(id))
      items.emplace_back(id);

  if (items.empty()) {
    INFO << "STEAM: No items present";
    // TODO: inform that no mods are present (not subscribed or ...)
    return vector<ModInfo>();
  }

  steam::ItemDetailsInfo detailsInfo;
  detailsInfo.longDescription = true;
  detailsInfo.playtimeStatsDays = 9999;
  detailsInfo.metadata = true;
  auto qid = ugc.createDetailsQuery(detailsInfo, items);
  ugc.waitForQueries({qid}, milliseconds(3000));

  if (ugc.queryStatus(qid) != QueryStatus::completed) {
    INFO << "STEAM: DetailsQuery failed: " << ugc.queryError(qid, "timeout (3 sec)");
    ugc.finishQuery(qid);
    return {};
  }

  auto infos = ugc.finishDetailsQuery(qid);
  for (auto& info : infos)
    friends.requestUserInfo(info.ownerId, true);
  vector<optional<string>> ownerNames(infos.size());
  auto retrieveUserNames = [&]() {
    bool done = true;
    for (int n = 0; n < infos.size(); n++) {
      if (!ownerNames[n])
        ownerNames[n] = friends.retrieveUserName(infos[n].ownerId);
      if (!ownerNames[n])
        done = false;
    }
    return done;
  };
  steam::sleepUntil(retrieveUserNames, milliseconds(1500));
  vector<ModInfo> out;

  for (int n = 0; n < infos.size(); n++) {
    auto& info = infos[n];
    if (!info.tags.contains("Mod") || !info.tags.contains(modVersion))
      continue;

    ModInfo mod;
    mod.author = ownerNames[n].value_or("unknown");
    mod.description = firstLines(info.description);
    mod.name = info.title;
    // TODO: playtimeSessions is not exactly the same as numGames
    mod.numGames = info.stats->playtimeSessions;
    mod.versionInfo.steamId = info.id.value;
    mod.versionInfo.version = steam::getItemVersion(info.metadata).value_or(0);
    mod.versionInfo.compatibilityTag = modVersion;
    mod.isSubscribed = subscribedItems.contains(info.id);
    mod.rating = (info.votesUp + info.votesDown > 0) ? double(info.votesUp) / (info.votesUp + info.votesDown) : -1.0;
    out.emplace_back(mod);
  }

  return out;
#else
  return none;
#endif
}

optional<vector<ModInfo>> FileSharing::getOnlineMods() {
  if (!options.getBoolValue(OptionId::ONLINE))
    return none;
  if (auto steamMods = getSteamMods())
    return steamMods;
  if (options.getBoolValue(OptionId::ONLINE))
    if (auto content = downloadContent(uploadUrl + "/get_mods.php"))
      return parseLines<ModInfo>(*content, [this](auto& e) { return parseModInfo(e, modVersion);});
  return none;
}

optional<string> FileSharing::downloadSteamMod(SteamId id_, const string& name, const DirectoryPath& modsDir,
    ProgressMeter& meter) {
#ifdef USE_STEAMWORKS
  if (!steam::Client::isAvailable())
    return "Steam client not available"_s;
  steam::ItemId id(id_);

  auto& ugc = steam::UGC::instance();
  auto& user = steam::User::instance();

  if (!ugc.isInstalled(id)) {
    if (!ugc.downloadItem(id, true))
      return string("Error while downloading mod.");

    // TODO: meter support
    while (!consumeCancelled()) {
      steam::runCallbacks();
      if (!ugc.isDownloading(id))
        break;
      sleep_for(milliseconds(50));
    }

    if (!ugc.isInstalled(id))
      return string("Error while downloading mod.");
  }

  auto instInfo = ugc.installInfo(id);
  if (!instInfo)
    return string("Error while retrieving installation info");
  return DirectoryPath::copyFiles(DirectoryPath(instInfo->folder), modsDir.subdirectory(name), true);
#else
  return string("Steam support is not available in this build");
#endif
}

optional<string> FileSharing::downloadMod(const string& modName, SteamId steamId, const DirectoryPath& modsDir, ProgressMeter& meter) {
  if (!!downloadSteamMod(steamId, modName, modsDir, meter)) {
    auto fileName = toString(steamId) + ".zip";
    if (auto err = download(fileName, "mods", modsDir, meter))
      return err;
    return unzip(modsDir.file(fileName).getPath(), modsDir.getPath());
  } else
    return none;
}

void FileSharing::cancel() {
  wasCancelled = true;
}

bool FileSharing::consumeCancelled() {
  return wasCancelled.exchange(false);
}

static size_t writeToFile(void *ptr, size_t size, size_t nmemb, FILE *stream) {
  return fwrite(ptr, size, nmemb, stream);
}

optional<string> FileSharing::download(const string& filename, const string& remoteDir, const DirectoryPath& dir, ProgressMeter& meter) {
  if (!options.getBoolValue(OptionId::ONLINE))
    return string("Downloading not enabled!");
  //progressFun = [&] (double p) { meter.setProgress(p);};
  if (CURL *curl = curl_easy_init()) {
    auto path = dir.file(filename);
    INFO << "Downloading to " << path;
    if (FILE* fp = fopen(path.getPath(), "wb")) {
      curl_easy_setopt(curl, CURLOPT_URL, escapeSpaces(uploadUrl + "/" + remoteDir + "/" + filename).c_str());
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToFile);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
      // Internal CURL progressmeter must be disabled if we provide our own callback
      curl_easy_setopt(curl, CURLOPT_NOPROGRESS, false);
      // Install the callback function
      auto callback = getCallbackData(this, meter);
      curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, &callback);
      curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progressFunction);
      curl_easy_setopt(curl, CURLOPT_FAILONERROR, true);
      CURLcode res = curl_easy_perform(curl);
      string ret;
      if(res != CURLE_OK)
        ret = string("Download failed: ") + curl_easy_strerror(res);
      curl_easy_cleanup(curl);
      fclose(fp);
      if (!ret.empty()) {
        remove(path.getPath());
        return ret;
      } else
        return none;
    } else
      return string("Failed to open file: "_s + path.getPath());
  } else
    return string("Failed to initialize libcurl");
}

