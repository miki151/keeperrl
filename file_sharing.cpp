#include "stdafx.h"
#include "file_sharing.h"
#include "progress_meter.h"
#include "save_file_info.h"
#include "parse_game.h"
#include "options.h"
#include "text_serialization.h"

#include <curl/curl.h>

FileSharing::FileSharing(const string& url, Options& o, long long id) : uploadUrl(url), options(o),
    uploadLoop(bindMethod(&FileSharing::uploadingLoop, this)), installId(id) {
  curl_global_init(CURL_GLOBAL_ALL);
}

FileSharing::~FileSharing() {
  // pushing null acts as a signal for the upload loop to finish
  uploadQueue.push(nullptr);
}

static atomic<bool> cancel(false);
typedef function<void(double)> ProgressCallback;

int progressFunction(void* ptr, double totalDown, double nowDown, double totalUp, double nowUp) {
  if (ptr) {
    ProgressCallback& progressCallback = *((ProgressCallback*)ptr);
    if (totalUp > 0)
      progressCallback(nowUp / totalUp);
    if (totalDown > 0)
      progressCallback(nowDown / totalDown);
  }
  if (cancel) {
    cancel = false;
    return 1;
  } else
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
  return s;
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

static optional<string> curlUpload(const char* path, const char* url, void* progressCallback, int timeout) {
  struct curl_httppost *formpost=NULL;
  struct curl_httppost *lastptr=NULL;

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
    // Internal CURL progressmeter must be disabled if we provide our own callback
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, false);
    if (progressCallback)
      curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, progressCallback);
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

optional<string> FileSharing::uploadSite(const FilePath& path, ProgressMeter& meter) {
  if (!options.getBoolValue(OptionId::ONLINE))
    return none;
  static ProgressCallback callback = [&] (double p) { meter.setProgress(p);};
  return curlUpload(path.getPath(), (uploadUrl + "/upload_site.php").c_str(), &callback, 0);
}

void FileSharing::uploadHighscores(const FilePath& path) {
  if (options.getBoolValue(OptionId::ONLINE))
    uploadQueue.push([this, path] {
      curlUpload(path.getPath(), (uploadUrl + "/upload_scores.php").c_str(), nullptr, 5);
    });
}

void FileSharing::uploadingLoop() {
  function<void()> uploadFun = uploadQueue.pop();
  if (uploadFun)
    uploadFun();
  else
    uploadLoop.setDone();
}

void FileSharing::uploadGameEvent(const GameEvent& data1) {
  GameEvent data(data1);
  data.emplace("installId", toString(installId));
  if (options.getBoolValue(OptionId::ONLINE) && options.getBoolValue(OptionId::GAME_EVENTS))
    uploadGameEventImpl(data, 5);
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
      curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ret);
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

static optional<string> downloadContent(const string& url) {
  if (CURL* curl = curl_easy_init()) {
    curl_easy_setopt(curl, CURLOPT_URL, escapeSpaces(url).c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, dataFun);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
    // Internal CURL progressmeter must be disabled if we provide our own callback
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, false);
    // Install the callback function
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

void FileSharing::uploadBoardMessage(const string& gameId, int hash, const string& author, const string& text) {
  uploadGameEvent({
      { "gameId", gameId },
      { "eventType", "boardMessage"},
      { "boardId", toString(hash) },
      { "author", author },
      { "text", text }
  });
}

void FileSharing::cancel() {
  ::cancel = true;
}

static size_t writeToFile(void *ptr, size_t size, size_t nmemb, FILE *stream) {
  return fwrite(ptr, size, nmemb, stream);
}

optional<string> FileSharing::download(const string& filename, const DirectoryPath& dir, ProgressMeter& meter) {
  if (!options.getBoolValue(OptionId::ONLINE))
    return string("Downloading not enabled!");
  //progressFun = [&] (double p) { meter.setProgress(p);};
  if (CURL *curl = curl_easy_init()) {
    auto path = dir.file(filename);
    INFO << "Downloading to " << path;
    if (FILE* fp = fopen(path.getPath(), "wb")) {
      curl_easy_setopt(curl, CURLOPT_URL, escapeSpaces(uploadUrl + "/uploads/" + filename).c_str());
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToFile);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
      // Internal CURL progressmeter must be disabled if we provide our own callback
      curl_easy_setopt(curl, CURLOPT_NOPROGRESS, false);
      // Install the callback function
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

