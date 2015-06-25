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

#ifndef _MEMORY_H
#define _MEMORY_H

#include "util.h"

class ViewObject;
class ViewIndex;

class MapMemory {
  public:
  MapMemory();
  MapMemory(const MapMemory&);
  void addObject(Vec2 pos, const ViewObject& obj);
  void update(Vec2, const ViewIndex&);
  const unordered_set<Vec2>& getUpdated() const;
  void clearUpdated() const;
  void clearSquare(Vec2 pos);
  bool hasViewIndex(Vec2 pos) const;
  const ViewIndex& getViewIndex(Vec2 pos) const;
  static const MapMemory& empty();

  template <class Archive> 
  void serialize(Archive& ar, const unsigned int version);

  private:
  HeapAllocated<Table<optional<ViewIndex>>> SERIAL(table);
  mutable unordered_set<Vec2> updated;
};

#endif
