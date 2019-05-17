#pragma once

#include "technology.h"
#include "workshop_array.h"
#include "immigrant_info.h"
#include "build_info.h"

class GameConfig;

struct InitialContentFactory {
  InitialContentFactory(const GameConfig*);
  Technology technology;
  vector<pair<string, WorkshopArray>> workshopGroups;
  map<string, vector<ImmigrantInfo>> immigrantsData;
  vector<pair<string, vector<BuildInfo>>> buildInfo;
};
