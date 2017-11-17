#pragma once

#include "util.h"

struct CollectiveName {
  string SERIAL(shortened);
  string SERIAL(full);
  string SERIAL(race);
  SERIALIZE_ALL(shortened, full, race)
};

