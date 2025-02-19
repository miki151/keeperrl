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
      [&](TechId techId) { return TSentence("TECH_REQUIREMENT", factory->technology.getName(techId));},
      [&](BuildInfo::DungeonLevel level) { return TSentence("DUNGEON_LEVEL_REQUIREMENT", toString(level)); }
  );
}

bool BuildInfo::canSelectRectangle() const {
  return type.visit<bool>(
        [](const auto&) { return true; },
        [](const BuildInfoTypes::PlaceItem&) { return false; }
  );
}

static const TString* getNameHelper(const BuildInfoTypes::BuildType& type, const ContentFactory* factory) {
  return type.visit<const TString*>(
      [&](const BuildInfoTypes::Furniture& f) { return &factory->furniture.getData(f.types[0]).getName(); },
      [&](const BuildInfoTypes::ClaimTile& e) { static TString ret = TStringId("BUILD_MENU_CLAIM_TILE"); return &ret; },
      [&](const BuildInfoTypes::UnclaimTile& e) { static TString ret = TStringId("BUILD_MENU_UNCLAIM_TILE"); return &ret; },
      [&](const BuildInfoTypes::Dig& e) { static TString ret = TStringId("BUILD_MENU_DIG"); return &ret; },
      [&](const BuildInfoTypes::CutTree& e) { static TString ret = TStringId("BUILD_MENU_CUT_TREE"); return &ret; },
      [&](const BuildInfoTypes::Dispatch& e) { static TString ret = TStringId("BUILD_MENU_PRIORITIZE"); return &ret; },
      [&](const BuildInfoTypes::ForbidZone& e) { static TString ret = TStringId("FORBIDDEN_ZONE_HIGHLIGHT"); return &ret; },
      [&](const BuildInfoTypes::PlaceMinion& e) { static TString ret = TStringId("BUILD_MENU_PLACE_MINION"); return &ret; },
      [&](const BuildInfoTypes::ImmediateDig& e) { static TString ret = TStringId("BUILD_MENU_IMMEDIATE_DIG"); return &ret; },
      [&](const BuildInfoTypes::PlaceItem& e) { static TString ret = TStringId("BUILD_MENU_PLACE_ITEM"); return &ret; },
      [&](const BuildInfoTypes::Zone& e) { static TString ret = *getDescription(getHighlight(e)); return &ret; },
      [&](const BuildInfoTypes::Chain& e) { return getNameHelper(e.front(), factory); },
      [&](const BuildInfoTypes::DestroyLayers& e) { static TString ret; return &ret; },
      [&](const BuildInfoTypes::FillPit& e) { static TString ret = TStringId("BUILD_MENU_FILL_PIT"); return &ret; }
  );
}

const TString& BuildInfo::getName(const ContentFactory* factory) const {
  if (name != TString())
    return name;
  return *getNameHelper(type, factory);
}