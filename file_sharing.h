#ifndef _FILE_SHARING_H
#define _FILE_SHARING_H

#include "util.h"

class ProgressMeter;

class FileSharing {
  public:
  FileSharing(const string& uploadUrl);
  optional<string> upload(const string& path, ProgressMeter&);
  function<void()> getCancelFun();
  
  private:
  string uploadUrl;
};

#endif
