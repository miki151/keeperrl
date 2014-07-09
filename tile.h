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

  static Tile empty();

  Color color;
  String text;
  bool symFont = false;
  double translucent = 0;
  bool noShadow = false;
  Tile(sf::Uint32 ch, Color col, bool sym = false);
  
  Tile(int x, int y, int num = 0, bool noShadow = false);
  Tile(const string&, bool noShadow = false);

  Tile& addConnection(set<Dir> c, int x, int y);
  Tile& addConnection(set<Dir> c, const string&);

  Tile& addBackground(int x, int y);
  Tile& addBackground(const string&);

  Tile& setTranslucent(double v);

  bool hasSpriteCoord();

  Vec2 getSpriteCoord();

  Optional<Vec2> getBackgroundCoord();

  Vec2 getSpriteCoord(const set<Dir>& c);

  int getTexNum();

  const static set<Dir> allDirs;

  private:
  Tile(Renderer::TileCoords, bool noShadow = false);
  Optional<Vec2> tileCoord;
  Optional<Vec2> backgroundCoord;
  int texNum = 0;
  unordered_map<set<Dir>, Vec2> connections;
};


#endif
