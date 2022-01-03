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

SERIALIZE_DEF(MapMemory, table)

MapMemory::MapMemory() {}

void MapMemory::addObject(Position pos, const ViewObject& obj) {
  CHECK(pos.isValid());
  auto& index = table->getOrInit(pos);
  index.insert(obj);
  index.setHighlight(HighlightType::MEMORY);
  updateUpdated(pos);
}

optional<ViewIndex&> MapMemory::getViewIndex(Position pos) {
  return table->getReferenceMaybe(pos);
}

optional<const ViewIndex&> MapMemory::getViewIndex(Position pos) const {
  return table->getReferenceMaybe(pos);
}

void MapMemory::update(Position pos, const ViewIndex& index1) {
  auto& index = table->getOrInit(pos);
  index = index1;
  index.setHighlight(HighlightType::MEMORY);
  if (index.hasObject(ViewLayer::CREATURE) &&
      !index.getObject(ViewLayer::CREATURE).hasModifier(ViewObjectModifier::REMEMBER))
    index.removeObject(ViewLayer::CREATURE);
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
