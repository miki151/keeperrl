/* Copyright (C) 2013-2014 Michal Brzozowski (rusolis@poczta.fm)

   This file is part of KeeperRL.

   KeeperRL is free software; you can redistribute it and/or modify it under the terms of the
   GNU General Public License as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   KeeperRL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
   even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along with this program.
   If not, see http://www.gnu.org/licenses/ . */

#include "stdafx.h"

#include "map_memory.h"
#include "level.h"
#include "view_object.h"

MapMemory::MapMemory() : table(Level::getMaxBounds()) {
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

void MapMemory::update(Vec2 pos, const ViewIndex& index) {
  table[pos] = index;
  table[pos]->setHighlight(HighlightType::MEMORY);
  table[pos]->removeObject(ViewLayer::CREATURE);
}

void MapMemory::clearSquare(Vec2 pos) {
  table[pos] = Nothing();
}

bool MapMemory::hasViewIndex(Vec2 pos) const {
  return !!table[pos];
}

ViewIndex MapMemory::getViewIndex(Vec2 pos) const {
  return *table[pos];
}
  
const MapMemory& MapMemory::empty() {
  static MapMemory mem;
  return mem;
}
