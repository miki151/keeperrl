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
#include "pretty_archive.h"
#include "furniture_layer.h"

namespace BuildInfoTypes {
  struct Furniture {
    vector<FurnitureType> SERIAL(types);
    CostInfo SERIAL(cost);
    bool SERIAL(noCredit) = false;
    optional<int> SERIAL(limit);
    SERIALIZE_ALL(NAMED(types), OPTION(cost), OPTION(noCredit), NAMED(limit))
  };
  using DestroyLayers = vector<FurnitureLayer>;
  using ImmediateDig = EmptyStruct<struct ImmediateDigTag>;
  using Dig = EmptyStruct<struct DigTag>;
  using CutTree = EmptyStruct<struct CutTreeTag>;
  using ClaimTile = EmptyStruct<struct ClaimTileTag>;
  using Dispatch = EmptyStruct<struct DispatchTag>;
  using ForbidZone = EmptyStruct<struct ForbidZoneTag>;
  using Zone = ZoneId;
  using PlaceMinion = EmptyStruct<struct PlaceMinionTag>;
  using PlaceItem = EmptyStruct<struct PlaceItemTag>;
  struct BuildType;
  using Chain = vector<BuildType>;
  #define VARIANT_TYPES_LIST\
    X(Furniture, 0)\
    X(ClaimTile, 1)\
    X(Zone, 2)\
    X(DestroyLayers, 3)\
    X(Dig, 4)\
    X(CutTree, 5)\
    X(Dispatch, 6)\
    X(ForbidZone, 7)\
    X(PlaceMinion, 8)\
    X(ImmediateDig, 9)\
    X(PlaceItem, 10)\
    X(Chain, 12)

  #define VARIANT_NAME BuildType

  #include "gen_variant.h"
  #include "gen_variant_serialize.h"
  #define DEFAULT_ELEM "Chain"
  inline
  #include "gen_variant_serialize_pretty.h"
  #undef DEFAULT_ELEM
  #undef VARIANT_TYPES_LIST
  #undef VARIANT_NAME

}

struct BuildInfo {
  using DungeonLevel = int;
  MAKE_VARIANT(Requirement, TechId, DungeonLevel);

  static string getRequirementText(Requirement);
  static bool meetsRequirement(const Collective*, Requirement);
  bool canSelectRectangle() const;

  BuildInfoTypes::BuildType SERIAL(type);
  string SERIAL(name);
  string SERIAL(groupName);
  string SERIAL(help);
  char SERIAL(hotkey) = 0;
  vector<Requirement> SERIAL(requirements);
  bool SERIAL(hotkeyOpensGroup) = false;
  optional<TutorialHighlight> SERIAL(tutorialHighlight);
  SERIALIZE_ALL(NAMED(type), NAMED(name), OPTION(groupName), OPTION(help), OPTION(hotkey), OPTION(requirements), OPTION(hotkeyOpensGroup), NAMED(tutorialHighlight))
};

static_assert(std::is_nothrow_move_constructible<BuildInfo>::value, "T should be noexcept MoveConstructible");
