#include "stdafx.h"

void MapMemory::addObject(Vec2 pos, const ViewObject& obj) {
  table[pos].insert(obj);
  table[pos].setHighlight(HighlightType::MEMORY);
}

void MapMemory::clearSquare(Vec2 pos) {
  table.erase(pos);
}

bool MapMemory::hasViewIndex(Vec2 pos) const {
  return table.count(pos);
}

ViewIndex MapMemory::getViewIndex(Vec2 pos) const {
  TRY(return table.at(pos), "No view index at " << pos);
}
  
const MapMemory& MapMemory::empty() {
  static MapMemory mem;
  return mem;
}
