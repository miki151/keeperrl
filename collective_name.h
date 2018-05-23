#pragma once

#include "util.h"

struct CollectiveName {
  string SERIAL(shortened);
  string SERIAL(full);
  string SERIAL(race);
  ViewId SERIAL(viewId);
  SERIALIZE_ALL(shortened, full, race, viewId)
};

