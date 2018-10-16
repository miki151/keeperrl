#pragma once

#include "util.h"
#include "my_containers.h"
#include "cost_info.h"
#include "enum_variant.h"
#include "zones.h"
#include "avatar_info.h"

struct BuildInfo {
  static const vector<BuildInfo>& get(AvatarVariant);

  struct FurnitureInfo {
    FurnitureInfo(FurnitureType type, CostInfo cost = CostInfo::noCost(), bool noCredit = false, optional<int> maxNumber = none);
    FurnitureInfo(vector<FurnitureType> t, CostInfo c = CostInfo::noCost(), bool n = false, optional<int> m = none);
    FurnitureInfo() {}
    vector<FurnitureType> types;
    CostInfo cost;
    bool noCredit;
    optional<int> maxNumber;
  } furnitureInfo;

  struct TrapInfo {
    TrapType type;
    ViewId viewId;
  } trapInfo;

  enum BuildType {
    DIG,
    FURNITURE,
    TRAP,
    DESTROY,
    ZONE,
    DISPATCH,
    CLAIM_TILE,
    FORBID_ZONE
  } buildType;

  enum class RequirementId {
    TECHNOLOGY,
    VILLAGE_CONQUERED,
  };
  typedef EnumVariant<RequirementId, TYPES(TechId),
      ASSIGN(TechId, RequirementId::TECHNOLOGY)> Requirement;

  static string getRequirementText(Requirement);
  static bool meetsRequirement(WConstCollective, Requirement);

  struct RoomInfo {
    string name;
    string description;
    vector<Requirement> requirements;
  };
  static vector<RoomInfo> getRoomInfo();

  string name;
  vector<Requirement> requirements;
  string help;
  char hotkey;
  string groupName;
  bool hotkeyOpensGroup = false;
  ZoneId zone;
  ViewId viewId;
  optional<TutorialHighlight> tutorialHighlight;
  vector<FurnitureLayer> destroyLayers;

  BuildInfo& setTutorialHighlight(TutorialHighlight t);

  BuildInfo(FurnitureInfo info, const string& n, vector<Requirement> req = {}, const string& h = "", char key = 0,
      string group = "", bool hotkeyOpens = false);

  BuildInfo(TrapInfo info, const string& n, vector<Requirement> req = {}, const string& h = "", char key = 0,
      string group = "");

  BuildInfo(BuildType type, const string& n, const string& h = "", char key = 0, string group = "");
  BuildInfo(const vector<FurnitureLayer>& layers, const string& n, const string& h = "", char key = 0, string group = "");

  BuildInfo(ZoneId zone, ViewId view, const string& n, const string& h = "", char key = 0, string group = "",
            bool hotkeyOpens = false);

};
