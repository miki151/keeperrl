#include "stdafx.h"
#include "zones.h"
#include "position.h"
#include "view_index.h"
#include "movement_type.h"
#include "collective.h"
#include "territory.h"

SERIALIZE_DEF(Zones, positions, zones)
SERIALIZATION_CONSTRUCTOR_IMPL(Zones)

Zones::Zones(Rectangle bounds) : zones(bounds) {
}

bool Zones::isZone(Position pos, ZoneId id) const {
  //PROFILE;
  return zones[pos.getCoord()].contains(id);
}

bool Zones::isAnyZone(Position pos, EnumSet<ZoneId> id) const {
  //PROFILE;
  return !zones[pos.getCoord()].intersection(id).isEmpty();
}

void Zones::setZone(Position pos, ZoneId id) {
  PROFILE;
  zones[pos.getCoord()].insert(id);
  positions[id].insert(pos);
  pos.setNeedsRenderUpdate(true);
}

void Zones::eraseZone(Position pos, ZoneId id) {
  PROFILE;
  zones[pos.getCoord()].erase(id);
  positions[id].erase(pos);
  pos.setNeedsRenderUpdate(true);
}

static ZoneId destroyedOnOrder[] = {
  ZoneId::FETCH_ITEMS,
  ZoneId::PERMANENT_FETCH_ITEMS,
  ZoneId::STORAGE_EQUIPMENT,
  ZoneId::STORAGE_RESOURCES
};

void Zones::onDestroyOrder(Position pos) {
  for (auto id : destroyedOnOrder)
    eraseZone(pos, id);
}

const PositionSet& Zones::getPositions(ZoneId id) const {
  return positions[id];
}

static HighlightType getHighlight(ZoneId id) {
  switch (id) {
    case ZoneId::FETCH_ITEMS:
      return HighlightType::FETCH_ITEMS;
    case ZoneId::PERMANENT_FETCH_ITEMS:
      return HighlightType::PERMANENT_FETCH_ITEMS;
    case ZoneId::STORAGE_EQUIPMENT:
      return HighlightType::STORAGE_EQUIPMENT;
    case ZoneId::STORAGE_RESOURCES:
      return HighlightType::STORAGE_RESOURCES;
    case ZoneId::QUARTERS1:
      return HighlightType::QUARTERS1;
    case ZoneId::QUARTERS2:
      return HighlightType::QUARTERS2;
    case ZoneId::QUARTERS3:
      return HighlightType::QUARTERS3;
  }
}

void Zones::setHighlights(Position pos, ViewIndex& index) const {
  PROFILE;
  for (auto id : ENUM_ALL(ZoneId))
    if (isZone(pos, id))
      index.setHighlight(getHighlight(id));
}

bool Zones::canSet(Position pos, ZoneId id, WConstCollective col) const {
  switch (id) {
    case ZoneId::STORAGE_EQUIPMENT:
    case ZoneId::STORAGE_RESOURCES:
      return pos.canEnterEmpty(MovementTrait::WALK);
    case ZoneId::QUARTERS1:
    case ZoneId::QUARTERS2:
    case ZoneId::QUARTERS3:
      return col->getTerritory().contains(pos);
    default:
      return true;
  }
}

void Zones::tick() {
  PROFILE_BLOCK("Zones::tick");
  for (auto pos : copyOf(positions[ZoneId::FETCH_ITEMS]))
    if (pos.getItems().empty())
      eraseZone(pos, ZoneId::FETCH_ITEMS);
}
