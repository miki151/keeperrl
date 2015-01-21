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

#include "map_layout.h"

const vector<ViewLayer>& MapLayout::getLayers() const {
  return layers;
}

MapLayout::MapLayout(Vec2 square, vector<ViewLayer> l) : layers(l), squareSize(square) {}

Vec2 MapLayout::getSquareSize() {
  return squareSize;
}

Vec2 MapLayout::projectOnScreen(Rectangle bounds, Vec2 screenPos, double x, double y) {
  return bounds.middle() + Vec2(x * squareSize.x, y * squareSize.y) - screenPos;
}

Vec2 MapLayout::projectOnMap(Rectangle bounds, Vec2 screenPos, Vec2 pos) {
  return (pos + screenPos - bounds.middle()).div(squareSize);
}

Rectangle MapLayout::getAllTiles(Rectangle screenBounds1, Rectangle tableBounds, Vec2 screenPos) {
  vector<Vec2> ret;
  Rectangle screenBounds = screenBounds1.minusMargin(-2 * squareSize.x);
  Rectangle grid(screenBounds.getSize().div(squareSize));
  Vec2 offset = screenPos.div(squareSize) - grid.middle();
  return tableBounds.intersection(grid.translate(offset));
}

