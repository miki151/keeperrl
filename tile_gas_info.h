#pragma once

#include "util.h"

#include "color.h"
#include "effect.h"

struct TileGasInfo {
  string SERIAL(name);
  Color SERIAL(color);
  double SERIAL(decrease);
  double SERIAL(spread);
  bool SERIAL(blocksVision) = false;
  optional<Effect> SERIAL(effect);
  SERIALIZE_ALL(NAMED(name), NAMED(color), NAMED(decrease), NAMED(spread), OPTION(blocksVision), NAMED(effect))
};