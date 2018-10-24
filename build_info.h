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
    bool SERIAL(noCredit) = false;
    optional<int> SERIAL(limit);
    SERIALIZE_ALL(NAMED(types), NAMED(cost), NAMED(noCredit), NAMED(limit))
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
  bool canSelectRectangle() const;

  using DestroyLayers = vector<FurnitureLayer>;
  using Dig = EmptyStruct<struct DigTag>;
  using ClaimTile = EmptyStruct<struct ClaimTileTag>;
  using Dispatch = EmptyStruct<struct DispatchTag>;
  using ForbidZone = EmptyStruct<struct ForbidZoneTag>;
  using Zone = ZoneId;
  MAKE_VARIANT(BuildType, Furniture, Trap, Zone, DestroyLayers, Dig, ClaimTile, Dispatch, ForbidZone);
  BuildType SERIAL(type);
  string SERIAL(name);
  string SERIAL(groupName);
  string SERIAL(help);
  char SERIAL(hotkey) = 0;
  vector<Requirement> SERIAL(requirements);
  bool SERIAL(hotkeyOpensGroup) = false;
  optional<TutorialHighlight> SERIAL(tutorialHighlight);
  SERIALIZE_ALL(NAMED(type), NAMED(name), NAMED(groupName), NAMED(help), NAMED(hotkey), NAMED(requirements), NAMED(hotkeyOpensGroup), NAMED(tutorialHighlight))
};
