#pragma once

#include "util.h"
#include "storage_id.h"

struct StorageInfo {
  StorageId SERIAL(storage);
  Collective* SERIAL(collective) = nullptr;
  SERIALIZE_ALL(storage, collective);
};
