#pragma once

#include "util.h"
#include "item_type.h"
#include "creature_factory.h"
#include "item_factory.h"
#include "furniture_list.h"
#include "inhabitants_info.h"
#include "building_id.h"
#include "building_info.h"
#include "name_generator_id.h"

struct StockpileInfo {
  ItemListId SERIAL(items);
  int SERIAL(count);
  optional<FurnitureType> SERIAL(furniture);
  SERIALIZE_ALL(NAMED(items), NAMED(count), NAMED(furniture))
};

RICH_ENUM(
  SettlementType,
  VILLAGE,
  SMALL_VILLAGE,
  FORREST_VILLAGE,
  FOREST,
  CASTLE,
  CASTLE2,
  COTTAGE,
  FORREST_COTTAGE,
  TOWER,
  TEMPLE,
  MINETOWN,
  ANT_NEST,
  SMALL_MINETOWN,
  VAULT,
  CAVE,
  SPIDER_CAVE,
  ISLAND_VAULT,
  ISLAND_VAULT_DOOR,
  CEMETERY,
  SWAMP,
  MOUNTAIN_LAKE
);

class CollectiveBuilder;

struct SettlementInfo {
  SettlementType SERIAL(type);
  InhabitantsInfo SERIAL(inhabitants);
  optional<InhabitantsInfo> SERIAL(corpses);
  optional<string> SERIAL(locationName);
  optional<NameGeneratorId> SERIAL(locationNameGen);
  TribeId SERIAL(tribe);
  optional<string> SERIAL(race);
  BuildingId SERIAL(buildingId);
  BuildingInfo buildingInfo;
  vector<StairKey> downStairs;
  vector<StairKey> upStairs;
  vector<StockpileInfo> SERIAL(stockpiles);
  optional<ItemType> SERIAL(lootItem);
  optional<ItemListId> SERIAL(shopItems);
  bool SERIAL(shopkeeperDead) = false;
  CollectiveBuilder* collective;
  vector<FurnitureListId> SERIAL(furniture);
  optional<FurnitureListId> SERIAL(outsideFeatures);
  bool SERIAL(closeToPlayer) = false;
  bool SERIAL(dontConnectCave) = false;
  bool SERIAL(dontBuildRoad) = false;
  bool SERIAL(anyPlayerDistance) = false;
  int SERIAL(surroundWithResources) = 0;
  optional<FurnitureType> SERIAL(extraResources);
  optional<int> SERIAL(cropsDistance);
  SERIALIZE_ALL(NAMED(type), OPTION(inhabitants), NAMED(corpses), NAMED(locationName), NAMED(locationNameGen), NAMED(tribe), NAMED(race), NAMED(buildingId), OPTION(stockpiles), NAMED(lootItem), NAMED(shopItems), OPTION(shopkeeperDead), OPTION(furniture), NAMED(outsideFeatures), OPTION(closeToPlayer), OPTION(dontConnectCave), OPTION(dontBuildRoad), OPTION(anyPlayerDistance), OPTION(surroundWithResources), NAMED(extraResources), NAMED(cropsDistance))
};
