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
#ifndef _KNOWN_TILES_H
#define _KNOWN_TILES_H

#include "util.h"

class KnownTiles {
  public:
  KnownTiles(Rectangle bounds);
  void addTile(Vec2);
  bool isKnown(Vec2) const;
  const set<Vec2>& getBorderTiles() const;

  SERIALIZATION_DECL(KnownTiles);

  private:
  Table<bool> SERIAL(known);
  set<Vec2> SERIAL(border);
};

#endif
