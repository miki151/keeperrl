#pragma once

#include "util.h"
#include "tile_info.h"

class GameConfig;

struct TilePaths {
  TilePaths(const GameConfig*);
  void merge(TilePaths);
  SERIALIZATION_CONSTRUCTOR(TilePaths)
  vector<TileInfo> SERIAL(definitions);
  vector<string> SERIAL(modDirs);
  SERIALIZE_ALL(definitions, modDirs)
};
