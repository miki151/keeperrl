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

#ifndef _TILE_H
#define _TILE_H

#include "view_object.h"
#include "renderer.h"

class Tile {
  public:
  static Tile getTile(const ViewObject& obj, bool sprite);
  static Color getColor(const ViewObject& object);

  Color color;
  String text;
  bool symFont = false;
  double translucent = 0;
  bool stickingOut = false;
  Tile(sf::Uint32 ch, Color col, bool sym = false) : color(col), text(ch), symFont(sym) {
  }
  Tile(int x, int y, int num = 0, bool _stickingOut = false) : stickingOut(_stickingOut),tileCoord(Vec2(x, y)), 
      texNum(num) {}

  Tile& addConnection(set<Dir> c, int x, int y) {
    connections.insert({c, Vec2(x, y)});
    return *this;
  }

  Tile& setTranslucent(double v) {
    translucent = v;
    return *this;
  }

  bool hasSpriteCoord() {
    return tileCoord;
  }

  Vec2 getSpriteCoord() {
    return *tileCoord;
  }

  Vec2 getSpriteCoord(set<Dir> c) {
    if (connections.count(c))
      return connections.at(c);
    else return *tileCoord;
  }

  int getTexNum() {
    CHECK(tileCoord) << "Not a sprite tile";
    return texNum;
  }

  private:
  Optional<Vec2> tileCoord;
  int texNum = 0;
  unordered_map<set<Dir>, Vec2> connections;
};


#endif
