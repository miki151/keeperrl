#include "stdafx.h"
#include "file_sharing.h"
#include "progress_meter.h"

#include <curl/curl.h>

FileSharing::FileSharing(const string& url) : uploadUrl(url) {
}

static function<void(double)> progressFun;

static atomic<bool> cancel(false);

int progressFunction(void* ptr, double totalDown, double nowDown, double totalUp, double nowUp) {
  if (totalUp > 0)
    progressFun(nowUp / totalUp);
  if (totalDown > 0)
    progressFun(nowDown / totalDown);
  if (cancel) {
    cancel = false;
    return 1;
  } else
    return 0;
}

size_t dataFun(void *buffer, size_t size, size_t nmemb, void *userp) {
  string& buf = *((string*) userp);
  buf += (char*) buffer;
  return size * nmemb;
}

static optional<string> curlUpload(const char* path, const char* url) {

  struct curl_httppost *formpost=NULL;
  struct curl_httppost *lastptr=NULL;

  curl_global_init(CURL_GLOBAL_ALL);

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

  if(CURL* curl = curl_easy_init()) {
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, dataFun);
    string ret;
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ret);
    /* what URL that receives this POST */ 
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
    // Internal CURL progressmeter must be disabled if we provide our own callback
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, false);
    // Install the callback function
    curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progressFunction);
    CURLcode res = curl_easy_perform(curl);
    if(res != CURLE_OK)
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

optional<string> FileSharing::upload(const string& path, ProgressMeter& meter) {
  progressFun = [&] (double p) { meter.setProgress(p);};
  return curlUpload(path.c_str(), (uploadUrl + "/upload.php").c_str());
}

static vector<FileSharing::GameInfo> parseGames(const string& s) {
  std::stringstream iss(s);
  vector<FileSharing::GameInfo> ret;
  while (!!iss) {
    char buf[100];
    iss.getline(buf, 100);
    if (!iss)
      break;
    Debug() << "Parsing " << string(buf);
    vector<string> fields = split(buf, {','});
    CHECK(fields.size() >= 3);
    Debug() << "Parsed " << fields;
    ret.push_back({fields[0], fields[1], fromString<int>(fields[2])});
  }
  return ret;
}

vector<FileSharing::GameInfo> FileSharing::listGames(int version) {
  if(CURL* curl = curl_easy_init()) {
    curl_easy_setopt(curl, CURLOPT_URL, (uploadUrl + "/get_games.php?version=" + toString(version)).c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, dataFun);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5);
    string ret;
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ret);
    curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    return parseGames(ret);
  }
  return {};
}

function<void()> FileSharing::getCancelFun() {
  return [] { cancel = true;};
}

static size_t writeToFile(void *ptr, size_t size, size_t nmemb, FILE *stream) {
  return fwrite(ptr, size, nmemb, stream);
}

optional<string> FileSharing::download(const string& filename, const string& dir, ProgressMeter& meter) {
  progressFun = [&] (double p) { meter.setProgress(p);};
  if (CURL *curl = curl_easy_init()) {
    string path = dir + "/" + filename;
    Debug() << "Downloading to " << path;
    if (FILE *fp = fopen(path.c_str(), "wb")) {
      curl_easy_setopt(curl, CURLOPT_URL, (uploadUrl + "/uploads/" + filename).c_str());
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToFile);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
      // Internal CURL progressmeter must be disabled if we provide our own callback
      curl_easy_setopt(curl, CURLOPT_NOPROGRESS, false);
      // Install the callback function
      curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progressFunction);
      CURLcode res = curl_easy_perform(curl);
      string ret;
      if(res != CURLE_OK)
        ret = string("Upload failed: ") + curl_easy_strerror(res);
      curl_easy_cleanup(curl);
      fclose(fp);
      if (!ret.empty()) {
        remove(path.c_str());
        return ret;
      } else
        return none;
    } else
      return string("Failed to open file: " + path);
  } else
    return string("Failed to initialize libcurl");
}
