#ifndef _FILE_SHARING_H
#define _FILE_SHARING_H

#include "util.h"

class ProgressMeter;

class FileSharing {
  public:
  FileSharing(const string& uploadUrl);

  optional<string> upload(const string& path, ProgressMeter&);
  struct GameInfo {
    string displayName;
    string filename;
    time_t time;
  };
  vector<GameInfo> listGames(int version);
  optional<string> download(const string& filename, const string& dir, ProgressMeter&);

  function<void()> getCancelFun();
  
  private:
  string uploadUrl;
};

#endif
