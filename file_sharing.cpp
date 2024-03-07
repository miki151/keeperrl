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

#include "steam_ugc.h"

#ifdef USE_STEAMWORKS
#include "steam_client.h"
#include "steam_user.h"
#include "steam_friends.h"
#endif

FileSharing::FileSharing(const string& url, const string& modVer, int saveVersion, Options& o, string id)
    : uploadUrl(url), modVersion(modVer), saveVersion(saveVersion), options(o),
      uploadLoop(bindMethod(&FileSharing::uploadingLoop, this)), installId(id) {
  curl_global_init(CURL_GLOBAL_ALL);
}

FileSharing::~FileSharing() {
  // pushing null acts as a signal for the upload loop to finish
  uploadQueue.push(nullptr);
}

const string& FileSharing::getInstallId() const {
  return installId;
}

struct FileSharing::CallbackData {
  function<void(double)> progressCallback;
  CancelFlag& cancel;
};

static int progressFunction(void* ptr, double totalDown, double nowDown, double totalUp, double nowUp) {
  auto callbackData = static_cast<FileSharing::CallbackData*>(ptr);
  if (totalUp > 0)
    callbackData->progressCallback(nowUp / totalUp);
  if (totalDown > 0)
    callbackData->progressCallback(nowDown / totalDown);
  if (callbackData->cancel.flag)
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

struct FileSharing::UploadedFile {
  const char* path;
  const char* paramName;
};

optional<string> FileSharing::uploadContent(vector<UploadedFile> files, const char* url, const CallbackData& callback, int timeout) {
  curl_httppost* formpost = nullptr;
  curl_httppost* lastptr = nullptr;

  for (auto file : files)
    curl_formadd(&formpost,
        &lastptr,
        CURLFORM_COPYNAME, file.paramName,
        CURLFORM_FILE, file.path,
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

static FileSharing::CallbackData getCallbackData(FileSharing::CancelFlag& cancel) {
  return { [] (double) {}, cancel };
}

static FileSharing::CallbackData getCallbackData(FileSharing::CancelFlag& cancel, ProgressMeter& meter) {
  return { [&meter] (double p) { meter.setProgress((float) p); }, cancel };
}

optional<string> FileSharing::uploadSite(CancelFlag& cancel, const FilePath& path, const string& title,
    const OldSavedGameInfo& info, ProgressMeter& meter, optional<string>& url) {
  if (!options.getBoolValue(OptionId::ONLINE))
    return "Online features not enabled!"_s;
  if (!!uploadSiteToSteam(cancel, path, title, info, meter, url))
    return uploadContent({UploadedFile{path.getPath(), "fileToUpload"},UploadedFile{retiredScreenshotFilename, "screenshot"}},
          (uploadUrl + "/upload_site.php").c_str(), getCallbackData(cancel, meter), 0);
  return none;
}

optional<string> FileSharing::downloadSite(CancelFlag& cancel, const SaveFileInfo& file, const DirectoryPath& targetDir, ProgressMeter& meter) {
  if (!file.steamId || !!downloadSteamSite(cancel, file, targetDir, meter))
    return download(cancel, file.filename, "dungeons", targetDir, meter);
  return none;
}

optional<string> FileSharing::uploadBugReport(CancelFlag& cancel, const string& text, optional<FilePath> savefile,
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
    auto callback = getCallbackData(cancel, meter);
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

void FileSharing::downloadPersonalMessage() {
  uploadQueue.push([this] {
    RecursiveLock lock(personalMessageMutex);
    CancelFlag c;
    personalMessage = downloadContent(c, uploadUrl + "/get_personal.php?installId=" + escapeEverything(installId)).value_or(""_s);
    auto prefix = "personal123"_s;
    if (startsWith(personalMessage, prefix))
      personalMessage = personalMessage.substr(prefix.size());
    else
      personalMessage = "";
  });
}

const std::string& FileSharing::getPersonalMessage() {
  RecursiveLock lock(personalMessageMutex);
  return personalMessage;
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
    //INFO << "Parsing " << string(buf);
    if (auto elem = parseLine(split(buf, {','})))
      ret.push_back(*elem);
  }
  return ret;

}

optional<string> FileSharing::downloadContent(CancelFlag& cancel, const string& url) {
  if (CURL* curl = curl_easy_init()) {
    curl_easy_setopt(curl, CURLOPT_URL, escapeSpaces(url).c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, dataFun);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, false);
    auto callback = getCallbackData(cancel);
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
  if (fields.size() < 3)
    return none;
  //INFO << "Parsed " << fields;
  FileSharing::SiteInfo elem {};
  TextInput input(unescapeEverything(fields[0]));
  input.getArchive() >> elem.fileInfo.filename >> elem.gameInfo;
  try {
    elem.fileInfo.date = fromString<int>(fields[1]);
    elem.version = fromString<int>(fields[2]);
    elem.fileInfo.download = true;
  } catch (cereal::Exception) {
    return none;
  } catch (ParsingException e) {
    return none;
  }
  return elem;
}

namespace {
struct SiteConquestInfo {
  string filename;
  int totalGames;
  int wonGames;
};
}

static optional<SiteConquestInfo> parseSiteConquest(const vector<string>& fields) {
  if (fields.size() < 3)
    return none;
  INFO << "Parsed " << fields;
  SiteConquestInfo elem {};
  try {
    elem.filename = unescapeEverything(fields[0]);
    elem.wonGames = fromString<int>(fields[1]);
    elem.totalGames = fromString<int>(fields[2]);
  } catch (cereal::Exception) {
    return none;
  } catch (ParsingException e) {
    return none;
  }
  return elem;
}

expected<vector<FileSharing::SiteInfo>, string> FileSharing::listSites(CancelFlag& cancel, ProgressMeter& progress) {
  if (!options.getBoolValue(OptionId::ONLINE))
    return make_unexpected("Please enable online features in the settings in order to download retired dungeons!"_s);
  vector<SiteConquestInfo> conquestInfo;
  thread downloadConquest([&conquestInfo, &cancel, this] {
    if (auto content = downloadContent(cancel, uploadUrl + "/get_sites.php"))
      conquestInfo = parseLines<SiteConquestInfo>(*content, parseSiteConquest);
  });
  optional<vector<FileSharing::SiteInfo>> ret = getSteamSites(cancel, progress);
  if (!ret)
    if (auto content = downloadContent(cancel, uploadUrl + "/dungeons.txt"))
      ret = parseLines<FileSharing::SiteInfo>(*content, parseSite);
  downloadConquest.join();
  if (!ret)
    return make_unexpected("Error fetching online dungeons."_s);
  for (auto& conquestElem : conquestInfo)
    for (auto& elem : *ret)
      if (elem.fileInfo.filename == conquestElem.filename) {
        elem.wonGames = conquestElem.wonGames;
        elem.totalGames = conquestElem.totalGames;
      }
  return *ret;
}

static optional<FileSharing::BoardMessage> parseBoardMessage(const vector<string>& fields) {
  if (fields.size() >= 2)
    return FileSharing::BoardMessage{unescapeEverything(fields[0]), unescapeEverything(fields[1])};
  else
    return none;
}

expected<vector<FileSharing::BoardMessage>, string> FileSharing::getBoardMessages(CancelFlag& cancel, int boardId) {
  if (options.getBoolValue(OptionId::ONLINE))
    if (auto content = downloadContent(cancel, uploadUrl + "/get_messages.php?boardId=" + toString(boardId)))
      return parseLines<FileSharing::BoardMessage>(*content, parseBoardMessage);
  return make_unexpected("Please enable online features in the settings in order to download messages!"_s);
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
  while (!text.empty() && text.back() == '\n')
    text.pop_back();
  return text;
}

static optional<ModInfo> parseModInfo(const vector<string>& fields, const string& modVersion) {
  if (fields.size() >= 9)
    if (auto numGames = fromStringSafe<int>(unescapeEverything(fields[3])))
      if (auto version = fromStringSafe<int>(unescapeEverything(fields[4])))
        if (auto steamId = fromStringSafe<SteamId>(unescapeEverything(fields[5])))
          if (auto upvotes = fromStringSafe<int>(unescapeEverything(fields[7])))
            if (auto downvotes = fromStringSafe<int>(unescapeEverything(fields[8])))
              if (fields[6] == modVersion)
                return ModInfo{unescapeEverything(fields[0]),
                      ModDetails{unescapeEverything(fields[1]), firstLines(unescapeEverything(fields[2]))},
                      ModVersionInfo{*steamId, *version, modVersion}, *upvotes, *downvotes, false, false, false, {}};
  return none;
}

struct SteamItemInfo {
  steam::ItemInfo info;
  optional<string> owner;
  bool subscribed;
  bool isOwner;
  bool isFriend;
};

template <typename T>
static vector<T> removeDuplicates(vector<T> input) {
  vector<T> ret;
  for (auto& elem : input)
    if (!ret.contains(elem))
      ret.push_back(std::move(elem));
  return ret;
}

static optional<vector<SteamItemInfo>> getSteamItems(const atomic<bool>& cancel, vector<string> tags,
    ProgressMeter& progress) {
#ifdef USE_STEAMWORKS
  if (!steam::Client::isAvailable())
    return none;
  // TODO: thread safety
  // TODO: filter mods by tags early
  auto& ugc = steam::UGC::instance();
  auto& user = steam::User::instance();
  auto& friends = steam::Friends::instance();

  vector<steam::ItemId> items;
  auto subscribedItems = ugc.subscribedItems();

  // TODO: Is this check necessary? Maybe we should try anyways?
  if (user.isLoggedOn()) {
    auto getForPage = [&](int page) {
      vector<steam::ItemId> ret;
      steam::FindItemInfo qinfo;
      qinfo.order = SteamFindOrder::playtime;
      qinfo.tags = tags;
      auto qid = ugc.createFindQuery(qinfo, page);
      ugc.waitForQueries({qid}, milliseconds(2000), cancel);
      if (!cancel && ugc.queryStatus(qid) == QueryStatus::completed) {
        ret = ugc.finishFindQuery(qid);
      } else {
        std::cout  << "STEAM: FindQuery failed: " << ugc.queryError(qid, "timeout (2 sec)") << std::endl;
        ugc.finishQuery(qid);
      }
      return ret;
    };
    int page = 1;
    while (1) {
      auto v = getForPage(page);
      if (cancel)
        return none;
      if (v.empty())
        break;
      else
        items.append(std::move(v));
      ++page;
    }
  }
  for (auto id : subscribedItems)
    if (!items.contains(id))
      items.emplace_back(id);
  items = removeDuplicates(items);
  if (items.empty()) {
    INFO << "STEAM: No items present";
    // TODO: inform that no mods are present (not subscribed or ...)
    return {};
  }

  auto getForPage = [&](vector<steam::ItemId> items) {
    steam::ItemDetailsInfo detailsInfo;
    detailsInfo.longDescription = true;
    detailsInfo.playtimeStatsDays = 9999;
    detailsInfo.metadata = true;
    auto qid = ugc.createDetailsQuery(detailsInfo, items);
    ugc.waitForQueries({qid}, milliseconds(3000), cancel);

    if (cancel || ugc.queryStatus(qid) != QueryStatus::completed) {
      INFO << "STEAM: DetailsQuery failed: " << ugc.queryError(qid, "timeout (3 sec)");
      ugc.finishQuery(qid);
      return vector<steam::ItemInfo>();
    }
    return ugc.finishDetailsQuery(qid);
  };
  vector<steam::ItemInfo> infos;
  for (int i = 0; i < items.size(); i += steam::UGC::maxItemsPerPage) {
    progress.setProgress(float(i) / items.size());
    auto results = getForPage(items.getSubsequence(i, steam::UGC::maxItemsPerPage));
    if (cancel)
      return none;
    if (results.empty())
      break;
    infos.append(std::move(results));
  }
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
  steam::sleepUntil(retrieveUserNames, milliseconds(1500), cancel);
  vector<SteamItemInfo> ret;
  auto friendIds = friends.ids();
  for (int i : All(infos))
    if ([&] { for (auto& tag : tags) if (!infos[i].tags.contains(tag)) return false; return true; }())
      ret.push_back(SteamItemInfo{
          infos[i],
          ownerNames[i],
          subscribedItems.contains(infos[i].id),
          infos[i].ownerId == user.id(),
          friendIds.contains(infos[i].ownerId)
      });
  return ret;
#else
  return none;
#endif
}

optional<vector<ModInfo>> FileSharing::getSteamMods(CancelFlag& cancel, ProgressMeter& progress) {
  vector<ModInfo> out;
  auto infos1 = getSteamItems(cancel.flag, {"Mod"_s, modVersion}, progress);
  if (!infos1 || cancel.flag)
    return none;
  auto& infos = *infos1;
  for (int n = 0; n < infos.size(); n++) {
    auto& info = infos[n].info;
    ModInfo mod;
    mod.details.author = infos[n].owner.value_or("unknown");
    mod.details.description = info.description;
    mod.name = info.title;
    // TODO: playtimeSessions is not exactly the same as numGames
    //mod.numGames = info.stats->playtimeSessions;
    mod.versionInfo.steamId = info.id.value;
    mod.versionInfo.version = (int) info.updateTime;
    mod.versionInfo.compatibilityTag = modVersion;
    mod.isSubscribed = infos[n].subscribed;
    mod.upvotes = info.votesUp;
    mod.downvotes = info.votesDown;
    mod.canUpload = infos[n].isOwner;
    out.emplace_back(mod);
  }
  return out;
}

optional<vector<FileSharing::SiteInfo>> FileSharing::getSteamSites(CancelFlag& cancel, ProgressMeter& progress) {
  vector<SiteInfo> out;
  auto infos1 = getSteamItems(cancel.flag, {"Dungeon"_s, toString(saveVersion)}, progress);
  if (!infos1 || cancel.flag)
    return none;
  auto& infos = *infos1;
  for (int n = 0; n < infos.size(); n++) {
    auto& info = infos[n].info;
    SiteInfo site {};
    site.version = saveVersion;
    site.fileInfo.date = info.updateTime;
    site.fileInfo.steamId = info.id.value;
    site.fileInfo.download = true;
    site.author = infos[n].owner.value_or("unknown");
    site.isFriend = infos[n].isFriend;
    site.subscribed = infos[n].subscribed;
    TextInput input(info.metadata);
    input.getArchive() >> site.fileInfo.filename >> site.gameInfo;
    out.push_back(std::move(site));
  }
  return out;
}

expected<vector<ModInfo>, string> FileSharing::getOnlineMods(CancelFlag& cancel, ProgressMeter& progress) {
  if (!options.getBoolValue(OptionId::ONLINE))
    return make_unexpected("Please enable online features in the settings in order to download mods."_s);
  if (auto steamMods = getSteamMods(cancel, progress))
    return *steamMods;
  if (auto content = downloadContent(cancel, uploadUrl + "/get_mods.txt"))
    return parseLines<ModInfo>(*content, [this](auto& e) { return parseModInfo(e, modVersion);});
  return make_unexpected("Error fetching online mods."_s);
}

optional<string> FileSharing::downloadSteamMod(CancelFlag& cancel, SteamId id_, const string& name,
    const DirectoryPath& modsDir, ProgressMeter& meter) {
#ifdef USE_STEAMWORKS
  if (!steam::Client::isAvailable())
    return "Steam client not available"_s;
  steam::ItemId id(id_);

  auto& ugc = steam::UGC::instance();
  auto& user = steam::User::instance();

  if (!ugc.isInstalled(id)) {
    if (!ugc.downloadItem(id, true))
      return string("Error while downloading mod.");

    while (!cancel.flag) {
      steam::runCallbacks();
      if (!ugc.isDownloading(id))
        break;
      if (auto info = ugc.downloadInfo(id))
        meter.setProgress(float(info->bytesDownloaded) / info->bytesTotal);
      sleep_for(milliseconds(50));
    }
    if (!ugc.isInstalled(id))
      return string("Error while downloading mod.");
  }

  auto instInfo = ugc.installInfo(id);
  if (!instInfo)
    return string("Error while retrieving installation info");
  return DirectoryPath(instInfo->folder).copyRecursively(modsDir.subdirectory(name));
#else
  return string("Steam support is not available in this build");
#endif
}

optional<string> FileSharing::downloadMod(CancelFlag& cancel, const string& modName, SteamId steamId,
    const DirectoryPath& modsDir, ProgressMeter& meter) {
  if (!!downloadSteamMod(cancel, steamId, modName, modsDir, meter)) {
    auto fileName = toString(modName) + ".zip";
    if (auto err = download(cancel, fileName, "mods", modsDir, meter))
      return err;
    return unzip(modsDir.file(fileName).getPath(), modsDir.getPath());
  } else
    return none;
}

optional<string> FileSharing::uploadMod(CancelFlag& cancel, ModInfo& modInfo, const DirectoryPath& modsDir, ProgressMeter& meter) {
#ifdef USE_STEAMWORKS
  if (!steam::Client::isAvailable())
    return "Steam client not available"_s;
  auto& ugc = steam::UGC::instance();
  //auto& user = steam::User::instance();

  steam::UpdateItemInfo info;
  if (modInfo.versionInfo.steamId != 0)
    info.id = steam::ItemId(modInfo.versionInfo.steamId);
  info.tags = {"Mod", modInfo.versionInfo.compatibilityTag};
  info.title = modInfo.name;
  auto previewFile = modsDir.subdirectory(modInfo.name).file("preview.png");
  if (previewFile.exists())
    info.previewFile = string(previewFile.absolute().getPath());
  info.folder = string(modsDir.subdirectory(modInfo.name).absolute().getPath());
  info.description = modInfo.details.description;
  info.visibility = SteamItemVisibility::public_;
  ugc.beginUpdateItem(info);

  // Item update may take some time; Should we loop indefinitely?
  optional<steam::UpdateItemResult> result;
  steam::sleepUntil([&]() {
    return (bool)(result = ugc.tryUpdateItem(meter)); },
    milliseconds(600 * 1000), cancel.flag);
  if (!result) {
    ugc.cancelUpdateItem();
    return "Uploading mod has timed out"_s;
  }
  if (result->valid()) {
    modInfo.versionInfo.steamId = *result->itemId;
    return none;
  } else {
    return *result->error;

    // Remove partially created item
    /*if (!itemInfo.id && result->itemId)
      ugc.deleteItem(*result->itemId);*/
  }

#else
  return string("Steam support is not available in this build");
#endif
}

#ifdef USE_STEAMWORKS
static string serializeInfo(const string& fileName, const OldSavedGameInfo& savedInfo) {
  TextOutput output;
  output.getArchive() << fileName << savedInfo;
  return output.getStream().str();
}
#endif

optional<string> FileSharing::uploadSiteToSteam(CancelFlag& cancel, const FilePath& path, const string& title,
    const OldSavedGameInfo& savedInfo, ProgressMeter& meter, optional<string>& url) {
#ifdef USE_STEAMWORKS
  if (!steam::Client::isAvailable())
    return "Steam client not available"_s;
  auto& ugc = steam::UGC::instance();
  //auto& user = steam::User::instance();

  steam::UpdateItemInfo info;
  info.tags = {"Dungeon", toString(saveVersion)};
  info.title = title;
  info.folder = string(path.absolute().getPath());
  info.metadata = serializeInfo(path.getFileName(), savedInfo);
  info.visibility = SteamItemVisibility::public_;
  auto screenshot = DirectoryPath::current().file(retiredScreenshotFilename);
  if (screenshot.exists())
    info.previewFile = string(screenshot.absolute().getPath());
  ugc.beginUpdateItem(info);

  // Item update may take some time; Should we loop indefinitely?
  optional<steam::UpdateItemResult> result;
  steam::sleepUntil([&]() { return (bool)(result = ugc.tryUpdateItem(meter)); }, milliseconds(600 * 1000), cancel.flag);
  if (cancel.flag)
    return none;
  if (!result) {
    ugc.cancelUpdateItem();
    return "Uploading mod has timed out"_s;
  }
  if (result->valid()) {
    url = toString(*result->itemId);
    return none;
  } else {
    return *result->error;

    // Remove partially created item
    /*if (!itemInfo.id && result->itemId)
      ugc.deleteItem(*result->itemId);*/
  }

#else
  return string("Steam support is not available in this build");
#endif
}

optional<string> FileSharing::downloadSteamSite(CancelFlag& cancel, const SaveFileInfo& file,
    const DirectoryPath& targetDir, ProgressMeter& meter) {
#ifdef USE_STEAMWORKS
  if (!steam::Client::isAvailable())
    return "Steam client not available"_s;
  steam::ItemId id(*file.steamId);

  auto& ugc = steam::UGC::instance();
  auto& user = steam::User::instance();

  if (!ugc.isInstalled(id)) {
    if (!ugc.downloadItem(id, true))
      return string("Error while downloading dungeon.");

    while (!cancel.flag) {
      steam::runCallbacks();
      if (!ugc.isDownloading(id))
        break;
      if (auto info = ugc.downloadInfo(id))
        meter.setProgress(float(info->bytesDownloaded) / info->bytesTotal);
      sleep_for(milliseconds(50));
    }
    if (!ugc.isInstalled(id))
      return string("Error while downloading dungeon.");
  }

  auto instInfo = ugc.installInfo(id);
  if (!instInfo)
    return string("Error while retrieving installation info");
  for (auto downloaded : DirectoryPath(instInfo->folder).getFiles())
    downloaded.copyTo(targetDir.file(file.filename));
  return none;
#else
  return string("Steam support is not available in this build");
#endif
}

static size_t writeToFile(void *ptr, size_t size, size_t nmemb, FILE *stream) {
  return fwrite(ptr, size, nmemb, stream);
}

optional<string> FileSharing::download(CancelFlag& cancel, const string& filename, const string& remoteDir,
    const DirectoryPath& dir, ProgressMeter& meter) {
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
      auto callback = getCallbackData(cancel, meter);
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

