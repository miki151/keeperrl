#pragma once

#include "util.h"

struct StorageInfo {
  StorageId SERIAL(storage);
  WCollective SERIAL(collective) = nullptr;
  SERIALIZE_ALL(storage, collective);
};
