#pragma once

#include "furniture_factory.h"
#include "creature_factory.h"
#include "z_level_info.h"
#include "resource_counts.h"
#include "tile_paths.h"
#include "enemy_info.h"
#include "technology.h"
#include "workshop_array.h"
#include "campaign_builder.h"
#include "external_enemies.h"
#include "item_attributes.h"
#include "custom_item_id.h"

class KeyVerifier;
class BuildInfo;

class ContentFactory {
  public:
  optional<string> readData(const GameConfig*);
  FurnitureFactory SERIAL(furniture);
  array<vector<ZLevelInfo>, 3> SERIAL(zLevels);
  vector<ResourceDistribution> SERIAL(resources);
  TilePaths SERIAL(tilePaths);
  map<EnemyId, EnemyInfo> SERIAL(enemies);
  vector<ExternalEnemy> SERIAL(externalEnemies);
  ItemFactory SERIAL(itemFactory);
  Technology SERIAL(technology);
  vector<pair<string, WorkshopArray>> SERIAL(workshopGroups);
  map<string, vector<ImmigrantInfo>> SERIAL(immigrantsData);
  vector<pair<string, vector<BuildInfo>>> SERIAL(buildInfo);
  VillainsTuple SERIAL(villains);
  GameIntros SERIAL(gameIntros);
  PlayerCreaturesInfo SERIAL(playerCreatures);
  map<CustomItemId, ItemAttributes> SERIAL(items);
  map<BuildingId, BuildingInfo> SERIAL(buildingInfo);
  void merge(ContentFactory);

  CreatureFactory& getCreatures();
  const CreatureFactory& getCreatures() const;

  ContentFactory();
  ~ContentFactory();
  ContentFactory(ContentFactory&&);

  template <class Archive>
  void serialize(Archive& ar, const unsigned int);

  private:
  CreatureFactory SERIAL(creatures);
  optional<string> readCreatureFactory(const GameConfig*, KeyVerifier*);
  optional<string> readFurnitureFactory(const GameConfig*, KeyVerifier*);
  optional<string> readVillainsTuple(const GameConfig*, KeyVerifier*);
  optional<string> readPlayerCreatures(const GameConfig*, KeyVerifier*);
  optional<string> readItems(const GameConfig*, KeyVerifier*);
  optional<string> readBuildingInfo(const GameConfig*, KeyVerifier*);
};
