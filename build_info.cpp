#include "stdafx.h"
#include "build_info.h"
#include "collective.h"
#include "technology.h"

bool BuildInfo::meetsRequirement(WConstCollective col, Requirement req) {
  return req.visit(
      [&](TechId techId) { return col->getTechnology().researched.count(techId);},
      [&](BuildInfo::DungeonLevel level) { return col->getDungeonLevel().level + 1>= level; }
  );
}

string BuildInfo::getRequirementText(Requirement req) {
  return req.visit(
      [&](TechId techId) { return "technology: "_s + techId.data();},
      [&](BuildInfo::DungeonLevel level) { return "at least level " + toString(level); }
  );
}

bool BuildInfo::canSelectRectangle() const {
  return type.visit(
        [](const auto&) { return true; },
        [](const Trap&) { return false; },
        [](const PlaceItem&) { return false; }
  );
}
