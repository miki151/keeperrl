#ifndef _SAVE_FILE_INFO_H
#define _SAVE_FILE_INFO_H

#include "util.h"

struct SaveFileInfo {
  string SERIAL(filename);
  time_t SERIAL(date);
  bool SERIAL(download);
  SERIALIZE_ALL(filename, date, download);
};
#endif

