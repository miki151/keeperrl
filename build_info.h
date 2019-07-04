#pragma once

#include "util.h"
#include "my_containers.h"
#include "cost_info.h"
#include "enum_variant.h"
#include "zones.h"
#include "avatar_info.h"
#include "view_id.h"
#include "furniture_type.h"
#include "tech_id.h"

struct BuildInfo {
  struct Furniture {
    vector<FurnitureType> SERIAL(types);
    CostInfo SERIAL(cost);
    bool SERIAL(noCredit) = false;
    optional<int> SERIAL(limit);
    SERIALIZE_ALL(NAMED(types), OPTION(cost), OPTION(noCredit), NAMED(limit))
  };

  struct Trap {
    FurnitureType SERIAL(type);
    ViewId SERIAL(viewId);
    SERIALIZE_ALL(type, viewId)
  };

  using DungeonLevel = int;
  MAKE_VARIANT(Requirement, TechId, DungeonLevel);

  static string getRequirementText(Requirement);
  static bool meetsRequirement(WConstCollective, Requirement);
  bool canSelectRectangle() const;

  using DestroyLayers = vector<FurnitureLayer>;
  using ImmediateDig = EmptyStruct<struct ImmediateDigTag>;
  using Dig = EmptyStruct<struct DigTag>;
  using ClaimTile = EmptyStruct<struct ClaimTileTag>;
  using Dispatch = EmptyStruct<struct DispatchTag>;
  using ForbidZone = EmptyStruct<struct ForbidZoneTag>;
  using Zone = ZoneId;
  using PlaceMinion = EmptyStruct<struct PlaceMinionTag>;
  using PlaceItem = EmptyStruct<struct PlaceItemTag>;
  MAKE_VARIANT(BuildType, Furniture, Trap, Zone, DestroyLayers, Dig, ClaimTile, Dispatch, ForbidZone, PlaceMinion, ImmediateDig, PlaceItem);
  BuildType SERIAL(type);
  string SERIAL(name);
  string SERIAL(groupName);
  string SERIAL(help);
  char SERIAL(hotkey) = 0;
  vector<Requirement> SERIAL(requirements);
  bool SERIAL(hotkeyOpensGroup) = false;
  optional<TutorialHighlight> SERIAL(tutorialHighlight);
  SERIALIZE_ALL(NAMED(type), NAMED(name), OPTION(groupName), OPTION(help), OPTION(hotkey), OPTION(requirements), OPTION(hotkeyOpensGroup), NAMED(tutorialHighlight))
};
