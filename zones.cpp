#include "stdafx.h"
#include "zones.h"
#include "position.h"
#include "view_index.h"
#include "movement_type.h"
#include "collective.h"
#include "territory.h"
#include "quarters.h"

SERIALIZE_DEF(Zones, positions, zones)
SERIALIZATION_CONSTRUCTOR_IMPL(Zones)

bool Zones::isZone(Position pos, ZoneId id) const {
  //PROFILE;
  if (auto z = zones.getReferenceMaybe(pos))
    if (z->contains(id))
      return true;
  return false;
}

bool Zones::isAnyZone(Position pos, EnumSet<ZoneId> id) const {
  //PROFILE;
  if (auto z = zones.getReferenceMaybe(pos))
    return !z->intersection(id).isEmpty();
  return false;
}

void Zones::setZone(Position pos, ZoneId id) {
  PROFILE;
  zones.getOrInit(pos).insert(id);
  positions[id].insert(pos);
  pos.setNeedsRenderAndMemoryUpdate(true);
}

void Zones::eraseZone(Position pos, ZoneId id) {
  PROFILE;
  zones.getOrInit(pos).erase(id);
  positions[id].erase(pos);
  pos.setNeedsRenderAndMemoryUpdate(true);
}

static ZoneId destroyedOnOrder[] = {
  ZoneId::FETCH_ITEMS,
  ZoneId::PERMANENT_FETCH_ITEMS,
  ZoneId::STORAGE_EQUIPMENT,
  ZoneId::STORAGE_RESOURCES,
  ZoneId::GUARD1,
  ZoneId::GUARD2,
  ZoneId::GUARD3
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
    case ZoneId::LEISURE:
      return HighlightType::LEISURE;
    case ZoneId::GUARD1:
      return HighlightType::GUARD_ZONE1;
    case ZoneId::GUARD2:
      return HighlightType::GUARD_ZONE2;
    case ZoneId::GUARD3:
      return HighlightType::GUARD_ZONE3;
  }
}

void Zones::setHighlights(Position pos, ViewIndex& index) const {
  PROFILE;
  for (auto id : ENUM_ALL(ZoneId))
    if (isZone(pos, id))
      index.setHighlight(getHighlight(id));
}

bool Zones::canSet(Position pos, ZoneId id, const Collective* col) const {
  switch (id) {
    case ZoneId::STORAGE_EQUIPMENT:
    case ZoneId::STORAGE_RESOURCES:
    case ZoneId::GUARD1:
    case ZoneId::GUARD2:
    case ZoneId::GUARD3:
      return pos.canEnterEmpty(MovementTrait::WALK);
    case ZoneId::QUARTERS1:
    case ZoneId::QUARTERS2:
    case ZoneId::QUARTERS3:
    case ZoneId::LEISURE:
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

ViewId getViewId(ZoneId id) {
  switch (id) {
    case ZoneId::QUARTERS1:
      return Quarters::getAllQuarters()[0].viewId;
    case ZoneId::QUARTERS2:
      return Quarters::getAllQuarters()[1].viewId;
    case ZoneId::QUARTERS3:
      return Quarters::getAllQuarters()[2].viewId;
    case ZoneId::LEISURE:
      return ViewId("quarters", Color(50, 50, 200));
    case ZoneId::FETCH_ITEMS:
    case ZoneId::PERMANENT_FETCH_ITEMS:
      return ViewId("fetch_icon");
    case ZoneId::STORAGE_EQUIPMENT:
      return ViewId("storage_equipment");
    case ZoneId::STORAGE_RESOURCES:
      return ViewId("storage_resources");
    case ZoneId::GUARD1:
      return ViewId("guard_zone");
    case ZoneId::GUARD2:
      return ViewId("guard_zone", Color::PURPLE);
    case ZoneId::GUARD3:
      return ViewId("guard_zone", Color::SKY_BLUE);
  }
}
