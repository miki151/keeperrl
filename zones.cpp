#include "stdafx.h"
#include "zones.h"
#include "position.h"
#include "view_index.h"
#include "movement_type.h"

SERIALIZE_DEF(Zones, zones)

bool Zones::isZone(Position pos, ZoneId id) const {
  return zones[id].count(pos);
}

void Zones::setZone(Position pos, ZoneId id) {
  if (pos.canEnterEmpty(MovementTrait::WALK)) {
    zones[id].insert(pos);
    pos.setNeedsRenderUpdate(true);
  }
}

void Zones::eraseZone(Position pos, ZoneId id) {
  zones[id].erase(pos);
  pos.setNeedsRenderUpdate(true);
}

void Zones::eraseZones(Position pos) {
  for (auto id : ENUM_ALL(ZoneId))
    eraseZone(pos, id);
}

const set<Position>& Zones::getPositions(ZoneId id) const {
  return zones[id];
}

static HighlightType getHighlight(ZoneId id) {
  switch (id) {
    case ZoneId::FETCH_ITEMS:
      return HighlightType::FETCH_ITEMS;
    case ZoneId::STORAGE_EQUIPMENT:
      return HighlightType::STORAGE_EQUIPMENT;
    case ZoneId::STORAGE_RESOURCES:
      return HighlightType::STORAGE_RESOURCES;
  }
}

void Zones::setHighlights(Position pos, ViewIndex& index) const {
  for (auto id : ENUM_ALL(ZoneId))
    if (isZone(pos, id))
      index.setHighlight(getHighlight(id));
}
