#pragma once

#include "furniture_factory.h"
#include "creature_factory.h"
#include "z_level_info.h"
#include "resource_counts.h"

struct ContentFactory {
  ContentFactory(NameGenerator, const GameConfig*);
  CreatureFactory SERIAL(creatures);
  FurnitureFactory SERIAL(furniture);
  array<vector<ZLevelInfo>, 3> SERIAL(zLevels);
  vector<ResourceDistribution> SERIAL(resources);
  void merge(ContentFactory);
  SERIALIZATION_DECL(ContentFactory)
};
