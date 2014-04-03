#include "stdafx.h"

#include "map_memory.h"

MapMemory::MapMemory() : table(600, 600) {
}

MapMemory::MapMemory(const MapMemory& other) : table(other.table.getWidth(), other.table.getHeight()) {
  for (Vec2 v : table.getBounds())
    table[v] = other.table[v];
}

template <class Archive> 
void MapMemory::serialize(Archive& ar, const unsigned int version) {
  ar & SVAR(table);
  CHECK_SERIAL;
}

SERIALIZABLE(MapMemory);

void MapMemory::addObject(Vec2 pos, const ViewObject& obj) {
  if (!table[pos])
    table[pos] = ViewIndex();
  table[pos]->insert(obj);
  table[pos]->setHighlight(HighlightType::MEMORY);
}

void MapMemory::clearSquare(Vec2 pos) {
  table[pos] = Nothing();
}

bool MapMemory::hasViewIndex(Vec2 pos) const {
  return table[pos];
}

ViewIndex MapMemory::getViewIndex(Vec2 pos) const {
  return *table[pos];
}
  
const MapMemory& MapMemory::empty() {
  static MapMemory mem;
  return mem;
}
