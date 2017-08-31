#pragma once

#include "item_type.h"
#include "creature_factory.h"
#include "item_factory.h"
#include "furniture_factory.h"

enum class BuildingId { WOOD, MUD, BRICK, WOOD_CASTLE, DUNGEON, DUNGEON_SURFACE};

RICH_ENUM(BiomeId,
  GRASSLAND,
  FORREST,
  MOUNTAIN
);

struct StockpileInfo {
  enum Type { GOLD, MINERALS } type;
  int number;
};

enum class SettlementType {
  VILLAGE,
  SMALL_VILLAGE,
  FORREST_VILLAGE,
  FOREST,
  CASTLE,
  CASTLE2,
  COTTAGE,
  FORREST_COTTAGE,
  TOWER,
  WITCH_HOUSE,
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
  MOUNTAIN_LAKE,
};

class CollectiveBuilder;

struct SettlementInfo {
  SettlementType type;
  optional<CreatureFactory> creatures;
  int numCreatures;
  optional<pair<CreatureFactory, int>> neutralCreatures;
  optional<string> locationName;
  TribeId tribe;
  optional<string> race;
  BuildingId buildingId;
  vector<StairKey> downStairs;
  vector<StairKey> upStairs;
  vector<StockpileInfo> stockpiles;
  optional<CreatureId> guardId;
  optional<ItemType> elderLoot;
  optional<ItemFactory> shopFactory;
  CollectiveBuilder* collective;
  optional<FurnitureFactory> furniture;
  optional<FurnitureFactory> outsideFeatures;
  bool closeToPlayer;
};
