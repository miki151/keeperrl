#pragma once

#include "furniture_factory.h"
#include "creature_factory.h"
#include "z_level_info.h"
#include "resource_counts.h"
#include "tile_paths.h"

struct ContentFactory {
  ContentFactory(NameGenerator, const GameConfig*, TilePaths);
  CreatureFactory SERIAL(creatures);
  FurnitureFactory SERIAL(furniture);
  array<vector<ZLevelInfo>, 3> SERIAL(zLevels);
  vector<ResourceDistribution> SERIAL(resources);
  TilePaths SERIAL(tilePaths);
  void merge(ContentFactory);
  SERIALIZATION_DECL(ContentFactory)
};
