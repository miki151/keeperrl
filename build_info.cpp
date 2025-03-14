#include "stdafx.h"
#include "build_info.h"
#include "collective.h"
#include "technology.h"
#include "furniture.h"
#include "view_index.h"
#include "zones.h"

bool BuildInfo::meetsRequirement(const Collective* col, Requirement req) {
  return req.visit(
      [&](TechId techId) { return col->getTechnology().researched.count(techId);},
      [&](BuildInfo::DungeonLevel level) { return col->getDungeonLevel().level + 1>= level; }
  );
}

TString BuildInfo::getRequirementText(Requirement req, const ContentFactory* factory) {
  return req.visit(
      [&](TechId techId) { return TSentence("MISSING_TECHNOLOGY", factory->technology.getName(techId));},
      [&](BuildInfo::DungeonLevel level) { return TSentence("DUNGEON_LEVEL_REQUIREMENT", TString(level)); }
  );
}

bool BuildInfo::canSelectRectangle() const {
  return type.visit<bool>(
        [](const auto&) { return true; },
        [](const BuildInfoTypes::PlaceItem&) { return false; }
  );
}

static TString getNameHelper(const BuildInfoTypes::BuildType& type, const ContentFactory* factory) {
  return type.visit<TString>(
      [&](const BuildInfoTypes::Furniture& f) { return factory->furniture.getData(f.types[0]).getName(); },
      [&](const BuildInfoTypes::ClaimTile& e) { return TStringId("BUILD_MENU_CLAIM_TILE"); },
      [&](const BuildInfoTypes::UnclaimTile& e) { return TStringId("BUILD_MENU_UNCLAIM_TILE"); },
      [&](const BuildInfoTypes::Dig& e) { return TStringId("BUILD_MENU_DIG"); },
      [&](const BuildInfoTypes::CutTree& e) { return TStringId("BUILD_MENU_CUT_TREE"); },
      [&](const BuildInfoTypes::Dispatch& e) { return TStringId("BUILD_MENU_PRIORITIZE"); },
      [&](const BuildInfoTypes::ForbidZone& e) { return TStringId("FORBIDDEN_ZONE_HIGHLIGHT"); },
      [&](const BuildInfoTypes::PlaceMinion& e) { return TStringId("BUILD_MENU_PLACE_MINION"); },
      [&](const BuildInfoTypes::ImmediateDig& e) { return TStringId("BUILD_MENU_IMMEDIATE_DIG"); },
      [&](const BuildInfoTypes::PlaceItem& e) { return TStringId("BUILD_MENU_PLACE_ITEM"); },
      [&](const BuildInfoTypes::Zone& e) { return *getDescription(getHighlight(e)); },
      [&](const BuildInfoTypes::Chain& e) { return getNameHelper(e.front(), factory); },
      [&](const BuildInfoTypes::DestroyLayers& e) { return TString("no name"_s); },
      [&](const BuildInfoTypes::FillPit& e) { return TStringId("BUILD_MENU_FILL_PIT"); }
  );
}

const TString& BuildInfo::getName(const ContentFactory* factory) const {
  if (name.empty())
    name = getNameHelper(type, factory);
  return name;
}