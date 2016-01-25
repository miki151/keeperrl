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

template <class Archive> 
void MapMemory::serialize(Archive& ar, const unsigned int version) {
  ar & SVAR(table);
}

SERIALIZABLE(MapMemory);

SERIALIZATION_CONSTRUCTOR_IMPL(MapMemory);

MapMemory::MapMemory(const vector<Level*>& levels) : table(levels) {
}

void MapMemory::addObject(Position pos, const ViewObject& obj) {
  CHECK(pos.isValid());
  if (!getViewIndex(pos))
    getViewIndex(pos) = ViewIndex();
  getViewIndex(pos)->insert(obj);
  getViewIndex(pos)->setHighlight(HighlightType::MEMORY);
  updateUpdated(pos);
}

optional<ViewIndex>& MapMemory::getViewIndex(Position pos) {
  return (*table)[pos];
}

const optional<ViewIndex>& MapMemory::getViewIndex(Position pos) const {
  static optional<ViewIndex> empty;
  if (!table->isValid(pos))
    return empty;
  return (*table)[pos];
}

void MapMemory::update(Position pos, const ViewIndex& index) {
  CHECK(pos.isValid());
  getViewIndex(pos) = index;
  getViewIndex(pos)->setHighlight(HighlightType::MEMORY);
  if (getViewIndex(pos)->hasObject(ViewLayer::CREATURE) && 
      !getViewIndex(pos)->getObject(ViewLayer::CREATURE).hasModifier(ViewObjectModifier::REMEMBER))
    getViewIndex(pos)->removeObject(ViewLayer::CREATURE);
  updateUpdated(pos);
}

void MapMemory::updateUpdated(Position pos) {
  if (pos.isValid())
    updated[pos.getLevel()->getUniqueId()].insert(pos);
}

void MapMemory::clearSquare(Position pos) {
  getViewIndex(pos) = none;
}

const MapMemory& MapMemory::empty() {
  static MapMemory mem;
  return mem;
} 

const unordered_set<Position, CustomHash<Position>>& MapMemory::getUpdated(const Level* level) const {
  return updated[level->getUniqueId()];
}

void MapMemory::clearUpdated(const Level* level) const {
  updated[level->getUniqueId()].clear();
}
