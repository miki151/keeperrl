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

  Tile addConnection(DirSet, TileCoord);
  Tile addOption(Dir, TileCoord);
  Tile setFloorBorders();

  Tile addBackground(TileCoord);

  Tile addExtraBorder(DirSet, TileCoord);
  Tile addExtraBorderId(ViewId);
  Tile addCorner(DirSet cornerDef, DirSet borders, TileCoord);
  Tile setTranslucent(double v);
  Tile addHighlight(TileCoord);
  optional<TileCoord> getHighlightCoord() const;

  const vector<ViewId>& getExtraBorderIds() const;
  bool hasExtraBorders() const;
  optional<TileCoord> getExtraBorderCoord(DirSet) const;

  bool hasSpriteCoord() const;

  TileCoord getSpriteCoord() const;
  TileCoord getSpriteCoord(DirSet) const;
  optional<TileCoord> getBackgroundCoord() const;

  bool hasCorners() const;
  const vector<TileCoord>& getCornerCoords(DirSet) const;

  private:
  static void loadTiles();
  static void loadUnicode();
  friend class TileCoordLookup;
  Tile();
  Tile(TileCoord);
 // Tile(const Tile&) = default;
  optional<TileCoord> tileCoord;
  optional<TileCoord> backgroundCoord;
  optional<TileCoord> highlightCoord;
  array<optional<TileCoord>, 256> connections;
  bool anyConnections = false;
  optional<pair<Dir, TileCoord>> connectionOption;
  array<vector<TileCoord>, 256> corners;
  array<optional<TileCoord>, 256> extraBorders;
  bool anyExtraBorders = false;
  vector<ViewId> extraBorderIds;
  static void addTile(ViewId, Tile);
  static void addSymbol(ViewId, Tile);
  static EnumMap<ViewId, optional<Tile>> tiles;
  static EnumMap<ViewId, optional<Tile>> symbols;
};


#endif
