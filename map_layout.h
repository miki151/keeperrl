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

#include <SFML/Graphics.hpp>

#include "util.h"
#include "enums.h"

class MapLayout {
  public:
  MapLayout() {}
  MapLayout(int squareW, int squareH, vector<ViewLayer> layers);

  vector<ViewLayer> getLayers() const;

  int squareWidth();
  int squareHeight();
  Vec2 projectOnScreen(Rectangle bounds, Vec2 mapPos);
  Vec2 projectOnMap(Rectangle bounds, Vec2 screenPos);
  Rectangle getAllTiles(Rectangle screenBounds, Rectangle tableBounds);
  void updatePlayerPos(Vec2);
  Vec2 getPlayerPos();

  private:
  vector<ViewLayer> layers;
  Vec2 center;
  int squareW;
  int squareH;
};


#endif
