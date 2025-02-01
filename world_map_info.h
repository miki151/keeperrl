#pragma once

#include "util.h"
#include "random_layout_id.h"
#include "t_string.h"

struct WorldMapInfo {
  TString SERIAL(name);
  RandomLayoutId SERIAL(layout);
  SERIALIZE_ALL(name, layout)
};