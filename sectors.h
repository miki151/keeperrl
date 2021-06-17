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

class Sectors {
  public:
  using ExtraConnections = Table<optional<Vec2>>;
  Sectors(Rectangle bounds, ExtraConnections);

  bool same(Vec2, Vec2) const;
  bool add(Vec2);
  bool remove(Vec2);
  void dump();
  bool contains(Vec2) const;
  int getNumSectors() const;
  void addExtraConnection(Vec2, Vec2);
  void removeExtraConnection(Vec2, Vec2);
  const ExtraConnections getExtraConnections() const;

  using SectorId = short;
  SectorId getLargest() const;
  bool isSector(Vec2, SectorId) const;

  private:
  vector<Vec2> getNeighbors(Vec2) const;
  void setSector(Vec2, SectorId);
  SectorId getNewSector();
  void join(Vec2, SectorId);
  vector<Vec2> getDisjoint(Vec2) const;
  Rectangle bounds;
  Table<SectorId> sectors;
  vector<int> sizes;
  ExtraConnections extraConnections;
};

