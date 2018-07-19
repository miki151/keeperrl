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

#pragma once

#include "renderer.h"
#include "util.h"
#include "view_id.h"

enum class ViewId;
class ViewObject;

class Tile {
  public:
  typedef Renderer::TileCoord TileCoord;
  static const Tile& getTile(ViewId, bool sprite);
  static const Tile& getTile(ViewId);
  static Color getColor(const ViewObject& object);

  static Tile empty();
  static Tile fromString(const string&, Color, bool symbol = false);
  static Tile byCoord(TileCoord);

  static void initialize(Renderer&, bool useTiles);

  Color color;
  string text;
  bool symFont = false;
  double translucent = 0;
  bool roundShadow = false;
  bool moveUp = false;
  bool floorBorders = false;
  bool wallShadow = false;

  Tile addConnection(DirSet, TileCoord);
  Tile addOption(Dir, TileCoord);
  Tile setFloorBorders();

  Tile addBackground(TileCoord);

  Tile addExtraBorder(DirSet, TileCoord);
  Tile addExtraBorderId(ViewId);
  Tile addCorner(DirSet cornerDef, DirSet borders, TileCoord);
  Tile setTranslucent(double v);
  Tile addHighlight(TileCoord);
  Tile setColor(Color);
  Tile setRoundShadow();
  Tile setMoveUp();
  Tile setWallShadow();
  optional<TileCoord> getHighlightCoord() const;

  const EnumSet<ViewId>& getExtraBorderIds() const;
  bool hasExtraBorders() const;
  bool hasAnyConnections() const;
  optional<TileCoord> getExtraBorderCoord(DirSet) const;

  bool hasSpriteCoord() const;

  TileCoord getSpriteCoord() const;
  TileCoord getSpriteCoord(DirSet) const;
  optional<TileCoord> getBackgroundCoord() const;

  bool hasAnyCorners() const;
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
  optional<pair<Dir, TileCoord>> connectionOption;
  array<vector<TileCoord>, 256> corners;
  array<optional<TileCoord>, 256> extraBorders;
  bool anyExtraBorders = false;
  bool anyConnections = false;
  bool anyCorners = false;
  DirSet connectionsMask = DirSet{Dir::N, Dir::E, Dir::S, Dir::W};
  EnumSet<ViewId> extraBorderIds;
  static void addTile(ViewId, Tile);
  static void addSymbol(ViewId, Tile);
};


