#include "stdafx.h"
#include "file_sharing.h"
#include "progress_meter.h"

#include <curl/curl.h>

FileSharing::FileSharing(const string& url) : uploadUrl(url) {
}

static function<void(double)> progressFun;

static bool cancel = false;

int progressFunction(void* ptr, double totalDown, double noDown, double totalUp, double nowUp) {
  if (totalUp > 0)
    progressFun(nowUp / totalUp);
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
  CURL *curl;

  struct curl_httppost *formpost=NULL;
  struct curl_httppost *lastptr=NULL;
  struct curl_slist *headerlist=NULL;
  static const char buf[] = "Expect:";

  curl_global_init(CURL_GLOBAL_ALL);

  /* Fill in the file upload field */ 
  curl_formadd(&formpost,
      &lastptr,
      CURLFORM_COPYNAME, "fileToUpload",
      CURLFORM_FILE, path,
      CURLFORM_END);

  /* Fill in the submit field too, even if this is rarely needed */ 
  curl_formadd(&formpost,
      &lastptr,
      CURLFORM_COPYNAME, "submit",
      CURLFORM_COPYCONTENTS, "send",
      CURLFORM_END);

  curl = curl_easy_init();
  /* initalize custom header list (stating that Expect: 100-continue is not
     wanted */ 
  headerlist = curl_slist_append(headerlist, buf);
  string ret;
  if(curl) {
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, dataFun);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ret);
    /* what URL that receives this POST */ 
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
    // Internal CURL progressmeter must be disabled if we provide our own callback
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, false);
    // Install the callback function
    curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progressFunction);
    /* Perform the request, res will get the return code */ 
    CURLcode res = curl_easy_perform(curl);
    /* Check for errors */ 
    if(res != CURLE_OK && res != CURLE_ABORTED_BY_CALLBACK)
      return string("Upload failed: ") + curl_easy_strerror(res);

    /* always cleanup */ 
    curl_easy_cleanup(curl);

    /* then cleanup the formpost chain */ 
    curl_formfree(formpost);
    /* free slist */ 
    curl_slist_free_all (headerlist);
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

function<void()> FileSharing::getCancelFun() {
  return [] { cancel = true;};
}

