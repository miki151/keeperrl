#pragma once

#include "util.h"

struct SaveFileInfo {
  string SERIAL(filename);
  time_t date;
  bool SERIAL(download);
  optional<SteamId> SERIAL(steamId);
  template <class Archive>
  void serialize(Archive& ar, const unsigned int) {
#ifdef WINDOWS
    int SERIAL(dateTmp) = 0;
#else
    time_t dateTmp = 0;
#endif
    ar(filename, dateTmp, download, steamId);
  }
  string getGameId() const {
    return filename.substr(0, filename.size() - 4);
  }
};

