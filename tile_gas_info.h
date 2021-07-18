#pragma once

#include "util.h"

#include "color.h"

struct TileGasInfo {
  string SERIAL(name);
  Color SERIAL(color);
  double SERIAL(decrease);
  double SERIAL(spread);
  bool SERIAL(blocksVision) = false;
  SERIALIZE_ALL(NAMED(name), NAMED(color), NAMED(decrease), NAMED(spread), OPTION(blocksVision))
};