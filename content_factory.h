#pragma once

#include "furniture_factory.h"
#include "creature_factory.h"
#include "z_level_info.h"
#include "resource_counts.h"
#include "tile_paths.h"
#include "enemy_info.h"
#include "technology.h"
#include "workshop_array.h"
#include "immigrant_info.h"
#include "build_info.h"
#include "campaign_builder.h"

class ContentFactory {
  public:
  optional<string> readData(NameGenerator, const GameConfig*);
  CreatureFactory SERIAL(creatures);
  FurnitureFactory SERIAL(furniture);
  array<vector<ZLevelInfo>, 3> SERIAL(zLevels);
  vector<ResourceDistribution> SERIAL(resources);
  TilePaths SERIAL(tilePaths);
  map<EnemyId, EnemyInfo> SERIAL(enemies);
  ItemFactory SERIAL(itemFactory);
  Technology SERIAL(technology);
  vector<pair<string, WorkshopArray>> SERIAL(workshopGroups);
  map<string, vector<ImmigrantInfo>> SERIAL(immigrantsData);
  vector<pair<string, vector<BuildInfo>>> SERIAL(buildInfo);
  VillainsTuple SERIAL(villains);
  GameIntros SERIAL(gameIntros);
  PlayerCreaturesInfo SERIAL(playerCreatures);
  void merge(ContentFactory);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int);

  private:
  optional<string> readCreatureFactory(NameGenerator, const GameConfig*);
  optional<string> readFurnitureFactory(const GameConfig*);
  optional<string> readVillainsTuple(const GameConfig*);
  optional<string> readPlayerCreatures(const GameConfig*);
};
