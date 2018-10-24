#include "stdafx.h"
#include "build_info.h"
#include "collective.h"
#include "technology.h"

bool BuildInfo::meetsRequirement(WConstCollective col, Requirement req) {
  return req.visit(
      [&](TechId techId) { return col->hasTech(techId);},
      [&](BuildInfo::DungeonLevel level) { return col->getDungeonLevel().level >= level; }
  );
}

string BuildInfo::getRequirementText(Requirement req) {
  return req.visit(
      [&](TechId techId) { return "technology: " + Technology::get(techId)->getName();},
      [&](BuildInfo::DungeonLevel level) { return "at least level " + toString(level); }
  );
}
