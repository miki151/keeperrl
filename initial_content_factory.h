#pragma once

#include "technology.h"
#include "workshop_array.h"
#include "immigrant_info.h"
#include "build_info.h"
#include "campaign_builder.h"

class GameConfig;

struct InitialContentFactory {
  InitialContentFactory(const GameConfig*);
  Technology technology;
  vector<pair<string, WorkshopArray>> workshopGroups;
  map<string, vector<ImmigrantInfo>> immigrantsData;
  vector<pair<string, vector<BuildInfo>>> buildInfo;
  VillainsTuple villains;
  GameIntros gameIntros;

  private:
  optional<string> readVillainsTuple(const GameConfig*);
};
