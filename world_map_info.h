#pragma once

#include "util.h"
#include "random_layout_id.h"


struct WorldMapInfo {
  string SERIAL(name);
  RandomLayoutId SERIAL(layout);
  SERIALIZE_ALL(name, layout)
};