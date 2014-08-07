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
  static Tile fromViewId(ViewId);
  static Color getColor(const ViewObject& object);

  static Tile empty();
  static Tile unicode(sf::Uint32 ch, ColorId, bool sym = false);
  static Tile fromString(const string&, ColorId, bool symbol = false);
  static Tile byCoord(int x, int y, int num = 0, bool noShadow = false);
  static Tile byName(const string&, bool noShadow = false);

  static void initialize();
  static void loadTiles();
  static void loadUnicode();

  Color color;
  String text;
  bool symFont = false;
  double translucent = 0;
  bool noShadow = false;

  Tile setNoShadow();

  Tile addConnection(EnumSet<Dir> c, int x, int y);
  Tile addConnection(EnumSet<Dir> c, const string&);
  Tile addOption(Dir, const string&);

  Tile addBackground(int x, int y);
  Tile addBackground(const string&);

  Tile setTranslucent(double v);

  bool hasSpriteCoord();

  Vec2 getSpriteCoord();

  Optional<Vec2> getBackgroundCoord();

  Vec2 getSpriteCoord(const EnumSet<Dir>& c);

  int getTexNum();

  private:
  Tile();
  Tile(int x, int y, int num = 0, bool noShadow = false);
  Tile(Renderer::TileCoords coords, bool noShadow);
  Optional<Vec2> tileCoord;
  Optional<Vec2> backgroundCoord;
  int texNum = 0;
  unordered_map<EnumSet<Dir>, Vec2> connections;
  Optional<pair<Dir, Vec2>> connectionOption;
};


#endif
