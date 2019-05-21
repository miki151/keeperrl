#pragma once

#include "util.h"
#include "tile_info.h"

class GameConfig;

struct TilePaths {
  TilePaths(const GameConfig*);
  void merge(TilePaths);
  SERIALIZATION_CONSTRUCTOR(TilePaths)
  vector<TileInfo> SERIAL(definitions);
  string SERIAL(mainMod);
  vector<string> SERIAL(mergedMods);
  SERIALIZE_ALL(definitions, mainMod, mergedMods)
};
