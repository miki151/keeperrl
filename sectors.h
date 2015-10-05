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

#ifndef _SECTORS_H
#define _SECTORS_H

#include "util.h"

class Sectors {
  public:
  Sectors(Rectangle bounds);

  bool same(Vec2, Vec2) const;
  void add(Vec2);
  void remove(Vec2);
  void dump();
  bool contains(Vec2) const;
  int getNumSectors() const;
  bool isChokePoint(Vec2) const;

  SERIALIZATION_DECL(Sectors);

  private:
  void setSector(Vec2, int);
  int getNewSector();
  void join(Vec2, int);
  vector<Vec2> getDisjoint(Vec2) const;
  Rectangle SERIAL(bounds);
  Table<int> SERIAL(sectors);
  vector<int> SERIAL(sizes);
};

#endif
