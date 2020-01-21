#pragma once

#include "util.h"
#include "tile_info.h"

class GameConfig;

struct TilePaths {
  TilePaths(vector<TileInfo>, vector<string> mainMods);
  void merge(TilePaths);
  SERIALIZATION_CONSTRUCTOR(TilePaths)
  vector<TileInfo> SERIAL(definitions);
  vector<string> SERIAL(mainMods);
  vector<string> SERIAL(mergedMods);
  SERIALIZE_ALL(definitions, mainMods, mergedMods)
};
