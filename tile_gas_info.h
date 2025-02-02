#pragma once

#include "util.h"

#include "color.h"
#include "effect.h"
#include "t_string.h"

struct TileGasInfo {
  TString SERIAL(name);
  Color SERIAL(color);
  double SERIAL(decrease);
  double SERIAL(spread);
  bool SERIAL(blocksVision) = false;
  optional<Effect> SERIAL(effect);
  SERIALIZE_ALL(NAMED(name), NAMED(color), NAMED(decrease), NAMED(spread), OPTION(blocksVision), NAMED(effect))
};