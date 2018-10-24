#pragma once

#include "util.h"
#include "my_containers.h"
#include "cost_info.h"
#include "enum_variant.h"
#include "zones.h"
#include "avatar_info.h"

struct BuildInfo {
  struct Furniture {
    vector<FurnitureType> SERIAL(types);
    CostInfo SERIAL(cost);
    bool SERIAL(noCredit);
    optional<int> SERIAL(maxNumber);
    SERIALIZE_ALL(types, cost, noCredit, maxNumber)
  };

  struct Trap {
    TrapType SERIAL(type);
    ViewId SERIAL(viewId);
    SERIALIZE_ALL(type, viewId)
  };

  using DungeonLevel = int;
  MAKE_VARIANT(Requirement, TechId, DungeonLevel);

  static string getRequirementText(Requirement);
  static bool meetsRequirement(WConstCollective, Requirement);

  using DestroyLayers = vector<FurnitureLayer>;
  using Dig = EmptyStruct<struct DigTag>;
  using ClaimTile = EmptyStruct<struct ClaimTileTag>;
  using Dispatch = EmptyStruct<struct DispatchTag>;
  using ForbidZone = EmptyStruct<struct ForbidZoneTag>;
  using Zone = ZoneId;
  MAKE_VARIANT(BuildType, Furniture, Trap, Zone, DestroyLayers, Dig, ClaimTile, Dispatch, ForbidZone);
  BuildType SERIAL(type);
  char SERIAL(hotkey);
  string SERIAL(name);
  vector<Requirement> SERIAL(requirements);
  string SERIAL(groupName);
  bool SERIAL(hotkeyOpensGroup);
  bool SERIAL(canSelectRectangle);
  optional<TutorialHighlight> SERIAL(tutorialHighlight);
  string SERIAL(help);
  SERIALIZE_ALL(type, hotkey, name, requirements, groupName, hotkeyOpensGroup, canSelectRectangle, tutorialHighlight, help)
};
