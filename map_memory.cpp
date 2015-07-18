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
#include "view_index.h"

MapMemory::MapMemory() : table(Level::getMaxBounds()) {
}

MapMemory::MapMemory(const MapMemory& other) : table(other.table->getWidth(), other.table->getHeight()) {
  for (Vec2 v : table->getBounds())
    (*table)[v] = (*other.table)[v];
}

template <class Archive> 
void MapMemory::serialize(Archive& ar, const unsigned int version) {
  ar & SVAR(table);
}

SERIALIZABLE(MapMemory);

void MapMemory::addObject(Vec2 pos, const ViewObject& obj) {
  if (pos.inRectangle(table->getBounds())) {
    if (!(*table)[pos])
      (*table)[pos] = ViewIndex();
    (*table)[pos]->insert(obj);
    (*table)[pos]->setHighlight(HighlightType::MEMORY);
    updated.insert(pos);
  }
}

void MapMemory::update(Vec2 pos, const ViewIndex& index) {
  if (pos.inRectangle(table->getBounds())) {
    (*table)[pos] = index;
    (*table)[pos]->setHighlight(HighlightType::MEMORY);
    (*table)[pos]->removeObject(ViewLayer::CREATURE);
    updated.insert(pos);
  }
}

void MapMemory::clearSquare(Vec2 pos) {
  if (pos.inRectangle(table->getBounds()))
    (*table)[pos] = none;
}

bool MapMemory::hasViewIndex(Vec2 pos) const {
  return pos.inRectangle(table->getBounds()) && !!(*table)[pos];
}

const ViewIndex& MapMemory::getViewIndex(Vec2 pos) const {
  if (pos.inRectangle(table->getBounds()))
    return *(*table)[pos];
  else {
    static ViewIndex ind;
    return ind;
  }
}
  
const MapMemory& MapMemory::empty() {
  static MapMemory mem;
  return mem;
} 

const unordered_set<Vec2>& MapMemory::getUpdated() const {
  return updated;
}

void MapMemory::clearUpdated() const {
  updated.clear();
}
