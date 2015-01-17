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
#include "view_id.h"
#include "util.h"

class Tile {
  public:
  typedef Renderer::TileCoord TileCoord;
  static const Tile& getTile(ViewId, bool sprite);
  static Color getColor(const ViewObject& object);

  static Tile empty();
  static Tile unicode(sf::Uint32 ch, ColorId, bool sym = false);
  static Tile fromString(const string&, ColorId, bool symbol = false);
  static Tile byCoord(TileCoord);

  static void initialize(Renderer&, bool useTiles);

  Color color;
  String text;
  bool symFont = false;
  double translucent = 0;
  bool noShadow = false;
  bool floorBorders = false;

  Tile setNoShadow();

  Tile addConnection(EnumSet<Dir>, TileCoord);
  Tile addOption(Dir, TileCoord);
  Tile setFloorBorders();

  Tile addBackground(TileCoord);

  Tile addExtraBorder(EnumSet<Dir>, TileCoord);
  Tile addExtraBorderId(ViewId);
  Tile addCorner(EnumSet<Dir> cornerDef, EnumSet<Dir> borders, TileCoord);
  Tile setTranslucent(double v);

  const vector<ViewId>& getExtraBorderIds() const;
  bool hasExtraBorders() const;
  optional<TileCoord> getExtraBorderCoord(const EnumSet<Dir>& c) const;

  bool hasSpriteCoord() const;

  TileCoord getSpriteCoord() const;
  TileCoord getSpriteCoord(const EnumSet<Dir>& c) const;
  optional<TileCoord> getBackgroundCoord() const;

  bool hasCorners() const;
  vector<TileCoord> getCornerCoords(const EnumSet<Dir>& c) const;

  private:
  static void loadTiles();
  static void loadUnicode();
  Tile();
  Tile(TileCoord);
  optional<TileCoord> tileCoord;
  optional<TileCoord> backgroundCoord;
  unordered_map<EnumSet<Dir>, TileCoord> connections;
  optional<pair<Dir, TileCoord>> connectionOption;
  struct CornerInfo {
    EnumSet<Dir> cornerDef;
    EnumSet<Dir> borders;
    TileCoord tileCoord;
  };
  vector<CornerInfo> corners;
  unordered_map<EnumSet<Dir>, TileCoord> extraBorders;
  vector<ViewId> extraBorderIds;
};


#endif
