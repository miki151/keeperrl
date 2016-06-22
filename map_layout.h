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

#ifndef _MAP_LAYOUT
#define _MAP_LAYOUT

#include "util.h"
#include "enums.h"

class MapLayout {
  public:
  MapLayout() {}
  MapLayout(Vec2 squareSize, vector<ViewLayer> layers);

  const vector<ViewLayer>& getLayers() const;

  Vec2 getSquareSize();
  Vec2 projectOnScreen(Rectangle bounds, Vec2 screenPos, double x, double y);
  Vec2 projectOnMap(Rectangle bounds, Vec2 screenPos, Vec2 pos);
  Rectangle getAllTiles(Rectangle screenBounds, Rectangle tableBounds, Vec2 screenPos);

  private:
  vector<ViewLayer> layers;
  Vec2 squareSize;
};


#endif
