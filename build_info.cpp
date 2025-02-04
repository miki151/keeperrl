#include "stdafx.h"
#include "build_info.h"
#include "collective.h"
#include "technology.h"

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
