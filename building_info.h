#pragma once

#include "furniture_type.h"


RICH_ENUM(
  WaterType,
  LAVA,
  WATER,
  ICE
);

struct BuildingInfo {
  struct DoorInfo {
    FurnitureType SERIAL(type);
    double SERIAL(prob) = 1;
    SERIALIZE_ALL(NAMED(type), OPTION(prob))
  };
  FurnitureType SERIAL(wall);
  optional<FurnitureType> SERIAL(floorInside);
  optional<FurnitureType> SERIAL(floorOutside);
  optional<DoorInfo> SERIAL(door);
  optional<FurnitureType> SERIAL(prettyFloor);
  optional<DoorInfo> SERIAL(gate);
  optional<FurnitureType> SERIAL(upStairs);
  optional<FurnitureType> SERIAL(downStairs);
  FurnitureType SERIAL(bridge) = FurnitureType("BRIDGE");
  vector<WaterType> SERIAL(water) = {WaterType::LAVA, WaterType::WATER};
  SERIALIZE_ALL(NAMED(wall), NAMED(floorInside), NAMED(floorOutside), NAMED(door), NAMED(prettyFloor), NAMED(gate), NAMED(upStairs), NAMED(downStairs), OPTION(water), OPTION(bridge))
};
