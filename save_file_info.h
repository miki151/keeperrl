#pragma once

#include "util.h"

struct SaveFileInfo {
  string SERIAL(filename);
  time_t SERIAL(date);
  bool SERIAL(download);
  SERIALIZE_ALL(filename, date, download)
};

