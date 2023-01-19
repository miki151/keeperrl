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

#pragma once

#include "util.h"
#include "position.h"
#include "position_map.h"
#include "hashing.h"

class ViewObject;
class ViewIndex;

class MapMemory {
  public:
  MapMemory();
  void addObject(Position, const ViewObject&);
  void update(Position, const ViewIndex&);
  const unordered_set<Position, CustomHash<Position>>& getUpdated(const Level*) const;
  void clearUpdated(const Level*) const;
  void clearSquare(Position pos);
  static const MapMemory& empty();
  optional<const ViewIndex&> getViewIndex(Position) const;
  bool containsLevel(Level*) const;

  template <class Archive>
  void serialize(Archive& ar, const unsigned int version);

  private:
  void updateUpdated(Position);
  optional<ViewIndex&> getViewIndex(Position);
  HeapAllocated<PositionMap<ViewIndex>> SERIAL(table);
  mutable map<int, PositionSet> updated;
};
