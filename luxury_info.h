#pragma once

#include "util.h"

struct LuxuryInfo {
  double SERIAL(luxury) = 0;
  SERIALIZE_ALL(luxury);
};
